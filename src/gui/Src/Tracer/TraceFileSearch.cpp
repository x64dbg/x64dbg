#include <assert.h>
#include <QCoreApplication>
#include "TraceFileReader.h"
#include "TraceFileSearch.h"
#include "zydis_wrapper.h"
#include "StringUtil.h"
#include "Configuration.h"

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
    GuiReferenceAddColumn(5, QCoreApplication::translate("TraceFileSearch", "Index").toUtf8().constData());
    GuiReferenceAddColumn(100, QCoreApplication::translate("TraceFileSearch", "Disassembly").toUtf8().constData());
    GuiReferenceAddCommand(QCoreApplication::translate("TraceFileSearch", "Follow index in trace").toUtf8().constData(), "gototrace 0x$1");
    GuiReferenceSetRowCount(0);

    REGISTERCONTEXT regcontext;
    for(unsigned long long index = 0; index < file->Length(); index++)
    {
        regcontext = file->Registers(index).regcontext;
        bool found = false;
        //Registers
#define FINDREG(fieldName) found |= inRange(regcontext.fieldName, start, end)
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
            for(int i = 0; i < memAccessCount; i++)
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
            GuiReferenceSetCellContent(count, 0, ToPtrString(file->Address(index)).toUtf8().constData());
            GuiReferenceSetCellContent(count, 1, file->getIndexText(index).toUtf8().constData());
            unsigned char opcode[16];
            int opcodeSize = 0;
            file->OpCode(index, opcode, &opcodeSize);
            zy.Disassemble(file->Address(index), opcode, opcodeSize);
            GuiReferenceSetCellContent(count, 2, zy.InstructionText(true).c_str());
            //GuiReferenceSetCurrentTaskProgress; GuiReferenceSetProgress
            count++;
        }
    }
    return count;
}

int TraceFileSearchMemReference(TraceFileReader* file, duint address)
{
    // Empty trace does not have references
    if(file->Length() == 0)
        return 0;

    // We now only support indexed search, so the dump index must be built first.
    if(!file->getDump()->isEnabled())
        return 0;

    // Create reference view
    GuiReferenceInitialize(QCoreApplication::translate("TraceFileSearch", "Reference").append(' ').append(ToPtrString(address)).toUtf8().constData());
    GuiReferenceAddColumn(sizeof(duint) * 2, QCoreApplication::translate("TraceFileSearch", "Address").toUtf8().constData());
    GuiReferenceAddColumn(5, QCoreApplication::translate("TraceFileSearch", "Index").toUtf8().constData());
    GuiReferenceAddColumn(100, QCoreApplication::translate("TraceFileSearch", "Disassembly").toUtf8().constData());
    GuiReferenceAddCommand(QCoreApplication::translate("TraceFileSearch", "Follow index in trace").toUtf8().constData(), "gototrace 0x$1");
    GuiReferenceSetRowCount(0);

    // Build the dump to the end
    file->buildDumpTo(file->Length() - 1);

    // Find references
    auto results = file->getReferences(address, address + sizeof(duint) - 1);

    // Collect results
    Zydis zy;
    int count = 0;
    for(size_t i = 0; i < results.size(); i++)
    {
        bool found = false;
        auto index = results[i];
        //Memory
        duint memAddr[MAX_MEMORY_OPERANDS];
        duint memOldContent[MAX_MEMORY_OPERANDS];
        duint memNewContent[MAX_MEMORY_OPERANDS];
        bool isValid[MAX_MEMORY_OPERANDS];
        int memAccessCount = file->MemoryAccessCount(index);
        if(memAccessCount > 0)
        {
            file->MemoryAccessInfo(index, memAddr, memOldContent, memNewContent, isValid);
            for(int i = 0; i < memAccessCount; i++)
            {
                found |= inRange(memAddr[i], address, address + sizeof(duint) - 1);
            }
            //Constants: TO DO
            //Populate reference view
            if(found)
            {
                GuiReferenceSetRowCount(count + 1);
                GuiReferenceSetCellContent(count, 0, ToPtrString(file->Address(index)).toUtf8().constData());
                GuiReferenceSetCellContent(count, 1, file->getIndexText(index).toUtf8().constData());
                unsigned char opcode[16];
                int opcodeSize = 0;
                file->OpCode(index, opcode, &opcodeSize);
                zy.Disassemble(file->Address(index), opcode, opcodeSize);
                GuiReferenceSetCellContent(count, 2, zy.InstructionText(true).c_str());
                //GuiReferenceSetCurrentTaskProgress; GuiReferenceSetProgress
                count++;
            }
        }
    }
    return count;
}

TRACEINDEX TraceFileSearchFuncReturn(TraceFileReader* file, TRACEINDEX start)
{
    auto mCsp = file->Registers(start).regcontext.csp;
    auto TID = file->ThreadId(start);
    Zydis zy;
    for(TRACEINDEX index = start; index < file->Length(); index++)
    {
        auto registers = file->Registers(index);
        if(mCsp <= registers.regcontext.csp && file->ThreadId(index) == TID) //"Run until return" should break only if RSP is bigger than or equal to current value
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
                if(zy.Disassemble(registers.regcontext.cip, data, opcodeSize) && zy.IsRet())
                    return index;
            }
        }
    }
    return start; //Nothing found, so just stay here
}

