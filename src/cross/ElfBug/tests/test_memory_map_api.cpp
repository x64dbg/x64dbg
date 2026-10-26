#include "ApiSession.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <climits>
#include <filesystem>
#include <thread>
#include "targets/TargetUtil.h"

namespace
{
    using namespace ElfBug;
    using namespace ElfBug::test;

    const ElfBugMemoryPage* HeaderRow(const std::vector<ElfBugMemoryPage> & pages, const std::string & name)
    {
        for(const auto & page : pages)
        {
            if(page.base == page.module_base && std::string(page.info).starts_with(name))
                return &page;
        }
        return nullptr;
    }

    bool HasSectionRow(const std::vector<ElfBugMemoryPage> & pages, const uint64_t moduleBase, const std::string & name)
    {
        return std::any_of(pages.begin(), pages.end(), [&](const ElfBugMemoryPage & page)
        {
            return page.module_base == moduleBase && name == page.section;
        });
    }

    struct Row
    {
        uint64_t base = 0;
        uint64_t size = 0;
        ElfBugRegionType type = ElfBugRegionType_Private;
        std::string perms;
        std::string section;
        std::string info;

        bool operator==(const Row &) const = default;
    };

    std::vector<Row> RowsOf(const std::vector<ElfBugMemoryPage> & pages, const uint64_t moduleBase)
    {
        std::vector<Row> rows;
        for(const auto & page : pages)
        {
            if(page.module_base == moduleBase)
                rows.push_back({page.base, page.size, page.type, page.perms, page.section, page.info});
        }
        return rows;
    }

    template<typename T>
    std::optional<T> TryRead(const ApiSession & s, const uint64_t address)
    {
        T value{};
        if(!ElfBugMemRead(s.dbg, address, &value, sizeof(value)))
            return std::nullopt;
        return value;
    }

    template<typename T>
    T Read(const ApiSession & s, const uint64_t address)
    {
        const auto value = TryRead<T>(s, address);
        REQUIRE(value.has_value());
        return *value;
    }

    ptr BssEnd(ApiSession & s)
    {
        return s.Symbol("mm_bss") + kMemMapBssSize - 1;
    }

    // Runs mem_map_target until its worker has published, then pauses it.
    void RunToParked(ApiSession & s)
    {
        REQUIRE(s.Started());
        REQUIRE(s.WaitForSystemBreakpoint());
        const ptr workerTid = s.Symbol("mm_worker_tid");
        ElfBugContinue(s.dbg);
        REQUIRE(WaitUntil([&] { return TryRead<int>(s, workerTid).value_or(0) != 0; }));
        REQUIRE(s.Pause());
    }

    // Writes 1 to one of a paused mem_map_target's flags, leaves it running and waits for it to answer done.
    void Request(ApiSession & s, const std::string & name)
    {
        const ptr flag = s.Symbol(name);
        const int request = 1;
        REQUIRE(ElfBugMemWrite(s.dbg, flag, &request, sizeof(request)));
        ElfBugContinue(s.dbg);
        REQUIRE(WaitUntil([&] { return TryRead<int>(s, flag).value_or(request) != request; }));
        REQUIRE(Read<int>(s, flag) == request + 1);
    }
}

TEST_CASE("C API exports the memory map with labels from the threads' registers", "[memorymap][api]")
{
    ApiSession s(FIXTURE("mem_map_target"));
    RunToParked(s);
    const std::string pid = std::to_string(ElfBugGetPid(s.dbg));
    const std::string tid = std::to_string(Read<int>(s, s.Symbol("mm_worker_tid")));

    const auto pages = ElfBugMemoryMap(s.dbg, false);
    const std::string mainStack = RequirePage(pages, Read<uintptr_t>(s, s.Symbol("mm_main_stack"))).info;
    REQUIRE(mainStack.find("Stack (" + pid + ")") != std::string::npos);
    const uintptr_t workerStack = Read<uintptr_t>(s, s.Symbol("mm_worker_stack"));
    REQUIRE(std::string(RequirePage(pages, workerStack).info) == "TLS (" + tid + "), Stack (" + tid + ")");

    const auto threads = ElfBugThreadList(s.dbg);
    const auto main = std::find_if(threads.begin(), threads.end(), [](const ElfBugThreadInfo & thread) { return thread.number == 0; });
    REQUIRE(main != threads.end());
    REQUIRE(main->fs_base != 0);
    for(const bool sectionView : {false, true})
    {
        CAPTURE(sectionView);
        const auto rows = ElfBugMemoryMap(s.dbg, sectionView);
        REQUIRE(std::string(RequirePage(rows, main->fs_base).info).find("TLS (" + pid + ")") != std::string::npos);
    }
}

