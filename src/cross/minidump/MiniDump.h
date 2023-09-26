#pragma once

#include "udmp-parser.h"
#include "udmp-utils.h"
#include "Disassembler/Architecture.h"

namespace MiniDump
{
    Architecture* Architecture();
    void Load(udmpparser::UserDumpParser* parser);
}
