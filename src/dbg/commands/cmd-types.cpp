#include "cmd-types.h"
#include "console.h"
#include "encodemap.h"
#include "value.h"
#include "types.h"
#include "memory.h"
#include "variable.h"

using namespace Types;

static CMDRESULT cbInstrDataGeneric(ENCODETYPE type, int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return STATUS_ERROR;
    duint addr;
    if(!valfromstring(argv[1], &addr, false))
        return STATUS_ERROR;
    duint size = 1;
    if(argc >= 3)
        if(!valfromstring(argv[2], &size, false))
            return STATUS_ERROR;
    bool created;
    if(!EncodeMapSetType(addr, size, type, &created))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "EncodeMapSetType failed..."));
        return STATUS_ERROR;
    }
    if(created)
        DbgCmdExec("disasm dis.sel()");
    else
        GuiUpdateDisassemblyView();
    return STATUS_ERROR;
}

CMDRESULT cbInstrDataUnknown(int argc, char* argv[])
{
    return cbInstrDataGeneric(enc_unknown, argc, argv);
}

CMDRESULT cbInstrDataByte(int argc, char* argv[])
{
    return cbInstrDataGeneric(enc_byte, argc, argv);
}

CMDRESULT cbInstrDataWord(int argc, char* argv[])
{
    return cbInstrDataGeneric(enc_word, argc, argv);
}

CMDRESULT cbInstrDataDword(int argc, char* argv[])
{
    return cbInstrDataGeneric(enc_dword, argc, argv);
}

CMDRESULT cbInstrDataFword(int argc, char* argv[])
{
    return cbInstrDataGeneric(enc_fword, argc, argv);
}

CMDRESULT cbInstrDataQword(int argc, char* argv[])
{
    return cbInstrDataGeneric(enc_qword, argc, argv);
}

CMDRESULT cbInstrDataTbyte(int argc, char* argv[])
{
    return cbInstrDataGeneric(enc_tbyte, argc, argv);
}

CMDRESULT cbInstrDataOword(int argc, char* argv[])
{
    return cbInstrDataGeneric(enc_oword, argc, argv);
}

CMDRESULT cbInstrDataMmword(int argc, char* argv[])
{
    return cbInstrDataGeneric(enc_mmword, argc, argv);
}

CMDRESULT cbInstrDataXmmword(int argc, char* argv[])
{
    return cbInstrDataGeneric(enc_xmmword, argc, argv);
}

CMDRESULT cbInstrDataYmmword(int argc, char* argv[])
{
    return cbInstrDataGeneric(enc_ymmword, argc, argv);
}

CMDRESULT cbInstrDataFloat(int argc, char* argv[])
{
    return cbInstrDataGeneric(enc_real4, argc, argv);
}

CMDRESULT cbInstrDataDouble(int argc, char* argv[])
{
    return cbInstrDataGeneric(enc_real8, argc, argv);
}

CMDRESULT cbInstrDataLongdouble(int argc, char* argv[])
{
    return cbInstrDataGeneric(enc_real10, argc, argv);
}

CMDRESULT cbInstrDataAscii(int argc, char* argv[])
{
    return cbInstrDataGeneric(enc_ascii, argc, argv);
}

CMDRESULT cbInstrDataUnicode(int argc, char* argv[])
{
    return cbInstrDataGeneric(enc_unicode, argc, argv);
}

CMDRESULT cbInstrDataCode(int argc, char* argv[])
{
    return cbInstrDataGeneric(enc_code, argc, argv);
}

CMDRESULT cbInstrDataJunk(int argc, char* argv[])
{
    return cbInstrDataGeneric(enc_junk, argc, argv);
}

CMDRESULT cbInstrDataMiddle(int argc, char* argv[])
{
    return cbInstrDataGeneric(enc_middle, argc, argv);
}

#define towner "cmd"

