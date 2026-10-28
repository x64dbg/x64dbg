#include "TestSupport.h"
#include <ElfBug/elf/ElfImage.h>
#include <ElfBug/elf/ElfImageCache.h>
#include <elf.h>
#include <sys/stat.h>
#include <algorithm>
#include <cstring>

namespace
{
    using namespace ElfBug;
    using namespace ElfBug::test;

    template<typename T>
    T ReadAt(const std::vector<std::byte> & bytes, const size_t offset)
    {
        T value{};
        std::memcpy(&value, bytes.data() + offset, sizeof(value));
        return value;
    }

    template<typename T>
    void WriteAt(std::vector<std::byte> & bytes, const size_t offset, const T & value)
    {
        std::memcpy(bytes.data() + offset, &value, sizeof(value));
    }

    size_t SectionHeaderOffset(const std::vector<std::byte> & bytes, const size_t index)
    {
        return ReadAt<Elf64_Ehdr>(bytes, 0).e_shoff + index * sizeof(Elf64_Shdr);
    }

    size_t SectionIndex(const std::vector<std::byte> & bytes, const std::string & name)
    {
        const auto header = ReadAt<Elf64_Ehdr>(bytes, 0);
        const auto names = ReadAt<Elf64_Shdr>(bytes, SectionHeaderOffset(bytes, header.e_shstrndx));
        for(size_t i = 0; i < header.e_shnum; ++i)
        {
            const auto section = ReadAt<Elf64_Shdr>(bytes, SectionHeaderOffset(bytes, i));
            if(name == reinterpret_cast<const char*>(bytes.data() + names.sh_offset + section.sh_name))
                return i;
        }
        FAIL("no section " << name);
        return 0;
    }

    std::vector<size_t> LoadOffsets(const std::vector<std::byte> & bytes)
    {
        std::vector<size_t> offsets;
        const auto header = ReadAt<Elf64_Ehdr>(bytes, 0);
        for(size_t i = 0; i < header.e_phnum; ++i)
        {
            const size_t offset = header.e_phoff + i * sizeof(Elf64_Phdr);
            if(ReadAt<Elf64_Phdr>(bytes, offset).p_type == PT_LOAD)
                offsets.push_back(offset);
        }
        REQUIRE_FALSE(offsets.empty());
        return offsets;
    }

    // Copies the size bytes at offset to the end of the image, zero-padded to newSize, and returns where the copy starts.
    size_t AppendTable(std::vector<std::byte> & bytes, const size_t offset, const size_t size, const size_t newSize)
    {
        const auto begin = bytes.begin() + static_cast<ptrdiff_t>(offset);
        std::vector<std::byte> table(begin, begin + static_cast<ptrdiff_t>(size));
        table.resize(newSize);
        const size_t appended = bytes.size();
        bytes.insert(bytes.end(), table.begin(), table.end());
        return appended;
    }

    // Moves the program headers to the end of the image, padded with PT_NULL entries up to count.
    void PadProgramHeaders(std::vector<std::byte> & bytes, const uint16_t count)
    {
        auto header = ReadAt<Elf64_Ehdr>(bytes, 0);
        header.e_phoff = AppendTable(bytes, header.e_phoff, header.e_phnum * sizeof(Elf64_Phdr), count * sizeof(Elf64_Phdr));
        header.e_phnum = count;
        WriteAt(bytes, 0, header);
    }

    // Moves the section headers to the end of the image, padded with SHT_NULL entries up to count, with the count in section 0.
    void PadSectionHeaders(std::vector<std::byte> & bytes, const uint32_t count)
    {
        auto header = ReadAt<Elf64_Ehdr>(bytes, 0);
        const uint64_t current = header.e_shnum != 0 ? header.e_shnum : ReadAt<Elf64_Shdr>(bytes, header.e_shoff).sh_size;
        header.e_shoff = AppendTable(bytes, header.e_shoff, current * sizeof(Elf64_Shdr), count * sizeof(Elf64_Shdr));
        header.e_shnum = 0;
        WriteAt(bytes, 0, header);
        auto first = ReadAt<Elf64_Shdr>(bytes, header.e_shoff);
        first.sh_size = count;
        WriteAt(bytes, header.e_shoff, first);
    }

