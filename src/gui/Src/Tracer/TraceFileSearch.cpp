#include "TraceFileReader.h"
#include "TraceFileSearch.h"
#include "zydis_wrapper.h"
#include "StringUtil.h"
#include <QCoreApplication>

static bool inRange(duint value, duint start, duint end)
{
    return value >= start && value <= end;
}

int TraceFileSearchConstantRange(TraceFileReader* file, duint start, duint end)
{
    int count = 0;
    Zydis zy;
    QString title;
    if(start == end)
        title = QCoreApplication::translate("TraceFileSearch", "Constant: %1").arg(ToPtrString(start));
    else
        title = QCoreApplication::translate("TraceFileSearch", "Range: %1-%2").arg(ToPtrString(start)).arg(ToPtrString(end));
    GuiReferenceInitialize(title.toUtf8().constData());
    GuiReferenceAddColumn(sizeof(duint) * 2, QCoreApplication::translate("TraceFileSearch", "Address").toUtf8().constData());
    GuiReferenceAddColumn(sizeof(duint) * 2, QCoreApplication::translate("TraceFileSearch", "Index").toUtf8().constData());
    GuiReferenceAddColumn(100, QCoreApplication::translate("TraceFileSearch", "Disassembly").toUtf8().constData());
    GuiReferenceSetRowCount(0);

    REGISTERCONTEXT regcontext;
    for(unsigned long long index = 0; index < file->Length(); index++)
    {
        regcontext = file->Registers(index).regcontext;
        bool found = false;
        //Registers
#define FINDREG(fieldName) found |= inRange(regcontext.##fieldName, start, end)
        FINDREG(cax);
        FINDREG(ccx);
        FINDREG(cdx);
        FINDREG(cbx);
        FINDREG(csp);
        FINDREG(cbp);
        FINDREG(csi);
        FINDREG(cdi);
        FINDREG(cip);
#ifdef _WIN64
        FINDREG(r8);
        FINDREG(r9);
        FINDREG(r10);
        FINDREG(r11);
        FINDREG(r12);
        FINDREG(r13);
        FINDREG(r14);
        FINDREG(r15);
#endif //_WIN64
#undef FINDREG
        //Memory
        duint memAddr[MAX_MEMORY_OPERANDS];
        duint memOldContent[MAX_MEMORY_OPERANDS];
        duint memNewContent[MAX_MEMORY_OPERANDS];
        bool isValid[MAX_MEMORY_OPERANDS];
        int memAccessCount = file->MemoryAccessCount(index);
        if(memAccessCount > 0)
        {
            file->MemoryAccessInfo(index, memAddr, memOldContent, memNewContent, isValid);
            for(size_t i = 0; i < memAccessCount; i++)
            {
                found |= inRange(memAddr[i], start, end);
                found |= inRange(memOldContent[i], start, end);
                found |= inRange(memNewContent[i], start, end);
            }
        }
        //Constants: TO DO
        //Populate reference view
        if(found)
        {
            GuiReferenceSetRowCount(count + 1);
            GuiReferenceSetCellContent(count, 0, ToPtrString(file->Registers(index).regcontext.cip).toUtf8().constData());
            GuiReferenceSetCellContent(count, 1, file->getIndexText(index).toUtf8().constData());
            unsigned char opcode[16];
            int opcodeSize = 0;
            file->OpCode(index, opcode, &opcodeSize);
            zy.Disassemble(file->Registers(index).regcontext.cip, opcode, opcodeSize);
            GuiReferenceSetCellContent(count, 2, zy.InstructionText(true).c_str());
            //GuiReferenceSetCurrentTaskProgress; GuiReferenceSetProgress
            count++;
        }
    }
    return count;
}

int TraceFileSearchMemReference(TraceFileReader* file, duint address)
{
    int count = 0;
    Zydis zy;
    GuiReferenceInitialize(QCoreApplication::translate("TraceFileSearch", "Reference").toUtf8().constData());
    GuiReferenceAddColumn(sizeof(duint) * 2, QCoreApplication::translate("TraceFileSearch", "Address").toUtf8().constData());
    GuiReferenceAddColumn(sizeof(duint) * 2, QCoreApplication::translate("TraceFileSearch", "Index").toUtf8().constData());
    GuiReferenceAddColumn(100, QCoreApplication::translate("TraceFileSearch", "Disassembly").toUtf8().constData());
    GuiReferenceSetRowCount(0);

    for(unsigned long long index = 0; index < file->Length(); index++)
    {
        bool found = false;
        //Memory
        duint memAddr[MAX_MEMORY_OPERANDS];
        duint memOldContent[MAX_MEMORY_OPERANDS];
        duint memNewContent[MAX_MEMORY_OPERANDS];
        bool isValid[MAX_MEMORY_OPERANDS];
        int memAccessCount = file->MemoryAccessCount(index);
        if(memAccessCount > 0)
        {
            file->MemoryAccessInfo(index, memAddr, memOldContent, memNewContent, isValid);
            for(size_t i = 0; i < memAccessCount; i++)
            {
                found |= inRange(memAddr[i], address, address + sizeof(duint) - 1);
            }
            //Constants: TO DO
            //Populate reference view
            if(found)
            {
                GuiReferenceSetRowCount(count + 1);
                GuiReferenceSetCellContent(count, 0, ToPtrString(file->Registers(index).regcontext.cip).toUtf8().constData());
                GuiReferenceSetCellContent(count, 1, file->getIndexText(index).toUtf8().constData());
                unsigned char opcode[16];
                int opcodeSize = 0;
                file->OpCode(index, opcode, &opcodeSize);
                zy.Disassemble(file->Registers(index).regcontext.cip, opcode, opcodeSize);
                GuiReferenceSetCellContent(count, 2, zy.InstructionText(true).c_str());
                //GuiReferenceSetCurrentTaskProgress; GuiReferenceSetProgress
                count++;
            }
        }
    }
    return count;
}

unsigned long long TraceFileSearchFuncReturn(TraceFileReader* file, unsigned long long start)
{
    auto mCsp = file->Registers(start).regcontext.csp;
    auto TID = file->ThreadId(start);
    Zydis zy;
    for(unsigned long long index = start; index < file->Length(); index++)
    {
        if(mCsp <= file->Registers(index).regcontext.csp && file->ThreadId(index) == TID) //"Run until return" should break only if RSP is bigger than or equal to current value
        {
            unsigned char data[16];
            int opcodeSize = 0;
            file->OpCode(index, data, &opcodeSize);
            if(data[0] == 0xC3 || data[0] == 0xC2) //retn instruction
                return index;
            else if(data[0] == 0x26 || data[0] == 0x36 || data[0] == 0x2e || data[0] == 0x3e || (data[0] >= 0x64 && data[0] <= 0x67) || data[0] == 0xf2 || data[0] == 0xf3 //instruction prefixes
#ifdef _WIN64
                    || (data[0] >= 0x40 && data[0] <= 0x4f)
#endif //_WIN64
                   )
            {
                zy.Disassemble(file->Registers(index).regcontext.cip, data, opcodeSize);
                if(zy.IsRet())
                    return index;
            }
        }
    }
    return start; //Nothing found, so just stay here
}
