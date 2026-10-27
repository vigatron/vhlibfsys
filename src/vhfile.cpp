/* ======================================================================================
 * Library       : vhlibfsys
 * Description   : A small C++17 filesystem helper library with 64-bit binary file I/O.
 * Revision      : 0.1.1
 * Source        : https://github.com/vigatron/vhlibfsys
 * Disclaimer    : Provided "AS IS", without warranty.
 * License       : MIT
 * File          : src/vhfile.cpp
 * Content size  : 5378
 * Date / Time   : 27-10-2026 21:54:50
 * MD5           : fc308dfb0a891d273c3bdffc0aea34d5
 * Notes         : MD5 = file content without header/footer
 * Encoding      : UTF-8
 * Author        : Viktor Glebov / V01G04A81
 * Copyright     : © 2026 Viktor Glebov
 * ========================[ BEGIN FILE CONTENT ]====================================== */
#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif

#include "vhfile.hpp"

#include <cerrno>
#include <cstring>
#include <limits>
#include <utility>

#ifdef _WIN32
#include <io.h>
#include <sys/stat.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace
{

/**
 * @brief Seek to an absolute 64-bit offset.
 */
bool seek64(FILE *file, std::uint64_t pos)
{
#ifdef _WIN32
    if (pos > static_cast<std::uint64_t>(std::numeric_limits<long long>::max()))
        return false;

    return _fseeki64(file, static_cast<long long>(pos), SEEK_SET) == 0;
#else
    const off_t value = static_cast<off_t>(pos);

    if (value < 0 || static_cast<std::uint64_t>(value) != pos)
        return false;

    return ::fseeko(file, value, SEEK_SET) == 0;
#endif
}

/**
 * @brief Get the current 64-bit offset.
 */
bool tell64(FILE *file, std::uint64_t &pos)
{
#ifdef _WIN32
    const __int64 value = _ftelli64(file);
#else
    const off_t value = ::ftello(file);
#endif

    if (value < 0)
        return false;

    pos = static_cast<std::uint64_t>(value);
    return true;
}

} // namespace

/**
 *
 */
VHFile::VHFile() : file(nullptr), writable(false) {}

/**
 *
 */
VHFile::VHFile(const std::string &filename) : file(nullptr), writable(false)
{
    assign(filename);
}

/**
 *
 */
VHFile::~VHFile()
{
    closefile();
}

/**
 *
 */
VHFile::VHFile(VHFile &&other) noexcept
    : fname(std::move(other.fname)),
      file(other.file),
      writable(other.writable)
{
    other.file = nullptr;
    other.writable = false;
}

/**
 *
 */
VHFile &VHFile::operator=(VHFile &&other) noexcept
{
    if (this != &other)
    {
        closefile();

        fname = std::move(other.fname);
        file = other.file;
        writable = other.writable;

        other.file = nullptr;
        other.writable = false;
    }

    return *this;
}

/**
 *
 */
void VHFile::assign(const std::string &filename)
{
    if (isopen())
        closefile();

    fname = filename;
}

/**
 *
 */
bool VHFile::isopen() const
{
    return file != nullptr;
}

/** */
bool VHFile::isclosed() const
{
    return file == nullptr;
}

/**
 *
 */
bool VHFile::filesize(std::uint64_t *size)
{
    if (size == nullptr)
        return false;

    if (isclosed() && !openfile())
        return false;

    // Make buffered writes visible to fstat().
    if (writable && std::fflush(file) != 0)
        return false;

#ifdef _WIN32
    struct _stat64 st;
    if (_fstat64(_fileno(file), &st) != 0)
        return false;
#else
    struct stat st;
    if (fstat(fileno(file), &st) != 0)
        return false;
#endif

    if (st.st_size < 0)
        return false;

    *size = static_cast<std::uint64_t>(st.st_size);

    return true;
}

/** */
bool VHFile::seek(std::uint64_t pos)
{
    if (isclosed() && !openfile())
        return false;

    return seek64(file, pos);
}

/** */
bool VHFile::pos(std::uint64_t *pos)
{
    if (pos == nullptr)
        return false;

    if (isclosed() && !openfile())
        return false;

    return tell64(file, *pos);
}

/**
 *
 */
bool VHFile::read(void *ptr, std::size_t sz, std::uint64_t seekoffs)
{
    //
    if (ptr == nullptr && sz != 0)
        return false;

    //
    if (isclosed() && !openfile())
        return false;

    //
    if (writable)
        return false;

    //
    if (seekoffs != 0 && !seek(seekoffs))
        return false;

    //
    if (sz == 0)
        return true;

    std::uint8_t *dst = static_cast<std::uint8_t *>(ptr);
    std::size_t remaining = sz;

    while (remaining != 0)
    {
        const std::size_t got = std::fread(dst, 1, remaining, file);

        if (got == 0)
            return false;

        dst += got;
        remaining -= got;
    }

    return true;
}

/**
 *
 */
bool VHFile::write(const void *ptr, std::size_t sz)
{
    if (ptr == nullptr && sz != 0)
        return false;

    if (isclosed() && !createfile())
        return false;

    if (!writable)
        return false;

    if (sz == 0)
        return true;

    const std::uint8_t *src = static_cast<const std::uint8_t *>(ptr);
    std::size_t remaining = sz;

    while (remaining != 0)
    {
        const std::size_t put = std::fwrite(src, 1, remaining, file);

        if (put == 0)
            return false;

        src += put;
        remaining -= put;
    }

    return true;
}

/**
 *
 */
bool VHFile::openfile()
{
    if (isopen())
        return !writable;

    file = std::fopen(fname.c_str(), "rb");
    if (file == nullptr)
    {
        dmperr();
        return false;
    }

    writable = false;
    return true;
}

/**
 *
 */
bool VHFile::createfile()
{
    if (isopen())
        return writable;

    file = std::fopen(fname.c_str(), "wb");
    if (file == nullptr)
    {
        dmpwerr();
        return false;
    }

    writable = true;
    return true;
}

/**
 *
 */
bool VHFile::closefile()
{
    if (isclosed())
        return false;

    const int result = std::fclose(file);

    file = nullptr;
    writable = false;

    return result == 0;
}

/**
 *
 */
void VHFile::dmperr()
{
    std::fprintf(
        stderr,
        "ERROR: File (%s) cannot be opened: %s\n",
        fname.c_str(),
        std::strerror(errno));
}

/**
 *
 */
void VHFile::dmpwerr()
{
    std::fprintf(
        stderr,
        "ERROR: File (%s) cannot be created: %s\n",
        fname.c_str(),
        std::strerror(errno));
}
/* ========================[  END FILE CONTENT  ]========================
 * Library          : vhlibfsys
 * File             : src/vhfile.cpp
 * Revision         : 0.1.1
 * Content size     : 5378
 * Date / Time      : 27-10-2026 21:54:50
 * MD5              : fc308dfb0a891d273c3bdffc0aea34d5
 * Copyright        : © 2026 Viktor Glebov
 * ====================================================================== */