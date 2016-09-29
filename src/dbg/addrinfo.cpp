/**
 @file addrinfo.cpp

 @brief Implements the addrinfo class.
 */

#include "addrinfo.h"
#include "debugger.h"
#include "memory.h"
#include "module.h"

///api functions
bool apienumexports(duint base, const EXPORTENUMCALLBACK & cbEnum)
{
    MEMORY_BASIC_INFORMATION mbi;
    VirtualQueryEx(fdProcessInfo->hProcess, (const void*)base, &mbi, sizeof(mbi));
    duint size = mbi.RegionSize;
    Memory<void*> buffer(size, "apienumexports:buffer");
    if(!MemRead(base, buffer(), size))
        return false;
    IMAGE_NT_HEADERS* pnth = (IMAGE_NT_HEADERS*)((duint)buffer() + GetPE32DataFromMappedFile((ULONG_PTR)buffer(), 0, UE_PE_OFFSET));
    duint export_dir_rva = pnth->OptionalHeader.DataDirectory[0].VirtualAddress;
    duint export_dir_size = pnth->OptionalHeader.DataDirectory[0].Size;
    IMAGE_EXPORT_DIRECTORY export_dir;
    memset(&export_dir, 0, sizeof(export_dir));
    MemRead((export_dir_rva + base), &export_dir, sizeof(export_dir));
    unsigned int NumberOfNames = export_dir.NumberOfNames;
    if(!export_dir.NumberOfFunctions || !NumberOfNames) //no named exports
        return false;
    char modname[MAX_MODULE_SIZE] = "";
    ModNameFromAddr(base, modname, true);
    duint original_name_va = export_dir.Name + base;
    char original_name[deflen] = "";
    memset(original_name, 0, sizeof(original_name));
    MemRead(original_name_va, original_name, deflen);
    char* AddrOfFunctions_va = (char*)(export_dir.AddressOfFunctions + base); //not a valid local pointer
    char* AddrOfNames_va = (char*)(export_dir.AddressOfNames + base); //not a valid local pointer
    char* AddrOfNameOrdinals_va = (char*)(export_dir.AddressOfNameOrdinals + base); //not a valid local pointer
    for(DWORD i = 0; i < NumberOfNames; i++)
    {
        DWORD curAddrOfName = 0;
        MemRead((duint)(AddrOfNames_va + sizeof(DWORD)*i), &curAddrOfName, sizeof(DWORD));
        char* cur_name_va = (char*)(curAddrOfName + base);
        char cur_name[deflen] = "";
        memset(cur_name, 0, deflen);
        MemRead((duint)cur_name_va, cur_name, deflen);
        WORD curAddrOfNameOrdinals = 0;
        MemRead((duint)(AddrOfNameOrdinals_va + sizeof(WORD)*i), &curAddrOfNameOrdinals, sizeof(WORD));
        DWORD curFunctionRva = 0;
        MemRead((duint)(AddrOfFunctions_va + sizeof(DWORD)*curAddrOfNameOrdinals), &curFunctionRva, sizeof(DWORD));

        if(curFunctionRva >= export_dir_rva && curFunctionRva < export_dir_rva + export_dir_size)
        {
            char forwarded_api[deflen] = "";
            memset(forwarded_api, 0, deflen);
            MemRead((curFunctionRva + base), forwarded_api, deflen);
            int len = (int)strlen(forwarded_api);
            int j = 0;
            while(forwarded_api[j] != '.' && j < len)
                j++;
            if(forwarded_api[j] == '.')
            {
                forwarded_api[j] = 0;
                HINSTANCE hTempDll = LoadLibraryExA(forwarded_api, 0, DONT_RESOLVE_DLL_REFERENCES | LOAD_LIBRARY_AS_DATAFILE);
                if(hTempDll)
                {
                    duint local_addr = (duint)GetProcAddress(hTempDll, forwarded_api + j + 1);
                    if(local_addr)
                    {
                        duint remote_addr = ImporterGetRemoteAPIAddress(fdProcessInfo->hProcess, local_addr);
                        cbEnum(base, modname, cur_name, remote_addr);
                    }
                }
            }
        }
        else
        {
            cbEnum(base, modname, cur_name, curFunctionRva + base);
        }
    }
    return true;
}

