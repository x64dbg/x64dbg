/**
@file symbolinfo.cpp

@brief Implements the symbolinfo class.
*/

#include "symbolinfo.h"
#include "debugger.h"
#include "console.h"
#include "module.h"
#include "addrinfo.h"
#include "dbghelp_safe.h"
#include "exception.h"
#include "WinInet-Downloader/downslib.h"
#include <shlwapi.h>

duint symbolDownloadingBase = 0;

struct SYMBOLCBDATA
{
    CBSYMBOLENUM cbSymbolEnum;
    void* user = nullptr;
};

bool SymEnum(duint Base, CBSYMBOLENUM EnumCallback, void* UserData, duint BeginRva, duint EndRva, unsigned int SymbolMask)
{
    SYMBOLCBDATA cbData;
    cbData.cbSymbolEnum = EnumCallback;
    cbData.user = UserData;

    SHARED_ACQUIRE(LockModules);
    MODINFO* modInfo = ModInfoFromAddr(Base);
    if(modInfo == nullptr)
        return false;

    if(SymbolMask & SYMBOL_MASK_SYMBOL && modInfo->symbols->isOpen())
    {
        modInfo->symbols->enumSymbols([&cbData, Base](const SymbolInfo & info)
        {
            SYMBOLPTR symbolptr;
            symbolptr.modbase = Base;
            symbolptr.symbol = &info;
            return cbData.cbSymbolEnum(&symbolptr, cbData.user);
        }, BeginRva, EndRva);
    }

    if(SymbolMask & SYMBOL_MASK_EXPORT)
    {
        auto entry = &modInfo->entrySymbol;

        // There is a pseudo-export for the entry point function
        auto emitEntryExport = [&]()
        {
            if(modInfo->entry != 0 && entry->rva >= BeginRva && entry->rva <= EndRva)
            {
                SYMBOLPTR symbolptr;
                symbolptr.modbase = Base;
                symbolptr.symbol = entry;
                entry = nullptr;
                return cbData.cbSymbolEnum(&symbolptr, cbData.user);
            }
            entry = nullptr;
            return true;
        };

        if(!modInfo->exportsByRva.empty())
        {
            auto it = modInfo->exportsByRva.begin();
            if(BeginRva > modInfo->exports[*it].rva)
            {
                it = std::lower_bound(modInfo->exportsByRva.begin(), modInfo->exportsByRva.end(), BeginRva, [&modInfo](size_t index, duint rva)
                {
                    return modInfo->exports[index].rva < rva;
                });
            }
            if(it != modInfo->exportsByRva.end())
            {
                for(; it != modInfo->exportsByRva.end(); it++)
                {
                    const auto & symbol = modInfo->exports[*it];
                    if(symbol.rva > EndRva)
                        break;

                    // This is only executed if there is another export after the entry
                    if(entry != nullptr && symbol.rva >= entry->rva)
                    {
                        if(!emitEntryExport())
                            return true;
                    }

                    SYMBOLPTR symbolptr;
                    symbolptr.modbase = Base;
                    symbolptr.symbol = &symbol;
                    if(!cbData.cbSymbolEnum(&symbolptr, cbData.user))
                        return true;
                }

                // This is executed if the entry is the last 'export'
                if(entry != nullptr)
                {
                    emitEntryExport();
                }
            }
            else
            {
                // This is executed if there are exports, but the range doesn't include any real ones
                emitEntryExport();
            }
        }
        else
        {
            // This is executed if there are no exports
            emitEntryExport();
        }
    }

    if(SymbolMask & SYMBOL_MASK_IMPORT && !modInfo->importsByRva.empty())
    {
        auto it = modInfo->importsByRva.begin();
        if(BeginRva > modInfo->imports[*it].iatRva)
        {
            it = std::lower_bound(modInfo->importsByRva.begin(), modInfo->importsByRva.end(), BeginRva, [&modInfo](size_t index, duint rva)
            {
                return modInfo->imports[index].iatRva < rva;
            });
        }
        if(it != modInfo->importsByRva.end())
        {
            for(; it != modInfo->importsByRva.end(); it++)
            {
                const auto & symbol = modInfo->imports[*it];
                if(symbol.iatRva > EndRva)
                    break;

                SYMBOLPTR symbolptr;
                symbolptr.modbase = Base;
                symbolptr.symbol = &symbol;
                if(!cbData.cbSymbolEnum(&symbolptr, cbData.user))
                    return true;
            }
        }
    }

    return true;
}

