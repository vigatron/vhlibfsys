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
#ifndef _WIN32

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
 * @brief Seeks to absolute position.
 */
bool VHFile::seek(std::uint64_t pos)
{
    if (isclosed() && !openfile())
        return false;

    return seek64_unix(file, pos);
}

/**
 * @brief Gets current file position.
 */
bool VHFile::pos(std::uint64_t *pos)
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
bool VHFile::filesize(std::uint64_t *size)
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

#endif /* !_WIN32 */
/* ========================[  END FILE CONTENT  ]========================
 * Library          : vhlibfsys
 * File             : src/vhfile_unix.cpp
 * Revision         : 0.1.2
 * Content size     : 1904
 * Date / Time      : 28-10-2026 14:11:58
 * MD5              : de09f3f5c349ac8283260b4d9682f17e
 * Copyright        : © 2026 Viktor Glebov
 * ====================================================================== */