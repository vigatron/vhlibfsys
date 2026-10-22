#include "vhplatform.hpp"

#include <fstream>

#include "vhfsys.hpp"

namespace fs = std::filesystem;

/**
 * 
 */
bool VHFSYS::DirExist(const std::filesystem::path &dirpath) {
    if (dirpath.empty() || !fs::exists(dirpath)) {
        return false;
    }
    return true;
}

/**
 * 
 */
bool VHFSYS::DirExist(const std::string &dirpath) {
    return DirExist(fs::path(dirpath));
}

/**
 * 
 */
verr VHFSYS::CreateNestedDir(const std::string &dirpath) {

    // Already exist ?
    if (DirExist(dirpath))
        return vok;

    // Failed to create directory ?
    if (!fs::create_directories(dirpath)) {
        return verror(1);
    }

    // Success
    return vok;
}

/**
 * Реализация чтения файла в вектор байт
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
 * 
 */
verr VHFSYS::WriteVectorToFile(const std::string &filename, const std::vector<uint8_t> &data) {

    fs::path filePath(filename);
    fs::path dir = filePath.parent_path();

    // Create directory if it does not exist
    if (!DirExist(dir)) {
        if (vok != CreateNestedDir(dir))
            return verror(1);
    }

    // Open file in binary mode
    std::ofstream out(filePath, std::ios::binary);
    if (!out) {
        // Failed to open file
        return verror(2);
    }

    // Write data to file
    out.write(reinterpret_cast<const char *>(data.data()),
              static_cast<std::streamsize>(data.size()));
    if (!out) {
        // Write error
        return verror(3);
    }

    return vok;
}
