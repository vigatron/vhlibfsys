#include "vhfile.hpp"

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

static void test_default_ctor_and_assign()
{
    VHFile f;
    TEST_ASSERT(f.isclosed());
    TEST_ASSERT(!f.isopen());

    fs::path p = make_temp_path("vhfile_default");
    f.assign(p.string());
    TEST_ASSERT(f.isclosed());
    std::error_code ec;
    fs::remove(p, ec);
}

static void test_ctor_with_filename_and_move()
{
    fs::path p = make_temp_path("vhfile_ctor");
    {
        VHFile f(p.string());
        TEST_ASSERT(f.isclosed());
        TEST_ASSERT(!f.isopen());

        VHFile g(std::move(f));
        TEST_ASSERT(g.isclosed());
        TEST_ASSERT(!g.isopen());
        // moved-from f should be closed
        TEST_ASSERT(f.isclosed());
    }

    VHFile a(make_temp_path("vhfile_move_a").string());
    VHFile b(make_temp_path("vhfile_move_b").string());
    a = std::move(b);
    TEST_ASSERT(a.isclosed());
    TEST_ASSERT(b.isclosed());

    std::error_code ec;
    fs::remove(make_temp_path("vhfile_cleanup"), ec);
    (void)a;
    (void)b;
}

static void test_openfile_createfile_closefile_modes()
{
    fs::path p = make_temp_path("vhfile_modes");
    std::error_code ec;
    fs::remove(p, ec);

    VHFile f(p.string());
    // open non-existent read-only should fail
    TEST_ASSERT(f.openfile() == false);
    TEST_ASSERT(f.isclosed());

    // create/truncate
    TEST_ASSERT(f.createfile() == true);
    TEST_ASSERT(f.isopen());
    TEST_ASSERT(!f.isclosed());
    // idempotent
    TEST_ASSERT(f.createfile() == true);

    // cannot open for read while writable
    TEST_ASSERT(f.openfile() == false);

    TEST_ASSERT(f.closefile() == true);
    TEST_ASSERT(f.isclosed());
    TEST_ASSERT(f.closefile() == false); // already closed

    // now open for read
    TEST_ASSERT(f.openfile() == true);
    TEST_ASSERT(f.isopen());
    // cannot create while read-only
    TEST_ASSERT(f.createfile() == false);
    TEST_ASSERT(f.closefile() == true);

    fs::remove(p, ec);
}

static void test_write_read_seek_pos_filesize()
{
    fs::path p = make_temp_path("vhfile_rw");
    std::error_code ec;
    fs::remove(p, ec);

    VHFile f(p.string());
    const char data[] = "HELLO_VHFILE";
    const size_t dsz = sizeof(data) - 1;

    // write on closed creates file
    TEST_ASSERT(f.write(data, dsz) == true);
    TEST_ASSERT(f.isopen());

    // filesize while writable should flush
    std::uint64_t sz = 0;
    TEST_ASSERT(f.filesize(&sz) == true);
    TEST_ASSERT(sz == static_cast<std::uint64_t>(dsz));

    std::uint64_t cur = 0;
    TEST_ASSERT(f.pos(&cur) == true);

    // write sz == 0
    TEST_ASSERT(f.write(nullptr, 0) == true);

    TEST_ASSERT(f.closefile() == true);

    // read
    VHFile r(p.string());
    char buf[64] = {0};
    TEST_ASSERT(r.read(buf, dsz) == true);
    TEST_ASSERT(std::string(buf, dsz) == std::string(data, dsz));

    // read with seekoffs
    char buf2[6] = {0};
    TEST_ASSERT(r.read(buf2, 5, 6) == true);
    (void)buf2;
    TEST_ASSERT(r.closefile() == true);

    // cannot read if writable - use separate file
    {
        fs::path pw = make_temp_path("vhfile_wonly");
        fs::remove(pw, ec);
        VHFile w2(pw.string());
        TEST_ASSERT(w2.createfile() == true);
        char tmp[4] = {0};
        TEST_ASSERT(w2.read(tmp, 1) == false);
        TEST_ASSERT(w2.closefile() == true);
        fs::remove(pw, ec);
    }

    // seek/pos
    VHFile s(p.string());
    TEST_ASSERT(s.seek(0) == true);
    std::uint64_t ppos = 123;
    TEST_ASSERT(s.pos(&ppos) == true);
    TEST_ASSERT(ppos == 0);
    TEST_ASSERT(s.read(buf, 1) == true);
    TEST_ASSERT(s.closefile() == true);

    fs::remove(p, ec);
}

int main()
{
    test_default_ctor_and_assign();
    test_ctor_with_filename_and_move();
    test_openfile_createfile_closefile_modes();
    test_write_read_seek_pos_filesize();

    if (g_failures == 0)
    {
        return 0;
    }

    std::fprintf(stderr, "Total failures: %d\n", g_failures);
    return 1;
}
