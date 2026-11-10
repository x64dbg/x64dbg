#include <ElfBug/process/TraceeMemory.h>
#include <ElfBug/process/ProcFs.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <unistd.h>

namespace ElfBug
{
    ssize_t ReadTraceeVm(const pid_t pid, const ptr address, void* buffer, const ptr size)
    {
        iovec local{};
        local.iov_base = buffer;
        local.iov_len = size;

        iovec remote{};
        remote.iov_base = reinterpret_cast<void*>(address);
        remote.iov_len = size;

        return process_vm_readv(pid, &local, 1, &remote, 1, 0);
    }

    ssize_t ReadTraceeMemory(const pid_t pid, const ptr address, void* buffer, const ptr size)
    {
        const ssize_t read = ReadTraceeVm(pid, address, buffer, size);
        if(read != -1)
            return read;
        const int fd = open(procfs::Path(pid, "mem").c_str(), O_RDONLY | O_CLOEXEC);
        if(fd == -1)
            return -1;
        const ssize_t fallback = pread(fd, buffer, size, static_cast<off_t>(address));
        close(fd);
        return fallback;
    }
}
