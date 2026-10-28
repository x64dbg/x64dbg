#include "TestSupport.h"
#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>
#include <ElfBug/elf/ElfImageCache.h>
#include <ElfBug/process/MemoryMap.h>
#include <ElfBug/process/TraceeMemory.h>
#include <ElfBug/types/Utf8.h>
#include <sys/mman.h>
#include <sys/personality.h>
#include <algorithm>
#include <array>
#include <string>
#include <vector>
#include "targets/TargetUtil.h"

namespace
{
    using namespace ElfBug;
    using namespace ElfBug::test;

    // Fixtures inherit the runner's environment, and glibc.mem.decorate_maps would rename the anonymous memory the labels are checked on.
    class GlibcTunablesCleared : public Catch::EventListenerBase
    {
    public:
        using EventListenerBase::EventListenerBase;

        void testRunStarting(const Catch::TestRunInfo &) override
        {
            unsetenv("GLIBC_TUNABLES");
        }
    };

    struct Mapping
    {
        uint64_t start = 0;
        std::string perms;
    };

    // Every maps entry whose path ends with suffix, lowest first.
    std::vector<Mapping> MappingsOf(const pid_t pid, const std::string_view suffix)
    {
        std::vector<Mapping> mappings;
        const std::string maps = procfs::ReadFile(procfs::Path(pid, "maps"));
        for(const std::string_view line : procfs::Split(maps, '\n'))
        {
            const auto entry = procfs::ParseMapsLine(line);
            if(entry && entry->path.ends_with(suffix))
                mappings.push_back({entry->start, std::string(entry->perms)});
        }
        return mappings;
    }

    // The end of the maps entry holding address, or 0.
    uint64_t MappingEnd(const pid_t pid, const uint64_t address)
    {
        const std::string maps = procfs::ReadFile(procfs::Path(pid, "maps"));
        for(const std::string_view line : procfs::Split(maps, '\n'))
        {
            const auto entry = procfs::ParseMapsLine(line);
            if(entry && address >= entry->start && address < entry->end)
                return entry->end;
        }
        return 0;
    }

    const MemoryRegion & RequireRegion(const MemoryMap & map, const uint64_t address)
    {
        const MemoryRegion* region = map.FindRegion(address);
        REQUIRE(region);
        return *region;
    }

    const MemoryModule & RequireModule(const MemoryMap & map, const uint64_t address)
    {
        const MemoryModule* module = map.FindModule(address);
        REQUIRE(module);
        return *module;
    }

    bool HasImageSection(const MemoryModule & module, const std::string & name)
    {
        return module.image && HasSection(*module.image, name);
    }

    // mem_map_target run without a tracer, once its worker has published.
    class MapTarget
    {
    public:
        explicit MapTarget(std::vector<std::string> environment = {})
            : mProcess(mPath, std::move(environment))
        {
            REQUIRE(WaitForExeced(Pid(), mPath));
            const ptr workerTid = Symbol("mm_worker_tid");
            REQUIRE(WaitUntil([&] { return ReadTraceeValue<int>(Pid(), workerTid).value_or(0) != 0; }));
        }

        [[nodiscard]] pid_t Pid() const { return mProcess.pid; }
        [[nodiscard]] const std::string & Path() const { return mPath; }

        [[nodiscard]] ptr Symbol(const std::string & name) const
        {
            const auto address = ResolveRuntimeAddress(mPath, Pid(), name);
            REQUIRE(address.has_value());
            return *address;
        }

        [[nodiscard]] ptr BssEnd() const
        {
            return Symbol("mm_bss") + kMemMapBssSize - 1;
        }

        // From the aux vector, so readelf-based checks don't take it from the map under test.
        [[nodiscard]] ptr Bias() const
        {
            const auto bias = GetExecLoadBias(Pid(), mPath);
            REQUIRE(bias.has_value());
            return *bias;
        }

