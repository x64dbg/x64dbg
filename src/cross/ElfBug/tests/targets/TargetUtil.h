#pragma once

#include <cstddef>
#include <ctime>

constexpr int kCloneTrapRounds = 128;

// mem_map_target's mm_bss, which spills past its file-backed page into the anonymous tail.
constexpr size_t kMemMapBssSize = 64 * 1024;
constexpr size_t kMemMapLongSections = 8;
// The anonymous span mem_map_target puts its own header page at both ends of.
constexpr size_t kMemMapPhantomSpan = 4 * 1024 * 1024;

// Where segfault.cpp stores; unmapped with ASLR off.
constexpr unsigned long kSegfaultAddress = 0xdead0000;

inline void nap(const long nanos)
{
    const timespec ts{0, nanos};
    nanosleep(&ts, nullptr);
}
