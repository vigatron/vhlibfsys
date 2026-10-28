#include "vhfsys.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

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
    test_readvector_writevector();
    test_extractfilename();

    if (g_failures == 0)
    {
        return 0;
    }

    std::fprintf(stderr, "Total failures: %d\n", g_failures);
    return 1;
}
