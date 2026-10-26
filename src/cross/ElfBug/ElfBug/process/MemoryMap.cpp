#include <ElfBug/process/MemoryMap.h>
#include <ElfBug/process/ProcFs.h>
#include <ElfBug/process/TraceeMemory.h>
#include <elf.h>
#include <algorithm>
#include <optional>
#include <string_view>
#include <utility>

namespace ElfBug
{
    namespace
    {
        constexpr std::string_view kHeapName = "[heap]";
        constexpr std::string_view kStackName = "[stack]";
        constexpr std::string_view kVdsoName = "[vdso]";
        constexpr std::string_view kGlibcBssName = "[anon: glibc: .bss ";
        constexpr std::string_view kSystemPrefixes[] =
        {
            "/lib/", "/lib32/", "/lib64/", "/usr/lib/", "/usr/lib32/", "/usr/lib64/",
        };

        std::vector<MemoryRegion> readRegions(const pid_t pid)
        {
            std::vector<MemoryRegion> regions;
            const std::string maps = procfs::ReadFile(procfs::Path(pid, "maps"));
            for(const std::string_view line : procfs::Split(maps, '\n'))
            {
                const auto entry = procfs::ParseMapsLine(line);
                if(!entry)
                    continue;
                MemoryRegion region;
                region.start = entry->start;
                region.end = entry->end;
                region.offset = entry->offset;
                region.file = entry->file;
                region.perms = std::string(entry->perms);
                region.name = std::string(entry->path);
                region.type = region.file.Known() || region.Shared() ? RegionType::Mapped : RegionType::Private;
                regions.push_back(std::move(region));
            }
            return regions;
        }

        Party partyOf(const std::string & path, const bool mainProgram)
        {
            if(mainProgram)
                return Party::User;
            if(path == kVdsoName)
                return Party::System;
            for(const auto prefix : kSystemPrefixes)
            {
                if(path.starts_with(prefix))
                    return Party::System;
            }
            return Party::User;
        }

        std::optional<MemoryModule> reuseModule(std::vector<MemoryModule> & previous, const MemoryRegion & first)
        {
            const auto found = std::lower_bound(previous.begin(), previous.end(), first.start, [](const MemoryModule & module, const uint64_t base)
            {
                return module.base < base;
            });
            if(found == previous.end() || found->base != first.start || found->file != first.file)
                return std::nullopt;
            return std::move(*found);
        }

        bool isAnonymous(const MemoryRegion & region)
        {
            return !region.file.Known() && region.name.empty();
        }

        bool joinsAnonymously(const MemoryModule & module, const MemoryRegion & region, const uint64_t joinedEnd)
        {
            if(region.file.Known() || !(region.name.empty() || region.name == kHeapName || region.name.starts_with(kGlibcBssName)))
                return false;
            for(const auto & segment : module.headers->Segments())
            {
                const bool inside = region.start < module.bias + segment.End() && region.end > module.bias + segment.vaddr;
                if(inside && (region.start == joinedEnd || (segment.flags & PF_X)))
                    return true;
            }
            return false;
        }

        bool sameFile(const MemoryRegion & region, const ImageId & file)
        {
            return file.Known() && region.file == file;
        }

        std::optional<size_t> regionIndex(const std::vector<MemoryRegion> & regions, const uint64_t address)
        {
            auto found = std::upper_bound(regions.begin(), regions.end(), address, [](const uint64_t value, const MemoryRegion & region)
            {
                return value < region.start;
            });
            if(found == regions.begin())
                return std::nullopt;
            --found;
            if(address >= found->end)
                return std::nullopt;
            return static_cast<size_t>(found - regions.begin());
        }

        // [vdso], or the first mapping of a file the loader split into segments.
        bool startsModule(const std::vector<MemoryRegion> & regions, const size_t firstIndex)
        {
            const MemoryRegion & first = regions[firstIndex];
            if(first.name == kVdsoName)
                return true;
            if(!first.file.Known() || first.offset != 0)
                return false;
            if(first.Executable())
                return true;
            for(size_t i = firstIndex + 1; i < regions.size(); ++i)
            {
                const MemoryRegion & region = regions[i];
                if(isAnonymous(region))
                    continue;
                if(!sameFile(region, first.file))
                    return false;
                if(region.Executable() || region.start - first.start != region.offset)
                    return true;
            }
            return false;
        }

        // Each PT_LOAD with file content is mapped from this file at its offset, or is code moved onto anonymous huge pages.
        bool segmentsMapped(const std::vector<MemoryRegion> & regions, const ImageId & file, const ElfImage & image, const uint64_t bias)
        {
            for(const auto & segment : image.Segments())
            {
                if(segment.filesz == 0)
                    continue;
                const uint64_t address = bias + PageFloor(segment.vaddr);
                const auto index = regionIndex(regions, address);
                if(!index)
                    return false;
                const MemoryRegion & region = regions[*index];
                if(isAnonymous(region) && (segment.flags & PF_X))
                    continue;
                if(!sameFile(region, file) || region.offset + (address - region.start) != PageFloor(segment.offset))
                    return false;
            }
            return true;
        }

