/* ======================================================================================
 * Library       : vhlibfsys
 * Description   : A small C++17 filesystem helper library with 64-bit binary file I/O.
 * Revision      : 0.1.1
 * Source        : https://github.com/vigatron/vhlibfsys
 * Disclaimer    : Provided "AS IS", without warranty.
 * License       : MIT
 * File          : src/vhfsys.cpp
 * Content size  : 5168
 * Date / Time   : 27-10-2026 22:12:08
 * MD5           : e35da9354ee0ac52863525582e7fef61
 * Notes         : MD5 = file content without header/footer
 * Encoding      : UTF-8
 * Author        : Viktor Glebov / V01G04A81
 * Copyright     : © 2026 Viktor Glebov
 * ========================[ BEGIN FILE CONTENT ]====================================== */
#include <fstream>
#include <filesystem>

#include "vhfsys.hpp"

namespace fs = std::filesystem;

#ifndef VHPLATFORM_INCLUDED
using verr=uint32_t;
#define vok ((verr)0)

verr verror(verr retcode) { return retcode; }
#endif

/**
 * @brief Checks whether the specified path refers to an existing file.
 *
 * @param filename Path to the file.
 *
 * @return true if the path exists and does not refer to a directory;
 *         false otherwise.
 *
 * @note This function does not verify file access permissions.
 */
bool VHFSYS::FileExist(const std::string &filename)
{
    if (!std::filesystem::exists(filename))
        return false;

    if (std::filesystem::is_directory(filename))
        return false;

    return true;
}

/**
 * @brief Checks whether the specified path refers to an existing directory.
 *
 * @param dirpath Path to the directory.
 *
 * @return true if the path refers to an existing directory;
 *         false otherwise.
 */
bool VHFSYS::DirExist(const std::filesystem::path &dirpath)
{
    if (dirpath.empty())
        return false;

    if (!fs::exists(dirpath))
        return false;

    if (!std::filesystem::is_directory(dirpath))
        return false;

    return true;
}

/**
 * @brief Checks whether the specified directory path exists.
 *
 * @param dirpath Directory path as a string.
 *
 * @return true if the path refers to an existing directory;
 *         false otherwise.
 */
bool VHFSYS::DirExist(const std::string &dirpath)
{
    return DirExist(fs::path(dirpath));
}

/**
 * @brief Creates a directory and any missing parent directories.
 *
 * @param dirpath Path to the directory to create.
 *
 * @return vok if the directory already exists or is created successfully;
 *         an error code otherwise.
 *
 * @note Existing parent directories are preserved.
 */
verr VHFSYS::CreateNestedDir(const std::string &dirpath)
{
    if (dirpath.empty())
        return verror(1);

    const fs::path path(dirpath);

    if (fs::is_directory(path))
        return vok;

    if (fs::exists(path))
        return verror(1);

    if (fs::create_directories(path) || fs::is_directory(path))
        return vok;

    return verror(1);
}

/**
 * @brief Reads the entire contents of a file into a byte vector.
 *
 * @param filename Path to the input file.
 *
 * @return A vector containing the file data.
 *         An empty vector is returned if the file is empty
 *         or an error occurs.
 *
 * @note The file is opened in binary mode.
 */
std::vector<uint8_t> VHFSYS::ReadVectorFromFile(const std::string &filename)
{
    std::vector<uint8_t> r;

    // Открываем файл в бинарном режиме с курсором в конце (ate), чтобы узнать размер
    std::ifstream in(filename, std::ios::binary | std::ios::ate);
    if (!in)
    {
        return r;
    }

    // Получаем размер файла
    std::streamsize size = in.tellg();
    if (size <= 0)
    {
        return r;
    }

    // Возвращаем курсор в начало для чтения
    in.seekg(0, std::ios::beg);

    // Выделяем память и считываем данные
    r.resize(size);
    in.read(reinterpret_cast<char *>(r.data()), size);

    // Если при чтении произошла ошибка, очищаем вектор
    if (!in)
    {
        r.clear();
    }

    return r;
}

/**
 * @brief Reads the entire contents of a file into a byte vector.
 *
 * @param filename Path to the input file.
 *
 * @return A vector containing the file data.
 *         An empty vector is returned if the file is empty
 *         or an error occurs.
 *
 * @note The file is opened in binary mode.
 */
verr VHFSYS::WriteVectorToFile(
    const std::string &filename,
    const std::vector<uint8_t> &data)
{
    if (filename.empty())
        return verror(1);

    const fs::path filePath(filename);
    const fs::path dir = filePath.parent_path();

    // Create parent directories only if the path is not empty.
    if (!dir.empty() && !DirExist(dir))
    {
        if (vok != CreateNestedDir(dir.string()))
            return verror(1);
    }

    std::ofstream out(filePath, std::ios::binary | std::ios::trunc);
    if (!out)
        return verror(2);

    if (!data.empty())
    {
        out.write(
            reinterpret_cast<const char *>(data.data()),
            static_cast<std::streamsize>(data.size()));
    }

    if (!out)
        return verror(3);

    out.close();

    if (out.fail())
        return verror(3);

    return vok;
}

/**
 * @brief Extracts the filename component from a path string.
 *
 * Supports both forward slashes ('/') and backslashes ('\\')
 * as path separators.
 *
 * @param fullPathFileName Full or relative path to process.
 *
 * @return The filename component, or an empty string if the path
 *         ends with a separator or the input is empty.
 */
std::string VHFSYS::ExtractFilename(const std::string &fullPathFileName)
{
    size_t last_slash = fullPathFileName.find_last_of("/\\");
    if (last_slash == std::string::npos)
    {
        return fullPathFileName;
    }
    return fullPathFileName.substr(last_slash + 1);
}
/* ========================[  END FILE CONTENT  ]========================
 * Library          : vhlibfsys
 * File             : src/vhfsys.cpp
 * Revision         : 0.1.1
 * Content size     : 5168
 * Date / Time      : 27-10-2026 22:12:08
 * MD5              : e35da9354ee0ac52863525582e7fef61
 * Copyright        : © 2026 Viktor Glebov
 * ====================================================================== */