CMDRESULT cbInstrAddType(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return STATUS_ERROR;
    if(!AddType(towner, argv[1], argv[2]))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "AddType failed"));
        return STATUS_ERROR;
    }
    dprintf_untranslated("typedef %s %s;\n", argv[2], argv[1]);
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrAddStruct(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return STATUS_ERROR;
    if(!AddStruct(towner, argv[1]))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "AddStruct failed"));
        return STATUS_ERROR;
    }
    dprintf_untranslated("struct %s;\n", argv[1]);
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrAddUnion(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return STATUS_ERROR;
    if(!AddUnion(towner, argv[1]))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "AddUnion failed"));
        return STATUS_ERROR;
    }
    dprintf_untranslated("union %s;\n", argv[1]);
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrAddMember(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 4))
        return STATUS_ERROR;
    auto parent = argv[1];
    auto type = argv[2];
    auto name = argv[3];
    int arrsize = 0, offset = -1;
    if(argc > 3)
    {
        duint value;
        if(!valfromstring(argv[4], &value, false))
            return STATUS_ERROR;
        arrsize = int(value);
        if(argc > 4)
        {
            if(!valfromstring(argv[5], &value, false))
                return STATUS_ERROR;
            offset = int(value);
        }
    }
    if(!AddMember(parent, type, name, arrsize, offset))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "AddMember failed"));
        return STATUS_ERROR;
    }
    dprintf_untranslated("%s: %s %s", parent, type, name);
    if(arrsize > 0)
        dprintf_untranslated("[%d]", arrsize);
    if(offset >= 0)
        dprintf_untranslated(" (offset: %d)", offset);
    dputs_untranslated(";");
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrAppendMember(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return STATUS_ERROR;
    auto type = argv[1];
    auto name = argv[2];
    int arrsize = 0, offset = -1;
    if(argc > 3)
    {
        duint value;
        if(!valfromstring(argv[3], &value, false))
            return STATUS_ERROR;
        arrsize = int(value);
        if(argc > 4)
        {
            if(!valfromstring(argv[4], &value, false))
                return STATUS_ERROR;
            offset = int(value);
        }
    }
    if(!AppendMember(type, name, arrsize, offset))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "AppendMember failed"));
        return STATUS_ERROR;
    }
    dprintf_untranslated("%s %s", type, name);
    if(arrsize > 0)
        dprintf_untranslated("[%d]", arrsize);
    if(offset >= 0)
        dprintf_untranslated(" (offset: %d)", offset);
    dputs_untranslated(";");
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrAddFunction(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return STATUS_ERROR;
    auto name = argv[1];
    auto rettype = argv[2];
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
            return STATUS_ERROR;
        }
        if(argc > 4)
        {
            duint value;
            if(!valfromstring(argv[4], &value, false))
                return STATUS_ERROR;
            noreturn = value != 0;
        }
    }
    if(!AddFunction(towner, name, rettype, callconv, noreturn))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "AddFunction failed"));
        return STATUS_ERROR;
    }
    dprintf_untranslated("%s %s();\n", rettype, name);
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrAddArg(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 4))
        return STATUS_ERROR;
    if(!AddArg(argv[1], argv[2], argv[3]))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "AddArg failed"));
        return STATUS_ERROR;
    }
    dprintf_untranslated("%s: %s %s;\n", argv[1], argv[2], argv[3]);
    return STATUS_CONTINUE;

}

CMDRESULT cbInstrAppendArg(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return STATUS_ERROR;
    if(!AppendArg(argv[1], argv[2]))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "AppendArg failed"));
        return STATUS_ERROR;
    }
    dprintf_untranslated("%s %s;\n", argv[1], argv[2]);
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrSizeofType(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return STATUS_ERROR;
    auto size = SizeofType(argv[1]);
    if(!size)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "SizeofType failed"));
        return STATUS_ERROR;
    }
    dprintf_untranslated("sizeof(%s) = %d\n", argv[1], size);
    varset("$result", size, false);
    return STATUS_CONTINUE;
}

