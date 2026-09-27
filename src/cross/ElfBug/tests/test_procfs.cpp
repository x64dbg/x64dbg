#include <catch2/catch_test_macros.hpp>
#include <ElfBug/process/ProcFs.h>
#include <sys/prctl.h>
#include <unistd.h>
#include <filesystem>
#include <string>

using namespace ElfBug;

TEST_CASE("ParseNumber accepts only a whole number", "[procfs]")
{
    REQUIRE(procfs::ParseNumber<int>(" 42\n") == 42);
    REQUIRE(procfs::ParseNumber<int>("-5") == -5);
    REQUIRE(procfs::ParseNumber<uint64_t>("7ffd1000", 16) == 0x7ffd1000u);
    REQUIRE_FALSE(procfs::ParseNumber<int>("42x").has_value());
    REQUIRE_FALSE(procfs::ParseNumber<int>("").has_value());
    REQUIRE_FALSE(procfs::ParseNumber<uint8_t>("300").has_value());
}

TEST_CASE("Split drops empty pieces", "[procfs]")
{
    REQUIRE(procfs::Split("a\n\nb\n", '\n') == std::vector<std::string_view> {"a", "b"});
    REQUIRE(procfs::Split("", '\n').empty());
}

TEST_CASE("FindValue matches a key only at the start of a line", "[procfs]")
{
    constexpr std::string_view status = "Name:\tcat\nPPid:\t1\nTracerPid:\t0\nPid:\t42\n";
    REQUIRE(procfs::FindValue(status, "Pid:") == "42");
    REQUIRE(procfs::FindValue(status, "PPid:") == "1");
    REQUIRE(procfs::FindValue(status, "TracerPid:") == "0");
    REQUIRE(procfs::FindValue(status, "Tgid:").empty());
}

TEST_CASE("StatFields numbers fields like the man page despite a hostile comm", "[procfs]")
{
    std::string stat = "1234 (a) b (c) S";
    for(size_t field = 4; field <= procfs::kStatPolicy; field++)
        stat += " " + std::to_string(field * 10);
    stat += "\n";

    const auto fields = procfs::StatFields(stat);
    REQUIRE(fields.size() == procfs::kStatPolicy + 1);
    REQUIRE(fields[1] == "1234");
    REQUIRE(fields[2] == "a) b (c");
    REQUIRE(fields[3] == "S");
    REQUIRE(fields[procfs::kStatUtime] == "140");
    REQUIRE(fields[procfs::kStatPolicy] == "410");
    REQUIRE(procfs::StatFields("no parentheses").empty());
}

TEST_CASE("ParseMapsLine reads file-backed, anonymous and pseudo mappings", "[procfs]")
{
    const auto file = procfs::ParseMapsLine("55d0c8a00000-55d0c8a02000 r-xp 00002000 08:01 1234567                    /usr/bin/cat");
    REQUIRE(file.has_value());
    REQUIRE(file->start == 0x55d0c8a00000u);
    REQUIRE(file->end == 0x55d0c8a02000u);
    REQUIRE(file->offset == 0x2000u);
    REQUIRE(file->perms == "r-xp");
    REQUIRE(file->path == "/usr/bin/cat");

    const auto anonymous = procfs::ParseMapsLine("7f0000000000-7f0000001000 rw-p 00000000 00:00 0");
    REQUIRE(anonymous.has_value());
    REQUIRE(anonymous->path.empty());

    const auto heap = procfs::ParseMapsLine("55d0c9000000-55d0c9021000 rw-p 00000000 00:00 0                          [heap]");
    REQUIRE(heap.has_value());
    REQUIRE(heap->path == "[heap]");

    REQUIRE_FALSE(procfs::ParseMapsLine("not a maps line").has_value());
}

TEST_CASE("ParseMapsLine keeps long paths and spaces and strips (deleted)", "[procfs]")
{
    const std::string prefix = "7f0000000000-7f0000001000 r-xp 00000000 08:01 42   ";
    const std::string longPath = "/" + std::string(600, 'd') + "/lib.so";

    REQUIRE(procfs::ParseMapsLine(prefix + longPath)->path == longPath);
    REQUIRE(procfs::ParseMapsLine(prefix + "/opt/my app/lib.so")->path == "/opt/my app/lib.so");
    REQUIRE(procfs::ParseMapsLine(prefix + "/tmp/gone.so (deleted)")->path == "/tmp/gone.so");
}

TEST_CASE("ReadFile and ReadLine read /proc and return empty for unreadable files", "[procfs]")
{
    REQUIRE(procfs::Path(12, "maps") == "/proc/12/maps");
    REQUIRE(procfs::TaskPath(12, 13, "stat") == "/proc/12/task/13/stat");

    char comm[16] = {};
    REQUIRE(prctl(PR_GET_NAME, comm) == 0);
    REQUIRE(procfs::ReadLine(procfs::Path(getpid(), "comm")) == comm);
    REQUIRE(procfs::ReadFile("/proc/self/exe").size() == std::filesystem::file_size("/proc/self/exe"));
    REQUIRE(procfs::ReadFile("/nonexistent/elfbug").empty());
}