        template<typename T>
        [[nodiscard]] T Published(const std::string & name) const
        {
            const auto value = ReadTraceeValue<T>(Pid(), Symbol(name));
            REQUIRE(value.has_value());
            return *value;
        }

        [[nodiscard]] std::vector<ThreadPointers> Threads() const
        {
            return
            {
                {Pid(), Published<uintptr_t>("mm_main_stack"), 0},
                {Published<int>("mm_worker_tid"), Published<uintptr_t>("mm_worker_stack"), Published<uintptr_t>("mm_worker_tcb")},
            };
        }

        // Rebuilds and attaches images the way the API's resolveImages does.
        void Build(MemoryMap & map)
        {
            map.Rebuild(Pid(), Threads());
            map.AttachImages(mImages.Load(Pid(), mImages.Generation(), map.PendingImages()));
        }

        // Writes request to one of the fixture's flags and returns its answer.
        int Ask(const std::string & flag, const int request) const
        {
            const ptr address = Symbol(flag);
            REQUIRE(WriteTraceeValue(Pid(), address, request));
            REQUIRE(WaitUntil([&] { return ReadTraceeValue<int>(Pid(), address) != request; }));
            const auto answer = ReadTraceeValue<int>(Pid(), address);
            REQUIRE(answer.has_value());
            return *answer;
        }

        void Request(const std::string & flag, const int request) const
        {
            REQUIRE(Ask(flag, request) == request + 1);
        }

    private:
        std::string mPath = FIXTURE("mem_map_target");
        UntracedProcess mProcess;
        ElfImageCache mImages;
    };
}

CATCH_REGISTER_LISTENER(GlibcTunablesCleared)

TEST_CASE("ReadTraceeMemory reads a process by pid", "[memorymap]")
{
    const std::array<char, 8> source{'e', 'l', 'f', 'b', 'u', 'g', '!', '\0'};
    std::array<char, 8> copy{};
    const auto address = reinterpret_cast<ptr>(source.data());
    REQUIRE(ReadTraceeMemory(getpid(), address, copy.data(), copy.size()) == static_cast<ssize_t>(copy.size()));
    REQUIRE(copy == source);
    REQUIRE(ReadTraceeMemory(getpid(), 0x10, copy.data(), copy.size()) == -1);
}

