#include "vhfsys.hpp"

#include <fstream>
#include <filesystem>
#include <system_error>

namespace fs = std::filesystem;

/**
 * @brief Checks whether the specified path refers to an existing file.
 * @details Returns true if path exists and is not a directory. Does not require regular file.
 * @param filename Path to the file.
 * @return true if exists and not directory, false otherwise.
 */
bool VHFSYS::FileExist(const std::string &filename)
{
    if (!fs::exists(filename))
        return false;

    if (fs::is_directory(filename))
        return false;

    return true;
}

/**
 * @brief Checks whether the specified path refers to an existing directory.
 * @param dirpath Path to the directory.
 * @return true if exists and is directory, false otherwise.
 */
bool VHFSYS::DirExist(const std::filesystem::path &dirpath) {
    if (dirpath.empty() || !fs::exists(dirpath)) {
        return false;
    }
    return fs::is_directory(dirpath);
}

/**
 * @brief Checks whether the specified directory path exists.
 * @param dirpath Directory path as a string.
 * @return true if exists and is directory, false otherwise.
 */
bool VHFSYS::DirExist(const std::string &dirpath) {
    return DirExist(fs::path(dirpath));
}

/**
 * @brief Creates a directory and any missing parent directories.
 * @param dirpath Path to the directory to create.
 * @return vok if the directory already exists or is created successfully, error code otherwise.
 */
VHFSYS::verr VHFSYS::CreateNestedDir(const std::string &dirpath) {
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
 * @brief Removes a directory and all its contents recursively.
 * @param dirpath Path to the directory to remove.
 * @return vok on success, error code otherwise.
 */
VHFSYS::verr VHFSYS::RemoveNestedDir(const std::string &dirpath)
{
    std::error_code ec;

    fs::remove_all(dirpath, ec);

    if (ec)
    {
        return verror(1);
    }

    return vok;
}


/**
 * @brief Reads the entire contents of a file into a byte vector.
 * @param filename Path to the input file.
 * @return Vector with file data, or empty vector on error/empty file.
 */
std::vector<uint8_t> VHFSYS::ReadVectorFromFile(const std::string &filename) {
    std::vector<uint8_t> r;
    
    // Открываем файл в бинарном режиме с курсором в конце (ate), чтобы узнать размер
    std::ifstream in(filename, std::ios::binary | std::ios::ate);
    if (!in) {
        return r;
    }

    // Получаем размер файла
    std::streamsize size = in.tellg();
    if (size <= 0) {
        return r;
    }

    // Возвращаем курсор в начало для чтения
    in.seekg(0, std::ios::beg);

    // Выделяем память и считываем данные
    r.resize(size);
    in.read(reinterpret_cast<char *>(r.data()), size);

    // Если при чтении произошла ошибка, очищаем вектор
    if (!in) {
        r.clear();
    }

    return r;
}

/**
 * @brief Writes byte vector to a file, creating parent directories if needed.
 * @param filename Path to the output file.
 * @param data Data to write.
 * @return 0 on success, 1 on empty filename/dir creation failure, 2 if cannot open file, 3 on write failure.
 */
VHFSYS::verr VHFSYS::WriteVectorToFile(const std::string &filename, const std::vector<uint8_t> &data) {

    if (filename.empty())
        return verror(1);

    fs::path filePath(filename);
    fs::path dir = filePath.parent_path();

    // Create parent directories only if the path is not empty.
    if (!dir.empty() && !DirExist(dir)) {
        if (vok != CreateNestedDir(dir.string()))
            return verror(1);
    }

    // Open file in binary mode (truncate)
    std::ofstream out(filePath, std::ios::binary | std::ios::trunc);
    if (!out) {
        // Failed to open file
        return verror(2);
    }

    // Write data to file
    if (!data.empty()) {
        out.write(reinterpret_cast<const char *>(data.data()),
                  static_cast<std::streamsize>(data.size()));
    }
    if (!out) {
        // Write error
        return verror(3);
    }

    return vok;
}

/**
 * @brief Extracts the filename component from a path string.
 * @details Supports both forward slashes '/' and backslashes '\\'.
 * @param fullPathFileName Full or relative path to process.
 * @return Filename component, or empty string if ends with separator or input empty.
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
