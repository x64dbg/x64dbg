#include <ElfBug/elf/ElfImageCache.h>
#include <ElfBug/process/ProcFs.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <utility>

namespace ElfBug
{
    namespace
    {
        bool readFully(const int fd, void* buffer, const size_t size, const uint64_t offset)
        {
            if(offset > static_cast<uint64_t>(INT64_MAX))
                return false;
            auto* out = static_cast<char*>(buffer);
            size_t done = 0;
            while(done < size)
            {
                const ssize_t n = pread(fd, out + done, size - done, static_cast<off_t>(offset + done));
                if(n == -1 && errno == EINTR)
                    continue;
                if(n <= 0)
                    return false;
                done += static_cast<size_t>(n);
            }
            return true;
        }

        // O_NONBLOCK so a FIFO at a module's path can't hang the open.
        constexpr int kOpenFlags = O_RDONLY | O_CLOEXEC | O_NONBLOCK | O_NOCTTY;

        std::shared_ptr<const ElfImage> parseFile(const int fd)
        {
            struct stat info = {};
            if(fstat(fd, &info) != 0 || !S_ISREG(info.st_mode))
                return nullptr;
            const ElfReader read = [fd](const uint64_t offset, void* buffer, const size_t size)
            {
                return readFully(fd, buffer, size, offset);
            };
            auto image = ElfImage::Parse(read, ElfImage::Parts::HeadersAndSections);
            return image ? std::make_shared<const ElfImage>(std::move(*image)) : nullptr;
        }

        ResolvedImage openImage(const pid_t pid, const ImageRequest & request, bool & mapFilesDenied)
        {
            ResolvedImage result{request.file};
            int fd = -1;
            if(!mapFilesDenied)
            {
                char name[64];
                std::snprintf(name, sizeof(name), "map_files/%" PRIx64 "-%" PRIx64, request.start, request.end);
                fd = open(procfs::Path(pid, name).c_str(), kOpenFlags);
                if(fd != -1)
                    result.exact = true;
                else
                    mapFilesDenied = errno == EPERM || errno == EACCES;
            }
            if(fd == -1 && request.mainProgram)
                fd = open(procfs::Path(pid, "exe").c_str(), kOpenFlags);
            if(fd == -1 && request.path.starts_with('/'))
            {
                // Paths in maps are relative to the tracee's root, which differs inside a container or chroot.
                fd = open((procfs::Path(pid, "root") + request.path).c_str(), kOpenFlags);
                if(fd == -1 && errno == ENOENT)
                    fd = open(request.path.c_str(), kOpenFlags);
            }
            if(fd != -1)
            {
                result.image = parseFile(fd);
                close(fd);
            }
            return result;
        }
    }

    std::vector<ResolvedImage> ElfImageCache::Load(const pid_t pid, const uint64_t generation, const std::vector<ImageRequest> & requests)
    {
        std::vector<ResolvedImage> results;
        results.reserve(requests.size());
        for(const auto & request : requests)
        {
            std::unique_lock lock(mMutex);
            if(generation != mGeneration)
                return {};
            auto found = mImages.find(request.file);
            if(found == mImages.end())
            {
                bool mapFilesDenied = mMapFilesDenied;
                lock.unlock();
                ResolvedImage image = openImage(pid, request, mapFilesDenied);
                lock.lock();
                if(generation != mGeneration)
                    return {};
                mMapFilesDenied |= mapFilesDenied;
                found = mImages.emplace(request.file, std::move(image)).first;
            }
            results.push_back(found->second);
        }
        return results;
    }

    void ElfImageCache::Clear()
    {
        std::lock_guard lock(mMutex);
        mImages.clear();
        mMapFilesDenied = false;
        ++mGeneration;
    }

    uint64_t ElfImageCache::Generation() const
    {
        std::lock_guard lock(mMutex);
        return mGeneration;
    }
}
