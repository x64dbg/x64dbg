// Axel '0vercl0k' Souchet - January 22 2022
#include "udmp-parser.h"
#include <algorithm>
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <optional>

//
// Bunch of utils.
//

namespace utils {

//
// Get a string representation of the memory state.
//

const char *StateToString(const uint32_t State) {
  switch (State) {
  case 0x10'00: {
    return "MEM_COMMIT";
  }

  case 0x20'00: {
    return "MEM_RESERVE";
  }

  case 0x1'00'00: {
    return "MEM_FREE";
  }

  default: {
    return "UNKNOWN";
  }
  }
}

const char *TypeToString(const uint32_t Type) {
  switch (Type) {
  case 0x2'00'00: {
    return "MEM_PRIVATE";
  }
  case 0x4'00'00: {
    return "MEM_MAPPED";
  }
  case 0x1'00'00'00: {
    return "MEM_IMAGE";
  }
  default: {
    return "UNKNOWN";
  }
  }
}

//
// Print an Intel X86 context like windbg.
//

void PrintX86Context(const udmpparser::Context32_t &C, const int Prefix = 0) {
  printf("%*ceax=%08" PRIx32 " ebx=%08" PRIx32 " ecx=%08" PRIx32
         " edx=%08" PRIx32 " esi=%08" PRIx32 " edi=%08" PRIx32 "\n",
         Prefix, ' ', C.Eax, C.Ebx, C.Ecx, C.Edx, C.Esi, C.Edi);
  printf("%*ceip=%08" PRIx32 " esp=%08" PRIx32 " ebp=%08" PRIx32 "\n", Prefix,
         ' ', C.Eip, C.Esp, C.Ebp);
  printf("%*ccs=%04x  ss=%04x  ds=%04x  es=%04x  fs=%04x gs=%04x              "
         "efl=%08x\n",
         Prefix, ' ', C.SegCs, C.SegSs, C.SegDs, C.SegEs, C.SegFs, C.SegGs,
         C.EFlags);
}

//
// Print an Intel X64 context like windbg.
//

