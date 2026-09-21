#pragma once

#include <ctime>

constexpr int kCloneTrapRounds = 128;

inline void nap(const long nanos)
{
    const timespec ts{0, nanos};
    nanosleep(&ts, nullptr);
}
