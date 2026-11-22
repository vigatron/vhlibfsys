#include "vhfsys.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

static_assert(noexcept(VHFSYS::FileExist(std::declval<const std::string &>())), "FileExist must be noexcept");
static_assert(noexcept(VHFSYS::DirExist(std::declval<const std::filesystem::path &>())), "DirExist(path) must be noexcept");
static_assert(noexcept(VHFSYS::DirExist(std::declval<const std::string &>())), "DirExist(string) must be noexcept");
static_assert(noexcept(VHFSYS::CreateNestedDir(std::declval<const std::string &>())), "CreateNestedDir must be noexcept");
static_assert(noexcept(VHFSYS::RemoveNestedDir(std::declval<const std::string &>())), "RemoveNestedDir must be noexcept");
static_assert(noexcept(VHFSYS::ReadVectorFromFile(std::declval<const std::string &>())), "ReadVectorFromFile must be noexcept");
static_assert(noexcept(VHFSYS::WriteVectorToFile(std::declval<const std::string &>(),
                                                 std::declval<const std::vector<std::uint8_t> &>())), "WriteVectorToFile must be noexcept");
static_assert(noexcept(VHFSYS::ExtractFilename(std::declval<const std::string &>())), "ExtractFilename must be noexcept");
static_assert(noexcept(VHFSYS::verror(0)), "verror must be noexcept");
static_assert(VHFSYS::vok == 0, "vok must be 0");

static int g_failures = 0;

#define TEST_ASSERT(cond)                                                                                              \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(cond))                                                                                                   \
        {                                                                                                              \
            ++g_failures;                                                                                              \
            std::fprintf(stderr, "TEST FAILED: %s:%d: %s\n", __FILE__, __LINE__, #cond);                              \
        }                                                                                                              \
    } while (0)

static fs::path make_temp_path(const std::string &prefix)
{
    const fs::path dir = fs::temp_directory_path();
    std::srand(static_cast<unsigned>(std::time(nullptr)) ^ static_cast<unsigned>(std::clock()));
    const unsigned rnd = static_cast<unsigned>(std::rand());
    fs::path p = dir / (prefix + "_" + std::to_string(rnd) + "_" + std::to_string(std::clock()));
    return p;
}

static void test_fileexist_direxist()
{
    fs::path fpath = make_temp_path("vhfsys_file");
    fs::path dpath = make_temp_path("vhfsys_dir");
    std::error_code ec;
    fs::remove(fpath, ec);
    fs::remove_all(dpath, ec);

    TEST_ASSERT(VHFSYS::FileExist(fpath.string()) == false);
    TEST_ASSERT(VHFSYS::DirExist(dpath.string()) == false);
    TEST_ASSERT(VHFSYS::DirExist(dpath) == false);

    // create dir
    TEST_ASSERT(fs::create_directories(dpath, ec));
    TEST_ASSERT(VHFSYS::DirExist(dpath.string()) == true);
    TEST_ASSERT(VHFSYS::DirExist(dpath) == true);
    // dir is not a file
    TEST_ASSERT(VHFSYS::FileExist(dpath.string()) == false);

    // create file
    {
        std::ofstream out(fpath, std::ios::binary);
        out << "abc";
    }
    TEST_ASSERT(VHFSYS::FileExist(fpath.string()) == true);
    TEST_ASSERT(VHFSYS::DirExist(fpath.string()) == false);

    fs::remove(fpath, ec);
    fs::remove_all(dpath, ec);
}

static void test_createnesteddir()
{
    fs::path base = make_temp_path("vhfsys_nested");
    fs::path nested = base / "a" / "b" / "c";
    std::error_code ec;
    fs::remove_all(base, ec);

    // empty path
    TEST_ASSERT(VHFSYS::CreateNestedDir(std::string("")) == 1);

    // create nested
    TEST_ASSERT(VHFSYS::CreateNestedDir(nested.string()) == 0);
    TEST_ASSERT(VHFSYS::DirExist(nested.string()) == true);
    // idempotent
    TEST_ASSERT(VHFSYS::CreateNestedDir(nested.string()) == 0);

    // path exists as file
    fs::path fpath = base / "afile";
    {
        std::ofstream out(fpath);
        out << "x";
    }
    TEST_ASSERT(VHFSYS::CreateNestedDir(fpath.string()) == 1);

    fs::remove_all(base, ec);
}