void PrintX64Context(const udmpparser::Context64_t &C, const int Prefix = 0) {
  printf("%*crax=%016" PRIx64 " rbx=%016" PRIx64 " rcx=%016" PRIx64 "\n",
         Prefix, ' ', C.Rax, C.Rbx, C.Rcx);
  printf("%*crdx=%016" PRIx64 " rsi=%016" PRIx64 " rdi=%016" PRIx64 "\n",
         Prefix, ' ', C.Rdx, C.Rsi, C.Rdi);
  printf("%*crip=%016" PRIx64 " rsp=%016" PRIx64 " rbp=%016" PRIx64 "\n",
         Prefix, ' ', C.Rip, C.Rsp, C.Rbp);
  printf("%*c r8=%016" PRIx64 "  r9=%016" PRIx64 " r10=%016" PRIx64 "\n",
         Prefix, ' ', C.R8, C.R9, C.R10);
  printf("%*cr11=%016" PRIx64 " r12=%016" PRIx64 " r13=%016" PRIx64 "\n",
         Prefix, ' ', C.R11, C.R12, C.R13);
  printf("%*cr14=%016" PRIx64 " r15=%016" PRIx64 "\n", Prefix, ' ', C.R14,
         C.R15);
  printf("%*ccs=%04x  ss=%04x  ds=%04x  es=%04x  fs=%04x gs=%04x              "
         "efl=%08x\n",
         Prefix, ' ', C.SegCs, C.SegSs, C.SegDs, C.SegEs, C.SegFs, C.SegGs,
         C.EFlags);
  printf("%*cfpcw=%04x    fpsw=%04x    fptw=%04x\n", Prefix, ' ', C.ControlWord,
         C.StatusWord, C.TagWord);
  printf("%*c  st0=%016" PRIx64 "%016" PRIx64 "       st1=%016" PRIx64
         "%016" PRIx64 "\n",
         Prefix, ' ', C.FloatRegisters[0].High, C.FloatRegisters[0].Low,
         C.FloatRegisters[1].High, C.FloatRegisters[1].Low);
  printf("%*c  st2=%016" PRIx64 "%016" PRIx64 "       st3=%016" PRIx64
         "%016" PRIx64 "\n",
         Prefix, ' ', C.FloatRegisters[2].High, C.FloatRegisters[2].Low,
         C.FloatRegisters[3].High, C.FloatRegisters[3].Low);
  printf("%*c  st4=%016" PRIx64 "%016" PRIx64 "       st5=%016" PRIx64
         "%016" PRIx64 "\n",
         Prefix, ' ', C.FloatRegisters[4].High, C.FloatRegisters[4].Low,
         C.FloatRegisters[5].High, C.FloatRegisters[5].Low);
  printf("%*c  st6=%016" PRIx64 "%016" PRIx64 "       st7=%016" PRIx64
         "%016" PRIx64 "\n",
         Prefix, ' ', C.FloatRegisters[6].High, C.FloatRegisters[6].Low,
         C.FloatRegisters[7].High, C.FloatRegisters[7].Low);
  printf("%*c xmm0=%016" PRIx64 "%016" PRIx64 "      xmm1=%016" PRIx64
         "%016" PRIx64 "\n",
         Prefix, ' ', C.Xmm0.High, C.Xmm0.Low, C.Xmm1.High, C.Xmm1.Low);
  printf("%*c xmm2=%016" PRIx64 "%016" PRIx64 "      xmm3=%016" PRIx64
         "%016" PRIx64 "\n",
         Prefix, ' ', C.Xmm2.High, C.Xmm2.Low, C.Xmm3.High, C.Xmm3.Low);
  printf("%*c xmm4=%016" PRIx64 "%016" PRIx64 "      xmm5=%016" PRIx64
         "%016" PRIx64 "\n",
         Prefix, ' ', C.Xmm4.High, C.Xmm4.Low, C.Xmm5.High, C.Xmm5.Low);
  printf("%*c xmm6=%016" PRIx64 "%016" PRIx64 "      xmm7=%016" PRIx64
         "%016" PRIx64 "\n",
         Prefix, ' ', C.Xmm6.High, C.Xmm6.Low, C.Xmm7.High, C.Xmm7.Low);
  printf("%*c xmm8=%016" PRIx64 "%016" PRIx64 "      xmm9=%016" PRIx64
         "%016" PRIx64 "\n",
         Prefix, ' ', C.Xmm8.High, C.Xmm8.Low, C.Xmm9.High, C.Xmm9.Low);
  printf("%*cxmm10=%016" PRIx64 "%016" PRIx64 "     xmm11=%016" PRIx64
         "%016" PRIx64 "\n",
         Prefix, ' ', C.Xmm10.High, C.Xmm10.Low, C.Xmm11.High, C.Xmm11.Low);
  printf("%*cxmm12=%016" PRIx64 "%016" PRIx64 "     xmm13=%016" PRIx64
         "%016" PRIx64 "\n",
         Prefix, ' ', C.Xmm12.High, C.Xmm12.Low, C.Xmm13.High, C.Xmm13.Low);
  printf("%*cxmm14=%016" PRIx64 "%016" PRIx64 "     xmm15=%016" PRIx64
         "%016" PRIx64 "\n",
         Prefix, ' ', C.Xmm14.High, C.Xmm14.Low, C.Xmm15.High, C.Xmm15.Low);
}

//
// Print a CPU context like windbg does.
//

void PrintContext(const udmpparser::Thread_t &T, const int Prefix = 0) {
  printf("%*cTID %" PRIx32 ", TEB %" PRIx64 "\n", Prefix, ' ', T.ThreadId,
         T.Teb);
  printf("%*cContext:\n", Prefix, ' ');
  const auto &C = T.Context;
  if (std::holds_alternative<udmpparser::Context32_t>(C)) {
    const auto &C32 = std::get<udmpparser::Context32_t>(C);
    return PrintX86Context(C32, Prefix + 2);
  }

  if (std::holds_alternative<udmpparser::Context64_t>(C)) {
    const auto &C64 = std::get<udmpparser::Context64_t>(C);
    return PrintX64Context(C64, Prefix + 2);
  }

  printf("%*cUnknown type of context!\n", Prefix, ' ');
}

void Hexdump(const uint64_t Address, const void *Buffer, size_t Len,
             const int Prefix = 0) {
  const uint8_t *Ptr = (uint8_t *)Buffer;

  for (size_t i = 0; i < Len; i += 16) {
    printf("%*c%" PRIx64 ": ", Prefix, ' ', Address + i);
    for (size_t j = 0; j < 16; j++) {
      if (i + j < Len) {
        printf("%02" PRIx8, Ptr[i + j]);
      } else {
        printf("   ");
      }
    }
    printf(" |");
    for (size_t j = 0; j < 16; j++) {
      if (i + j < Len) {
        printf("%c", isprint(Ptr[i + j]) ? char(Ptr[i + j]) : '.');
      } else {
        printf(" ");
      }
    }
    printf("|\n");
  }
}

} // namespace utils

//
// Delimiter.
//

#define DELIMITER                                                              \
  "----------------------------------------------------------------------"     \
  "----------"

//
// The options available for the parser.
//

struct Options_t {

  //
  // This is enabled if -h is used.
  //

  bool ShowHelp = false;

  //
  // This is enabled if -a is used.
  //

