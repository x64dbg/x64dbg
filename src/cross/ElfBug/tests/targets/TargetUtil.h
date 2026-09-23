#pragma once

#include <ctime>

constexpr int kCloneTrapRounds = 128;

// Where segfault.cpp stores. Nothing is mapped there with ASLR off, and being non-zero is
// what lets a test tell a reported fault address apart from a field nobody filled in.
constexpr unsigned long kSegfaultAddress = 0xdead0000;

inline void nap(const long nanos)
{
    const timespec ts{0, nanos};
    nanosleep(&ts, nullptr);
}