TEST_CASE("C API export returns the full count and fills only up to capacity", "[memorymap][api]")
{
    ApiSession s(FIXTURE("mem_map_target"));
    RunToParked(s);
    const auto all = ElfBugMemoryMap(s.dbg, false);
    REQUIRE(all.size() > 1);
    REQUIRE(ElfBugGetMemoryMap(s.dbg, false, nullptr, 0) == all.size());

    ElfBugMemoryPage rows[2] = {};
    rows[1].base = 0xdead;
    REQUIRE(ElfBugGetMemoryMap(s.dbg, false, rows, 1) == all.size());
    REQUIRE(rows[0].base == all[0].base);
    REQUIRE(std::string(rows[0].info) == all[0].info);
    REQUIRE(rows[1].base == 0xdead);
}

TEST_CASE("C API vector helper grows past its first guess at the row count", "[memorymap][api]")
{
    ApiSession s(FIXTURE("mem_map_target"));
    RunToParked(s);
    Request(s, "mm_split_many");
    REQUIRE(s.Pause());

    const auto all = ElfBugMemoryMap(s.dbg, false);
    REQUIRE(all.size() > kMemMapManyRegions);
    REQUIRE(ElfBugGetMemoryMap(s.dbg, false, nullptr, 0) == all.size());
    const uintptr_t many = Read<uintptr_t>(s, s.Symbol("mm_many"));
    for(size_t i = 0; i < kMemMapManyRegions; ++i)
    {
        const ElfBugMemoryPage & page = RequirePage(all, many + i * 4096);
        REQUIRE(page.base == many + i * 4096);
        REQUIRE(page.size == 4096);
    }
}

TEST_CASE("C API rows carry section names, parties and module bases", "[memorymap][api]")
{
    ApiSession s(FIXTURE("mem_map_target"));
    RunToParked(s);
    const ptr code = s.Symbol("mm_code");
    const ptr bssEnd = BssEnd(s);

    uint64_t exeBase = 0;
    REQUIRE(ElfBugModBaseFromAddr(s.dbg, code, &exeBase));
    uint64_t tailBase = 0;
    REQUIRE(ElfBugModBaseFromAddr(s.dbg, bssEnd, &tailBase));
    REQUIRE(tailBase == exeBase);
    char name[64] = {};
    REQUIRE(ElfBugModNameFromAddr(s.dbg, bssEnd, name, sizeof(name), false));
    REQUIRE(std::string(name) == "mem_map_target");

    const auto regions = ElfBugMemoryMap(s.dbg, false);
    const ElfBugMemoryPage & codeRow = RequirePage(regions, code);
    REQUIRE(std::string(codeRow.section) == ".text");
    REQUIRE(codeRow.type == ElfBugRegionType_Image);
    REQUIRE(codeRow.party == ElfBugParty_User);
    REQUIRE(codeRow.module_base == exeBase);

    const ElfBugMemoryPage & tail = RequirePage(regions, bssEnd);
    REQUIRE(tail.type == ElfBugRegionType_Image);
    REQUIRE(tail.module_base == exeBase);
    REQUIRE(std::string(tail.info).find("\".bss\"") != std::string::npos);

    const auto pieces = ElfBugMemoryMap(s.dbg, true);
    REQUIRE(std::string(RequirePage(pieces, code).info) == " \".text\"");
    uint64_t base = 0;
    uint64_t size = 0;
    REQUIRE(ElfBugMemFindBaseAddr(s.dbg, code, &base, &size));
    REQUIRE(base == codeRow.base);
    REQUIRE(size == codeRow.size);
}

TEST_CASE("C API reads the map again while the target runs", "[memorymap][api]")
{
    ApiSession s(FIXTURE("mem_map_target"));
    RunToParked(s);
    REQUIRE(HeaderRow(ElfBugMemoryMap(s.dbg, false), "libmm_gap.so") == nullptr);

    Request(s, "mm_load_gap");
    REQUIRE_FALSE(ElfBugIsPaused(s.dbg));
    const auto running = ElfBugMemoryMap(s.dbg, false);
    const ElfBugMemoryPage* gap = HeaderRow(running, "libmm_gap.so");
    REQUIRE(gap);
    uint64_t base = 0;
    REQUIRE(ElfBugModBaseFromAddr(s.dbg, gap->base, &base));
    REQUIRE(base == gap->module_base);
}

