/* ======================================================================================
 * Library       : vhlibfsys
 * Description   : A small C++17 filesystem helper library with 64-bit binary file I/O.
 * Revision      : 0.1.2
 * Source        : https://github.com/vigatron/vhlibfsys
 * Disclaimer    : Provided "AS IS", without warranty.
 * License       : MIT
 * File          : src/vhfile_unix.cpp
 * Content size  : 1904
 * Date / Time   : 28-10-2026 14:11:58
 * MD5           : de09f3f5c349ac8283260b4d9682f17e
 * Notes         : MD5 = file content without header/footer
 * Encoding      : UTF-8
 * Author        : Viktor Glebov / V01G04A81
 * Copyright     : © 2026 Viktor Glebov
 * ========================[ BEGIN FILE CONTENT ]====================================== */

#if defined(__linux__)

#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif

#include "vhfile.hpp"

#include <cerrno>
#include <cstring>
#include <limits>
#include <utility>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace
{

/**
 * @brief Copies a filename, returning an empty string on allocation failure.
 * @param filename Source filename.
 * @return Copy of filename, or empty string if allocation fails.
 */
std::string copy_filename_or_empty(const std::string &filename) noexcept
{
    try
    {
        return std::string(filename);
    }
    catch (...)
    {
        return std::string();
    }
}

/**
 * @brief Seek to an absolute 64-bit offset (POSIX).
 * @param file File handle.
 * @param pos Absolute position.
 * @return true on success.
 */
bool seek64_unix(FILE *file, std::uint64_t pos)
{
    const off_t value = static_cast<off_t>(pos);

    if (value < 0 || static_cast<std::uint64_t>(value) != pos)
        return false;

    return ::fseeko(file, value, SEEK_SET) == 0;
}

/**
 * @brief Get the current 64-bit offset (POSIX).
 * @param file File handle.
 * @param pos Output position.
 * @return true on success.
 */
bool tell64_unix(FILE *file, std::uint64_t &pos)
{
    const off_t value = ::ftello(file);

    if (value < 0)
        return false;

    pos = static_cast<std::uint64_t>(value);
    return true;
}

} // namespace

/**
 * @brief Default constructor. Creates a closed file object with empty filename.
 */
VHFile::VHFile() noexcept : fname(), file(nullptr), writable(false) {}


/**
 * @brief Constructor that stores filename. Does not open the file.
 * @param filename Path to the file to be opened later.
 */
VHFile::VHFile(const std::string &filename) noexcept
    : fname(copy_filename_or_empty(filename)),
      file(nullptr),
      writable(false)
{
}

/**
 * @brief Destructor. Closes the file if open.
 */
VHFile::~VHFile()
{
    closefile();
}

/**
 * @brief Move constructor. Transfers ownership of file handle and filename.
 * @param other Source object to move from. Left in closed state.
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
 * @brief Move assignment operator. Closes current handle and transfers ownership from other.
 * @param other Source object to move from. Left in closed state.
 * @return Reference to this object.
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
 * @brief Assigns a new filename. Closes current file if open, does not open new file.
 * @param filename New path to file.
 */
void VHFile::assign(const std::string &filename) noexcept
{
    try
    {
        std::string tmp(filename);

        if (isopen())
            closefile();

        fname.swap(tmp);
    }
    catch (...)
    {
        // Leave the previous filename and open handle intact if allocation fails.
    }
}

/**
 * @brief Checks if file is open.
 * @return true if file handle != nullptr.
 */
bool VHFile::isopen() const noexcept
{
    return file != nullptr;
}


/**
 * @brief Checks if file is closed.
 * @return true if file handle == nullptr.
 */
bool VHFile::isclosed() const noexcept
{
    return file == nullptr;
}

