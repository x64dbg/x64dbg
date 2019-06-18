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
    std::vector<char> decoratedSymbol;
    std::vector<char> undecoratedSymbol;
};

void SymEnum(duint Base, CBSYMBOLENUM EnumCallback, void* UserData)
{
    SYMBOLCBDATA cbData;
    cbData.cbSymbolEnum = EnumCallback;
    cbData.user = UserData;
    cbData.decoratedSymbol.resize(MAX_SYM_NAME + 1);
    cbData.undecoratedSymbol.resize(MAX_SYM_NAME + 1);

    {
        SHARED_ACQUIRE(LockModules);
        MODINFO* modInfo = ModInfoFromAddr(Base);
        if(modInfo)
        {
            for(size_t i = 0; i < modInfo->exports.size(); i++)
            {
                SYMBOLPTR symbolptr;
                symbolptr.modbase = Base;
                symbolptr.symbol = &modInfo->exports.at(i);
                cbData.cbSymbolEnum(&symbolptr, cbData.user);
            }

            // Emit pseudo entry point symbol
            {
                SYMBOLPTR symbolptr;
                symbolptr.modbase = Base;
                symbolptr.symbol = &modInfo->entrySymbol;
                cbData.cbSymbolEnum(&symbolptr, cbData.user);
            }

            for(size_t i = 0; i < modInfo->imports.size(); i++)
            {
                SYMBOLPTR symbolptr;
                symbolptr.modbase = Base;
                symbolptr.symbol = &modInfo->imports.at(i);
                cbData.cbSymbolEnum(&symbolptr, cbData.user);
            }
            if(modInfo->symbols->isOpen())
            {
                modInfo->symbols->enumSymbols([&cbData, Base](const SymbolInfo & info)
                {
                    SYMBOLPTR symbolptr;
                    symbolptr.modbase = Base;
                    symbolptr.symbol = &info;
                    return cbData.cbSymbolEnum(&symbolptr, cbData.user);
                });
            }
        }
    }
}

void SymEnumFromCache(duint Base, CBSYMBOLENUM EnumCallback, void* UserData)
{
    SymEnum(Base, EnumCallback, UserData);
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

    auto result = downslib_download(symbolUrl.c_str(), destinationPath.c_str(), "x64dbg", 1000, [](unsigned long long read_bytes, unsigned long long total_bytes)
    {
        if(total_bytes)
        {
            auto progress = (double)read_bytes / (double)total_bytes;
            GuiSymbolSetProgress((int)(progress * 100.0));
        }
        return true;
    });
    GuiSymbolSetProgress(0);

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

String SymGetSymbolicName(duint Address)
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
        return "";
    }

    if(hasModule)
        return StringUtils::sprintf("<%s.%s>", modname, label);
    return StringUtils::sprintf("<%s>", label);
}

bool SymGetSourceLine(duint Cip, char* FileName, int* Line, DWORD* disp)
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