static void test_removenesteddir()
{
    fs::path base = make_temp_path("vhfsys_remove");
    fs::path nested = base / "a" / "b";
    std::error_code ec;
    fs::remove_all(base, ec);

    // Empty path is rejected.
    TEST_ASSERT(VHFSYS::RemoveNestedDir(std::string("")) == 1);

    // Filesystem roots and the current/parent directory are rejected before any FS access.
    TEST_ASSERT(VHFSYS::RemoveNestedDir(std::string("/")) == 1);
    TEST_ASSERT(VHFSYS::RemoveNestedDir(std::string(".")) == 1);
    TEST_ASSERT(VHFSYS::RemoveNestedDir(std::string("..")) == 1);
    TEST_ASSERT(VHFSYS::RemoveNestedDir(std::string("/..")) == 1);

    // Strict mode: any ".." component in the original path is rejected.
    TEST_ASSERT(VHFSYS::RemoveNestedDir((base / ".." / "x").string()) == 1);
    TEST_ASSERT(VHFSYS::RemoveNestedDir(std::string("a/../b")) == 1);

#if defined(_WIN32)
    TEST_ASSERT(VHFSYS::RemoveNestedDir(std::string("C:\\")) == 1);
    TEST_ASSERT(VHFSYS::RemoveNestedDir(std::string("C:")) == 1);
#endif

    // Refuse to remove the current working directory.
    {
        const fs::path old_cwd = fs::current_path(ec);
        TEST_ASSERT(!ec);
        fs::create_directories(base, ec);
        TEST_ASSERT(!ec);
        fs::current_path(base, ec);
        TEST_ASSERT(!ec);
        TEST_ASSERT(VHFSYS::RemoveNestedDir(base.string()) == 1);
        fs::current_path(old_cwd, ec);
        TEST_ASSERT(!ec);
    }

    // Missing path is idempotent success.
    TEST_ASSERT(VHFSYS::RemoveNestedDir((base / "missing").string()) == 0);

    // Existing nested directory is removed recursively.
    fs::create_directories(nested, ec);
    TEST_ASSERT(VHFSYS::DirExist(nested.string()) == true);
    TEST_ASSERT(VHFSYS::RemoveNestedDir(base.string()) == 0);
    TEST_ASSERT(fs::exists(base) == false);
}

static void test_writevector_errors()
{
    fs::path dir = make_temp_path("vhfsys_write_errors");
    std::error_code ec;
    fs::remove_all(dir, ec);
    fs::create_directories(dir, ec);

    // Empty filename.
    TEST_ASSERT(VHFSYS::WriteVectorToFile(std::string(""),
                                          std::vector<std::uint8_t>{0x01}) == 1);

    // Parent path is a regular file, not a directory.
    fs::path filePath = dir / "afile";
    {
        std::ofstream out(filePath);
        out << "x";
    }
    TEST_ASSERT(VHFSYS::WriteVectorToFile((filePath / "child").string(),
                                          std::vector<std::uint8_t>{0x01}) == 1);

    // Output path is an existing directory; opening it for writing must fail.
    TEST_ASSERT(VHFSYS::WriteVectorToFile(dir.string(),
                                          std::vector<std::uint8_t>{0x01}) == 2);

#if defined(__linux__)
    // Linux-only check: /dev/full always fails writes with ENOSPC.
    TEST_ASSERT(VHFSYS::WriteVectorToFile(std::string("/dev/full"),
                                          std::vector<std::uint8_t>{0x01}) == 3);
#endif

    fs::remove_all(dir, ec);
}

static void test_readvector_writevector()
{
    fs::path dir = make_temp_path("vhfsys_vecdir");
    fs::path fpath = dir / "data.bin";
    std::error_code ec;
    fs::remove_all(dir, ec);

    // write to non-existent dir - should create
    std::vector<uint8_t> data = {0x00, 0x01, 0x02, 0xFF};
    VHFSYS::verr rc = VHFSYS::WriteVectorToFile(fpath.string(), data);
    TEST_ASSERT(rc == 0);
    TEST_ASSERT(VHFSYS::FileExist(fpath.string()) == true);

    std::vector<uint8_t> rd = VHFSYS::ReadVectorFromFile(fpath.string());
    TEST_ASSERT(rd.size() == data.size());
    for (size_t i = 0; i < data.size(); ++i)
    {
        TEST_ASSERT(rd[i] == data[i]);
    }

    // empty data
    fs::path fempty = dir / "empty.bin";
    TEST_ASSERT(VHFSYS::WriteVectorToFile(fempty.string(), std::vector<uint8_t>()) == 0);
    std::vector<uint8_t> rdem = VHFSYS::ReadVectorFromFile(fempty.string());
    TEST_ASSERT(rdem.empty());

    // read non-existent
    std::vector<uint8_t> rdnf = VHFSYS::ReadVectorFromFile((dir / "nope.bin").string());
    TEST_ASSERT(rdnf.empty());

    fs::remove_all(dir, ec);
}

static void test_extractfilename()
{
    TEST_ASSERT(VHFSYS::ExtractFilename("/home/user/file.bin") == "file.bin");
    TEST_ASSERT(VHFSYS::ExtractFilename("C:\\data\\model.gguf") == "model.gguf");
    TEST_ASSERT(VHFSYS::ExtractFilename("file.bin") == "file.bin");
    TEST_ASSERT(VHFSYS::ExtractFilename("/home/user/") == "");
    TEST_ASSERT(VHFSYS::ExtractFilename("") == "");
    TEST_ASSERT(VHFSYS::ExtractFilename("C:\\") == "");
    TEST_ASSERT(VHFSYS::ExtractFilename("a/b\\c.txt") == "c.txt");
}

int main()
{
    test_fileexist_direxist();
    test_createnesteddir();
    test_removenesteddir();
    test_writevector_errors();
    test_readvector_writevector();
    test_extractfilename();

    if (g_failures == 0)
    {
        return 0;
    }

    std::fprintf(stderr, "Total failures: %d\n", g_failures);
    return 1;
}