    // Moves the section name table to the end of the image, zero-padded to size.
    void PadNameTable(std::vector<std::byte> & bytes, const size_t size)
    {
        const size_t namesOffset = SectionHeaderOffset(bytes, ReadAt<Elf64_Ehdr>(bytes, 0).e_shstrndx);
        auto names = ReadAt<Elf64_Shdr>(bytes, namesOffset);
        names.sh_offset = AppendTable(bytes, names.sh_offset, names.sh_size, size);
        names.sh_size = size;
        WriteAt(bytes, namesOffset, names);
    }

    // Appends a copy of the name table with the new name, so no other name moves.
    void RenameSection(std::vector<std::byte> & bytes, const std::string & from, const std::string & to)
    {
        const size_t sectionOffset = SectionHeaderOffset(bytes, SectionIndex(bytes, from));
        const size_t namesOffset = SectionHeaderOffset(bytes, ReadAt<Elf64_Ehdr>(bytes, 0).e_shstrndx);
        auto section = ReadAt<Elf64_Shdr>(bytes, sectionOffset);
        auto names = ReadAt<Elf64_Shdr>(bytes, namesOffset);
        section.sh_name = static_cast<uint32_t>(names.sh_size);
        names.sh_offset = AppendTable(bytes, names.sh_offset, names.sh_size, names.sh_size + to.size() + 1);
        std::memcpy(bytes.data() + names.sh_offset + section.sh_name, to.data(), to.size());
        names.sh_size += to.size() + 1;
        WriteAt(bytes, sectionOffset, section);
        WriteAt(bytes, namesOffset, names);
    }

    ImageRequest RequestForOwnExecutable()
    {
        const std::string exe = procfs::ReadLink("/proc/self/exe");
        const std::string maps = procfs::ReadFile("/proc/self/maps");
        for(const std::string_view line : procfs::Split(maps, '\n'))
        {
            const auto entry = procfs::ParseMapsLine(line);
            if(entry && entry->path == exe && entry->offset == 0)
                return {entry->file, exe, entry->start, entry->end};
        }
        FAIL("the test binary is not mapped");
        return {};
    }
}

TEST_CASE("ElfImage reads the segments and allocated sections of a fixture", "[elf]")
{
    const std::string path = FIXTURE("hello_elfbug");
    const auto image = ParseBytes(ReadFileBytes(path));
    REQUIRE(image.has_value());
    REQUIRE_FALSE(image->Segments().empty());

    const auto expected = ReadImageSections(path);
    REQUIRE(image->Sections().size() == expected.size());
    for(size_t i = 0; i < expected.size(); ++i)
    {
        CAPTURE(expected[i].name);
        REQUIRE(image->Sections()[i].name == expected[i].name);
        REQUIRE(image->Sections()[i].addr == expected[i].address);
        REQUIRE(image->Sections()[i].size == expected[i].size);
    }
    const auto & last = image->Sections().back();
    REQUIRE(image->End() >= last.addr + last.size);
}

TEST_CASE("ElfImage parses only the headers when asked to", "[elf]")
{
    const auto bytes = ReadFileBytes(FIXTURE("hello_elfbug"));
    const auto full = ParseBytes(bytes);
    const auto headers = ParseBytes(bytes, ElfImage::Parts::Headers);
    REQUIRE(full.has_value());
    REQUIRE(headers.has_value());
    REQUIRE(headers->Sections().empty());
    REQUIRE(headers->Segments().size() == full->Segments().size());
    const auto headerBytes = headers->HeaderBytes();
    const auto fullBytes = full->HeaderBytes();
    REQUIRE(std::equal(headerBytes.begin(), headerBytes.end(), fullBytes.begin(), fullBytes.end()));
    const auto ehdr = ReadAt<Elf64_Ehdr>(bytes, 0);
    REQUIRE(headerBytes.size() == sizeof(Elf64_Ehdr) + ehdr.e_phnum * sizeof(Elf64_Phdr));
}

TEST_CASE("ElfImage computes the load bias from where a segment is mapped", "[elf]")
{
    const auto image = ParseBytes(ReadFileBytes(FIXTURE("hello_elfbug")));
    REQUIRE(image.has_value());
    const auto & segments = image->Segments();
    const auto code = std::find_if(segments.begin(), segments.end(), [](const ElfSegment & segment)
    {
        return segment.flags & PF_X;
    });
    REQUIRE(code != segments.end());
    REQUIRE(code->vaddr != 0);

    constexpr uint64_t bias = 0x555555554000;
    REQUIRE(image->LoadBias(bias + PageFloor(code->vaddr), PageFloor(code->offset)) == bias);
    REQUIRE_FALSE(image->LoadBias(bias, 0x7fff0000).has_value());
}

