#ifndef _MODULE_H
#define _MODULE_H

#include "_global.h"
#include <functional>

#include "symbolsourcebase.h"

// Macros to safely access IMAGE_NT_HEADERS fields since the compile-time typedef of this struct may not match the actual file bitness.
// Never access OptionalHeader.xx values directly unless they have the same size and offset on 32 and 64 bit. IMAGE_FILE_HEADER fields are safe to use
#define IMAGE32(NtHeaders) ((NtHeaders) != nullptr && (NtHeaders)->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
#define IMAGE64(NtHeaders) ((NtHeaders) != nullptr && (NtHeaders)->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
#define HEADER_FIELD(NtHeaders, Field) (IMAGE64(NtHeaders) \
    ? ((PIMAGE_NT_HEADERS64)(NtHeaders))->OptionalHeader.Field : (IMAGE32(NtHeaders) \
        ? ((PIMAGE_NT_HEADERS32)(NtHeaders))->OptionalHeader.Field \
        : 0))
#define THUNK_VAL(NtHeaders, Ptr, Val) (IMAGE64(NtHeaders) \
    ? ((PIMAGE_THUNK_DATA64)(Ptr))->Val : (IMAGE32(NtHeaders) \
        ? ((PIMAGE_THUNK_DATA32)(Ptr))->Val \
        : 0))

struct MODSECTIONINFO
{
    duint addr; // Virtual address
    duint size; // Virtual size
    char name[MAX_SECTION_SIZE * 5]; // Escaped section name
};

struct MODRELOCATIONINFO
{
    DWORD rva; // Virtual address
    BYTE type; // Relocation type (IMAGE_REL_BASED_*)
    WORD size;

    bool Contains(duint Address) const
    {
        return Address >= rva && Address < rva + size;
    }
};

struct PdbValidationData
{
    GUID guid;
    DWORD signature = 0;
    DWORD age = 0;

    PdbValidationData()
    {
        memset(&guid, 0, sizeof(guid));
    }
};

struct MODEXPORT : SymbolInfoGui
{
    DWORD ordinal = 0;
    DWORD rva = 0;
    bool forwarded = false;
    String forwardName;
    String name;
    String undecoratedName;

    virtual void convertToGuiSymbol(duint base, SYMBOLINFO* info) const override;
};

struct MODIMPORT : SymbolInfoGui
{
    size_t moduleIndex = 0; //index in MODINFO.importModules
    DWORD iatRva = 0;
    duint ordinal = -1; //equal to -1 if imported by name
    String name;
    String undecoratedName;

    virtual void convertToGuiSymbol(duint base, SYMBOLINFO* info) const override;
};

struct MODINFO
{
    duint base = 0; // Module base
    duint size = 0; // Module size
    duint hash = 0; // Full module name hash
    duint entry = 0; // Entry point
    duint headerImageBase = 0; // ImageBase field in OptionalHeader

    char name[MAX_MODULE_SIZE]; // Module name (without extension)
    char extension[MAX_MODULE_SIZE]; // File extension (including the dot)
    char path[MAX_PATH]; // File path (in UTF8)

    PIMAGE_NT_HEADERS headers = nullptr; // Image headers. Always use HEADER_FIELD() to access OptionalHeader values

    std::vector<MODSECTIONINFO> sections;
    std::vector<MODRELOCATIONINFO> relocations;
    std::vector<duint> tlsCallbacks;
#if _WIN64
    std::vector<RUNTIME_FUNCTION> runtimeFunctions; //sorted by (begin, end)
#endif // _WIN64

    MODEXPORT entrySymbol;

    std::vector<MODEXPORT> exports;
    DWORD exportOrdinalBase = 0; //ordinal - 'exportOrdinalBase' = index in 'exports'
    std::vector<NameIndex> exportsByName; //index in 'exports', sorted by export name
    std::vector<size_t> exportsByRva; //index in 'exports', sorted by rva

    std::vector<String> importModules;
    std::vector<MODIMPORT> imports;
    std::vector<size_t> importsByRva; //index in 'imports', sorted by rva

    SymbolSourceBase* symbols = nullptr;
    String pdbSignature;
    String pdbFile;
    PdbValidationData pdbValidation;
    std::vector<String> pdbPaths; // Possible PDB paths (tried in order)

    HANDLE fileHandle = nullptr;
    DWORD loadedSize = 0;
    HANDLE fileMap = nullptr;
    ULONG_PTR fileMapVA = 0;

    int party;  // Party. Currently used value: 0: User, 1: System

    MODINFO()
    {
        memset(name, 0, sizeof(name));
        memset(extension, 0, sizeof(extension));
        memset(path, 0, sizeof(path));
    }

    ~MODINFO()
    {
        unmapFile();
        unloadSymbols();
        GuiInvalidateSymbolSource(base);
    }

    bool loadSymbols(const String & pdbPath, bool forceLoad);
    void unloadSymbols();
    void unmapFile();
    const MODEXPORT* findExport(duint rva) const;
};

bool ModLoad(duint Base, duint Size, const char* FullPath);
bool ModUnload(duint Base);
void ModClear();
MODINFO* ModInfoFromAddr(duint Address);
bool ModNameFromAddr(duint Address, char* Name, bool Extension);
duint ModBaseFromAddr(duint Address);
// Get a unique hash for an address in the module.
// IMPORTANT: If you want to get a hash for the module base, pass the base
duint ModHashFromAddr(duint Address);
duint ModHashFromName(const char* Module);
duint ModContentHashFromAddr(duint Address);
duint ModBaseFromName(const char* Module);
duint ModSizeFromAddr(duint Address);
std::string ModNameFromHash(duint Hash);
bool ModSectionsFromAddr(duint Address, std::vector<MODSECTIONINFO>* Sections);
duint ModEntryFromAddr(duint Address);
int ModPathFromAddr(duint Address, char* Path, int Size);
int ModPathFromName(const char* Module, char* Path, int Size);

/// <summary>
/// Enumerate all loaded modules with a function.
/// A shared lock on the modules is held until this function returns.
/// </summary>
/// <param name="cbEnum">Enumeration function.</param>
void ModEnum(const std::function<void(const MODINFO &)> & cbEnum);

int ModGetParty(duint Address);
void ModSetParty(duint Address, int Party);
bool ModRelocationsFromAddr(duint Address, std::vector<MODRELOCATIONINFO> & Relocations);
bool ModRelocationAtAddr(duint Address, MODRELOCATIONINFO* Relocation);
bool ModRelocationsInRange(duint Address, duint Size, std::vector<MODRELOCATIONINFO> & Relocations);

#endif // _MODULE_H
