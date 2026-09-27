#pragma once

#include <ctime>

constexpr int kCloneTrapRounds = 128;

// Where segfault.cpp stores; unmapped with ASLR off.
constexpr unsigned long kSegfaultAddress = 0xdead0000;

inline void nap(const long nanos)
{
    const timespec ts{0, nanos};
    nanosleep(&ts, nullptr);
}