        std::optional<MemoryModule> detectModule(const pid_t pid, const std::vector<MemoryRegion> & regions, const MemoryRegion & first)
        {
            // The vDSO has no file on disk, so its sections are read straight from memory.
            const bool vdso = first.name == kVdsoName;
            const ElfReader read = [pid, base = first.start](const uint64_t offset, void* buffer, const size_t size)
            {
                return ReadTraceeMemory(pid, base + offset, buffer, size) == static_cast<ssize_t>(size);
            };
            auto image = ElfImage::Parse(read, vdso ? ElfImage::Parts::HeadersAndSections : ElfImage::Parts::Headers);
            if(!image)
                return std::nullopt;
            const auto bias = image->LoadBias(first.start, first.offset);
            if(!bias)
                return std::nullopt;
            if(!vdso && !segmentsMapped(regions, first.file, *image, *bias))
                return std::nullopt;

            MemoryModule module;
            module.file = first.file;
            module.base = first.start;
            module.bias = *bias;
            module.headers = std::make_shared<const ElfImage>(std::move(*image));
            if(vdso)
            {
                module.imageResolved = true;
                module.image = module.headers;
            }
            return module;
        }

        void splitRegion(std::vector<MemoryRegion> & regions, const size_t index, const uint64_t at)
        {
            MemoryRegion rest = regions[index];
            rest.start = at;
            regions[index].end = at;
            regions.insert(regions.begin() + static_cast<std::ptrdiff_t>(index) + 1, std::move(rest));
        }

        void placeLabels(std::vector<MemoryRegion> & regions, const std::span<const ThreadPointers> threads)
        {
            const auto place = [&regions](const uint64_t address, std::string text)
            {
                if(address == 0)
                    return;
                if(const auto index = regionIndex(regions, address))
                    regions[*index].labels.push_back({address, std::move(text)});
            };
            for(const auto & thread : threads)
            {
                const std::string tid = std::to_string(thread.tid);
                place(thread.threadPointer, "TLS (" + tid + ")");
                place(thread.stackPointer, "Stack (" + tid + ")");
            }
        }

        std::vector<const ElfSection*> sectionsIn(const MemoryModule & module, const uint64_t start, const uint64_t end)
        {
            std::vector<const ElfSection*> sections;
            if(!module.image)
                return sections;
            for(const auto & section : module.image->Sections())
            {
                if(module.bias + section.addr < end && module.bias + section.addr + section.size > start)
                    sections.push_back(&section);
            }
            return sections;
        }

        const ElfSection* sectionAt(const MemoryModule & module, const uint64_t address)
        {
            if(!module.image)
                return nullptr;
            const ElfSection* found = nullptr;
            for(const auto & section : module.image->Sections())
            {
                if(address >= module.bias + section.addr && address < module.bias + section.addr + section.size)
                    found = &section;
            }
            return found;
        }

        void appendSectionName(std::string & info, const ElfSection & section)
        {
            info += info.empty() ? " \"" : ", \"";
            info += section.name;
            info += '"';
        }

        MemoryPage pageFor(const MemoryRegion & region, const MemoryModule* module, const uint64_t start, const uint64_t end, std::string info)
        {
            MemoryPage page;
            page.base = start;
            page.size = end - start;
            page.moduleBase = module ? module->base : 0;
            page.type = region.type;
            page.party = module ? module->party : Party::User;
            page.perms = region.perms;
            page.info = std::move(info);
            for(const auto & label : region.labels)
            {
                if(label.address < start || label.address >= end)
                    continue;
                if(!page.info.empty())
                    page.info += ", ";
                page.info += label.text;
            }
            if(page.info.empty() && page.perms.starts_with("---"))
                page.info = "Reserved";
            return page;
        }

        MemoryPage plainPage(const MemoryRegion & region)
        {
            std::string info = region.name;
            if(region.name == kHeapName)
                info = "Heap";
            else if(region.name == kStackName)
                info = region.labels.empty() ? "Stack" : "";
            return pageFor(region, nullptr, region.start, region.end, std::move(info));
        }

        MemoryPage regionPage(const MemoryRegion & region, const MemoryModule & module)
        {
            std::string info = region.start == module.base ? std::string(module.FileName()) : std::string();
            std::string mainSection;
            uint64_t largest = 0;
            for(const ElfSection* section : sectionsIn(module, region.start, region.end))
            {
                appendSectionName(info, *section);
                const uint64_t start = module.bias + section->addr;
                const uint64_t overlap = std::min(start + section->size, region.end) - std::max(start, region.start);
                if(overlap > largest)
                {
                    largest = overlap;
                    mainSection = section->name;
                }
            }
            MemoryPage page = pageFor(region, &module, region.start, region.end, std::move(info));
            page.section = std::move(mainSection);
            return page;
        }

