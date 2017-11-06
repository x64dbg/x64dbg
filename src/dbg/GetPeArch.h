#pragma once

#include <windows.h>

enum class PeArch
{
    Invalid,
    Native86,
    Native64,
    Dotnet86,
    Dotnet64,
    DotnetAnyCpu,
    DotnetAnyCpuPrefer32
};

// Thanks to blaquee for the investigative work!
static PeArch GetPeArch(const wchar_t* szFileName)
{
    auto result = PeArch::Invalid;
    auto hFile = CreateFileW(szFileName, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if(hFile != INVALID_HANDLE_VALUE)
    {
        // SEC_IMAGE will load the file like the Windows loader would, saving us RVA -> File offset conversion crap.
        auto hMappedFile = CreateFileMappingW(hFile, nullptr, PAGE_READONLY | SEC_IMAGE, 0, 0, nullptr);
        if(hMappedFile)
        {
            auto fileMap = MapViewOfFile(hMappedFile, FILE_MAP_READ, 0, 0, 0);
            if(fileMap)
            {
                __try
                {
                    auto pidh = PIMAGE_DOS_HEADER(fileMap);
                    if(pidh->e_magic == IMAGE_DOS_SIGNATURE)
                    {
                        auto pnth = PIMAGE_NT_HEADERS(ULONG_PTR(fileMap) + pidh->e_lfanew);
                        if(pnth->Signature == IMAGE_NT_SIGNATURE)
                        {
                            auto Machine = pnth->FileHeader.Machine;
                            if(Machine == IMAGE_FILE_MACHINE_I386 || Machine == IMAGE_FILE_MACHINE_AMD64)
                            {
                                auto isDll = (pnth->FileHeader.Characteristics & IMAGE_FILE_DLL) == IMAGE_FILE_DLL;
                                auto isFile86 = Machine == IMAGE_FILE_MACHINE_I386;

                                // Set the native architecture of the PE file (to still have something to show for if the COM directory is invalid).
                                result = isFile86 ? PeArch::Native86 : PeArch::Native64;

                                // Get the address and size of the COM (.NET) directory.
                                ULONG_PTR comAddr = 0, comSize = 0;
                                if(isFile86) // x86
                                {
                                    auto pnth32 = PIMAGE_NT_HEADERS32(pnth);
                                    comAddr = pnth32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress;
                                    comSize = pnth32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].Size;
                                }
                                else // x64
                                {
                                    auto pnth64 = PIMAGE_NT_HEADERS64(pnth);
                                    comAddr = pnth64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress;
                                    comSize = pnth64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].Size;
                                }

                                // Check if the file has a (valid) COM (.NET) directory.
                                if(comAddr && comSize >= sizeof(IMAGE_COR20_HEADER))
                                {
                                    // Find out which flavor of dotnet we're dealing with. Specifically,
                                    // "Any CPU" can be compiled with two flavors, "Prefer 32 bit" or not.
                                    // Without the 32bit preferred flag, the loader will load the .NET
                                    // environment based on the current platforms bitness (x86 or x64)
                                    // Class libraries (DLLs) cannot specify the "Prefer 32 bit".
                                    // https://mega.nz/#!vx5nVILR!jLafWGWhhsC0Qo5fE-3oEIc-uHBcRpraOo8L_KlUeXI
                                    // Binaries that do not have COMIMAGE_FLAGS_ILONLY appear to be executed
                                    // in a process that matches their native type.
                                    // https://github.com/x64dbg/x64dbg/issues/1758

                                    auto pcorh = PIMAGE_COR20_HEADER(ULONG_PTR(fileMap) + comAddr);
                                    if(pcorh->cb == sizeof(IMAGE_COR20_HEADER))
                                    {
                                        auto flags = pcorh->Flags;
#define test(x) (flags & x) == x
#define MY_COMIMAGE_FLAGS_32BITPREFERRED 0x00020000
                                        if(isFile86) // x86
                                        {
                                            if(test(COMIMAGE_FLAGS_32BITREQUIRED))
                                                result = !isDll && test(MY_COMIMAGE_FLAGS_32BITPREFERRED) ? PeArch::DotnetAnyCpuPrefer32 : PeArch::Dotnet86;
                                            else if(test(COMIMAGE_FLAGS_ILONLY))
                                                result = PeArch::DotnetAnyCpu;
                                            else
                                                result = PeArch::Dotnet86;
                                        }
                                        else // x64
                                            result = PeArch::Dotnet64;
#undef MY_COMIMAGE_FLAGS_32BITPREFERRED
#undef test
                                    }
                                }
                            }
                        }
                    }
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                }
                UnmapViewOfFile(fileMap);
            }
            CloseHandle(hMappedFile);
        }
        CloseHandle(hFile);
    }
    return result;
}