TEST_CASE("ElfImage refuses headers it cannot trust", "[elf]")
{
    auto bytes = ReadFileBytes(FIXTURE("hello_elfbug"));
    REQUIRE(ParseBytes(bytes).has_value());
    auto header = ReadAt<Elf64_Ehdr>(bytes, 0);

    SECTION("a truncated image has no headers")
    {
        bytes.resize(32);
        REQUIRE_FALSE(ParseBytes(bytes).has_value());
    }
    SECTION("only executables and shared objects are images")
    {
        header.e_type = ET_REL;
        WriteAt(bytes, 0, header);
        REQUIRE_FALSE(ParseBytes(bytes).has_value());
    }
    SECTION("up to 1024 program headers are read")
    {
        PadProgramHeaders(bytes, 1024);
        REQUIRE(ParseBytes(bytes).has_value());
        PadProgramHeaders(bytes, 1025);
        REQUIRE_FALSE(ParseBytes(bytes).has_value());
    }
    SECTION("a segment that wraps the address space is refused")
    {
        const size_t offset = LoadOffsets(bytes).front();
        auto load = ReadAt<Elf64_Phdr>(bytes, offset);
        load.p_vaddr = ~uint64_t{0xfff};
        load.p_memsz = load.p_filesz + 0x1000;
        WriteAt(bytes, offset, load);
        REQUIRE_FALSE(ParseBytes(bytes).has_value());
    }
}

TEST_CASE("ElfImage keeps only the segments when the section data is bad", "[elf]")
{
    auto bytes = ReadFileBytes(FIXTURE("hello_elfbug"));
    REQUIRE(ParseBytes(bytes).has_value());
    auto header = ReadAt<Elf64_Ehdr>(bytes, 0);

    SECTION("up to 65536 section headers are read, counted through extended numbering")
    {
        PadSectionHeaders(bytes, 65536);
        const auto within = ParseBytes(bytes);
        REQUIRE(within.has_value());
        REQUIRE(HasSection(*within, ".text"));
        PadSectionHeaders(bytes, 65537);
        const auto over = ParseBytes(bytes);
        REQUIRE(over.has_value());
        REQUIRE_FALSE(over->Segments().empty());
        REQUIRE(over->Sections().empty());
    }
    SECTION("a name table up to 1MB is read")
    {
        PadNameTable(bytes, 1024 * 1024);
        const auto within = ParseBytes(bytes);
        REQUIRE(within.has_value());
        REQUIRE(HasSection(*within, ".text"));
        PadNameTable(bytes, 1024 * 1024 + 1);
        const auto over = ParseBytes(bytes);
        REQUIRE(over.has_value());
        REQUIRE(over->Sections().empty());
    }
    SECTION("a section count past the end keeps the segments only")
    {
        header.e_shnum = 60000;
        WriteAt(bytes, 0, header);
        const auto image = ParseBytes(bytes);
        REQUIRE(image.has_value());
        REQUIRE_FALSE(image->Segments().empty());
        REQUIRE(image->Sections().empty());
    }
    SECTION("an unterminated name table keeps the segments only")
    {
        const auto names = ReadAt<Elf64_Shdr>(bytes, SectionHeaderOffset(bytes, header.e_shstrndx));
        bytes[names.sh_offset + names.sh_size - 1] = std::byte{'x'};
        const auto image = ParseBytes(bytes);
        REQUIRE(image.has_value());
        REQUIRE_FALSE(image->Segments().empty());
        REQUIRE(image->Sections().empty());
    }
}

TEST_CASE("ElfImage ends a segment past its file part even when memsz is smaller", "[elf]")
{
    auto bytes = ReadFileBytes(FIXTURE("hello_elfbug"));
    const size_t offset = LoadOffsets(bytes).back();
    auto load = ReadAt<Elf64_Phdr>(bytes, offset);
    load.p_memsz = load.p_filesz - 1;
    WriteAt(bytes, offset, load);
    const auto image = ParseBytes(bytes);
    REQUIRE(image.has_value());
    REQUIRE(image->End() == load.p_vaddr + load.p_filesz);
}