        void appendSectionPages(std::vector<MemoryPage> & pages, const MemoryRegion & region, const MemoryModule & module)
        {
            std::vector<uint64_t> cuts{region.start};
            for(const ElfSection* section : sectionsIn(module, region.start, region.end))
            {
                if(module.bias + section->addr > region.start)
                    cuts.push_back(module.bias + section->addr);
            }
            std::sort(cuts.begin(), cuts.end());
            cuts.erase(std::unique(cuts.begin(), cuts.end()), cuts.end());

            for(size_t i = 0; i < cuts.size(); ++i)
            {
                const uint64_t start = cuts[i];
                const uint64_t end = i + 1 < cuts.size() ? cuts[i + 1] : region.end;
                const ElfSection* section = sectionAt(module, start);
                std::string info;
                if(section)
                    appendSectionName(info, *section);
                else if(start == module.base)
                    info = module.FileName();
                MemoryPage page = pageFor(region, &module, start, end, std::move(info));
                if(section)
                    page.section = section->name;
                pages.push_back(std::move(page));
            }
        }
    }

    std::string_view MemoryModule::FileName() const
    {
        const std::string_view name = path;
        const size_t slash = name.find_last_of('/');
        return slash == std::string_view::npos ? name : name.substr(slash + 1);
    }

    void MemoryMap::Rebuild(const pid_t pid, const std::span<const ThreadPointers> threads)
    {
        std::vector<MemoryModule> previous = std::move(mModules);
        mModules.clear();
        mRegions = readRegions(pid);
        const std::string exePath = procfs::ReadLink(procfs::Path(pid, "exe"));

        for(size_t i = 0; i < mRegions.size(); ++i)
        {
            const MemoryRegion & first = mRegions[i];
            if(first.module || !startsModule(mRegions, i))
                continue;

            std::optional<MemoryModule> module = reuseModule(previous, first);
            if(!module)
                module = detectModule(pid, mRegions, first);
            if(!module)
                continue;
            module->path = first.name;
            module->mainProgram = first.name == exePath;
            module->party = partyOf(first.name, module->mainProgram);

            const size_t index = mModules.size();
            const uint64_t end = module->bias + module->headers->End();
            mRegions[i].module = index;
            mRegions[i].type = RegionType::Image;
            uint64_t joinedEnd = mRegions[i].end;
            for(size_t j = i + 1; j < mRegions.size() && mRegions[j].start < end; ++j)
            {
                if(mRegions[j].module)
                    continue;
                if(!sameFile(mRegions[j], module->file) && !joinsAnonymously(*module, mRegions[j], joinedEnd))
                    continue;
                if(!mRegions[j].file.Known() && mRegions[j].end > PageCeil(end))
                    splitRegion(mRegions, j, PageCeil(end));
                mRegions[j].module = index;
                mRegions[j].type = RegionType::Image;
                joinedEnd = mRegions[j].end;
            }
            mModules.push_back(std::move(*module));
        }
        placeLabels(mRegions, threads);
    }

    void MemoryMap::Clear()
    {
        mRegions.clear();
        mModules.clear();
    }

    std::vector<ImageRequest> MemoryMap::PendingImages() const
    {
        std::vector<ImageRequest> requests;
        for(const auto & module : mModules)
        {
            if(module.imageResolved)
                continue;
            const bool asked = std::any_of(requests.begin(), requests.end(), [&](const ImageRequest & request)
            {
                return request.file == module.file;
            });
            if(asked)
                continue;
            requests.push_back({module.file, module.path, module.base, FindRegion(module.base)->end, module.mainProgram});
        }
        return requests;
    }

    void MemoryMap::AttachImages(const std::vector<ResolvedImage> & images)
    {
        for(const auto & resolved : images)
        {
            for(auto & module : mModules)
            {
                if(module.imageResolved || module.file != resolved.file)
                    continue;
                module.imageResolved = true;
                if(!resolved.image)
                    continue;
                // A file found by path may not be the one mapped, so it has to match the headers in memory.
                const auto fileBytes = resolved.image->HeaderBytes();
                const auto memoryBytes = module.headers->HeaderBytes();
                if(resolved.exact || std::equal(fileBytes.begin(), fileBytes.end(), memoryBytes.begin(), memoryBytes.end()))
                    module.image = resolved.image;
            }
        }
    }

    const MemoryRegion* MemoryMap::FindRegion(const uint64_t address) const
    {
        const auto index = regionIndex(mRegions, address);
        return index ? &mRegions[*index] : nullptr;
    }

    const MemoryModule* MemoryMap::FindModule(const uint64_t address) const
    {
        const MemoryRegion* region = FindRegion(address);
        return region && region->module ? &mModules[*region->module] : nullptr;
    }

    std::vector<MemoryPage> MemoryMap::Pages(const bool sectionView) const
    {
        std::vector<MemoryPage> pages;
        pages.reserve(mRegions.size());
        for(const auto & region : mRegions)
        {
            if(!region.module)
                pages.push_back(plainPage(region));
            else if(sectionView)
                appendSectionPages(pages, region, mModules[*region.module]);
            else
                pages.push_back(regionPage(region, mModules[*region.module]));
        }
        return pages;
    }
}
