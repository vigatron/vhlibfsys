/* ======================================================================================
 * Library       : vhlibfsys
 * Description   : A small C++17 filesystem helper library with 64-bit binary file I/O.
 * Revision      : 0.1.1
 * Source        : https://github.com/vigatron/vhlibfsys
 * Disclaimer    : Provided "AS IS", without warranty.
 * License       : MIT
 * File          : src/vhfsys.hpp
 * Content size  : 662
 * Date / Time   : 27-10-2026 21:54:50
 * MD5           : e54614b9b903ce98ad2250f83d21c37e
 * Notes         : MD5 = file content without header/footer
 * Encoding      : UTF-8
 * Author        : Viktor Glebov / V01G04A81
 * Copyright     : © 2026 Viktor Glebov
 * ========================[ BEGIN FILE CONTENT ]====================================== */
#pragma once

#include <filesystem>
#include <string>
#include <vector>

class VHFSYS
{
    public:

        using verr=uint32_t;

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
 * Revision         : 0.1.1
 * Content size     : 662
 * Date / Time      : 27-10-2026 21:54:50
 * MD5              : e54614b9b903ce98ad2250f83d21c37e
 * Copyright        : © 2026 Viktor Glebov
 * ====================================================================== */