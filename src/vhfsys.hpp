/* ======================================================================================
 * Library       : vhlibfsys
 * Description   : A small C++17 filesystem helper library with 64-bit binary file I/O.
 * Revision      : 0.1.2
 * Source        : https://github.com/vigatron/vhlibfsys
 * Disclaimer    : Provided "AS IS", without warranty.
 * License       : MIT
 * File          : src/vhfsys.hpp
 * Content size  : 2683
 * Date / Time   : 28-10-2026 14:11:58
 * MD5           : 766264515df5a6295596550cb516f215
 * Notes         : MD5 = file content without header/footer
 * Encoding      : UTF-8
 * Author        : Viktor Glebov / V01G04A81
 * Copyright     : © 2026 Viktor Glebov
 * ========================[ BEGIN FILE CONTENT ]====================================== */
#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

/**
 * @brief Stateless filesystem helper class.
 *
 * @details
 * Provides static helpers for file/directory existence checks, recursive directory creation,
 * whole-file read/write to/from std::vector<uint8_t>, and filename extraction.
 */
class VHFSYS
{
public:
    /**
     * @brief Error/result code type.
     */
    using verr = uint32_t;

    /**
     * @brief Checks whether the specified path refers to an existing file.
     * @details Returns true if path exists and is not a directory. Does not require regular file.
     * @param filename Path to the file.
     * @return true if exists and not directory, false otherwise.
     */
    static bool FileExist(const std::string &filename);

    /**
     * @brief Checks whether the specified path refers to an existing directory.
     * @param dirpath Path to the directory.
     * @return true if exists and is directory, false otherwise.
     */
    static bool DirExist(const std::filesystem::path &dirpath);

    /**
     * @brief Checks whether the specified directory path exists.
     * @param dirpath Directory path as a string.
     * @return true if exists and is directory, false otherwise.
     */
    static bool DirExist(const std::string &dirpath);

    /**
     * @brief Creates a directory and any missing parent directories.
     * @param dirpath Path to the directory to create.
     * @return 0 if directory exists or created successfully, error code otherwise.
     */
    static verr CreateNestedDir(const std::string &dirpath);

    /**
     * @brief Reads the entire contents of a file into a byte vector.
     * @param filename Path to the input file.
     * @return Vector with file data, or empty vector on error/empty file.
     */
    static std::vector<uint8_t> ReadVectorFromFile(const std::string &filename);

    /**
     * @brief Writes byte vector to a file, creating parent directories if needed.
     * @param filename Path to the output file.
     * @param data Data to write.
     * @return 0 on success, 1 on empty filename/dir creation failure, 2 if cannot open file, 3 on write/close failure.
     */
    static verr WriteVectorToFile(const std::string &filename, const std::vector<uint8_t> &data);

    /**
     * @brief Extracts the filename component from a path string.
     * @details Supports both forward slashes '/' and backslashes '\\'.
     * @param fullPathFileName Full or relative path to process.
     * @return Filename component, or empty string if ends with separator or input empty.
     */
    static std::string ExtractFilename(const std::string &fullPathFileName);
};
/* ========================[  END FILE CONTENT  ]========================
 * Library          : vhlibfsys
 * File             : src/vhfsys.hpp
 * Revision         : 0.1.2
 * Content size     : 2683
 * Date / Time      : 28-10-2026 14:11:58
 * MD5              : 766264515df5a6295596550cb516f215
 * Copyright        : © 2026 Viktor Glebov
 * ====================================================================== */