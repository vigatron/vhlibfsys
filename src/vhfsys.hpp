#pragma once

#include "vhplatform.hpp"

#include <filesystem>


class VHFSYS
{
    public:


        static bool DirExist(const std::filesystem::path & dirpath);

        static bool DirExist(const std::string & dirpath);

        static verr CreateNestedDir(const std::string & dirpath);

        static std::vector<uint8_t> ReadVectorFromFile(const std::string &filename);

        static verr WriteVectorToFile(const std::string &filename, const std::vector<uint8_t> &data);

};