  bool ShowAll = false;

  //
  // This is enabled if -mods is used.
  //

  bool ShowLoadedModules = false;

  //
  // This is enabled if -mem is used.
  //

  bool ShowMemoryMap = false;

  //
  // This is enabled if -t is used.
  //

  bool ShowThreads = false;

  //
  // This holds a TID if -t is followed by an integer.
  //

  std::optional<uint32_t> Tid;

  //
  // This is enabled if -t main is used.
  //

  bool ShowForegroundThread = false;

  //
  // This is set if -dump <addr> is used.
  //

  std::optional<uint64_t> DumpAddress;

  //
  // The path to the dump file.
  //

  std::string_view DumpPath;
};

//
// Help menu.
//

void Help() {
  printf("parser.exe [-a] [-mods] [-mem] [-t [<TID|main>] [-h] [-dump <addr>] "
         "<dump path>\n");
  printf("\n");
  printf("Examples:\n");
  printf("  Show all:\n");
  printf("    parser.exe -a user.dmp\n");
  printf("  Show loaded modules:\n");
  printf("    parser.exe -mods user.dmp\n");
  printf("  Show memory map:\n");
  printf("    parser.exe -mem user.dmp\n");
  printf("  Show all threads:\n");
  printf("    parser.exe -t user.dmp\n");
  printf("  Show thread w/ specific TID:\n");
  printf("    parser.exe -t 1337 user.dmp\n");
  printf("  Show foreground thread:\n");
  printf("    parser.exe -t main user.dmp\n");
  printf("  Dump a memory page at a specific address:\n");
  printf("    parser.exe -dump 0x7ff00 user.dmp\n");
  printf("\n");
}

