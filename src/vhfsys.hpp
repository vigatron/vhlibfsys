/* ======================================================================================
 * Library       : vhlibfsys
 * Description   : A small C++17 filesystem helper library with 64-bit binary file I/O.
 * Revision      : 0.1.0
 * Source        : https://github.com/vigatron/vhlibfsys
 * Disclaimer    : Provided "AS IS", without warranty.
 * License       : MIT
 * File          : src/vhfsys.hpp
 * Content size  : 624
 * Date / Time   : 27-10-2026 21:37:19
 * MD5           : e0c2f07556b5ac059c4be8e76ae5ec52
 * Notes         : MD5 = file content without header/footer
 * Encoding      : UTF-8
 * Author        : Viktor Glebov / V01G04A81
 * Copyright     : © 2026 Viktor Glebov
 * ========================[ BEGIN FILE CONTENT ]====================================== */
#pragma once

#include "vhplatform.hpp"

#include <filesystem>


class VHFSYS
{
    public:

        static bool FileExist(const std::string & filename);

        static bool DirExist(const std::filesystem::path & dirpath);

        static bool DirExist(const std::string & dirpath);

        static verr CreateNestedDir(const std::string & dirpath);

        static std::vector<uint8_t> ReadVectorFromFile(const std::string &filename);

        static verr WriteVectorToFile(const std::string &filename, const std::vector<uint8_t> &data);

        static std::string ExtractFilename(const std::string &fullPathFileName);
};
/* ========================[  END FILE CONTENT  ]========================
 * Library          : vhlibfsys
 * File             : src/vhfsys.hpp
 * Revision         : 0.1.0
 * Content size     : 624
 * Date / Time      : 27-10-2026 21:37:19
 * MD5              : e0c2f07556b5ac059c4be8e76ae5ec52
 * Copyright        : © 2026 Viktor Glebov
 * ====================================================================== */