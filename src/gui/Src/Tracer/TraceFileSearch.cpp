#include "TraceFileReader.h"
#include "TraceFileSearch.h"
#include "zydis_wrapper.h"
#include "StringUtil.h"
#include <QCoreApplication>

static bool inRange(duint value, duint start, duint end)
{
    return value >= start && value <= end;
}

static QString getIndexText(TraceFileReader* file, duint index)
{
    QString indexString;
    indexString = QString::number(index, 16).toUpper();
    if(file->Length() < 16)
        return indexString;
    int digits;
    digits = floor(log2(file->Length() - 1) / 4) + 1;
    digits -= indexString.size();
    while(digits > 0)
    {
        indexString = '0' + indexString;
        digits = digits - 1;
    }
    return indexString;
}

int TraceFileSearchConstantRange(TraceFileReader* file, duint start, duint end)
{
    int count = 0;
    Zydis cp;
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

    for(unsigned long long index = 0; index < file->Length(); index++)
    {
        bool found = false;
        //Registers
#define FINDREG(fieldName) found |= inRange(file->Registers(index).regcontext.##fieldName, start, end)
        FINDREG(cax);
        FINDREG(cbx);
        FINDREG(ccx);
        FINDREG(cdx);
        FINDREG(csp);
        FINDREG(cbp);
        FINDREG(csi);
        FINDREG(cdi);
        FINDREG(cip);
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
            GuiReferenceSetCellContent(count, 1, getIndexText(file, index).toUtf8().constData());
            unsigned char opcode[16];
            int opcodeSize = 0;
            file->OpCode(index, opcode, &opcodeSize);
            cp.Disassemble(file->Registers(index).regcontext.cip, opcode, opcodeSize);
            GuiReferenceSetCellContent(count, 2, cp.InstructionText(true).c_str());
            //GuiReferenceSetCurrentTaskProgress; GuiReferenceSetProgress
            count++;
        }
    }
    return count;
}

int TraceFileSearchMemReference(TraceFileReader* file, duint address)
{
    int count = 0;
    Zydis cp;
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
                GuiReferenceSetCellContent(count, 1, getIndexText(file, index).toUtf8().constData());
                unsigned char opcode[16];
                int opcodeSize = 0;
                file->OpCode(index, opcode, &opcodeSize);
                cp.Disassemble(file->Registers(index).regcontext.cip, opcode, opcodeSize);
                GuiReferenceSetCellContent(count, 2, cp.InstructionText(true).c_str());
                //GuiReferenceSetCurrentTaskProgress; GuiReferenceSetProgress
                count++;
            }
        }
    }
    return count;
}