bool SymGetModuleList(std::vector<SYMBOLMODULEINFO>* List)
{
    ModEnum([List](const MODINFO & mod)
    {
        SYMBOLMODULEINFO curMod;
        curMod.base = mod.base;
        strcpy_s(curMod.name, mod.name);
        strcat_s(curMod.name, mod.extension);
        List->push_back(curMod);
    });
    return true;
}

void SymUpdateModuleList()
{
    // Build the vector of modules
    std::vector<SYMBOLMODULEINFO> modList;

    if(!SymGetModuleList(&modList))
    {
        GuiSymbolUpdateModuleList(0, nullptr);
        return;
    }

    // Create a new array to be sent to the GUI thread
    size_t moduleCount = modList.size();
    SYMBOLMODULEINFO* data = (SYMBOLMODULEINFO*)BridgeAlloc(moduleCount * sizeof(SYMBOLMODULEINFO));

    // Direct copy from std::vector data
    memcpy(data, modList.data(), moduleCount * sizeof(SYMBOLMODULEINFO));

    // Send the module data to the GUI for updating
    GuiSymbolUpdateModuleList((int)moduleCount, data);
}

static void SymSetProgress(int percentage, const char* pdbBaseFile)
{
    if(percentage == 0)
        GuiAddStatusBarMessage(StringUtils::sprintf("%s\n", pdbBaseFile).c_str());
    else
        GuiAddStatusBarMessage(StringUtils::sprintf("%s %d%%\n", pdbBaseFile, percentage).c_str());
    GuiSymbolSetProgress(percentage);
}