bool apienumimports(duint base, const IMPORTENUMCALLBACK & cbEnum)
{
    // Variables
    bool readSuccess;
    Memory<char*> importName(MAX_IMPORT_SIZE + 1, "apienumimports:buffer");
    char importModuleName[MAX_MODULE_SIZE] = "";
    duint regionSize;
    ULONG_PTR importTableRva, importTableSize;
    MEMORY_BASIC_INFORMATION mbi;
    PIMAGE_IMPORT_DESCRIPTOR importTableVa;
    IMAGE_IMPORT_DESCRIPTOR importDescriptor;
    PIMAGE_THUNK_DATA imageIATVa, imageINTVa;
    IMAGE_THUNK_DATA imageOftThunkData, imageFtThunkData;
    PIMAGE_IMPORT_BY_NAME pImageImportByNameVa;

    // Get page size
    VirtualQueryEx(fdProcessInfo->hProcess, (const void*)base, &mbi, sizeof(mbi));
    regionSize = mbi.RegionSize;
    Memory<void*> buffer(regionSize, "apienumimports:buffer");

    // Read first page into buffer
    if(!MemRead(base, buffer(), regionSize))
        return false;

    // Import Table address and size
    importTableRva = GetPE32DataFromMappedFile((duint)buffer(), 0, UE_IMPORTTABLEADDRESS);
    importTableSize = GetPE32DataFromMappedFile((duint)buffer(), 0, UE_IMPORTTABLESIZE);

    // Return if no imports
    if(!importTableSize)
        return false;

    importTableVa = (PIMAGE_IMPORT_DESCRIPTOR)(base + importTableRva);

    readSuccess = MemRead((duint)importTableVa, &importDescriptor, sizeof(importDescriptor));

    // Loop through all dlls
    while(readSuccess && importDescriptor.FirstThunk)
    {
        // Copy module name into importModuleName
        MemRead((duint)(base + importDescriptor.Name), &importModuleName, MAX_MODULE_SIZE);

        imageIATVa = (PIMAGE_THUNK_DATA)(base + importDescriptor.FirstThunk);
        imageINTVa = (PIMAGE_THUNK_DATA)(base + importDescriptor.OriginalFirstThunk);

        if(!MemRead((duint)imageIATVa, &imageFtThunkData, sizeof(imageFtThunkData)))
            return false;

        if(!MemRead((duint)imageINTVa, &imageOftThunkData, sizeof(imageOftThunkData)))
            return false;

        // Loop through all imported function in this dll
        while(imageFtThunkData.u1.AddressOfData)
        {
            pImageImportByNameVa = (PIMAGE_IMPORT_BY_NAME)(base + imageOftThunkData.u1.AddressOfData);

            // Read every IMPORT_BY_NAME.name
            if(!MemRead((duint)pImageImportByNameVa + sizeof(WORD), importName(), MAX_IMPORT_SIZE))
                return false;

            // Callback
            cbEnum(base, imageFtThunkData.u1.Function, importName(), importModuleName);

            // Move to next address in the INT
            imageINTVa++;
            if(!MemRead((duint)imageINTVa, &imageOftThunkData, sizeof(imageOftThunkData)))
                return false;


            // Move to next address in the IAT and read it into imageFtThunkData
            imageIATVa++;
            if(!MemRead((duint)imageIATVa, &imageFtThunkData, sizeof(imageFtThunkData)))
                return false;
        }

        importTableVa++;
        readSuccess = MemRead((duint)importTableVa, &importDescriptor, sizeof(importDescriptor));
    }

    return true;
}