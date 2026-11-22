#include "vhfile.hpp"

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

static_assert(std::is_nothrow_default_constructible<VHFile>::value, "VHFile() must be noexcept");
static_assert(std::is_nothrow_constructible<VHFile, const std::string &>::value, "VHFile(string) must be noexcept");
static_assert(std::is_nothrow_move_constructible<VHFile>::value, "VHFile move constructor must be noexcept");
static_assert(std::is_nothrow_move_assignable<VHFile>::value, "VHFile move assignment must be noexcept");
static_assert(noexcept(std::declval<VHFile &>().assign(std::declval<const std::string &>())), "assign must be noexcept");
static_assert(noexcept(std::declval<VHFile &>().filesize(nullptr)), "filesize must be noexcept");
static_assert(noexcept(std::declval<VHFile &>().pos(nullptr)), "pos must be noexcept");
static_assert(noexcept(std::declval<VHFile &>().seek(0)), "seek must be noexcept");
static_assert(noexcept(std::declval<VHFile &>().read(nullptr, 0)), "read must be noexcept");
static_assert(noexcept(std::declval<VHFile &>().write(nullptr, 0)), "write must be noexcept");
static_assert(noexcept(std::declval<VHFile &>().openfile()), "openfile must be noexcept");
static_assert(noexcept(std::declval<VHFile &>().createfile()), "createfile must be noexcept");
static_assert(noexcept(std::declval<VHFile &>().closefile()), "closefile must be noexcept");
static_assert(noexcept(std::declval<const VHFile &>().isopen()), "isopen must be noexcept");
static_assert(noexcept(std::declval<const VHFile &>().isclosed()), "isclosed must be noexcept");

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

static void test_filesize_closed()
{
    fs::path p = make_temp_path("vhfile_fsize");
    std::error_code ec;
    fs::remove(p, ec);

    const char data[] = "CLOSED_SIZE";
    const size_t dsz = sizeof(data) - 1;

    {
        VHFile w(p.string());
        TEST_ASSERT(w.write(data, dsz) == true);
        TEST_ASSERT(w.closefile() == true);
    }

    // filesize on a closed file must open it read-only and return the size
    VHFile r(p.string());
    TEST_ASSERT(r.isclosed());
    TEST_ASSERT(r.filesize(nullptr) == false);

    std::uint64_t sz = 0;
    TEST_ASSERT(r.filesize(&sz) == true);
    TEST_ASSERT(sz == static_cast<std::uint64_t>(dsz));

    TEST_ASSERT(r.closefile() == true);

    // non-existent file -> false
    VHFile nf((p.parent_path() / "nope_fsize.bin").string());
    std::uint64_t bad = 0;
    TEST_ASSERT(nf.filesize(&bad) == false);

    fs::remove(p, ec);
}

int main()
{
    test_default_ctor_and_assign();
    test_ctor_with_filename_and_move();
    test_openfile_createfile_closefile_modes();
    test_write_read_seek_pos_filesize();
    test_filesize_closed();

    if (g_failures == 0)
    {
        return 0;
    }

    std::fprintf(stderr, "Total failures: %d\n", g_failures);
    return 1;
}