struct PrintVisitor : TypeManager::Visitor
{
    explicit PrintVisitor(duint data = 0, int maxPtrDepth = 0)
        : mAddr(data), mMaxPtrDepth(maxPtrDepth) { }

    template<typename T>
    static String basicPrint(void* data, const char* format)
    {
        return StringUtils::sprintf(format, *(T*)data);
    }

    bool visitType(const Member & member, const Type & type) override
    {
        String valueStr;
        Memory<unsigned char*> data(type.size);
        if(mAddr)
        {
            if(MemRead(mAddr + mOffset, data(), data.size()))
            {
                valueStr.assign(" = ");
                switch(type.primitive)
                {
                case Int8:
                    valueStr += basicPrint<char>(data(), "%c");
                    break;
                case Uint8:
                    valueStr += basicPrint<unsigned char>(data(), "0x%02X");
                    break;
                case Int16:
                    valueStr += basicPrint<short>(data(), "%d");
                    break;
                case Uint16:
                    valueStr += basicPrint<short>(data(), "%u");
                    break;
                case Int32:
                    valueStr += basicPrint<int>(data(), "%d");
                    break;
                case Uint32:
                    valueStr += basicPrint<unsigned int>(data(), "%u");
                    break;
                case Int64:
                    valueStr += basicPrint<long long>(data(), "%lld");
                    break;
                case Uint64:
                    valueStr += basicPrint<unsigned long long>(data(), "%llu");
                    break;
                case Dsint:
#ifdef _WIN64
                    valueStr += basicPrint<dsint>(data(), "%lld");
#else
                    valueStr += basicPrint<dsint>(data(), "%d");
#endif //_WIN64
                    break;
                case Duint:
#ifdef _WIN64
                    valueStr += basicPrint<duint>(data(), "%llu");
#else
                    valueStr += basicPrint<duint>(data(), "%u");
#endif //_WIN64
                    break;
                case Float:
                    valueStr += basicPrint<float>(data(), "%f");
                    break;
                case Double:
                    valueStr += basicPrint<double>(data(), "%f");
                    break;
                case Pointer:
                    valueStr += basicPrint<void*>(data(), "0x%p");
                    break;
                case PtrString:
                {
                    valueStr += basicPrint<char*>(data(), "0x%p");
                    Memory<char*> strdata(MAX_STRING_SIZE + 1);
                    if(MemRead(*(duint*)data(), strdata(), strdata.size() - 1))
                    {
                        valueStr += " \"";
                        valueStr += strdata();
                        valueStr.push_back('\"');
                    }
                    else
                        valueStr += " ???";
                }
                break;

                case PtrWString:
                {
                    valueStr += basicPrint<wchar_t*>(data(), "0x%p");
                    Memory<wchar_t*> strdata(MAX_STRING_SIZE * 2 + 2);
                    if(MemRead(*(duint*)data(), strdata(), strdata.size() - 2))
                    {
                        valueStr += " L\"";
                        valueStr += StringUtils::Utf16ToUtf8(strdata());
                        valueStr.push_back('\"');
                    }
                    else
                        valueStr += " ???";
                }
                break;

                default:
                    return false;
                }
            }
            else
                valueStr = " ???";
        }
        indent();
        auto ptype = mParents.empty() ? Parent::Struct : parent().type;
        if(ptype == Parent::Array)
            dprintf_untranslated("%s[%u]%s;", member.name.c_str(), parent().index++, valueStr.c_str());
        else
            dprintf_untranslated("%s %s%s;", type.name.c_str(), member.name.c_str(), valueStr.c_str());
        dputs_untranslated(type.pointto.empty() || mPtrDepth >= mMaxPtrDepth ? "" : " {");
        if(ptype != Parent::Union)
            mOffset += type.size;
        return true;
    }

