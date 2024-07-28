/**
 @file _dbgfunctions.cpp

 @brief Implements the dbgfunctions class.
 */

#include "_global.h"
#include "_dbgfunctions.h"
#include "assemble.h"
#include "debugger.h"
#include "jit.h"
#include "patches.h"
#include "memory.h"
#include "disasm_fast.h"
#include "stackinfo.h"
#include "symbolinfo.h"
#include "module.h"
#include "exhandlerinfo.h"
#include "breakpoint.h"
#include "threading.h"
#include "stringformat.h"
#include "TraceRecord.h"
#include "mnemonichelp.h"
#include "handles.h"
#include "../bridge/bridgelist.h"
#include "tcpconnections.h"
#include "watch.h"
#include "animate.h"
#include "thread.h"
#include "comment.h"
#include "exception.h"
#include "database.h"
#include "dbghelp_safe.h"
#include "types.h"

static DBGFUNCTIONS _dbgfunctions;

const DBGFUNCTIONS* dbgfunctionsget()
{
    return &_dbgfunctions;
}

static int SymAutoComplete(const char* Search, char** Buffer, int MaxSymbols)
{
    //TODO: refactor this in a function because this pattern will become common
    std::vector<duint> mods;
    ModEnum([&mods](const MODINFO & info)
    {
        mods.push_back(info.base);
    });

    std::unordered_set<std::string> visited;

    static const bool caseSensitiveAutoComplete = settingboolget("Gui", "CaseSensitiveAutoComplete");

    int count = 0;
    std::string prefix(Search);
    for(duint base : mods)
    {
        if(count == MaxSymbols)
            break;

        SHARED_ACQUIRE(LockModules);
        auto modInfo = ModInfoFromAddr(base);
        if(!modInfo)
            continue;

        auto addName = [Buffer, MaxSymbols, &visited, &count](const std::string & name)
        {
            if(visited.count(name))
                return true;
            visited.insert(name);
            Buffer[count] = (char*)BridgeAlloc(name.size() + 1);
            memcpy(Buffer[count], name.c_str(), name.size() + 1);
            return ++count < MaxSymbols;
        };

        NameIndex::findByPrefix(modInfo->exportsByName, prefix, [modInfo, &addName](const NameIndex & index)
        {
            return addName(modInfo->exports[index.index].name);
        }, caseSensitiveAutoComplete);

        if(count == MaxSymbols)
            break;

        if(modInfo->symbols->isOpen())
        {
            modInfo->symbols->findSymbolsByPrefix(prefix, [&addName](const SymbolInfo & symInfo)
            {
                return addName(symInfo.decoratedName);
            }, caseSensitiveAutoComplete);
        }
    }

    std::stable_sort(Buffer, Buffer + count, [](const char* a, const char* b)
    {
        return (caseSensitiveAutoComplete ? strcmp : StringUtils::hackicmp)(a, b) < 0;
    });

    return count;
}

