/**
 @file reference.cpp

 @brief Implements the reference class.
 */

#include "reference.h"
#include "memory.h"
#include "console.h"
#include "module.h"
#include "threading.h"

/**
@brief RefFind Find reference to the buffer by a given criterion.
@param Address The base address of the buffer
@param Size The size of the buffer
@param Callback The callback that is invoked to identify whether an instruction satisfies the criterion. prototype: bool callback(Capstone* disasm, BASIC_INSTRUCTION_INFO* basicinfo, REFINFO* refinfo)
@param UserData The data that will be passed to Callback
@param Silent If true, no log will be outputed.
@param Name The name of the reference criterion. Not null.
@param type The type of the memory buffer. Possible values:CURRENT_REGION,CURRENT_MODULE,ALL_MODULES
@param disasmText If false, disassembled text will not be available.
*/
int RefFind(duint Address, duint Size, CBREF Callback, void* UserData, bool Silent, const char* Name, REFFINDTYPE type, bool disasmText)
{
    char fullName[deflen];
    char moduleName[MAX_MODULE_SIZE];
    duint scanStart, scanSize;
    REFINFO refInfo;

    if(type == CURRENT_REGION) // Search in current Region
    {
        duint regionSize = 0;
        duint regionBase = MemFindBaseAddr(Address, &regionSize, true);

        // If the memory page wasn't found, fail
        if(!regionBase || !regionSize)
        {
            if(!Silent)
                dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid memory page 0x%p\n"), Address);

            return 0;
        }

        // Assume the entire range is used
        scanStart = regionBase;
        scanSize  = regionSize;

        // Otherwise use custom boundaries if size was supplied
        if(Size)
        {
            duint maxsize = Size - (Address - regionBase);

            // Make sure the size fits in one page
            scanStart = Address;
            scanSize  = min(Size, maxsize);
        }

        // Determine the full module name
        if(ModNameFromAddr(scanStart, moduleName, true))
            sprintf_s(fullName, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "%s (Region %s)")), Name, moduleName);
        else
            sprintf_s(fullName, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "%s (Region %p)")), Name, scanStart);

        // Initialize disassembler
        Zydis cp;

        // Allow an "initialization" notice
        refInfo.refcount = 0;
        refInfo.userinfo = UserData;
        refInfo.name = fullName;

        RefFindInRange(scanStart, scanSize, Callback, UserData, Silent, refInfo, cp, true, [](int percent)
        {
            GuiReferenceSetCurrentTaskProgress(percent, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Region Search")));
            GuiReferenceSetProgress(percent);
        }, disasmText);
    }
    else if(type == CURRENT_MODULE) // Search in current Module
    {
        SHARED_ACQUIRE(LockModules);
        auto modInfo = ModInfoFromAddr(Address);

        if(!modInfo)
        {
            if(!Silent)
                dprintf(QT_TRANSLATE_NOOP("DBG", "Couldn't locate module for 0x%p\n"), Address);

            return 0;
        }

        duint modBase = modInfo->base;
        duint modSize = modInfo->size;

        SHARED_RELEASE();

        scanStart = modBase;
        scanSize  = modSize;

        // Determine the full module name
        if(ModNameFromAddr(scanStart, moduleName, true))
            sprintf_s(fullName, "%s (%s)", Name, moduleName);
        else
            sprintf_s(fullName, "%s (%p)", Name, scanStart);

        // Initialize disassembler
        Zydis cp;

        // Allow an "initialization" notice
        refInfo.refcount = 0;
        refInfo.userinfo = UserData;
        refInfo.name = fullName;

        RefFindInRange(scanStart, scanSize, Callback, UserData, Silent, refInfo, cp, true, [](int percent)
        {
            GuiReferenceSetCurrentTaskProgress(percent, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Module Search")));
            GuiReferenceSetProgress(percent);
        }, disasmText);
    }
    else if(type == ALL_MODULES) // Search in all Modules
    {
        bool initCallBack = true;

        struct RefModInfo
        {
            duint base;
            duint size;
            char name[MAX_MODULE_SIZE];
        };
        std::vector<RefModInfo> modList;
        ModEnum([&modList](const MODINFO & mod)
        {
            RefModInfo info;
            info.base = mod.base;
            info.size = mod.size;
            strncpy_s(info.name, mod.name, _TRUNCATE);
            strncat_s(info.name, mod.extension, _TRUNCATE);
            modList.push_back(info);
        });

        if(!modList.size())
        {
            if(!Silent)
                dprintf(QT_TRANSLATE_NOOP("DBG", "Couldn't get module list"));

            return 0;
        }

        // Initialize disassembler
        Zydis cp;

        // Determine the full module
        sprintf_s(fullName, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "All Modules (%s)")), Name);

        // Allow an "initialization" notice
        refInfo.refcount = 0;
        refInfo.userinfo = UserData;
        refInfo.name = fullName;

        for(duint i = 0; i < modList.size(); i++)
        {
            scanStart = modList[i].base;
            scanSize = modList[i].size;

            if(i != 0)
                initCallBack = false;

            RefFindInRange(scanStart, scanSize, Callback, UserData, Silent, refInfo, cp, initCallBack, [&i, &modList](int percent)
            {
                float fPercent = (float)percent / 100.f;
                float fTotalPercent = ((float)i + fPercent) / (float)modList.size();

                int totalPercent = (int)floor(fTotalPercent * 100.f);

                GuiReferenceSetCurrentTaskProgress(percent, modList[i].name);
                GuiReferenceSetProgress(totalPercent);
            }, disasmText);
        }
    }
    else
        return 0;

    GuiReferenceSetProgress(100);
    GuiReferenceReloadData();
    return refInfo.refcount;
}

int RefFindInRange(duint scanStart, duint scanSize, CBREF Callback, void* UserData, bool Silent, REFINFO & refInfo, Zydis & cp, bool initCallBack, const CBPROGRESS & cbUpdateProgress, bool disasmText)
{
    // Allocate and read a buffer from the remote process
    Memory<unsigned char*> data(scanSize, "reffind:data");

    memset(data(), 0xCC, data.size());
    MemReadDumb(scanStart, data(), scanSize);

    if(initCallBack)
        Callback(0, 0, &refInfo);

    //concurrency::parallel_for(duint (0), scanSize, [&](duint i)
    for(duint i = 0; i < scanSize;)
    {
        // Print the progress every 4096 bytes
        if((i % 0x1000) == 0)
        {
            // Percent = (current / total) * 100
            // Integer = floor(percent)
            int percent = (int)floor(((float)i / (float)scanSize) * 100.0f);

            cbUpdateProgress(percent);
        }

        // Disassemble the instruction
        int disasmMaxSize = min(MAX_DISASM_BUFFER, (int)(scanSize - i)); // Prevent going past the boundary
        int disasmLen = 1;

        if(cp.Disassemble(scanStart, data() + i, disasmMaxSize))
        {
            BASIC_INSTRUCTION_INFO basicinfo;
            fillbasicinfo(&cp, &basicinfo, disasmText);

            if(Callback(&cp, &basicinfo, &refInfo))
                refInfo.refcount++;

            disasmLen = cp.Size();
        }
        else
        {
            // Invalid instruction detected, so just skip the byte
        }

        scanStart += disasmLen;
        i += disasmLen;
    }

    cbUpdateProgress(100);
    return refInfo.refcount;
}
