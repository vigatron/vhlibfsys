#include "vhfsys.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;

/**
 * @brief Checks whether the specified path refers to an existing file.
 * @details Returns true if path exists and is not a directory. Does not require regular file.
 * @param filename Path to the file.
 * @return true if exists and not directory, false otherwise.
 */
bool VHFSYS::FileExist(const std::string &filename) noexcept
{
    try
    {
        std::error_code ec;

        if (!fs::exists(filename, ec) || ec)
            return false;

        if (fs::is_directory(filename, ec) || ec)
            return false;

        return true;
    }
    catch (...)
    {
        return false;
    }
}

/**
 * @brief Checks whether the specified path refers to an existing directory.
 * @param dirpath Path to the directory.
 * @return true if exists and is directory, false otherwise.
 */
bool VHFSYS::DirExist(const std::filesystem::path &dirpath) noexcept
{
    try
    {
        if (dirpath.empty())
            return false;

        std::error_code ec;

        if (!fs::exists(dirpath, ec) || ec)
            return false;

        return fs::is_directory(dirpath, ec) && !ec;
    }
    catch (...)
    {
        return false;
    }
}

/**
 * @brief Checks whether the specified directory path exists.
 * @param dirpath Directory path as a string.
 * @return true if exists and is directory, false otherwise.
 */
bool VHFSYS::DirExist(const std::string &dirpath) noexcept
{
    try
    {
        return DirExist(fs::path(dirpath));
    }
    catch (...)
    {
        return false;
    }
}

/**
 * @brief Creates a directory and any missing parent directories.
 * @param dirpath Path to the directory to create.
 * @return vok if the directory already exists or is created successfully, error code otherwise.
 */
VHFSYS::verr VHFSYS::CreateNestedDir(const std::string &dirpath) noexcept
{
    try
    {
        if (dirpath.empty())
            return verror(1);

        const fs::path path(dirpath);
        std::error_code ec;

        if (fs::is_directory(path, ec))
            return vok;

        // is_directory() reports ENOENT for a non-existent path; that is not fatal.
        ec.clear();

        if (fs::exists(path, ec))
            return verror(1);

        if (ec)
            return verror(1);

        fs::create_directories(path, ec);
        if (ec)
            return verror(1);

        ec.clear();

        if (fs::is_directory(path, ec))
            return vok;

        return verror(1);
    }
    catch (...)
    {
        return verror(1);
    }
}

/**
 * @brief Removes a directory and all its contents recursively.
 * @param dirpath Path to the directory to remove.
 * @return vok on success, error code otherwise.
 */
VHFSYS::verr VHFSYS::RemoveNestedDir(const std::string &dirpath) noexcept
{
    try
    {
        if (dirpath.empty())
            return verror(1);

        std::error_code ec;
        fs::remove_all(dirpath, ec);

        if (ec)
            return verror(1);

        return vok;
    }
    catch (...)
    {
        return verror(1);
    }
}

/**
 * @brief Reads the entire contents of a file into a byte vector.
 * @param filename Path to the input file.
 * @return Vector with file data, or empty vector on error/empty file.
 */
std::vector<std::uint8_t> VHFSYS::ReadVectorFromFile(const std::string &filename) noexcept
{
    try
    {
        std::vector<std::uint8_t> r;

        std::ifstream in(filename, std::ios::binary | std::ios::ate);
        if (!in)
            return r;

        const std::streamsize size = in.tellg();
        if (size <= 0 ||
            static_cast<std::uint64_t>(size) >
                static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()))
        {
            return r;
        }

        in.seekg(0, std::ios::beg);
        if (!in)
            return r;

        r.resize(static_cast<std::size_t>(size));
        in.read(reinterpret_cast<char *>(r.data()), size);

        if (!in)
            r.clear();

        return r;
    }
    catch (...)
    {
        return {};
    }
}

/**
 * @brief Writes byte vector to a file, creating parent directories if needed.
 * @param filename Path to the output file.
 * @param data Data to write.
 * @return 0 on success, 1 on empty filename/dir creation failure, 2 if cannot open file, 3 on write/close failure.
 */
VHFSYS::verr VHFSYS::WriteVectorToFile(
    const std::string &filename,
    const std::vector<std::uint8_t> &data) noexcept
{
    try
    {
        if (filename.empty())
            return verror(1);

        const fs::path filePath(filename);
        const fs::path dir = filePath.parent_path();

        if (!dir.empty() && !DirExist(dir))
        {
            if (CreateNestedDir(dir.string()) != vok)
                return verror(1);
        }

        if (data.size() >
            static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max()))
        {
            return verror(3);
        }

        std::ofstream out(filePath, std::ios::binary | std::ios::trunc);
        if (!out)
            return verror(2);

        if (!data.empty())
        {
            out.write(reinterpret_cast<const char *>(data.data()),
                      static_cast<std::streamsize>(data.size()));
        }

        if (!out)
            return verror(3);

        out.flush();
        if (!out)
            return verror(3);

        out.close();
        if (!out)
            return verror(3);

        return vok;
    }
    catch (...)
    {
        return verror(3);
    }
}

/**
 * @brief Extracts the filename component from a path string.
 * @details Supports both forward slashes '/' and backslashes '\\'.
 * @param fullPathFileName Full or relative path to process.
 * @return Filename component, or empty string if ends with separator or input empty.
 */
std::string VHFSYS::ExtractFilename(const std::string &fullPathFileName) noexcept
{
    try
    {
        const std::size_t last_slash = fullPathFileName.find_last_of("/\\");
        if (last_slash == std::string::npos)
        {
            return fullPathFileName;
        }

        return fullPathFileName.substr(last_slash + 1);
    }
    catch (...)
    {
        return {};
    }
}