#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace ElfBug
{
    constexpr uint64_t kPageSize = 0x1000;

    inline uint64_t PageFloor(const uint64_t value)
    {
        return value & ~(kPageSize - 1);
    }

    inline uint64_t PageCeil(const uint64_t value)
    {
        return PageFloor(value + kPageSize - 1);
    }

    struct ElfSegment
    {
        uint64_t vaddr = 0;
        uint64_t memsz = 0;
        uint64_t offset = 0;
        uint64_t filesz = 0;
        uint32_t flags = 0;

        [[nodiscard]] uint64_t End() const { return vaddr + std::max(memsz, filesz); }
    };

    struct ElfSection
    {
        std::string name;
        uint64_t addr = 0;
        uint64_t size = 0;
    };

    // Reads at a file offset. A reader over tracee memory adds the image base, which only holds where the file is mapped linearly.
    using ElfReader = std::function<bool(uint64_t offset, void* buffer, size_t size)>;

    class ElfImage
    {
    public:
        enum class Parts
        {
            Headers,
            HeadersAndSections,
        };

        static std::optional<ElfImage> Parse(const ElfReader & read, Parts parts);

        [[nodiscard]] const std::vector<ElfSegment> & Segments() const { return mSegments; }
        [[nodiscard]] const std::vector<ElfSection> & Sections() const { return mSections; }
        [[nodiscard]] std::span<const std::byte> HeaderBytes() const { return mHeaderBytes; }
        [[nodiscard]] std::optional<uint64_t> LoadBias(uint64_t mappingStart, uint64_t mappingOffset) const;
        [[nodiscard]] uint64_t End() const;

    private:
        std::vector<ElfSegment> mSegments;
        std::vector<ElfSection> mSections;
        std::vector<std::byte> mHeaderBytes;
    };
}