TEST_CASE("C API picks up a library loaded between stops and leaves the other modules' rows alone", "[memorymap][api]")
{
    ApiSession s(FIXTURE("mem_map_target"));
    RunToParked(s);
    uint64_t exeBase = 0;
    REQUIRE(ElfBugModBaseFromAddr(s.dbg, s.Symbol("mm_code"), &exeBase));

    const auto before = ElfBugMemoryMap(s.dbg, false);
    const ElfBugMemoryPage* libc = HeaderRow(before, "libc.so.6");
    REQUIRE(libc);
    const uint64_t libcBase = libc->module_base;
    const auto exeRows = RowsOf(before, exeBase);
    const auto libcRows = RowsOf(before, libcBase);

    Request(s, "mm_load_gap");
    REQUIRE(s.Pause());

    const auto after = ElfBugMemoryMap(s.dbg, false);
    REQUIRE(RowsOf(after, exeBase) == exeRows);
    REQUIRE(RowsOf(after, libcBase) == libcRows);

    const ElfBugMemoryPage* gap = HeaderRow(after, "libmm_gap.so");
    REQUIRE(gap);
    const uint64_t gapBase = gap->module_base;
    REQUIRE(HasSectionRow(ElfBugMemoryMap(s.dbg, true), gapBase, ".text"));
    const auto hole = std::find_if(after.begin(), after.end(), [&](const ElfBugMemoryPage & page)
    {
        return page.module_base == gapBase && std::string(page.perms) == "---p";
    });
    REQUIRE(hole != after.end());
    REQUIRE(std::string(hole->info) == "Reserved");
    REQUIRE(hole->type == ElfBugRegionType_Image);
    uint64_t holeBase = 0;
    REQUIRE(ElfBugModBaseFromAddr(s.dbg, hole->base, &holeBase));
    REQUIRE(holeBase == gapBase);
}

TEST_CASE("C API keeps the mapped files' section names when the files on disk are replaced or deleted", "[memorymap][api]")
{
    namespace fs = std::filesystem;
    const fs::path dir = fs::path(ELFBUG_TESTS_TARGETS_DIR) / "elfbug_replaced";
    const RemoveOnExit cleanup{dir};
    fs::create_directories(dir);
    const fs::path target = dir / "replaced_target";
    const fs::path library = dir / "libmm_gap.so";
    fs::copy_file(FIXTURE("mem_map_target"), target, fs::copy_options::overwrite_existing);
    fs::copy_file(FIXTURE("libmm_gap.so"), library, fs::copy_options::overwrite_existing);

    ApiSession s(target.string());
    RunToParked(s);
    const ptr bssEnd = BssEnd(s);
    Request(s, "mm_load_gap");
    REQUIRE(s.Pause());

    SECTION("both files were replaced on disk")
    {
        fs::copy_file(FIXTURE("hello_elfbug"), dir / "impostor", fs::copy_options::overwrite_existing);
        fs::rename(dir / "impostor", target);
        fs::copy_file(FIXTURE("hello_elfbug"), dir / "impostor", fs::copy_options::overwrite_existing);
        fs::rename(dir / "impostor", library);
    }
    SECTION("both files were deleted")
    {
        fs::remove(target);
        fs::remove(library);
    }

    uint64_t exeBase = 0;
    REQUIRE(ElfBugModBaseFromAddr(s.dbg, bssEnd, &exeBase));
    const auto pages = ElfBugMemoryMap(s.dbg, false);
    REQUIRE(std::string(RequirePage(pages, exeBase).info).starts_with("replaced_target"));
    const ElfBugMemoryPage & tail = RequirePage(pages, bssEnd);
    REQUIRE(tail.type == ElfBugRegionType_Image);
    REQUIRE(tail.module_base == exeBase);

    const auto pieces = ElfBugMemoryMap(s.dbg, true);
    REQUIRE(HasSectionRow(pieces, exeBase, ".text"));
    const ElfBugMemoryPage* gap = HeaderRow(pages, "libmm_gap.so");
    REQUIRE(gap);
    REQUIRE(HasSectionRow(pieces, gap->module_base, ".text") == MapFilesOpenable());
}