bool SymDownloadSymbol(duint Base, const char* SymbolStore)
{
    struct DownloadBaseGuard
    {
        DownloadBaseGuard(duint downloadBase) { symbolDownloadingBase = downloadBase; GuiRepaintTableView(); }
        ~DownloadBaseGuard() { symbolDownloadingBase = 0; GuiRepaintTableView(); }
    } g(Base);
#define symprintf(format, ...) GuiSymbolLogAdd(StringUtils::sprintf(GuiTranslateText(format), __VA_ARGS__).c_str())

    // Default to Microsoft's symbol server
    if(!SymbolStore)
        SymbolStore = "https://msdl.microsoft.com/download/symbols";

    String pdbSignature, pdbFile;
    {
        SHARED_ACQUIRE(LockModules);
        auto info = ModInfoFromAddr(Base);
        if(!info)
        {
            symprintf(QT_TRANSLATE_NOOP("DBG", "Module not found...\n"));
            return false;
        }
        pdbSignature = info->pdbSignature;
        pdbFile = info->pdbFile;
    }
    if(pdbSignature.empty() || pdbFile.empty()) // TODO: allow using module filename instead of pdbFile ?
    {
        symprintf(QT_TRANSLATE_NOOP("DBG", "Module has no symbol information...\n"));
        return false;
    }
    auto found = strrchr(pdbFile.c_str(), '\\');
    auto pdbBaseFile = found ? found + 1 : pdbFile.c_str();

    // TODO: strict checks if this path is absolute
    WString destinationPath(StringUtils::Utf8ToUtf16(szSymbolCachePath));
    if(destinationPath.empty())
    {
        symprintf(QT_TRANSLATE_NOOP("DBG", "No destination symbol path specified...\n"));
        return false;
    }
    CreateDirectoryW(destinationPath.c_str(), nullptr);
    if(destinationPath.back() != L'\\')
        destinationPath += L'\\';
    destinationPath += StringUtils::Utf8ToUtf16(pdbBaseFile);
    CreateDirectoryW(destinationPath.c_str(), nullptr);
    destinationPath += L'\\';
    destinationPath += StringUtils::Utf8ToUtf16(pdbSignature);
    CreateDirectoryW(destinationPath.c_str(), nullptr);
    destinationPath += '\\';
    destinationPath += StringUtils::Utf8ToUtf16(pdbBaseFile);

    String symbolUrl(SymbolStore);
    if(symbolUrl.empty())
    {
        symprintf(QT_TRANSLATE_NOOP("DBG", "No symbol store URL specified...\n"));
        return false;
    }
    if(symbolUrl.back() != '/')
        symbolUrl += '/';
    symbolUrl += StringUtils::sprintf("%s/%s/%s", pdbBaseFile, pdbSignature.c_str(), pdbBaseFile);

    symprintf(QT_TRANSLATE_NOOP("DBG", "Downloading symbol %s\n  Signature: %s\n  Destination: %s\n  URL: %s\n"), pdbBaseFile, pdbSignature.c_str(), StringUtils::Utf16ToUtf8(destinationPath).c_str(), symbolUrl.c_str());

    auto result = downslib_download(symbolUrl.c_str(), destinationPath.c_str(), "x64dbg", 1000, [](void* userdata, unsigned long long read_bytes, unsigned long long total_bytes)
    {
        if(total_bytes)
        {
            auto progress = (double)read_bytes / (double)total_bytes;
            SymSetProgress((int)(progress * 100.0), (const char*)userdata);
        }
        return true;
    }, (void*)pdbBaseFile);
    SymSetProgress(0, pdbBaseFile);

    switch(result)
    {
    case downslib_error::ok:
        break;
    case downslib_error::createfile:
        //TODO: handle ERROR_SHARING_VIOLATION (unload symbols and try again)
        symprintf(QT_TRANSLATE_NOOP("DBG", "Failed to create destination file (%s)...\n"), ErrorCodeToName(GetLastError()).c_str());
        return false;
    case downslib_error::inetopen:
        symprintf(QT_TRANSLATE_NOOP("DBG", "InternetOpen failed (%s)...\n"), ErrorCodeToName(GetLastError()).c_str());
        return false;
    case downslib_error::openurl:
        symprintf(QT_TRANSLATE_NOOP("DBG", "InternetOpenUrl failed (%s)...\n"), ErrorCodeToName(GetLastError()).c_str());
        return false;
    case downslib_error::statuscode:
        symprintf(QT_TRANSLATE_NOOP("DBG", "Connection succeeded, but download failed (status code: %d)...\n"), GetLastError());
        return false;
    case downslib_error::cancel:
        symprintf(QT_TRANSLATE_NOOP("DBG", "Download interrupted...\n"), ErrorCodeToName(GetLastError()).c_str());
        return false;
    case downslib_error::incomplete:
        symprintf(QT_TRANSLATE_NOOP("DBG", "Download incomplete...\n"), ErrorCodeToName(GetLastError()).c_str());
        return false;
    default:
        __debugbreak();
    }

    {
        EXCLUSIVE_ACQUIRE(LockModules);
        auto info = ModInfoFromAddr(Base);
        if(!info)
        {
            // TODO: this really isn't supposed to happen, but could if the module is suddenly unloaded
            dputs("module not found...");
            return false;
        }

        // trigger a symbol load
        info->loadSymbols(StringUtils::Utf16ToUtf8(destinationPath), bForceLoadSymbols);
    }

    return true;

#undef symprintf
}

