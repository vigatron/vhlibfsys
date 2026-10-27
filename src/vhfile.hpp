/* ======================================================================================
 * Library       : vhlibfsys
 * Description   : A small C++17 filesystem helper library with 64-bit binary file I/O.
 * Revision      : 0.1.1
 * Source        : https://github.com/vigatron/vhlibfsys
 * Disclaimer    : Provided "AS IS", without warranty.
 * License       : MIT
 * File          : src/vhfile.hpp
 * Content size  : 1066
 * Date / Time   : 27-10-2026 21:54:50
 * MD5           : 18dc6cfcdfc5d1c825febd299642995f
 * Notes         : MD5 = file content without header/footer
 * Encoding      : UTF-8
 * Author        : Viktor Glebov / V01G04A81
 * Copyright     : © 2026 Viktor Glebov
 * ========================[ BEGIN FILE CONTENT ]====================================== */
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>

class VHFile
{

public:
    //
    VHFile();

    //
    explicit VHFile(const std::string &filename);

    //
    ~VHFile();

    // A file handle is unique: copying it would close the same FILE twice.
    VHFile(const VHFile &) = delete;
    VHFile &operator=(const VHFile &) = delete;

    VHFile(VHFile &&other) noexcept;
    VHFile &operator=(VHFile &&other) noexcept;

    //
    void assign(const std::string &filename);

    //
    bool filesize(std::uint64_t *size);

    //
    bool pos(std::uint64_t *pos);

    //
    bool seek(std::uint64_t pos);

    //
    bool read(void *ptr, std::size_t sz, std::uint64_t seekoffs = 0);

    //
    bool write(const void *ptr, std::size_t sz);

    //
    bool openfile();

    //
    bool createfile();

    //
    bool closefile();

    //
    bool isopen() const;

    //
    bool isclosed() const;

private:
    //
    void dmperr();
    //
    void dmpwerr();

    std::string fname;
    FILE *file;
    bool writable;
};
/* ========================[  END FILE CONTENT  ]========================
 * Library          : vhlibfsys
 * File             : src/vhfile.hpp
 * Revision         : 0.1.1
 * Content size     : 1066
 * Date / Time      : 27-10-2026 21:54:50
 * MD5              : 18dc6cfcdfc5d1c825febd299642995f
 * Copyright        : © 2026 Viktor Glebov
 * ====================================================================== */