int TraceFileSearchMemPattern(TraceFileReader* file, const QString & pattern)
{
    if(!file->getDump()->isEnabled())
        return 0;
    std::vector<unsigned char> data, mask;
    bool high = true;
    for(auto ch : pattern)
    {
        if(ch > 0x80)
            return 0;
        char c = ch.toLatin1();
        if(isspace(c))
            continue;
        if(high)
        {
            if(c == '?')
            {
                data.push_back((char)0);
                mask.push_back((char)0);
            }
            else if(c >= '0' && c <= '9')
            {
                data.push_back((char)((c - '0') << 4));
                mask.push_back((char)0xf0);
            }
            else if(c >= 'A' && c <= 'F')
            {
                data.push_back((char)((c - 'A' + 10) << 4));
                mask.push_back((char)0xf0);
            }
            else if(c >= 'a' && c <= 'f')
            {
                data.push_back((char)((c - 'a' + 10) << 4));
                mask.push_back((char)0xf0);
            }
            else
            {
                return 0;
            }
        }
        else
        {
            unsigned char* dataEnd = &data[data.size() - 1];
            unsigned char* maskEnd = &mask[mask.size() - 1];
            if(c == '?')
            {
                // do nothing
            }
            else if(c >= '0' && c <= '9')
            {
                *dataEnd |= (char)(c - '0');
                *maskEnd |= (char)0xf;
            }
            else if(c >= 'A' && c <= 'F')
            {
                *dataEnd |= (char)(c - 'A' + 10);
                *maskEnd |= (char)0xf;
            }
            else if(c >= 'a' && c <= 'f')
            {
                *dataEnd |= (char)(c - 'a' + 10);
                *maskEnd |= (char)0xf;
            }
            else
            {
                return 0;
            }
        }
        high = !high;
    }
    // Create reference view
    QString patternshort;
    for(int i = 0; i < (data.size() > 16 ? 16 : data.size()); i++)
    {
        patternshort += ToByteString(data[i]);
        if((mask[i] & 0xf0) == 0)
            patternshort[patternshort.size() - 2] = '?';
        if((mask[i] & 0x0f) == 0)
            patternshort[patternshort.size() - 1] = '?';
    }
    if(data.size() > 16)
        patternshort += "...";
    QString patterntitle = QCoreApplication::translate("TraceFileSearch", "Pattern: %1").arg(patternshort);
    GuiReferenceInitialize(patterntitle.toUtf8().constData());
    GuiReferenceAddColumn(sizeof(duint) * 2, QCoreApplication::translate("TraceFileSearch", "Address").toUtf8().constData());
    GuiReferenceAddColumn(20, QCoreApplication::translate("TraceFileSearch", "Start Index").toUtf8().constData());
    GuiReferenceAddColumn(20, QCoreApplication::translate("TraceFileSearch", "End Index").toUtf8().constData());
    GuiReferenceAddColumn(sizeof(duint) * 2, ArchValue("EIP", "RIP"));
    GuiReferenceAddColumn(100, QCoreApplication::translate("TraceFileSearch", "Disassembly").toUtf8().constData());
    GuiReferenceSetRowCount(0);
    GuiReferenceAddCommand(QCoreApplication::translate("TraceFileSearch", "Follow start index in trace").toUtf8().constData(), "gototrace 0x$1");
    GuiReferenceAddCommand(QCoreApplication::translate("TraceFileSearch", "Follow end index in trace").toUtf8().constData(), "gototrace 0x$2");

    // Build the dump to the end
    file->buildDumpTo(file->Length() - 1);
    int count = 0;
    Zydis zy;
    file->getDump()->findAllMem(data.data(), mask.data(), data.size(), [&](duint address, TRACEINDEX startIndex, TRACEINDEX endIndex)
    {
        GuiReferenceSetRowCount(count + 1);
        GuiReferenceSetCellContent(count, 0, ToPtrString(address).toUtf8().constData());
        GuiReferenceSetCellContent(count, 1, file->getIndexText(startIndex).toUtf8().constData());
        GuiReferenceSetCellContent(count, 2, file->getIndexText(endIndex).toUtf8().constData());
        if(startIndex > 0)
        {
            duint cip;
            cip = file->Address(startIndex);
            GuiReferenceSetCellContent(count, 3, ToPtrString(cip).toUtf8().constData());
            unsigned char opcode[16];
            int opcodeSize = 0;
            file->OpCode(startIndex, opcode, &opcodeSize);
            zy.Disassemble(cip, opcode, opcodeSize);
            GuiReferenceSetCellContent(count, 4, zy.InstructionText(true).c_str());
        }
        else
        {
            // It has been like that since the start of trace
            GuiReferenceSetCellContent(count, 3, ArchValue("00000000", "0000000000000000"));
            GuiReferenceSetCellContent(count, 4, "");
        }
        count++;
        return count < 5000;
    });
    return count;
}