void SymDownloadAllSymbols(const char* SymbolStore)
{
    // Default to Microsoft's symbol server
    if(!SymbolStore)
        SymbolStore = "https://msdl.microsoft.com/download/symbols";

    //TODO: refactor this in a function because this pattern will become common
    std::vector<duint> mods;
    ModEnum([&mods](const MODINFO & info)
    {
        mods.push_back(info.base);
    });

    for(duint base : mods)
        SymDownloadSymbol(base, SymbolStore);
}

bool SymAddrFromName(const char* Name, duint* Address)
{
    if(!Name || Name[0] == '\0')
        return false;

    if(!Address)
        return false;

    // Skip 'OrdinalXXX'
    if(_strnicmp(Name, "Ordinal#", 8) == 0 && strlen(Name) > 8)
    {
        const char* Name1 = Name + 8;
        bool notNonNumbersFound = true;
        do
        {
            if(!(Name1[0] >= '0' && Name1[0] <= '9'))
            {
                notNonNumbersFound = false;
                break;
            }
            Name1++;
        }
        while(Name1[0] != 0);
        if(notNonNumbersFound)
            return false;
    }

    //TODO: refactor this in a function because this pattern will become common
    std::vector<duint> mods;
    ModEnum([&mods](const MODINFO & info)
    {
        mods.push_back(info.base);
    });
    std::string name(Name);
    for(duint base : mods)
    {
        SHARED_ACQUIRE(LockModules);
        auto modInfo = ModInfoFromAddr(base);
        if(modInfo && modInfo->symbols->isOpen())
        {
            SymbolInfo symInfo;
            if(modInfo->symbols->findSymbolByName(name, symInfo, true))
            {
                *Address = base + symInfo.rva;
                return true;
            }
        }
    }
    return false;
}

String SymGetSymbolicName(duint Address, bool IncludeAddress)
{
    //
    // This resolves an address to a module and symbol:
    // [modname.]symbolname
    //
    char label[MAX_SYM_NAME];
    char modname[MAX_MODULE_SIZE];
    auto hasModule = ModNameFromAddr(Address, modname, false);

    // User labels have priority, but if one wasn't found,
    // default to a symbol lookup
    if(!DbgGetLabelAt(Address, SEG_DEFAULT, label))
    {
        if(hasModule)
            return StringUtils::sprintf("%s.%p", modname, Address);
        else if(IncludeAddress)
            return StringUtils::sprintf("%p", Address);
        else
            return "";
    }

    if(hasModule)
    {
        if(IncludeAddress)
            return StringUtils::sprintf("<%s.%s> (%p)", modname, label, Address);
        else
            return StringUtils::sprintf("<%s.%s>", modname, label);
    }
    else
    {
        if(IncludeAddress)
            return StringUtils::sprintf("<%s> (%p)", label, Address);
        else
            return StringUtils::sprintf("<%s>", label);
    }
}

bool SymbolFromAddressExact(duint address, SYMBOLINFO* info)
{
    if(address == 0)
        return false;

    SHARED_ACQUIRE(LockModules);
    MODINFO* modInfo = ModInfoFromAddr(address);
    if(modInfo == nullptr)
        return false;

    duint base = modInfo->base;
    duint rva = address - base;

    // search in symbols
    if(modInfo->symbols->isOpen())
    {
        SymbolInfo symInfo;
        if(modInfo->symbols->findSymbolExact(rva, symInfo))
        {
            symInfo.copyToGuiSymbol(base, info);
            return true;
        }
    }

    // search in module exports
    {
        auto modExport = modInfo->findExport(rva);
        if(modExport != nullptr)
        {
            modExport->copyToGuiSymbol(base, info);
            return true;
        }
    }

    if(modInfo->entry != 0 && modInfo->entrySymbol.rva == rva)
    {
        modInfo->entrySymbol.convertToGuiSymbol(base, info);
        return true;
    }

    // search in module imports
    {
        auto modImport = modInfo->findImport(rva);
        if(modImport != nullptr)
        {
            modImport->copyToGuiSymbol(base, info);
            return true;
        }
    }

    return false;
}