TEST_CASE("C API returns no memory map once the session is over", "[memorymap][api]")
{
    REQUIRE(ElfBugMemoryMap(nullptr, false).empty());

    SECTION("the process exits")
    {
        ApiSession s(FIXTURE("segfault"));
        REQUIRE(s.Started());
        REQUIRE(s.WaitForSystemBreakpoint());
        const ptr site = s.Symbol("sf_fault_site");
        REQUIRE_FALSE(ElfBugMemoryMap(s.dbg, false).empty());

        ElfBugContinue(s.dbg);
        REQUIRE(s.events.WaitFor([&] { return s.events.exceptionSignal.has_value(); }));
        ElfBugContinue(s.dbg);
        REQUIRE(s.WaitForExit());

        REQUIRE(ElfBugMemoryMap(s.dbg, false).empty());
        uint64_t base = 0;
        REQUIRE_FALSE(ElfBugModBaseFromAddr(s.dbg, site, &base));
    }
    SECTION("the debugger detaches")
    {
        ApiSession s(FIXTURE("run_endlessly"));
        REQUIRE(s.Started());
        REQUIRE(s.WaitForSystemBreakpoint());
        REQUIRE_FALSE(ElfBugMemoryMap(s.dbg, false).empty());
        const pid_t pid = ElfBugGetPid(s.dbg);

        ElfBugDetach(s.dbg);
        REQUIRE(s.WaitForDetach());
        const ProcessReaper reaper(pid);
        REQUIRE(ElfBugMemoryMap(s.dbg, false).empty());
    }
}

TEST_CASE("C API builds the memory map of an attached process", "[memorymap][api][attach]")
{
    const std::string path = FIXTURE("mem_map_target");
    UntracedProcess target(path);
    REQUIRE(WaitForExeced(target.pid, path));
    const auto workerTid = ResolveRuntimeAddress(path, target.pid, "mm_worker_tid");
    REQUIRE(workerTid.has_value());
    REQUIRE(WaitUntil([&] { return ReadTraceeValue<int>(target.pid, *workerTid).value_or(0) != 0; }));

    ApiSession s(path, target.pid);
    REQUIRE(s.Started());
    REQUIRE(s.WaitForAttachBreakpoint());

    const auto pages = ElfBugMemoryMap(s.dbg, false);
    REQUIRE(std::string(RequirePage(pages, s.Symbol("mm_code")).section) == ".text");
    const std::string tid = std::to_string(Read<int>(s, *workerTid));
    const uintptr_t workerStack = Read<uintptr_t>(s, s.Symbol("mm_worker_stack"));
    REQUIRE(std::string(RequirePage(pages, workerStack).info) == "TLS (" + tid + "), Stack (" + tid + ")");
}

TEST_CASE("C API counts only readable memory as a valid pointer", "[memorymap][api]")
{
    ApiSession s(FIXTURE("mem_map_target"));
    RunToParked(s);
    REQUIRE(ElfBugMemIsValidPtr(s.dbg, s.Symbol("mm_data")));
    REQUIRE_FALSE(ElfBugMemIsValidPtr(s.dbg, Read<uintptr_t>(s, s.Symbol("mm_reserved"))));
}

TEST_CASE("C API cuts info at its buffer without splitting a character", "[memorymap][api]")
{
    ApiSession s(FIXTURE("mem_map_target"));
    RunToParked(s);
    const std::string path = Read<std::array<char, PATH_MAX>>(s, s.Symbol("mm_file_path")).data();
    REQUIRE(path.size() == ELFBUG_MEMORY_INFO_SIZE);

    const auto pages = ElfBugMemoryMap(s.dbg, false);
    REQUIRE(std::string(RequirePage(pages, Read<uintptr_t>(s, s.Symbol("mm_file"))).info) == path.substr(0, path.size() - 2));
}

TEST_CASE("C API lookups stay correct while exports rebuild the map and load images", "[memorymap][api]")
{
    ApiSession s(FIXTURE("mem_map_target"));
    RunToParked(s);
    const ptr code = s.Symbol("mm_code");

    std::atomic<int> answered{0};
    std::atomic<int> wrong{0};
    {
        std::jthread lookups([&](const std::stop_token stop)
        {
            while(!stop.stop_requested())
            {
                if(ElfBugMemIsCodePtr(s.dbg, code))
                    ++answered;
                else
                    ++wrong;
            }
        });
        REQUIRE(WaitUntil([&] { return answered > 0; }));
        const auto exportBoth = [&]
        {
            REQUIRE_FALSE(ElfBugMemoryMap(s.dbg, false).empty());
            REQUIRE_FALSE(ElfBugMemoryMap(s.dbg, true).empty());
        };

        exportBoth();
        ElfBugContinue(s.dbg);
        exportBoth();
        REQUIRE(s.Pause());
        Request(s, "mm_load_gap");
        REQUIRE(s.Pause());
        ElfBugContinue(s.dbg);
        const auto pieces = ElfBugMemoryMap(s.dbg, true);
        const ElfBugMemoryPage* gap = HeaderRow(pieces, "libmm_gap.so");
        REQUIRE(gap);
        REQUIRE(HasSectionRow(pieces, gap->module_base, ".text"));
        REQUIRE(s.Pause());
        exportBoth();
    }
    REQUIRE(wrong == 0);
}