TEST_CASE("ElfImage leaves out empty sections and lld's .relro_padding", "[elf]")
{
    auto bytes = ReadFileBytes(FIXTURE("hello_elfbug"));
    REQUIRE(ParseBytes(bytes).has_value());

    SECTION("lld's .relro_padding is left out, so the padding folds into the section before it")
    {
        RenameSection(bytes, ".bss", ".relro_padding");
        const auto image = ParseBytes(bytes);
        REQUIRE(image.has_value());
        REQUIRE_FALSE(HasSection(*image, ".relro_padding"));
        REQUIRE(HasSection(*image, ".data"));
    }
    SECTION("a zero-size section is left out")
    {
        const size_t offset = SectionHeaderOffset(bytes, SectionIndex(bytes, ".text"));
        auto text = ReadAt<Elf64_Shdr>(bytes, offset);
        text.sh_size = 0;
        WriteAt(bytes, offset, text);
        const auto image = ParseBytes(bytes);
        REQUIRE(image.has_value());
        REQUIRE_FALSE(HasSection(*image, ".text"));
        REQUIRE(HasSection(*image, ".data"));
    }
}

TEST_CASE("ElfImageCache parses a mapped file once and shares it", "[elf]")
{
    const auto request = RequestForOwnExecutable();
    ElfImageCache cache;
    const auto first = cache.Load(getpid(), cache.Generation(), {request});
    REQUIRE(first.size() == 1);
    REQUIRE(first[0].image);
    REQUIRE(HasSection(*first[0].image, ".text"));
    REQUIRE(first[0].exact == MapFilesOpenable());
    REQUIRE(cache.Load(getpid(), cache.Generation(), {request})[0].image == first[0].image);
    cache.Clear();
    REQUIRE(cache.Load(getpid(), cache.Generation(), {request})[0].image != first[0].image);
}

TEST_CASE("ElfImageCache discards a load made with a stale generation", "[elf]")
{
    const auto request = RequestForOwnExecutable();
    ElfImageCache cache;
    const auto generation = cache.Generation();
    cache.Clear();
    REQUIRE(cache.Load(getpid(), generation, {request}).empty());
    const auto fresh = cache.Load(getpid(), cache.Generation(), {request});
    REQUIRE(fresh.size() == 1);
    REQUIRE(fresh[0].image);
}

TEST_CASE("ElfImageCache reports a file it cannot open as null", "[elf]")
{
    ElfImageCache cache;
    const ImageRequest missing{{0, 42}, "/nonexistent/elfbug.so", 0x1000, 0x2000};
    const auto result = cache.Load(getpid(), cache.Generation(), {missing});
    REQUIRE(result.size() == 1);
    REQUIRE_FALSE(result[0].image);
    REQUIRE_FALSE(result[0].exact);
}

TEST_CASE("ElfImageCache never waits on a path that is not a regular file", "[elf]")
{
    namespace fs = std::filesystem;
    const fs::path dir = fs::path(ELFBUG_TESTS_TARGETS_DIR) / "elfbug_fifo";
    const RemoveOnExit cleanup{dir};
    fs::create_directories(dir);
    const fs::path fifo = dir / "libfifo.so";
    REQUIRE(mkfifo(fifo.c_str(), 0600) == 0);

    std::jthread unblocker([&](const std::stop_token stop)
    {
        for(int i = 0; i < 200 && !stop.stop_requested(); ++i)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        const int fd = open(fifo.c_str(), O_WRONLY | O_NONBLOCK | O_CLOEXEC);
        if(fd != -1)
            close(fd);
    });
    ElfImageCache cache;
    const auto started = std::chrono::steady_clock::now();
    const auto result = cache.Load(getpid(), cache.Generation(), {{{0, 7}, fifo.string(), 0, 0}});
    const auto elapsed = std::chrono::steady_clock::now() - started;
    unblocker.request_stop();
    REQUIRE(elapsed < std::chrono::seconds(1));
    REQUIRE(result.size() == 1);
    REQUIRE_FALSE(result[0].image);
}

TEST_CASE("ElfImageCache opens the main executable through /proc/<pid>/exe", "[elf]")
{
    auto request = RequestForOwnExecutable();
    request.path = "/nonexistent/elfbug_tests";
    request.mainProgram = true;
    request.start = 0;
    request.end = 0;
    ElfImageCache cache;
    const auto result = cache.Load(getpid(), cache.Generation(), {request});
    REQUIRE(result.size() == 1);
    REQUIRE(result[0].image);
    REQUIRE(HasSection(*result[0].image, ".text"));
    REQUIRE_FALSE(result[0].exact);
}