bool SymbolFromAddressExactOrLower(duint address, SYMBOLINFO* info)
{
    if(address == 0)
        return false;

    SHARED_ACQUIRE(LockModules);
    MODINFO* modInfo = ModInfoFromAddr(address);
    if(modInfo == nullptr)
        return false;

    duint rva = address - modInfo->base;

    // search in module symbols
    if(modInfo->symbols->isOpen())
    {
        SymbolInfo symInfo;
        if(modInfo->symbols->findSymbolExactOrLower(rva, symInfo))
        {
            symInfo.copyToGuiSymbol(modInfo->base, info);
            return true;
        }
    }

    // search in module exports
    if(!modInfo->exports.empty())
    {
        auto it = [&]()
        {
            auto it = std::lower_bound(modInfo->exportsByRva.begin(), modInfo->exportsByRva.end(), rva, [&modInfo](size_t index, duint rva)
            {
                return modInfo->exports.at(index).rva < rva;
            });
            // not found
            if(it == modInfo->exportsByRva.end())
                return --it;
            // exact match
            if(modInfo->exports[*it].rva == rva)
                return it;
            // right now 'it' points to the first element bigger than rva
            return it == modInfo->exportsByRva.begin() ? modInfo->exportsByRva.end() : --it;
        }();

        if(it != modInfo->exportsByRva.end())
        {
            const auto & symbol = modInfo->exports[*it];
            symbol.copyToGuiSymbol(modInfo->base, info);
            return true;
        }
    }

    return false;
}

bool SymGetSourceLine(duint Cip, char* FileName, int* Line, duint* disp)
{
    SHARED_ACQUIRE(LockModules);
    MODINFO* modInfo = ModInfoFromAddr(Cip);
    if(!modInfo)
        return false;

    SymbolSourceBase* sym = modInfo->symbols;
    if(!sym || sym == &EmptySymbolSource)
        return false;

    LineInfo lineInfo;
    if(!sym->findSourceLineInfo(Cip - modInfo->base, lineInfo))
        return false;

    if(disp)
        *disp = lineInfo.disp;

    if(Line)
        *Line = lineInfo.lineNumber;

    if(FileName)
    {
        strncpy_s(FileName, MAX_STRING_SIZE, lineInfo.sourceFile.c_str(), _TRUNCATE);

        // Check if it was a full path
        if(!PathIsRelativeW(StringUtils::Utf8ToUtf16(lineInfo.sourceFile).c_str()))
            return true;

        // Construct full path from pdb path
        {
            SHARED_ACQUIRE(LockModules);
            MODINFO* info = ModInfoFromAddr(Cip);
            if(!info)
                return true;

            String sourceFilePath = info->symbols->loadedSymbolPath();

            // Strip the name, leaving only the file directory
            size_t bslash = sourceFilePath.rfind('\\');
            if(bslash != String::npos)
                sourceFilePath.resize(bslash + 1);
            sourceFilePath += lineInfo.sourceFile;

            // Attempt to remap the source file if it exists (more heuristics could be added in the future)
            if(FileExists(sourceFilePath.c_str()))
            {
                if(info->symbols->mapSourceFilePdbToDisk(lineInfo.sourceFile, sourceFilePath))
                {
                    strncpy_s(FileName, MAX_STRING_SIZE, sourceFilePath.c_str(), _TRUNCATE);
                }
            }
        }
    }

    return true;
}

bool SymGetSourceAddr(duint Module, const char* FileName, int Line, duint* Address)
{
    SHARED_ACQUIRE(LockModules);
    auto modInfo = ModInfoFromAddr(Module);
    if(!modInfo)
        return false;

    auto sym = modInfo->symbols;
    if(!sym || sym == &EmptySymbolSource)
        return false;

    LineInfo lineInfo;
    if(!sym->findSourceLineInfo(FileName, Line, lineInfo))
        return false;

    *Address = lineInfo.rva + modInfo->base;
    return true;
}
