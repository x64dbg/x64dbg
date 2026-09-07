#pragma once

#include <cstddef>
#include <ElfBug/types/ElfBug.h>
#include <ElfBug/types/Global.h>

namespace ElfBug
{
    // Sets nextAddr to rip + instructionLength on a non-None result, 0 otherwise.
    StepOverKind ClassifyStepOver(const uint8* bytes, size_t n, ptr rip, ptr & nextAddr);
}
