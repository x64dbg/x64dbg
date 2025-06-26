#include "cmd-types.h"
#include "console.h"
#include "encodemap.h"
#include "value.h"
#include "types.h"
#include "variable.h"
#include "filehelper.h"
#include "typevisitor.h"

using namespace Types;

static bool cbInstrDataGeneric(ENCODETYPE type, int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    duint addr;
    if(!valfromstring(argv[1], &addr, false))
        return false;
    duint size = 1;
    if(argc >= 3)
        if(!valfromstring(argv[2], &size, false))
            return false;
    bool created;
    if(!EncodeMapSetType(addr, size, type, &created))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "EncodeMapSetType failed..."));
        return false;
    }
    if(created)
        DbgCmdExec("disasm dis.sel()");
    else
        GuiUpdateDisassemblyView();
    return false;
}

bool cbInstrDataUnknown(int argc, char* argv[])
{
    return cbInstrDataGeneric(enc_unknown, argc, argv);
}

bool cbInstrDataByte(int argc, char* argv[])
{
    return cbInstrDataGeneric(enc_byte, argc, argv);
}

bool cbInstrDataWord(int argc, char* argv[])
{
    return cbInstrDataGeneric(enc_word, argc, argv);
}

bool cbInstrDataDword(int argc, char* argv[])
{
    return cbInstrDataGeneric(enc_dword, argc, argv);
}

bool cbInstrDataFword(int argc, char* argv[])
{
    return cbInstrDataGeneric(enc_fword, argc, argv);
}

bool cbInstrDataQword(int argc, char* argv[])
{
    return cbInstrDataGeneric(enc_qword, argc, argv);
}

bool cbInstrDataTbyte(int argc, char* argv[])
{
    return cbInstrDataGeneric(enc_tbyte, argc, argv);
}

bool cbInstrDataOword(int argc, char* argv[])
{
    return cbInstrDataGeneric(enc_oword, argc, argv);
}

bool cbInstrDataMmword(int argc, char* argv[])
{
    return cbInstrDataGeneric(enc_mmword, argc, argv);
}

bool cbInstrDataXmmword(int argc, char* argv[])
{
    return cbInstrDataGeneric(enc_xmmword, argc, argv);
}

bool cbInstrDataYmmword(int argc, char* argv[])
{
    return cbInstrDataGeneric(enc_ymmword, argc, argv);
}

bool cbInstrDataFloat(int argc, char* argv[])
{
    return cbInstrDataGeneric(enc_real4, argc, argv);
}

bool cbInstrDataDouble(int argc, char* argv[])
{
    return cbInstrDataGeneric(enc_real8, argc, argv);
}

bool cbInstrDataLongdouble(int argc, char* argv[])
{
    return cbInstrDataGeneric(enc_real10, argc, argv);
}

bool cbInstrDataAscii(int argc, char* argv[])
{
    return cbInstrDataGeneric(enc_ascii, argc, argv);
}

bool cbInstrDataUnicode(int argc, char* argv[])
{
    return cbInstrDataGeneric(enc_unicode, argc, argv);
}

bool cbInstrDataCode(int argc, char* argv[])
{
    return cbInstrDataGeneric(enc_code, argc, argv);
}

bool cbInstrDataJunk(int argc, char* argv[])
{
    return cbInstrDataGeneric(enc_junk, argc, argv);
}

bool cbInstrDataMiddle(int argc, char* argv[])
{
    return cbInstrDataGeneric(enc_middle, argc, argv);
}

#define towner "cmd"

bool cbInstrAddType(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return false;
    if(!AddType(towner, argv[1], argv[2]))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "AddType failed"));
        return false;
    }
    GuiTypeListUpdated();
    dprintf_untranslated("typedef %s %s;\n", argv[2], argv[1]);
    return true;
}

bool cbInstrAddStruct(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    if(!AddStruct(towner, argv[1]))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "AddStruct failed"));
        return false;
    }
    GuiTypeListUpdated();
    dprintf_untranslated("struct %s;\n", argv[1]);
    return true;
}

bool cbInstrAddUnion(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    if(!AddUnion(towner, argv[1]))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "AddUnion failed"));
        return false;
    }
    GuiTypeListUpdated();
    dprintf_untranslated("union %s;\n", argv[1]);
    return true;
}

bool cbInstrAddMember(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 4))
        return false;
    auto parent = argv[1];
    auto type = argv[2];
    auto name = argv[3];
    int arraySize = 0, offset = -1;
    if(argc > 4)
    {
        duint value;
        if(!valfromstring(argv[4], &value, false))
            return false;
        arraySize = int(value);
        if(argc > 5)
        {
            if(!valfromstring(argv[5], &value, false))
                return false;
            offset = int(value);
        }
    }
    if(!AddMember(parent, type, name, arraySize, offset))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "AddMember failed"));
        return false;
    }
    GuiTypeListUpdated();
    dprintf_untranslated("%s: %s %s", parent, type, name);
    if(arraySize > 0)
        dprintf_untranslated("[%d]", arraySize);
    if(offset >= 0)
        dprintf_untranslated(" (offset: %d)", offset);
    dputs_untranslated(";");
    return true;
}

bool cbInstrAppendMember(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return false;
    auto type = argv[1];
    auto name = argv[2];
    int arraySize = 0, offset = -1;
    if(argc > 3)
    {
        duint value;
        if(!valfromstring(argv[3], &value, false))
            return false;
        arraySize = int(value);
        if(argc > 4)
        {
            if(!valfromstring(argv[4], &value, false))
                return false;
            offset = int(value);
        }
    }
    if(!AppendMember(type, name, arraySize, offset))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "AppendMember failed"));
        return false;
    }
    GuiTypeListUpdated();
    dprintf_untranslated("%s %s", type, name);
    if(arraySize > 0)
        dprintf_untranslated("[%d]", arraySize);
    if(offset >= 0)
        dprintf_untranslated(" (offset: %d)", offset);
    dputs_untranslated(";");
    return true;
}