    bool visitStructUnion(const Member & member, const StructUnion & type) override
    {
        indent();
        dprintf_untranslated("%s %s {\n", type.isunion ? "union" : "struct", type.name.c_str());
        mParents.push_back(Parent(type.isunion ? Parent::Union : Parent::Struct));
        return true;
    }

    bool visitArray(const Member & member) override
    {
        indent();
        dprintf_untranslated("%s %s[%d] {\n", member.type.c_str(), member.name.c_str(), member.arrsize);
        mParents.push_back(Parent(Parent::Array));
        return true;
    }

    bool visitPtr(const Member & member, const Type & type) override
    {
        auto offset = mOffset;
        auto res = visitType(member, type); //print the pointer value
        if(mPtrDepth >= mMaxPtrDepth)
            return false;
        duint value = 0;
        if(!mAddr || !MemRead(mAddr + offset, &value, sizeof(value)))
            return false;
        mParents.push_back(Parent(Parent::Pointer));
        parent().offset = mOffset;
        parent().addr = mAddr;
        mOffset = 0;
        mAddr = value;
        mPtrDepth++;
        return res;
    }

    bool visitBack(const Member & member) override
    {
        if(parent().type == Parent::Pointer)
        {
            mOffset = parent().offset;
            mAddr = parent().addr;
            mPtrDepth--;
        }
        mParents.pop_back();
        indent();
        if(parent().type == Parent::Array)
            dprintf_untranslated("};\n");
        else
            dprintf_untranslated("} %s;\n", member.name.c_str());
        return true;
    }

private:
    struct Parent
    {
        enum Type
        {
            Struct,
            Union,
            Array,
            Pointer
        };

        Type type;
        unsigned int index = 0;
        duint addr = 0;
        duint offset = 0;

        explicit Parent(Type type)
            : type(type) { }
    };

    Parent & parent()
    {
        return mParents[mParents.size() - 1];
    }

    void indent() const
    {
        if(mAddr)
            dprintf_untranslated("%p ", mAddr + mOffset);
        for(auto i = 0; i < int(mParents.size()) * 2; i++)
            dprintf_untranslated(" ");
    }

    std::vector<Parent> mParents;
    duint mOffset = 0;
    duint mAddr = 0;
    int mPtrDepth = 0;
    int mMaxPtrDepth = 0;
};

CMDRESULT cbInstrVisitType(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return STATUS_ERROR;
    auto type = argv[1];
    auto name = "visit";
    duint addr = 0;
    auto maxPtrDepth = 0;
    if(argc > 2)
    {
        if(!valfromstring(argv[2], &addr, false))
            return STATUS_ERROR;
        if(argc > 3)
        {
            duint value;
            if(!valfromstring(argv[3], &value, false))
                return STATUS_ERROR;
            maxPtrDepth = int(value);
            if(argc > 4)
                name = argv[4];
        }
    }
    PrintVisitor visitor(addr, maxPtrDepth);
    if(!VisitType(type, name, visitor))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "VisitType failed"));
        return STATUS_ERROR;
    }
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrClearTypes(int argc, char* argv[])
{
    auto owner = towner;
    if(argc > 1)
        owner = argv[1];
    ClearTypes(owner);
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrRemoveType(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return STATUS_ERROR;
    if(!RemoveType(argv[1]))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "RemoveType failed"));
        return STATUS_ERROR;
    }
    dprintf(QT_TRANSLATE_NOOP("DBG", "Type %s removed\n"), argv[1]);
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrEnumTypes(int argc, char* argv[])
{
    std::vector<TypeManager::Summary> typeList;
    EnumTypes(typeList);
    for(auto & type : typeList)
    {
        if(type.owner.empty())
            type.owner.assign("x64dbg");
        dprintf_untranslated("%s: %s %s, sizeof(%s) = %d\n", type.owner.c_str(), type.kind.c_str(), type.name.c_str(), type.name.c_str(), type.size);
    }
    return STATUS_CONTINUE;
}
