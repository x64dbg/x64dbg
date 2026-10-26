#pragma once

#include <ElfBug/elf/ElfImage.h>
#include <ElfBug/types/ImageId.h>
#include <sys/types.h>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace ElfBug
{
    struct ImageRequest
    {
        ImageId file;
        std::string path;
        // One mapping of the file, for /proc/<pid>/map_files.
        uint64_t start = 0;
        uint64_t end = 0;
        // The main program, so /proc/<pid>/exe can open it without root.
        bool mainProgram = false;
    };

    struct ResolvedImage
    {
        ImageId file;
        std::shared_ptr<const ElfImage> image;
        // Opened through map_files: the mapped inode itself, so no validation is needed.
        bool exact = false;
    };

    // Each file is parsed once per session, with no lock held so Clear never waits on file I/O.
    // A Load started before the last Clear returns nothing.
    class ElfImageCache
    {
    public:
        std::vector<ResolvedImage> Load(pid_t pid, uint64_t generation, const std::vector<ImageRequest> & requests);
        void Clear();
        [[nodiscard]] uint64_t Generation() const;

    private:
        mutable std::mutex mMutex;
        std::map<ImageId, ResolvedImage> mImages;
        uint64_t mGeneration = 0;
        bool mMapFilesDenied = false;
    };
}