bool cbInstrAddFunction(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return false;
    auto name = argv[1];
    auto returnType = argv[2];
    auto callconv = Cdecl;
    auto noreturn = false;
    if(argc > 3)
    {
        if(scmp(argv[3], "cdecl"))
            callconv = Cdecl;
        else if(scmp(argv[3], "stdcall"))
            callconv = Stdcall;
        else if(scmp(argv[3], "thiscall"))
            callconv = Thiscall;
        else if(scmp(argv[3], "delphi"))
            callconv = Delphi;
        else
        {
            dprintf(QT_TRANSLATE_NOOP("DBG", "Unknown calling convention \"%s\"\n"), argv[3]);
            return false;
        }
        if(argc > 4)
        {
            duint value;
            if(!valfromstring(argv[4], &value, false))
                return false;
            noreturn = value != 0;
        }
    }
    if(!AddFunction(towner, name, returnType, callconv, noreturn))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "AddFunction failed"));
        return false;
    }
    GuiTypeListUpdated();
    dprintf_untranslated("%s %s();\n", returnType, name);
    return true;
}

bool cbInstrAddArg(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 4))
        return false;
    if(!AddArg(argv[1], argv[2], argv[3]))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "AddArg failed"));
        return false;
    }
    GuiTypeListUpdated();
    dprintf_untranslated("%s: %s %s;\n", argv[1], argv[2], argv[3]);
    return true;
}

bool cbInstrAppendArg(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return false;
    if(!AppendArg(argv[1], argv[2]))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "AppendArg failed"));
        return false;
    }
    GuiTypeListUpdated();
    dprintf_untranslated("%s %s;\n", argv[1], argv[2]);
    return true;
}

bool cbInstrSizeofType(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    auto size = SizeofType(argv[1]);
    if(!size)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "SizeofType failed"));
        return false;
    }
    dprintf_untranslated("sizeof(%s) = %d\n", argv[1], size);
    varset("$result", size, false);
    return true;
}

bool cbInstrVisitType(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;

    std::string type;
    if(argv[1][0] == '#')
    {
        duint typeId;
        if(!valfromstring(argv[1] + 1, &typeId, false))
            return false;

        const TypeBase* typeDescriptor = LookupTypeById(typeId);
        if(!typeDescriptor || !typeDescriptor->name.c_str())
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "Invalid type ID"));
            return false;
        }

        type = typeDescriptor->name;
    }
    else
    {
        type = argv[1];
    }

    auto name = "";
    duint addr = 0;
    auto maxPtrDepth = gDefaultMaxPtrDepth;

    if(argc > 2)
    {
        if(!valfromstring(argv[2], &addr, false))
            return false;
        if(argc > 3)
        {
            duint value;
            if(!valfromstring(argv[3], &value, false))
                return false;
            if(dsint(value) >= 0)
                maxPtrDepth = int(value);
            if(argc > 4)
                name = argv[4];
        }
    }

    NodeVisitor visitor(GuiTypeAddNode, nullptr, addr);
    visitor.mMaxPtrDepth = maxPtrDepth;
    visitor.mCreateLabels = true;
    if(!VisitType(type, name, visitor))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "VisitType failed"));
        return false;
    }

    GuiShowStructView();
    GuiUpdateAllViews();
    dputs(QT_TRANSLATE_NOOP("DBG", "Done!"));
    return true;
}

bool cbInstrClearTypes(int argc, char* argv[])
{
    auto owner = towner;
    if(argc > 1)
        owner = argv[1];
    ClearTypes(owner);
    GuiTypeListUpdated();
    return true;
}

bool cbInstrRemoveType(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    if(!RemoveType(argv[1]))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "RemoveType failed"));
        return false;
    }
    GuiTypeListUpdated();
    dprintf(QT_TRANSLATE_NOOP("DBG", "Type %s removed\n"), argv[1]);
    return true;
}

bool cbInstrEnumTypes(int argc, char* argv[])
{
    std::vector<TypeManager::Summary> typeList;
    EnumTypes(typeList);
    for(auto & type : typeList)
    {
        if(type.owner.empty())
            type.owner.assign("x64dbg");
        dprintf_untranslated("%s: %s %s, sizeof(%s) = %d\n", type.owner.c_str(), type.kind.c_str(), type.name.c_str(), type.name.c_str(), type.size);
    }
    return true;
}

bool cbInstrLoadTypes(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    auto owner = FileHelper::GetFileName(argv[1]);
    ClearTypes(owner);
    if(!LoadTypesFile(argv[1], owner))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "LoadTypes failed"));
        return false;
    }
    GuiTypeListUpdated();
    dputs(QT_TRANSLATE_NOOP("DBG", "Types loaded"));
    return true;
}

bool cbInstrParseTypes(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    auto owner = FileHelper::GetFileName(argv[1]);
    ClearTypes(owner);
    std::string data;
    if(!FileHelper::ReadAllText(argv[1], data))
    {
        dputs("failed to read file!");
        return false;
    }

    int errorCount = 0;
    if(!ParseTypes(data, owner, errorCount))
    {
        dprintf("Failed to parse header: %s (%d errors, see log)\n", argv[1], errorCount);
        return false;
    }

    GuiTypeListUpdated();
    dprintf("Parsed header: %s\n", argv[1]);
    return true;
}