/**
 * @brief Reads exactly sz bytes into ptr.
 * @details If file is closed, opens read-only. Returns false if file is open for writing.
 *          Loops until all bytes read or error. If seekoffs != 0, seeks to seekoffs first.
 * @param ptr Destination buffer. Must not be nullptr if sz != 0.
 * @param sz Number of bytes to read.
 * @param seekoffs Absolute offset to seek to before reading, or 0 for no seek.
 * @return true if exactly sz bytes read (or sz == 0), false on error/short read.
 */
bool VHFile::read(void *ptr, std::size_t sz, std::uint64_t seekoffs) noexcept
{
    if (ptr == nullptr && sz != 0)
        return false;

    if (isclosed() && !openfile())
        return false;

    if (writable)
        return false;

    if (seekoffs != 0 && !seek(seekoffs))
        return false;

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
 * @brief Writes exactly sz bytes from ptr.
 * @details If file is closed, creates/truncates for writing. Returns false if file is open for reading.
 *          Loops until all bytes written or error.
 * @param ptr Source buffer. Must not be nullptr if sz != 0.
 * @param sz Number of bytes to write.
 * @return true if exactly sz bytes written (or sz == 0), false on error/short write.
 */
bool VHFile::write(const void *ptr, std::size_t sz) noexcept
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
 * @brief Opens file for reading in binary mode ("rb").
 * @details Idempotent if already open read-only. Returns false if already open for writing.
 * @return true if open for reading, false on failure.
 */
bool VHFile::openfile() noexcept
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
 * @brief Opens file for writing in binary mode ("wb"), creating/truncating.
 * @details Idempotent if already open writable. Returns false if already open for reading.
 * @return true if open for writing, false on failure.
 */
bool VHFile::createfile() noexcept
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
 * @brief Closes file handle.
 * @details Resets handle to nullptr even if fclose fails. Safe to call multiple times.
 * @return true if fclose succeeded, false if already closed or fclose failed.
 */
bool VHFile::closefile() noexcept
{
    if (isclosed())
        return false;

    const int result = std::fclose(file);

    file = nullptr;
    writable = false;

    return result == 0;
}

/**
 * @brief Prints read error message to stderr with filename and errno.
 */
void VHFile::dmperr() noexcept
{
    std::fprintf(
        stderr,
        "ERROR: File (%s) cannot be opened: %s\n",
        fname.c_str(),
        std::strerror(errno));
}

/**
 * @brief Prints write/create error message to stderr with filename and errno.
 */
void VHFile::dmpwerr() noexcept
{
    std::fprintf(
        stderr,
        "ERROR: File (%s) cannot be created: %s\n",
        fname.c_str(),
        std::strerror(errno));
}

/**
 * @brief Seeks to absolute position.
 */
bool VHFile::seek(std::uint64_t pos) noexcept
{
    if (isclosed() && !openfile())
        return false;

    return seek64_unix(file, pos);
}

/**
 * @brief Gets current file position.
 */
bool VHFile::pos(std::uint64_t *pos) noexcept
{
    if (pos == nullptr)
        return false;

    if (isclosed() && !openfile())
        return false;

    return tell64_unix(file, *pos);
}

/**
 * @brief Gets file size in bytes (POSIX).
 */
bool VHFile::filesize(std::uint64_t *size) noexcept
{
    if (size == nullptr)
        return false;

    if (isclosed() && !openfile())
        return false;

    if (writable && std::fflush(file) != 0)
        return false;

    struct stat st;
    if (fstat(fileno(file), &st) != 0)
        return false;

    if (st.st_size < 0)
        return false;

    *size = static_cast<std::uint64_t>(st.st_size);

    return true;
}

#endif /* __linux__ */

/* ========================[  END FILE CONTENT  ]========================
 * Library          : vhlibfsys
 * File             : src/vhfile_unix.cpp
 * Revision         : 0.1.2
 * Content size     : 1904
 * Date / Time      : 28-10-2026 14:11:58
 * MD5              : de09f3f5c349ac8283260b4d9682f17e
 * Copyright        : © 2026 Viktor Glebov
 * ====================================================================== */