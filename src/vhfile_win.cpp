/* ======================================================================================
 * Library       : vhlibfsys
 * Description   : A small C++17 filesystem helper library with 64-bit binary file I/O.
 * Revision      : 0.1.2
 * Source        : https://github.com/vigatron/vhlibfsys
 * Disclaimer    : Provided "AS IS", without warranty.
 * License       : MIT
 * File          : src/vhfile_win.cpp
 * Content size  : 1878
 * Date / Time   : 28-10-2026 14:11:58
 * MD5           : d9701be913e1be0615deeb4f12e6b24a
 * Notes         : MD5 = file content without header/footer
 * Encoding      : UTF-8
 * Author        : Viktor Glebov / V01G04A81
 * Copyright     : © 2026 Viktor Glebov
 * ========================[ BEGIN FILE CONTENT ]====================================== */
#ifdef _WIN32

#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif

#include "vhfile.hpp"

#include <cerrno>
#include <cstring>
#include <limits>
#include <utility>

#include <io.h>
#include <sys/stat.h>

namespace
{

/**
 * @brief Seek to an absolute 64-bit offset (Windows).
 * @param file File handle.
 * @param pos Absolute position.
 * @return true on success.
 */
bool seek64_win(FILE *file, std::uint64_t pos)
{
    if (pos > static_cast<std::uint64_t>(std::numeric_limits<long long>::max()))
        return false;

    return _fseeki64(file, static_cast<long long>(pos), SEEK_SET) == 0;
}

/**
 * @brief Get the current 64-bit offset (Windows).
 * @param file File handle.
 * @param pos Output position.
 * @return true on success.
 */
bool tell64_win(FILE *file, std::uint64_t &pos)
{
    const __int64 value = _ftelli64(file);

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

    return seek64_win(file, pos);
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

    return tell64_win(file, *pos);
}

/**
 * @brief Gets file size in bytes (Windows).
 */
bool VHFile::filesize(std::uint64_t *size)
{
    if (size == nullptr)
        return false;

    if (isclosed() && !openfile())
        return false;

    if (writable && std::fflush(file) != 0)
        return false;

    struct _stat64 st;
    if (_fstat64(_fileno(file), &st) != 0)
        return false;

    if (st.st_size < 0)
        return false;

    *size = static_cast<std::uint64_t>(st.st_size);

    return true;
}

#endif /* _WIN32 */
/* ========================[  END FILE CONTENT  ]========================
 * Library          : vhlibfsys
 * File             : src/vhfile_win.cpp
 * Revision         : 0.1.2
 * Content size     : 1878
 * Date / Time      : 28-10-2026 14:11:58
 * MD5              : d9701be913e1be0615deeb4f12e6b24a
 * Copyright        : © 2026 Viktor Glebov
 * ====================================================================== */