// Linked for 2MB pages, then moves its own code onto anonymous memory the way huge-page text remappers do.
#include <link.h>
#include <sys/mman.h>
#include <cstdint>
#include <cstring>
#include "TargetUtil.h"

extern "C"
{
    // 1 once the code is anonymous, 2 if moving it failed.
    volatile int ht_remapped = 0;

    void ht_code()
    {
    }
}

namespace
{
    struct CodeRange
    {
        uintptr_t start = 0;
        size_t size = 0;
    };

    int findCode(dl_phdr_info* info, size_t, void* data)
    {
        auto* code = static_cast<CodeRange*>(data);
        for(int i = 0; i < info->dlpi_phnum; ++i)
        {
            const ElfW(Phdr) & program = info->dlpi_phdr[i];
            if(program.p_type != PT_LOAD || !(program.p_flags & PF_X))
                continue;
            const uintptr_t start = (info->dlpi_addr + program.p_vaddr) & ~uintptr_t{0xfff};
            const uintptr_t end = (info->dlpi_addr + program.p_vaddr + program.p_memsz + 0xfff) & ~uintptr_t{0xfff};
            *code = {start, end - start};
        }
        return 1;
    }

    bool moveCode()
    {
        CodeRange code;
        dl_iterate_phdr(findCode, &code);
        if(code.size == 0)
            return false;
        void* copy = mmap(nullptr, code.size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if(copy == MAP_FAILED)
            return false;
        std::memcpy(copy, reinterpret_cast<void*>(code.start), code.size);
        return mprotect(copy, code.size, PROT_READ | PROT_EXEC) == 0 &&
               mremap(copy, code.size, code.size, MREMAP_MAYMOVE | MREMAP_FIXED, reinterpret_cast<void*>(code.start)) != MAP_FAILED;
    }
}

int main()
{
    ht_remapped = moveCode() ? 1 : 2;
    for(;;)
        nap(1000000);
}
