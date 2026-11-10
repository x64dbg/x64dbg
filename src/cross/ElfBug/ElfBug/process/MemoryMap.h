#pragma once

#include <ElfBug/elf/ElfImage.h>
#include <ElfBug/elf/ElfImageCache.h>
#include <ElfBug/types/ImageId.h>
#include <sys/types.h>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace ElfBug
{
    enum class RegionType
    {
        Private,
        Image,
        Mapped,
    };

    enum class Party
    {
        User,
        System,
    };

    struct ThreadPointers
    {
        pid_t tid = 0;
        uint64_t stackPointer = 0;
        uint64_t threadPointer = 0;
    };

    struct RegionLabel
    {
        uint64_t address = 0;
        std::string text;
    };

    struct MemoryRegion
    {
        uint64_t start = 0;
        uint64_t end = 0;
        uint64_t offset = 0;
        ImageId file;
        std::string perms;
        // The path, or the pseudo name such as [heap].
        std::string name;
        RegionType type = RegionType::Private;
        std::optional<size_t> module;
        std::vector<RegionLabel> labels;

        [[nodiscard]] bool Readable() const { return perms[0] == 'r'; }
        [[nodiscard]] bool Executable() const { return perms[2] == 'x'; }
        [[nodiscard]] bool Shared() const { return perms[3] == 's'; }
    };

    struct MemoryModule
    {
        std::string path;
        ImageId file;
        uint64_t base = 0;
        uint64_t bias = 0;
        Party party = Party::User;
        bool mainProgram = false;
        // Read from tracee memory.
        std::shared_ptr<const ElfImage> headers;
        bool imageResolved = false;
        // Null once resolved if the file couldn't be read or doesn't match memory.
        std::shared_ptr<const ElfImage> image;

        [[nodiscard]] std::string_view FileName() const;
    };

    struct MemoryPage
    {
        uint64_t base = 0;
        uint64_t size = 0;
        uint64_t moduleBase = 0;
        RegionType type = RegionType::Private;
        Party party = Party::User;
        std::string perms;
        std::string section;
        std::string info;
    };

    // Modules come from /proc/<pid>/maps rather than r_debug, which is only filled in once ld.so is done.
    // Maps also works at the first stop, for static binaries and for dlmopen namespaces.
    class MemoryMap
    {
    public:
        // Reads only /proc and tracee memory. Section names come later, through PendingImages and AttachImages.
        void Rebuild(pid_t pid, std::span<const ThreadPointers> threads);
        void Clear();

        [[nodiscard]] std::vector<ImageRequest> PendingImages() const;
        void AttachImages(const std::vector<ResolvedImage> & images);

        [[nodiscard]] const MemoryRegion* FindRegion(uint64_t address) const;
        [[nodiscard]] const MemoryModule* FindModule(uint64_t address) const;
        [[nodiscard]] std::vector<MemoryPage> Pages(bool sectionView) const;

    private:
        std::vector<MemoryRegion> mRegions;
        std::vector<MemoryModule> mModules;
    };
}