int main(int argc, char *argv[]) {

  //
  // Parse the options.
  //

  Options_t Opts;
  for (int ArgIdx = 1; ArgIdx < argc; ArgIdx++) {
    const std::string_view Arg(argv[ArgIdx]);
    const bool IsLastArg = (ArgIdx + 1) >= argc;
    if (Arg == "-a") {
      Opts.ShowAll = true;
    } else if (Arg == "-h") {
      Opts.ShowHelp = true;
    } else if (Arg == "-mods") {
      Opts.ShowLoadedModules = true;
    } else if (Arg == "-mem") {
      Opts.ShowMemoryMap = true;
    } else if (Arg == "-t") {
      Opts.ShowThreads = true;

      //
      // Verify that there's enough argument for an integer and the last
      // argument which is the dump.
      //
      const std::string_view NextArg(IsLastArg ? "" : argv[ArgIdx + 1]);
      if (NextArg == "main") {
        Opts.ShowForegroundThread = true;
      } else {
        char *End = nullptr;
        const uint32_t Tid = std::strtoul(NextArg.data(), &End, 0);
        const bool Valid = errno == 0 && End != NextArg.data();
        if (Valid) {
          Opts.Tid = Tid;
        }
      }

      //
      // If we grabbed a TID or set the foreground thread, skip an argument.
      //

      if (Opts.Tid.has_value() || Opts.ShowForegroundThread) {
        ArgIdx++;
      }
    } else if (Arg == "-dump") {
      const std::string_view NextArg(IsLastArg ? "" : argv[ArgIdx + 1]);
      char *End = nullptr;
      const uint64_t Address = std::strtoull(NextArg.data(), &End, 0);
      const bool Valid = errno == 0 && End != NextArg.data();
      if (Valid) {
        Opts.DumpAddress = Address;
        ArgIdx++;
      }
    } else if (IsLastArg) {

      //
      // If this is the last argument, then this is the dump path.
      //

      Opts.DumpPath = Arg;
    } else {

      //
      // Otherwise it seems that the user passed something wrong?
      //

      printf("The argument %s is not recognized.\n\n", Arg.data());
      Help();
      return EXIT_FAILURE;
    }
  }

  //
  // Display help if wanted or no argument were specified.
  //

  if (argc == 1 || Opts.ShowHelp) {
    Help();
    return argc == 1 ? EXIT_FAILURE : EXIT_SUCCESS;
  }

  //
  // Initialize the parser.
  //

  udmpparser::UserDumpParser UserDump;
  if (!UserDump.Parse(Opts.DumpPath.data())) {
    printf("Loading '%s' failed.\n", Opts.DumpPath.data());
    return EXIT_FAILURE;
  }

  //
  // Show the loaded modules.
  //

  if (Opts.ShowLoadedModules || Opts.ShowAll) {
    printf(DELIMITER "\nLoaded modules:\n");
    const auto &Modules = UserDump.GetModules();
    for (const auto &[Base, ModuleInfo] : Modules) {
      printf("  %016" PRIx64 ": %s\n", Base, ModuleInfo.ModuleName.c_str());
    }
  }

  //
  // Show the memory map.
  //

  if (Opts.ShowMemoryMap || Opts.ShowAll) {
    printf(DELIMITER "\nMemory map:\n");
    for (const auto &[_, Descriptor] : UserDump.GetMem()) {

      //
      // Get a string representation for the state of the region.
      //

      const char *State = utils::StateToString(Descriptor.State);

      //
      // Get a string representation for the memory type of the region.
      //

      const char *Type = "";
      if (strcmp(State, "MEM_FREE")) {
        Type = utils::TypeToString(Descriptor.Type);
      }

      //
      // Display start / end / size / state / type of the region.
      //

      const uint64_t BaseAddress = Descriptor.BaseAddress;
      const uint64_t RegionSize = Descriptor.RegionSize;
      const uint64_t EndAddress = BaseAddress + RegionSize;
      printf("  %16" PRIx64 " %16" PRIx64 " %16" PRIx64 " %11s %11s",
             BaseAddress, EndAddress, RegionSize, State, Type);

      //
      // Is the region actually part of a module?
      //

      const auto &Module = UserDump.GetModule(BaseAddress);

      //
      // If we found a module that overlaps with this region, let's dump it as
      // well.
      //

      if (Module != nullptr) {

        //
        // Find the last '\' to get the module name off its path.
        //

        const auto &ModulePathName = Module->ModuleName;
        auto ModuleNameOffset = ModulePathName.find_last_of('\\');
        if (ModuleNameOffset == ModulePathName.npos) {
          ModuleNameOffset = 0;
        } else {
          ModuleNameOffset++;
        }

        //
        // Show the module path & module name.
        //

        printf("   [%s; \"%s\"]", &ModulePathName[ModuleNameOffset],
               ModulePathName.c_str());
      }

      //
      // Dump the first 4 bytes of the region if its available in the dump.
      //

      if (Descriptor.DataSize >= 4) {
        printf("   %02" PRIx8 " %02" PRIx8 " %02" PRIx8 " %02" PRIx8 "...",
               Descriptor.Data[0], Descriptor.Data[1], Descriptor.Data[2],
               Descriptor.Data[3]);
      }

      //
      // Phew! We are done for this region :)
      //

      printf("\n");
    }
  }

  //
  // Show the threads.
  //

  if (Opts.ShowThreads || Opts.ShowAll) {
    printf(DELIMITER "\nThreads:\n");
    const auto &ForegroundTid = UserDump.GetForegroundThreadId();
    for (const auto &[Tid, Thread] : UserDump.GetThreads()) {

      //
      // If we are looking for a specific TID, skip unless we have a match.
      //

      if (Opts.Tid && Tid != *Opts.Tid) {
        continue;
      }

      //
      // If we are looking for the foreground thread, skip unless we have a
      // match.
      //

      if (Opts.ShowForegroundThread && Tid != *ForegroundTid) {
        continue;
      }

      //
      // If we have a match or no filters, dump the context.
      //

      utils::PrintContext(Thread, 2);
    }
  }

  //
  // Dump virtual memory.
  //

  if (Opts.DumpAddress.has_value()) {
    printf(DELIMITER "\nMemory:\n");

    //
    // Find a block of virtual memory that overlaps with the address we want to
    // dump.
    //

    const uint64_t DumpAddress = *Opts.DumpAddress;
    const auto &Block = UserDump.GetMemBlock(DumpAddress);

    //
    // If we found a match, let's go.
    //

    if (Block != nullptr) {

      //
      // Display basic information about the matching memory region.
      //

      const uint64_t BlockStart = Block->BaseAddress;
      const uint64_t BlockSize = Block->DataSize;
      const uint64_t BlockEnd = BlockStart + BlockSize;
      printf("%016" PRIx64 " -> %016" PRIx64 "\n", BlockStart, BlockEnd);
      if (BlockSize > 0) {

        //
        // Calculate where from we need to start dumping, and the appropriate
        // amount of bytes to dump.
        //

        const uint64_t OffsetFromStart = DumpAddress - BlockStart;
        const uint64_t Remaining = BlockSize - OffsetFromStart;
        const uint64_t MaxSize = 0x100;
        const uint64_t DumpSize = std::min(MaxSize, Remaining);
        utils::Hexdump(BlockStart + OffsetFromStart,
                       Block->Data + OffsetFromStart, size_t(DumpSize), 2);
      } else {
        printf("The dump does not have the content of the memory at %" PRIx64
               "\n",
               DumpAddress);
      }
    } else {
      printf("No memory block were found for %" PRIx64 ".\n", DumpAddress);
    }
  }

  return EXIT_SUCCESS;
}