TEST_CASE("ReadTraceeMemory reads a page with no access", "[memorymap]")
{
    void* page = mmap(nullptr, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    REQUIRE(page != MAP_FAILED);
    const std::array<char, 8> source{'e', 'l', 'f', 'b', 'u', 'g', '!', '\0'};
    std::memcpy(page, source.data(), source.size());
    REQUIRE(mprotect(page, 4096, PROT_NONE) == 0);
    std::array<char, 8> copy{};
    const ssize_t read = ReadTraceeMemory(getpid(), reinterpret_cast<ptr>(page), copy.data(), copy.size());
    munmap(page, 4096);
    REQUIRE(read == static_cast<ssize_t>(copy.size()));
    REQUIRE(copy == source);
}

TEST_CASE("Utf8Prefix never ends inside a character", "[memorymap]")
{
    const std::string text = "ab\xc3\xa9";
    REQUIRE(Utf8Prefix(text, 10) == text);
    REQUIRE(Utf8Prefix(text, 4) == text);
    REQUIRE(Utf8Prefix(text, 3) == "ab");
    REQUIRE(Utf8Prefix(text, 2) == "ab");
    REQUIRE(Utf8Prefix(text, 0).empty());

    const std::string invalid = "ab" + std::string(6, '\x80');
    REQUIRE(Utf8Prefix(invalid, 7).size() == 4);
}

TEST_CASE("MemoryMap groups the executable with its anonymous .bss tail", "[memorymap]")
{
    MapTarget target;
    MemoryMap map;
    target.Build(map);

    const ptr code = target.Symbol("mm_code");
    const MemoryModule & exe = RequireModule(map, code);
    REQUIRE(exe.path == target.Path());
    REQUIRE(exe.base == GetExecBase(target.Pid(), target.Path()));
    REQUIRE(RequireRegion(map, code).type == RegionType::Image);

    const MemoryRegion & tail = RequireRegion(map, target.BssEnd());
    REQUIRE_FALSE(tail.file.Known());
    REQUIRE(tail.type == RegionType::Image);
    REQUIRE(map.FindModule(target.BssEnd()) == &exe);
}

TEST_CASE("MemoryMap marks the executable User and libc System", "[memorymap]")
{
    MapTarget target;
    MemoryMap map;
    target.Build(map);

    REQUIRE(RequireModule(map, target.Symbol("mm_code")).party == Party::User);

    const auto libc = MappingsOf(target.Pid(), "/libc.so.6");
    REQUIRE_FALSE(libc.empty());
    const MemoryModule & libcModule = RequireModule(map, libc.front().start);
    REQUIRE(libcModule.party == Party::System);
    REQUIRE(HasImageSection(libcModule, ".text"));
}

TEST_CASE("MemoryMap keeps data files and anonymous memory out of modules", "[memorymap]")
{
    MapTarget target;
    MemoryMap map;
    target.Build(map);

    const MemoryRegion & reserved = RequireRegion(map, target.Published<uintptr_t>("mm_reserved"));
    REQUIRE(reserved.perms == "---p");
    REQUIRE(reserved.type == RegionType::Private);
    REQUIRE_FALSE(reserved.module);

    const MemoryRegion & shared = RequireRegion(map, target.Published<uintptr_t>("mm_shared"));
    REQUIRE(shared.type == RegionType::Mapped);
    REQUIRE_FALSE(shared.module);

    const MemoryRegion & file = RequireRegion(map, target.Published<uintptr_t>("mm_file"));
    REQUIRE(file.type == RegionType::Mapped);
    REQUIRE_FALSE(file.module);

    REQUIRE(map.FindModule(target.Published<uintptr_t>("mm_heap")) == nullptr);
}

TEST_CASE("MemoryMap loads section names only through AttachImages", "[memorymap]")
{
    MapTarget target;
    MemoryMap map;
    map.Rebuild(target.Pid(), target.Threads());
    const ptr code = target.Symbol("mm_code");
    const MemoryModule & exe = RequireModule(map, code);
    REQUIRE(exe.mainProgram);
    REQUIRE_FALSE(exe.imageResolved);
    REQUIRE_FALSE(exe.image);

    const auto pending = map.PendingImages();
    const auto isMain = [](const ImageRequest & request)
    {
        return request.mainProgram;
    };
    REQUIRE(std::count_if(pending.begin(), pending.end(), isMain) == 1);
    const auto request = std::find_if(pending.begin(), pending.end(), isMain);
    REQUIRE(request->file == exe.file);
    REQUIRE(request->path == target.Path());
    REQUIRE(request->start == exe.base);

    ElfImageCache cache;
    map.AttachImages(cache.Load(target.Pid(), cache.Generation(), pending));
    REQUIRE(HasImageSection(RequireModule(map, code), ".text"));
    REQUIRE(map.PendingImages().empty());
}

TEST_CASE("MemoryMap shows no sections from a file whose headers differ from memory", "[memorymap]")
{
    MapTarget target;
    MemoryMap map;
    map.Rebuild(target.Pid(), target.Threads());
    const ptr code = target.Symbol("mm_code");
    const ImageId file = RequireModule(map, code).file;

    auto parsed = ParseBytes(ReadFileBytes(FIXTURE("hello_elfbug")));
    REQUIRE(parsed.has_value());
    const auto impostor = std::make_shared<const ElfImage>(std::move(*parsed));

    SECTION("a file found by path is checked against memory")
    {
        map.AttachImages({{file, impostor, false}});
        REQUIRE(RequireModule(map, code).imageResolved);
        REQUIRE_FALSE(RequireModule(map, code).image);
    }
    SECTION("the mapped inode itself is trusted")
    {
        map.AttachImages({{file, impostor, true}});
        REQUIRE(RequireModule(map, code).image == impostor);
    }
}

TEST_CASE("MemoryMap carries modules over between rebuilds", "[memorymap]")
{
    MapTarget target;
    MemoryMap map;
    target.Build(map);
    const ptr code = target.Symbol("mm_code");
    const ElfImage* image = RequireModule(map, code).image.get();
    REQUIRE(image);

    target.Request("mm_load_gap", 1);
    map.Rebuild(target.Pid(), target.Threads());
    REQUIRE(RequireModule(map, code).image.get() == image);
    const auto pending = map.PendingImages();
    REQUIRE(pending.size() == 1);
    REQUIRE(pending.front().path.ends_with("/libmm_gap.so"));
}

TEST_CASE("MemoryMap detects a module again when another file sits at its base", "[memorymap]")
{
    const std::string original = FIXTURE("mem_map_target");
    const std::string copy = original + "." + std::to_string(getpid());
    std::filesystem::copy_file(original, copy, std::filesystem::copy_options::overwrite_existing);
    struct Remover
    {
        std::string path;
        ~Remover()
        {
            std::error_code error;
            std::filesystem::remove(path, error);
        }
    } remover{copy};

    const int persona = personality(0xffffffff);
    REQUIRE(persona != -1);
    personality(static_cast<unsigned long>(persona) | ADDR_NO_RANDOMIZE);
    UntracedProcess first(original);
    UntracedProcess second(copy);
    personality(static_cast<unsigned long>(persona));
    REQUIRE(WaitForExeced(first.pid, original));
    REQUIRE(WaitForExeced(second.pid, copy));
    const auto base = GetExecBase(first.pid, original);
    REQUIRE(base.has_value());
    REQUIRE(GetExecBase(second.pid, copy) == base);

    MemoryMap map;
    map.Rebuild(first.pid, {});
    const ImageId before = RequireModule(map, *base).file;
    map.Rebuild(second.pid, {});
    const MemoryModule & module = RequireModule(map, *base);
    REQUIRE(module.file != before);
    REQUIRE(module.file == RequireRegion(map, *base).file);
    REQUIRE(module.path == copy);
}

TEST_CASE("MemoryMap detects static and static-pie programs", "[memorymap]")
{
    // st_bss in static_target.c, which spills past its file-backed page.
    constexpr size_t kStaticBssSize = 64 * 1024;
    for(const char* name : {"static_target", "static_pie_target"})
    {
        CAPTURE(name);
        const std::string path = FIXTURE(name);
        UntracedProcess process(path);
        REQUIRE(WaitForExeced(process.pid, path));
        const auto ready = ResolveRuntimeAddress(path, process.pid, "st_ready");
        const auto bss = ResolveRuntimeAddress(path, process.pid, "st_bss");
        REQUIRE(ready.has_value());
        REQUIRE(bss.has_value());
        REQUIRE(WaitUntil([&] { return ReadTraceeValue<int>(process.pid, *ready).value_or(0) == 1; }));

        MemoryMap map;
        map.Rebuild(process.pid, {});
        ElfImageCache cache;
        map.AttachImages(cache.Load(process.pid, cache.Generation(), map.PendingImages()));
        const MemoryModule & program = RequireModule(map, *ready);
        REQUIRE(program.path == path);
        REQUIRE(program.mainProgram);
        REQUIRE(program.base == GetExecBase(process.pid, path));
        REQUIRE(HasImageSection(program, ".text"));
        REQUIRE(map.FindModule(*bss + kStaticBssSize - 1) == &program);
    }
}

TEST_CASE("MemoryMap gives each dlmopen copy of a library its own module", "[memorymap]")
{
    MapTarget target;
    target.Request("mm_load_data", 1);
    target.Request("mm_isolate_data", 1);
    MemoryMap map;
    target.Build(map);

    std::vector<const MemoryModule*> copies;
    for(const auto & mapping : MappingsOf(target.Pid(), "/libmm_data.so"))
    {
        const MemoryModule* module = map.FindModule(mapping.start);
        REQUIRE(module);
        if(module->base == mapping.start)
            copies.push_back(module);
    }
    REQUIRE(copies.size() == 2);
    REQUIRE(copies[0]->file == copies[1]->file);
    for(const MemoryModule* module : copies)
        REQUIRE(HasImageSection(*module, ".rodata"));
}

TEST_CASE("MemoryMap puts loader holes inside their library and drops it when unloaded", "[memorymap]")
{
    MapTarget target;
    target.Request("mm_load_gap", 1);
    MemoryMap map;
    target.Build(map);

    const auto mappings = MappingsOf(target.Pid(), "/libmm_gap.so");
    REQUIRE_FALSE(mappings.empty());
    const uint64_t base = mappings.front().start;
    const auto hole = std::find_if(mappings.begin(), mappings.end(), [](const Mapping & mapping)
    {
        return mapping.perms == "---p";
    });
    REQUIRE(hole != mappings.end());

    const MemoryModule & gap = RequireModule(map, base);
    REQUIRE(gap.base == base);
    REQUIRE(HasImageSection(gap, ".text"));
    REQUIRE(map.FindModule(hole->start) == &gap);
    REQUIRE(RequireRegion(map, hole->start).type == RegionType::Image);
    for(const bool sectionView : {false, true})
    {
        CAPTURE(sectionView);
        const auto pages = map.Pages(sectionView);
        const MemoryPage & page = RequirePage(pages, hole->start);
        REQUIRE(page.info == "Reserved");
        REQUIRE(page.type == RegionType::Image);
        REQUIRE(page.moduleBase == base);
    }

    target.Request("mm_load_gap", 4);
    map.Rebuild(target.Pid(), target.Threads());
    REQUIRE(MappingsOf(target.Pid(), "/libmm_gap.so").empty());
    REQUIRE(map.FindModule(base) == nullptr);
}

TEST_CASE("MemoryMap leaves anonymous memory mapped into a loader hole out of the library", "[memorymap]")
{
    MapTarget target;
    target.Request("mm_load_gap", 1);
    const auto mappings = MappingsOf(target.Pid(), "/libmm_gap.so");
    REQUIRE_FALSE(mappings.empty());
    const auto hole = std::find_if(mappings.begin(), mappings.end(), [](const Mapping & mapping)
    {
        return mapping.perms == "---p";
    });
    REQUIRE(hole != mappings.end());
    REQUIRE(WriteTraceeValue(target.Pid(), target.Symbol("mm_gap_fill"), static_cast<uintptr_t>(hole->start)));
    target.Request("mm_fill_gap", 1);
    MemoryMap map;
    target.Build(map);

    REQUIRE(map.FindModule(mappings.front().start));
    const MemoryRegion & filled = RequireRegion(map, hole->start);
    REQUIRE_FALSE(filled.file.Known());
    REQUIRE(filled.type == RegionType::Private);
    REQUIRE(map.FindModule(hole->start) == nullptr);
}

TEST_CASE("MemoryMap does not mistake a whole-file mapping of an ELF for a module", "[memorymap]")
{
    MapTarget target;
    MemoryMap map;
    target.Build(map);

    for(const char* name : {"mm_self_image", "mm_self_exec"})
    {
        CAPTURE(name);
        const uintptr_t address = target.Published<uintptr_t>(name);
        REQUIRE(address != 0);
        REQUIRE(map.FindModule(address) == nullptr);
        REQUIRE(RequireRegion(map, address).type == RegionType::Mapped);
    }

    const auto pages = map.Pages(false);
    const auto headers = std::count_if(pages.begin(), pages.end(), [](const MemoryPage & page)
    {
        return page.base == page.moduleBase && page.info.starts_with("mem_map_target");
    });
    REQUIRE(headers == 1);
}

TEST_CASE("MemoryMap takes anonymous memory only for an image's code", "[memorymap]")
{
    MapTarget target;
    MemoryMap map;
    target.Build(map);

    const uintptr_t phantom = target.Published<uintptr_t>("mm_phantom");
    REQUIRE(phantom != 0);
    const uintptr_t middle = phantom + kMemMapPhantomSpan / 2;
    REQUIRE(map.FindModule(phantom) == nullptr);
    REQUIRE(map.FindModule(middle) == nullptr);
    REQUIRE(RequireRegion(map, phantom).type == RegionType::Mapped);
    REQUIRE(RequireRegion(map, middle).type == RegionType::Private);
}

TEST_CASE("MemoryMap keeps code moved onto anonymous memory in its module across a gap", "[memorymap]")
{
    const std::string path = FIXTURE("huge_text_target");
    UntracedProcess process(path);
    REQUIRE(WaitForExeced(process.pid, path));
    const auto remapped = ResolveRuntimeAddress(path, process.pid, "ht_remapped");
    REQUIRE(remapped.has_value());
    int state = 0;
    REQUIRE(WaitUntil([&]
    {
        state = ReadTraceeValue<int>(process.pid, *remapped).value_or(0);
        return state != 0;
    }));
    REQUIRE(state == 1);
    const auto code = ResolveRuntimeAddress(path, process.pid, "ht_code");
    REQUIRE(code.has_value());

    MemoryMap map;
    map.Rebuild(process.pid, {});
    const MemoryRegion & moved = RequireRegion(map, *code);
    REQUIRE_FALSE(moved.file.Known());
    REQUIRE(map.FindRegion(moved.start - 1) == nullptr);
    REQUIRE(RequireModule(map, *code).path == path);
    REQUIRE(moved.type == RegionType::Image);
}

TEST_CASE("MemoryMap ends a module's anonymous memory at the first gap", "[memorymap]")
{
    MapTarget target;
    target.Request("mm_punch_bss", 1);
    MemoryMap map;
    target.Build(map);

    const MemoryModule & exe = RequireModule(map, target.Symbol("mm_code"));
    REQUIRE(map.FindModule(target.Symbol("mm_bss")) == &exe);
    REQUIRE(map.FindModule(target.BssEnd()) == nullptr);
    REQUIRE(RequireRegion(map, target.BssEnd()).type == RegionType::Private);
}

TEST_CASE("MemoryMap leaves memory the kernel merged onto a .bss tail out of the module", "[memorymap]")
{
    MapTarget target({"MM_EXTEND_BSS=1"});
    REQUIRE(target.Published<int>("mm_extend_bss") == 2);
    const uintptr_t extension = target.Published<uintptr_t>("mm_extension");
    if(MappingEnd(target.Pid(), target.BssEnd()) <= extension)
        SKIP("the kernel kept the extension apart from the .bss tail");
    MemoryMap map;
    target.Build(map);

    const MemoryModule & exe = RequireModule(map, target.Symbol("mm_code"));
    REQUIRE(map.FindModule(target.BssEnd()) == &exe);
    REQUIRE(map.FindModule(extension) == nullptr);
    const MemoryRegion & rest = RequireRegion(map, extension);
    REQUIRE(rest.type == RegionType::Private);
    REQUIRE(rest.start == RequireRegion(map, target.BssEnd()).end);
}

TEST_CASE("MemoryMap joins a .bss tail that glibc names with decorate_maps", "[memorymap]")
{
    MapTarget target({"GLIBC_TUNABLES=glibc.mem.decorate_maps=1"});
    MemoryMap map;
    target.Build(map);

    const auto tails = MappingsOf(target.Pid(), "/libc.so.6]");
    if(tails.empty())
        SKIP("this glibc does not name .bss tails");
    const uint64_t tail = tails.front().start;
    REQUIRE(RequireModule(map, tail).path.ends_with("/libc.so.6"));
    REQUIRE(RequireRegion(map, tail).type == RegionType::Image);
}

TEST_CASE("MemoryMap detects a library with no executable mapping", "[memorymap]")
{
    MapTarget target;
    target.Request("mm_load_data", 1);
    MemoryMap map;
    target.Build(map);

    const auto mappings = MappingsOf(target.Pid(), "/libmm_data.so");
    REQUIRE(mappings.size() >= 2);
    REQUIRE(std::none_of(mappings.begin(), mappings.end(), [](const Mapping & mapping)
    {
        return mapping.perms[2] == 'x';
    }));

    const MemoryModule & data = RequireModule(map, mappings.front().start);
    REQUIRE(data.base == mappings.front().start);
    REQUIRE(HasImageSection(data, ".rodata"));
    for(const auto & mapping : mappings)
        REQUIRE(map.FindModule(mapping.start) == &data);
}

TEST_CASE("Region view names the sections in each module region", "[memorymap][pages]")
{
    MapTarget target;
    MemoryMap map;
    target.Build(map);
    const auto pages = map.Pages(false);
    const ptr code = target.Symbol("mm_code");
    const uint64_t exeBase = RequireModule(map, code).base;

    const auto sections = ReadImageSections(target.Path());
    const uint64_t bias = target.Bias();
    const auto expectedInfo = [&](const MemoryPage & page)
    {
        std::string info = page.base == exeBase ? "mem_map_target" : "";
        for(const auto & section : sections)
        {
            const uint64_t start = bias + section.address;
            if(start + section.size <= page.base || start >= page.base + page.size)
                continue;
            if(!info.empty())
                info += ",";
            info += " \"" + section.name + "\"";
        }
        return info;
    };

    for(const ptr address : {code, exeBase, target.Symbol("mm_data"), target.BssEnd()})
    {
        const MemoryPage & page = RequirePage(pages, address);
        CAPTURE(page.base);
        REQUIRE(page.info == expectedInfo(page));
    }
    REQUIRE(RequirePage(pages, code).section == ".text");
}

TEST_CASE("Section view lists every byte of region view exactly once", "[memorymap][pages]")
{
    MapTarget target;
    MemoryMap map;
    target.Build(map);
    const auto regions = map.Pages(false);
    const auto pieces = map.Pages(true);

    size_t next = 0;
    for(const auto & region : regions)
    {
        CAPTURE(region.base, region.info);
        uint64_t cursor = region.base;
        while(cursor < region.base + region.size)
        {
            REQUIRE(next < pieces.size());
            const MemoryPage & piece = pieces[next++];
            REQUIRE(piece.base == cursor);
            REQUIRE(piece.size > 0);
            REQUIRE(piece.perms == region.perms);
            REQUIRE(piece.moduleBase == region.moduleBase);
            cursor += piece.size;
        }
        REQUIRE(cursor == region.base + region.size);
    }
    REQUIRE(next == pieces.size());
}

TEST_CASE("Section view names every module row", "[memorymap][pages]")
{
    MapTarget target;
    target.Request("mm_load_gap", 1);
    MemoryMap map;
    target.Build(map);

    for(const auto & piece : map.Pages(true))
    {
        CAPTURE(piece.base, piece.perms);
        if(piece.moduleBase != 0)
            REQUIRE_FALSE(piece.info.empty());
    }
}

TEST_CASE("Section view attributes every byte of every section to it", "[memorymap][pages]")
{
    MapTarget target;
    MemoryMap map;
    target.Build(map);
    const uint64_t bias = target.Bias();
    const auto pieces = map.Pages(true);

    size_t checked = 0;
    for(const auto & section : ReadImageSections(target.Path()))
    {
        CAPTURE(section.name);
        const uint64_t start = bias + section.address;
        const uint64_t end = start + section.size;
        uint64_t covered = start;
        for(const auto & piece : pieces)
        {
            if(piece.base + piece.size <= start || piece.base >= end)
                continue;
            REQUIRE(piece.section == section.name);
            if(piece.base <= covered)
                covered = std::max(covered, piece.base + piece.size);
        }
        REQUIRE(covered >= end);
        ++checked;
    }
    REQUIRE(checked > 10);

    const MemoryPage & header = RequirePage(pieces, RequireModule(map, target.Symbol("mm_code")).base);
    REQUIRE(header.info == "mem_map_target");
    REQUIRE(header.section.empty());
}

TEST_CASE("Pages name every section however long the info gets", "[memorymap][pages]")
{
    MapTarget target;
    MemoryMap map;
    target.Build(map);
    const uint64_t bias = target.Bias();

    std::vector<SectionInfo> longSections;
    for(const auto & section : ReadImageSections(target.Path()))
    {
        if(section.name.starts_with(".mm_section_with_a_long_name_"))
            longSections.push_back(section);
    }
    REQUIRE(longSections.size() == kMemMapLongSections);

    const auto regions = map.Pages(false);
    const auto pieces = map.Pages(true);
    for(const auto & section : longSections)
    {
        CAPTURE(section.name);
        const uint64_t address = bias + section.address;
        REQUIRE(RequirePage(regions, address).info.find("\"" + section.name + "\"") != std::string::npos);
        REQUIRE(RequirePage(pieces, address).info == " \"" + section.name + "\"");
    }
}

TEST_CASE("Pages show pseudo regions by their labels and no-access memory as Reserved", "[memorymap][pages]")
{
    MapTarget target;
    MemoryMap map;
    target.Build(map);
    const std::string pid = std::to_string(target.Pid());
    const std::string tid = std::to_string(target.Published<int>("mm_worker_tid"));
    const uintptr_t mainStack = target.Published<uintptr_t>("mm_main_stack");
    const uintptr_t workerStack = target.Published<uintptr_t>("mm_worker_stack");

    REQUIRE(RequireRegion(map, mainStack).name == "[stack]");
    REQUIRE(&RequireRegion(map, target.Published<uintptr_t>("mm_worker_tcb")) == &RequireRegion(map, workerStack));
    for(const bool sectionView : {false, true})
    {
        CAPTURE(sectionView);
        const auto pages = map.Pages(sectionView);
        REQUIRE(RequirePage(pages, target.Published<uintptr_t>("mm_heap")).info == "Heap");
        REQUIRE(RequirePage(pages, mainStack).info == "Stack (" + pid + ")");
        REQUIRE(RequirePage(pages, workerStack).info == "TLS (" + tid + "), Stack (" + tid + ")");
        REQUIRE(RequirePage(pages, target.Published<uintptr_t>("mm_reserved")).info == "Reserved");
    }
}

TEST_CASE("Pages keep the name of a named anonymous mapping", "[memorymap][pages]")
{
    MapTarget target;
    if(target.Published<int>("mm_named_ok") == 0)
        SKIP("the kernel has no PR_SET_VMA_ANON_NAME");
    MemoryMap map;
    target.Build(map);
    REQUIRE(RequirePage(map.Pages(false), target.Published<uintptr_t>("mm_named")).info == "[anon:elfbug]");
}

TEST_CASE("Pages show the vdso as a System image with its own sections", "[memorymap][pages]")
{
    MapTarget target;
    MemoryMap map;
    target.Build(map);
    const auto vdso = MappingsOf(target.Pid(), "[vdso]");
    REQUIRE(vdso.size() == 1);

    const auto regions = map.Pages(false);
    const MemoryPage & header = RequirePage(regions, vdso.front().start);
    REQUIRE(header.info.starts_with("[vdso]"));
    REQUIRE(header.party == Party::System);
    REQUIRE(header.type == RegionType::Image);

    const auto pieces = map.Pages(true);
    REQUIRE(std::any_of(pieces.begin(), pieces.end(), [&](const MemoryPage & piece)
    {
        return piece.moduleBase == vdso.front().start && piece.section == ".text";
    }));
}