void dbgfunctionsinit()
{
    _dbgfunctions.AssembleAtEx = [](duint addr, const char* instruction, char* error, bool fillnop)
    {
        return assembleat(addr, instruction, nullptr, error, fillnop);
    };
    _dbgfunctions.SectionFromAddr = [](duint addr, char* section)
    {
        std::vector<MODSECTIONINFO> sections;
        if(ModSectionsFromAddr(addr, &sections))
        {
            for(const auto & cur : sections)
            {
                if(addr >= cur.addr && addr < cur.addr + (cur.size + (0x1000 - 1) & ~(0x1000 - 1)))
                {
                    strncpy_s(section, MAX_SECTION_SIZE * 5, cur.name, _TRUNCATE);
                    return true;
                }
            }
        }
        return false;
    };
    _dbgfunctions.ModNameFromAddr = ModNameFromAddr;
    _dbgfunctions.ModBaseFromAddr = ModBaseFromAddr;
    _dbgfunctions.ModBaseFromName = ModBaseFromName;
    _dbgfunctions.ModSizeFromAddr = ModSizeFromAddr;
    _dbgfunctions.ModGetParty = ModGetParty;
    _dbgfunctions.ModSetParty = ModSetParty;
    _dbgfunctions.WatchIsWatchdogTriggered = WatchIsWatchdogTriggered;
    _dbgfunctions.Assemble = assemble;
    _dbgfunctions.PatchGet = [](duint addr)
    {
        return PatchGet(addr, nullptr);
    };
    _dbgfunctions.PatchInRange = [](duint start, duint end)
    {
        if(start > end)
            std::swap(start, end);

        for(duint i = start; i <= end; i++)
        {
            if(PatchGet(i, nullptr))
                return true;
        }

        return false;
    };
    _dbgfunctions.MemPatch = [](duint va, const unsigned char* src, duint size)
    {
        return MemPatch(va, src, size, nullptr);
    };
    _dbgfunctions.PatchRestoreRange = [](duint start, duint end)
    {
        if(start > end)
            std::swap(start, end);

        for(duint i = start; i <= end; i++)
            PatchDelete(i, true);

        GuiUpdatePatches();
    };
    _dbgfunctions.PatchEnum = [](DBGPATCHINFO * patchlist, size_t* cbsize)
    {
        static_assert(sizeof(DBGPATCHINFO) == sizeof(PATCHINFO), "");
        return PatchEnum((PATCHINFO*)patchlist, cbsize);
    };
    _dbgfunctions.PatchRestore = [](duint addr)
    {
        return PatchDelete(addr, true);
    };
    _dbgfunctions.PatchFile = [](DBGPATCHINFO * patchlist, int count, const char* szFileName, char* error)
    {
        return PatchFile((PATCHINFO*)patchlist, count, szFileName, error);
    };
    _dbgfunctions.ModPathFromAddr = ModPathFromAddr;
    _dbgfunctions.ModPathFromName = ModPathFromName;
    _dbgfunctions.DisasmFast = disasmfast;
    _dbgfunctions.MemUpdateMap = []()
    {
        MemUpdateMap();
        GuiUpdateMemoryView();
    };
    _dbgfunctions.GetCallStack = [](DBGCALLSTACK * callstack)
    {
        if(hActiveThread)
            stackgetcallstack(GetContextDataEx(hActiveThread, UE_CSP), (CALLSTACK*)callstack);
    };
    _dbgfunctions.GetSEHChain = [](DBGSEHCHAIN * sehchain)
    {
        std::vector<duint> SEHList;
        ExHandlerGetSEH(SEHList);
        sehchain->total = SEHList.size();
        if(sehchain->total > 0)
        {
            sehchain->records = (DBGSEHRECORD*)BridgeAlloc(sehchain->total * sizeof(DBGSEHRECORD));
            for(size_t i = 0; i < sehchain->total; i++)
            {
                sehchain->records[i].addr = SEHList[i];
                MemRead(SEHList[i] + 4, &sehchain->records[i].handler, sizeof(duint));
            }
        }
        else
        {
            sehchain->records = nullptr;
        }
    };
    _dbgfunctions.SymbolDownloadAllSymbols = SymDownloadAllSymbols;
    _dbgfunctions.GetJit = [](char* jit, bool jit64)
    {
        arch dummy;
        char jit_tmp[JIT_ENTRY_MAX_SIZE] = "";
        if(jit != NULL)
        {
            if(!dbggetjit(jit_tmp, jit64 ? x64 : x32, &dummy, NULL))
                return false;
            strcpy_s(jit, MAX_SETTING_SIZE, jit_tmp);
        }
        else // if jit input == NULL: it returns false if there are not an OLD JIT STORED.
        {
            Memory<char*> oldjit(MAX_SETTING_SIZE + 1);
            if(!BridgeSettingGet("JIT", "Old", oldjit()))
                return false;
        }

        return true;
    };
    _dbgfunctions.GetJitAuto = [](bool * jit_auto)
    {
        return dbggetjitauto(jit_auto, notfound, NULL, NULL);
    };
    _dbgfunctions.GetDefJit = dbggetdefjit;
    _dbgfunctions.GetProcessList = [](DBGPROCESSINFO** entries, int* count)
    {
        std::vector<PROCESSENTRY32> infoList;
        std::vector<std::string> commandList;
        std::vector<std::string> winTextList;
        if(!dbglistprocesses(&infoList, &commandList, &winTextList))
            return false;
        *count = (int)infoList.size();
        if(!*count)
        {
            *entries = nullptr;
            return false;
        }
        *entries = (DBGPROCESSINFO*)BridgeAlloc(*count * sizeof(DBGPROCESSINFO));
        for(int i = 0; i < *count; i++)
        {
            (*entries)[*count - i - 1].dwProcessId = infoList.at(i).th32ProcessID;
            strncpy_s((*entries)[*count - i - 1].szExeFile, infoList.at(i).szExeFile, _TRUNCATE);
            strncpy_s((*entries)[*count - i - 1].szExeMainWindowTitle, winTextList.at(i).c_str(), _TRUNCATE);
            strncpy_s((*entries)[*count - i - 1].szExeArgs, commandList.at(i).c_str(), _TRUNCATE);
        }
        return true;
    };
    _dbgfunctions.GetPageRights = MemGetPageRights;
    _dbgfunctions.SetPageRights = MemSetPageRights;
    _dbgfunctions.PageRightsToString = MemPageRightsToString;
    _dbgfunctions.IsProcessElevated = BridgeIsProcessElevated;
    _dbgfunctions.GetCmdline = [](char* cmd_line, size_t* cbsize)
    {
        if(!cmd_line && !cbsize)
            return false;
        char* cmdline;
        if(!dbggetcmdline(&cmdline, NULL, fdProcessInfo->hProcess))
            return false;
        if(!cmd_line && cbsize)
            *cbsize = strlen(cmdline) + sizeof(char);
        else if(cmd_line)
            memcpy(cmd_line, cmdline, strlen(cmdline) + 1);
        efree(cmdline, "_getcmdline:cmdline");
        return true;
    };
    _dbgfunctions.SetCmdline = [](const char* cmd_line)
    {
        return dbgsetcmdline(cmd_line, nullptr);
    };
    _dbgfunctions.FileOffsetToVa = valfileoffsettova;
    _dbgfunctions.VaToFileOffset = valvatofileoffset;
    _dbgfunctions.GetAddrFromLine = [](const char* szSourceFile, int line, duint * disp)
    {
        if(disp)
            *disp = 0;
        return (duint)0;
    };
    _dbgfunctions.GetSourceFromAddr = [](duint addr, char* szSourceFile, int* line)
    {
        char sourceFile[MAX_STRING_SIZE] = "";
        if(!SymGetSourceLine(addr, sourceFile, line))
            return false;
        if(!FileExists(sourceFile))
            return false;
        if(szSourceFile)
            strcpy_s(szSourceFile, MAX_STRING_SIZE, sourceFile);
        return true;
    };
    _dbgfunctions.ValFromString = [](const char* string, duint * value)
    {
        return valfromstring(string, value);
    };
    _dbgfunctions.PatchGetEx = [](duint addr, DBGPATCHINFO * patch)
    {
        return PatchGet(addr, (PATCHINFO*)patch);
    };
    _dbgfunctions.GetBridgeBp = [](BPXTYPE type, duint addr, BRIDGEBP * bp)
    {
        BP_TYPE bptype;
        switch(type)
        {
        case bp_normal:
            bptype = BPNORMAL;
            break;
        case bp_hardware:
            bptype = BPHARDWARE;
            break;
        case bp_memory:
            bptype = BPMEMORY;
            break;
        case bp_dll:
            bptype = BPDLL;
            addr = ModHashFromName(reinterpret_cast<const char*>(addr));
            break;
        case bp_exception:
            bptype = BPEXCEPTION;
            break;
        default:
            return false;
        }
        SHARED_ACQUIRE(LockBreakpoints);
        auto bpInfo = BpInfoFromAddr(bptype, addr);
        if(!bpInfo)
            return false;
        if(bp)
        {
            BpToBridge(bpInfo, bp);
            bp->addr = addr;
        }
        return true;
    };
    _dbgfunctions.StringFormatInline = [](const char* format, size_t resultSize, char* result)
    {
        if(!format || !result)
            return false;
        strncpy_s(result, resultSize, stringformatinline(format).c_str(), _TRUNCATE);
        return true;
    };
    _dbgfunctions.GetMnemonicBrief = [](const char* mnem, size_t resultSize, char* result)
    {
        if(!result)
            return;
        strncpy_s(result, resultSize, MnemonicHelp::getBriefDescription(mnem).c_str(), _TRUNCATE);
    };
    _dbgfunctions.GetTraceRecordHitCount = [](duint address)
    {
        return TraceRecord.getHitCount(address);
    };
    _dbgfunctions.GetTraceRecordByteType = [](duint address)
    {
        return (TRACERECORDBYTETYPE)TraceRecord.getByteType(address);
    };
    _dbgfunctions.SetTraceRecordType = [](duint pageAddress, TRACERECORDTYPE type)
    {
        return TraceRecord.setTraceRecordType(pageAddress, (TraceRecordManager::TraceRecordType)type);
    };
    _dbgfunctions.GetTraceRecordType = [](duint pageAddress)
    {
        return (TRACERECORDTYPE)TraceRecord.getTraceRecordType(pageAddress);
    };
    _dbgfunctions.EnumHandles = [](ListOf(HANDLEINFO) handles)
    {
        std::vector<HANDLEINFO> handleV;
        if(!HandlesEnum(handleV))
            return false;
        return BridgeList<HANDLEINFO>::CopyData(handles, handleV);
    };
    _dbgfunctions.GetHandleName = [](duint handle, char* name, size_t nameSize, char* typeName, size_t typeNameSize)
    {
        String nameS;
        String typeNameS;
        if(!HandlesGetName(HANDLE(handle), nameS, typeNameS))
            return false;
        strncpy_s(name, nameSize, nameS.c_str(), _TRUNCATE);
        strncpy_s(typeName, typeNameSize, typeNameS.c_str(), _TRUNCATE);
        return true;
    };
    _dbgfunctions.EnumTcpConnections = [](ListOf(TCPCONNECTIONINFO) connections)
    {
        std::vector<TCPCONNECTIONINFO> connectionsV;
        if(!TcpEnumConnections(fdProcessInfo->dwProcessId, connectionsV))
            return false;
        return BridgeList<TCPCONNECTIONINFO>::CopyData(connections, connectionsV);
    };
    _dbgfunctions.GetDbgEvents = dbggetdbgevents;
    _dbgfunctions.MemIsCodePage = MemIsCodePage;
    _dbgfunctions.AnimateCommand = dbganimatecommand;
    _dbgfunctions.DbgSetDebuggeeInitScript = dbgsetdebuggeeinitscript;
    _dbgfunctions.DbgGetDebuggeeInitScript = dbggetdebuggeeinitscript;
    _dbgfunctions.EnumWindows = [](ListOf(WINDOW_INFO) windows)
    {
        std::vector<WINDOW_INFO> windowInfoV;
        if(!HandlesEnumWindows(windowInfoV))
            return false;
        return BridgeList<WINDOW_INFO>::CopyData(windows, windowInfoV);
    };
    _dbgfunctions.EnumHeaps = [](ListOf(HEAPINFO) heaps)
    {
        std::vector<HEAPINFO> heapInfoV;
        if(!HandlesEnumHeaps(heapInfoV))
            return false;
        return BridgeList<HEAPINFO>::CopyData(heaps, heapInfoV);
    };
    _dbgfunctions.ThreadGetName = ThreadGetName;
    _dbgfunctions.IsDepEnabled = dbgisdepenabled;
    _dbgfunctions.GetCallStackEx = [](DBGCALLSTACK * callstack, bool cache)
    {
        static_assert(sizeof(CALLSTACK) == sizeof(DBGCALLSTACK), "");
        auto csp = GetContextDataEx(hActiveThread, UE_CSP);
        if(!cache)
            stackupdatecallstack(csp);
        stackgetcallstack(csp, (CALLSTACK*)callstack);
    };
    _dbgfunctions.GetUserComment = CommentGet;
    _dbgfunctions.EnumConstants = [](ListOf(CONSTANTINFO) constants)
    {
        auto constantsV = ConstantList();
        BridgeList<CONSTANTINFO>::CopyData(constants, constantsV);
    };
    _dbgfunctions.EnumErrorCodes = [](ListOf(CONSTANTINFO) errorcodes)
    {
        auto errorcodesV = ErrorCodeList();
        BridgeList<CONSTANTINFO>::CopyData(errorcodes, errorcodesV);
    };
    _dbgfunctions.EnumExceptions = [](ListOf(CONSTANTINFO) exceptions)
    {
        auto exceptionsV = ExceptionList();
        BridgeList<CONSTANTINFO>::CopyData(exceptions, exceptionsV);
    };
    _dbgfunctions.MemBpSize = [](duint addr)
    {
        SHARED_ACQUIRE(LockBreakpoints);
        auto info = BpInfoFromAddr(BPMEMORY, addr);
        return info ? info->memsize : 0;
    };
    _dbgfunctions.ModRelocationsFromAddr = [](duint addr, ListOf(DBGRELOCATIONINFO) relocations)
    {
        std::vector<MODRELOCATIONINFO> infos;
        if(!ModRelocationsFromAddr(addr, infos))
            return false;

        BridgeList<MODRELOCATIONINFO>::CopyData(relocations, infos);
        return true;
    };
    _dbgfunctions.ModRelocationAtAddr = [](duint addr, DBGRELOCATIONINFO * relocation)
    {
        static_assert(sizeof(DBGRELOCATIONINFO) == sizeof(MODRELOCATIONINFO), "");
        return ModRelocationAtAddr(addr, (MODRELOCATIONINFO*)relocation);
    };
    _dbgfunctions.ModRelocationsInRange = [](duint addr, duint size, ListOf(DBGRELOCATIONINFO) relocations)
    {
        std::vector<MODRELOCATIONINFO> infos;
        if(!ModRelocationsInRange(addr, size, infos))
            return false;

        BridgeList<MODRELOCATIONINFO>::CopyData(relocations, infos);
        return true;
    };
    _dbgfunctions.DbGetHash = DbGetHash;
    _dbgfunctions.SymAutoComplete = SymAutoComplete;
    _dbgfunctions.RefreshModuleList = SymUpdateModuleList;
    _dbgfunctions.GetAddrFromLineEx = [](duint mod, const char* szSourceFile, int line)
    {
        duint addr = 0;
        if(SymGetSourceAddr(mod, szSourceFile, line, &addr))
            return addr;
        return (duint)0;
    };
    _dbgfunctions.ModSymbolStatus = [](duint base)
    {
        SHARED_ACQUIRE(LockModules);
        auto modInfo = ModInfoFromAddr(base);
        if(!modInfo)
            return MODSYMUNLOADED;
        bool isOpen = modInfo->symbols->isOpen();
        bool isLoading = modInfo->symbols->isLoading();
        if(isOpen && !isLoading)
            return MODSYMLOADED;
        else if(isOpen && isLoading)
            return MODSYMLOADING;
        else if(!isOpen && symbolDownloadingBase == base)
            return MODSYMLOADING;
        else
            return MODSYMUNLOADED;
    };
    _dbgfunctions.GetCallStackByThread = [](HANDLE thread, DBGCALLSTACK * callstack)
    {
        if(thread)
            stackgetcallstackbythread(thread, (CALLSTACK*)callstack);
    };
    _dbgfunctions.EnumStructs = [](CBSTRING callback, void* userdata)
    {
        std::vector<Types::TypeManager::Summary> types;
        EnumTypes(types);
        for(const auto & type : types)
        {
            if(type.kind == "struct" || type.kind == "union" || type.kind == "class")
                callback(type.name.c_str(), userdata);
        }
    };

    // New breakpoint API
    _dbgfunctions.BpRefList = [](duint * count)
    {
        auto refs = BpRefList();
        *count = refs.size();
        auto result = (BP_REF*)BridgeAlloc(refs.size() * sizeof(BP_REF));
        memcpy(result, refs.data(), refs.size() * sizeof(BP_REF));
        return result;
    };
    _dbgfunctions.BpRefVa = [](BP_REF * ref, BPXTYPE type, duint va)
    {
        return BpRefVa(*ref, type, va);
    };
    _dbgfunctions.BpRefRva = [](BP_REF * ref, BPXTYPE type, const char* module, duint rva)
    {
        return BpRefRva(*ref, type, module, rva);
    };
    _dbgfunctions.BpRefDll = [](BP_REF * ref, const char* module)
    {
        BpRefDll(*ref, module);
    };
    _dbgfunctions.BpRefException = [](BP_REF * ref, unsigned int code)
    {
        BpRefException(*ref, code);
    };
    _dbgfunctions.BpRefExists = [](const BP_REF * ref)
    {
        return BpRefExists(*ref);
    };
    _dbgfunctions.BpGetFieldNumber = [](const BP_REF * ref, BP_FIELD field, duint * value)
    {
        return BpGetFieldNumber(*ref, field, *value);
    };
    _dbgfunctions.BpSetFieldNumber = [](const BP_REF * ref, BP_FIELD field, duint value)
    {
        return BpSetFieldNumber(*ref, field, value);
    };
    _dbgfunctions.BpGetFieldText = [](const BP_REF * ref, BP_FIELD field, CBSTRING callback, void* userdata)
    {
        return BpGetFieldText(*ref, field, callback, userdata);
    };
    _dbgfunctions.BpSetFieldText = [](const BP_REF * ref, BP_FIELD field, const char* value)
    {
        return BpSetFieldText(*ref, field, value);
    };
}
