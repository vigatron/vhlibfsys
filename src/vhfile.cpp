/* ======================================================================================
 * Library       : vhlibfsys
 * Description   : A small C++17 filesystem helper library with 64-bit binary file I/O.
 * Revision      : 0.1.1
 * Source        : https://github.com/vigatron/vhlibfsys
 * Disclaimer    : Provided "AS IS", without warranty.
 * License       : MIT
 * File          : src/vhfile.cpp
 * Content size  : 5378
 * Date / Time   : 27-10-2026 22:12:08
 * MD5           : fc308dfb0a891d273c3bdffc0aea34d5
 * Notes         : MD5 = file content without header/footer
 * Encoding      : UTF-8
 * Author        : Viktor Glebov / V01G04A81
 * Copyright     : © 2026 Viktor Glebov
 * ========================[ BEGIN FILE CONTENT ]====================================== */

#include "vhfile.hpp"

#include <cerrno>
#include <cstring>
#include <utility>

/**
 * @brief Default constructor. Creates a closed file object with empty filename.
 */
VHFile::VHFile() : fname(), file(nullptr), writable(false) {}

/**
 * @brief Constructor that stores filename. Does not open the file.
 * @param filename Path to the file to be opened later.
 */
VHFile::VHFile(const std::string &filename) : fname(filename), file(nullptr), writable(false)
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
void VHFile::assign(const std::string &filename)
{
    if (isopen())
        closefile();

    fname = filename;
}

/**
 * @brief Checks if file is open.
 * @return true if file handle != nullptr.
 */
bool VHFile::isopen() const
{
    return file != nullptr;
}

/**
 * @brief Checks if file is closed.
 * @return true if file handle == nullptr.
 */
bool VHFile::isclosed() const
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
bool VHFile::read(void *ptr, std::size_t sz, std::uint64_t seekoffs)
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
 * @brief Opens file for reading in binary mode ("rb").
 * @details Idempotent if already open read-only. Returns false if already open for writing.
 * @return true if open for reading, false on failure.
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
 * @brief Opens file for writing in binary mode ("wb"), creating/truncating.
 * @details Idempotent if already open writable. Returns false if already open for reading.
 * @return true if open for writing, false on failure.
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
 * @brief Closes file handle.
 * @details Resets handle to nullptr even if fclose fails. Safe to call multiple times.
 * @return true if fclose succeeded, false if already closed or fclose failed.
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
 * @brief Prints read error message to stderr with filename and errno.
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
 * @brief Prints write/create error message to stderr with filename and errno.
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
 * Date / Time      : 27-10-2026 22:12:08
 * MD5              : fc308dfb0a891d273c3bdffc0aea34d5
 * Copyright        : © 2026 Viktor Glebov
 * ====================================================================== */