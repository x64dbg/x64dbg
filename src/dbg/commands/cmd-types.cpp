#include "cmd-types.h"
#include "console.h"
#include "encodemap.h"
#include "value.h"
#include "types.h"
#include "memory.h"
#include "variable.h"
#include "filehelper.h"
#include "label.h"

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
    int arrsize = 0, offset = -1;
    if(argc > 4)
    {
        duint value;
        if(!valfromstring(argv[4], &value, false))
            return false;
        arrsize = int(value);
        if(argc > 5)
        {
            if(!valfromstring(argv[5], &value, false))
                return false;
            offset = int(value);
        }
    }
    if(!AddMember(parent, type, name, arrsize, offset))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "AddMember failed"));
        return false;
    }
    dprintf_untranslated("%s: %s %s", parent, type, name);
    if(arrsize > 0)
        dprintf_untranslated("[%d]", arrsize);
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
    int arrsize = 0, offset = -1;
    if(argc > 3)
    {
        duint value;
        if(!valfromstring(argv[3], &value, false))
            return false;
        arrsize = int(value);
        if(argc > 4)
        {
            if(!valfromstring(argv[4], &value, false))
                return false;
            offset = int(value);
        }
    }
    if(!AppendMember(type, name, arrsize, offset))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "AppendMember failed"));
        return false;
    }
    dprintf_untranslated("%s %s", type, name);
    if(arrsize > 0)
        dprintf_untranslated("[%d]", arrsize);
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
    if(!AddFunction(towner, name, rettype, callconv, noreturn))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "AddFunction failed"));
        return false;
    }
    dprintf_untranslated("%s %s();\n", rettype, name);
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

struct PrintVisitor : TypeManager::Visitor
{
    explicit PrintVisitor(duint data = 0, int maxPtrDepth = 0)
        : mAddr(data), mMaxPtrDepth(maxPtrDepth)
    {
    }

    template <typename T>
    static String basicPrint(void* data, const char* format)
    {
        return StringUtils::sprintf(format, *(T*)data);
    }

    template <typename T, typename U>
    static String basicPrint(void* data, const char* format)
    {
        return StringUtils::sprintf(format, *(T*)data, *(U*)data);
    }

    static bool cbPrintPrimitive(const TYPEDESCRIPTOR* type, char* dest, size_t* destCount)
    {
        if(!type->addr || !type->bitSize)
        {
            *dest = '\0';
            return true;
        }
        String valueStr;

        Memory<unsigned char*> data((type->bitSize + 7) / 8);
        if(MemRead(type->addr + type->offset + type->bitOffset / 8, data(), (type->bitSize + 7) / 8))
        {
            if(type->reverse)
                std::reverse(data(), data() + data.size());

            size_t bitsFromStart = type->bitOffset % 8;
            if(bitsFromStart != 0)  // have to shift the data over
            {
                uint8_t currentCarry = 0;
                for(size_t i = data.size(); i > 0; i--)
                {
                    uint8_t newCarry = data()[i - 1] << 8 - bitsFromStart;
                    data()[i - 1] = data()[i - 1] >> bitsFromStart | currentCarry;

                    currentCarry = newCarry;
                }
            }

            if(type->bitSize % 8 != 0)
            {
                uint8_t mask = (1 << type->bitSize % 8) - 1;
                data()[(type->bitSize + 7) / 8 - 1] &= mask;
            }

            switch(Primitive(type->id))
            {
            case Void:
                valueStr.clear();
                break;
            case Int8:
                valueStr += StringUtils::sprintf("0x%02X, '%s'", *(unsigned char*)data(), StringUtils::Escape(*(char*)data()).c_str());
                break;
            case Uint8:
                valueStr += basicPrint<unsigned char, unsigned char>(data(), "0x%02X, %d");
                break;
            case Int16:
                valueStr += basicPrint<unsigned short, short>(data(), "0x%04X, %d");
                break;
            case Uint16:
                valueStr += basicPrint<unsigned short, unsigned short>(data(), "0x%04X, %u");
                break;
            case Int32:
                valueStr += basicPrint<unsigned int, int>(data(), "0x%08X, %d");
                break;
            case Uint32:
                valueStr += basicPrint<unsigned int, unsigned int>(data(), "0x%08X, %u");
                break;
            case Int64:
                valueStr += basicPrint<unsigned long long, long long>(data(), "0x%016llX, %lld");
                break;
            case Uint64:
                valueStr += basicPrint<unsigned long long, unsigned long long>(data(), "0x%016llX, %llu");
                break;
            case Dsint:
#ifdef _WIN64
                valueStr += basicPrint<duint, dsint>(data(), "0x%016llX, %lld");
#else
                valueStr += basicPrint<duint, dsint>(data(), "0x%08X, %d");
#endif //_WIN64
                break;
            case Duint:
#ifdef _WIN64
                valueStr += basicPrint<duint, duint>(data(), "0x%016llX, %llu");
#else
                valueStr += basicPrint<duint, duint>(data(), "0x%08X, %u");
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
                valueStr += basicPrint<char*>(data(), "0x%p ");
                Memory<char*> strdata(MAX_STRING_SIZE + 1);
                if(MemRead(*(duint*)data(), strdata(), strdata.size() - 1))
                {
                    valueStr += "\"";
                    valueStr += StringUtils::Escape(strdata());
                    valueStr.push_back('\"');
                }
                else
                    valueStr += "???";
            }
            break;

            case PtrWString:
            {
                valueStr += basicPrint<wchar_t*>(data(), "0x%p ");
                Memory<wchar_t*> strdata(MAX_STRING_SIZE * 2 + 2);
                if(MemRead(*(duint*)data(), strdata(), strdata.size() - 2))
                {
                    valueStr += "L\"";
                    valueStr += StringUtils::Utf16ToUtf8(strdata());
                    valueStr.push_back('\"');
                }
                else
                    valueStr += "???";
            }
            break;

            default:
                return false;
            }
        }
        else
            valueStr = "???";
        if(*destCount <= valueStr.size())
        {
            *destCount = valueStr.size() + 1;
            return false;
        }
        strcpy_s(dest, *destCount, valueStr.c_str());
        return true;
    }

    static bool cbPrintEnum(const TYPEDESCRIPTOR* type, char* dest, size_t* destCount)
    {
        if(!type->addr || !type->userdata)
        {
            *dest = '\0';
            return true;
        }

        String valueStr;
        const size_t byteSize = (type->bitSize + 7) / 8; // convert bit size to byte size
        Memory<unsigned char*> data(byteSize);

        if(MemRead(type->addr + type->offset, data(), data.size()))
        {
            if(type->reverse)
                std::reverse(data(), data() + data.size());

            const size_t bitOffset = type->offset % 8;
            uint64_t extractedValue = 0;

            memcpy(&extractedValue, data(), std::min(sizeof(extractedValue), data.size()));
            extractedValue >>= bitOffset;
            extractedValue &= (1ULL << byteSize) - 1;

            const auto enumData = static_cast<Enum*>(type->userdata);
            if(enumData->isFlags)
            {
                bool first = true;
                uint64_t remainingBits = extractedValue;

                for(const auto & member : enumData->members)
                {
                    if((extractedValue & member.first) == member.first && member.first != 0)
                    {
                        if(!first)
                            valueStr += " | ";

                        valueStr += member.second;
                        remainingBits &= ~member.first;
                        first = false;
                    }
                }

                if(remainingBits != 0)
                {
                    if(!first)
                        valueStr += " | ";

                    valueStr += StringUtils::sprintf("0x%llX", remainingBits);
                }

                if(first)
                    valueStr = StringUtils::sprintf("0x%llX", extractedValue);
            }
            else
            {
                auto it = std::find_if(enumData->members.begin(), enumData->members.end(),
                                       [extractedValue](const std::pair<uint64_t, std::string> & member)
                {
                    return member.first == extractedValue;
                });

                if(it != enumData->members.end())
                    valueStr = it->second;
                else
                    valueStr = StringUtils::sprintf("0x%llX", extractedValue);
            }
        }
        else
            valueStr = "???";

        if(*destCount <= valueStr.size())
        {
            *destCount = valueStr.size() + 1;
            return false;
        }

        strcpy_s(dest, *destCount, valueStr.c_str());
        return true;
    }

    bool visitType(const Member & member, const Type & type) override
    {
        if(!mParents.empty() && parent().type == Parent::Union)
        {
            mOffset = parent().offset;
            mBitOffset = 0;
        }

        String tname;
        auto ptype = mParents.empty() ? Parent::Struct : parent().type;
        if(ptype == Parent::Array)
        {
            tname = StringUtils::sprintf("%s[%u]", member.name.c_str(), parent().index++);
        }
        else
        {
            if(member.bitfield)
                tname = StringUtils::sprintf("%s %s : %d", member.assignedType.c_str(), member.name.c_str(), member.bitSize);
            else
                tname = StringUtils::sprintf("%s %s", member.assignedType.c_str(), member.name.c_str());

            // Prepend struct/union to pointer types
            if(!type.pointto.empty())
            {
                auto ptrname = StructUnionPtrType(type.pointto);
                if(!ptrname.empty())
                    tname = ptrname + " " + tname;
            }
        }

        std::string path;
        for(size_t i = 0; i < mPath.size(); i++)
        {
            if(ptype == Parent::Array && i + 1 == mPath.size())
                break;
            path.append(mPath[i]);
        }
        path.append(member.name);

        auto ptr = mAddr + mOffset;
        if(MemIsValidReadPtr(ptr))
        {
            if(!LabelGet(ptr, nullptr) && (!mParents.empty() && (parent().index == 1 || ptype != Parent::Array)))
                LabelSet(ptr, path.c_str(), false, true);
        }

        TYPEDESCRIPTOR td = { };
        td.expanded = false;
        td.reverse = false;
        td.name = tname.c_str();
        td.addr = mAddr;
        td.offset = mOffset;
        td.id = type.primitive;
        td.bitSize = type.sizeFUCK;
        if(td.bitSize < 0)
            __debugbreak();

        td.callback = cbPrintPrimitive;
        td.userdata = nullptr;
        td.bitOffset = mBitOffset;
        mNode = GuiTypeAddNode(mParents.empty() ? nullptr : parent().node, &td);

        if(!member.bitfield)
        {
            mBitOffset = 0;
            mOffset += td.bitSize / 8;
        }
        else
        {
            mBitOffset += member.bitSize;
        }

        return true;
    }

    bool visitStructUnion(const Member & member, const StructUnion & type) override
    {
        if(!mParents.empty() && parent().type == Parent::Type::Union)
        {
            mOffset = parent().offset;
            mBitOffset = 0;
        }

        std::string targetType;
        if(member.assignedType.find("__anonymous") == 0)
            targetType = "";
        else
            targetType = member.assignedType;

        std::string targetName;
        if(member.name.find("__anonymous") == 0)
            targetName = "";
        else
            targetName = member.name;

        String tname = StringUtils::sprintf("%s %s %s", type.isUnion ? "union" : "struct", targetType.c_str(), targetName.c_str());

        TYPEDESCRIPTOR td = { };
        td.expanded = true;
        td.reverse = false;
        td.name = tname.c_str();
        td.addr = mAddr;
        td.offset = mOffset;
        td.id = Typedef;
        td.bitSize = type.sizeFUCK;
        td.callback = nullptr;
        td.userdata = nullptr;
        auto node = GuiTypeAddNode(mParents.empty() ? nullptr : parent().node, &td);

        mPath.push_back((member.name == "display" ? type.name : member.assignedType) + ".");
        mParents.emplace_back(type.isUnion ? Parent::Union : Parent::Struct);
        parent().node = node;
        parent().size = td.bitSize / 8;
        parent().offset = mOffset;
        return true;
    }

    bool visitEnum(const Member & member, const Enum & num) override
    {
        if(!mParents.empty() && parent().type == Parent::Type::Union)
        {
            mOffset = parent().offset;
            mBitOffset = 0;
        }

        String tname = StringUtils::sprintf("enum %s %s", member.assignedType.c_str(), member.name.c_str());

        TYPEDESCRIPTOR td = { };
        td.expanded = true;
        td.reverse = false;
        td.name = tname.c_str();
        td.addr = mAddr;
        td.offset = mOffset;
        td.id = Typedef;
        td.bitSize = num.sizeFUCK;
        td.callback = cbPrintEnum;
        td.userdata = const_cast<Enum*>(&num);
        mNode = GuiTypeAddNode(mParents.empty() ? nullptr : parent().node, &td);
        mOffset += td.bitSize / 8;

        return true;
    }

    bool visitArray(const Member & member) override
    {
        String tname = StringUtils::sprintf("%s %s[%d]", member.assignedType.c_str(), member.name.c_str(), member.arrsize);

        TYPEDESCRIPTOR td = { };
        td.expanded = member.arrsize <= 5;
        td.reverse = false;
        td.name = tname.c_str();
        td.addr = mAddr;
        td.offset = mOffset;
        td.id = Typedef;
        td.bitSize = member.arrsize * SizeofType(member.type);
        td.callback = nullptr;
        td.userdata = nullptr;
        auto node = GuiTypeAddNode(mParents.empty() ? nullptr : parent().node, &td);

        mPath.push_back(member.name + ".");
        mParents.emplace_back(Parent::Array);
        parent().node = node;
        parent().size = td.bitSize / 8;
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

        mPath.push_back(member.assignedType + "->");
        mParents.emplace_back(Parent::Pointer);
        parent().offset = mOffset;
        parent().addr = mAddr;
        parent().node = mNode;
        parent().size = type.sizeFUCK / 8;
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
        else if(parent().type == Parent::Union)
        {
            mOffset = parent().offset + parent().size;
        }
        mParents.pop_back();
        mPath.pop_back();
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
        void* node = nullptr;
        int size = 0;

        explicit Parent(Type type)
            : type(type)
        {
        }
    };

    Parent & parent()
    {
        return mParents[mParents.size() - 1];
    }

    std::vector<Parent> mParents;
    duint mOffset = 0;
    duint mBitOffset = 0;
    duint mAddr = 0;
    int mPtrDepth = 0;
    int mMaxPtrDepth = 0;
    void* mNode = nullptr;
    std::vector<String> mPath;
};

bool cbInstrVisitType(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    auto type = argv[1];
    auto name = "display";
    duint addr = 0;
    auto maxPtrDepth = 0;
    if(argc > 2)
    {
        if(!valfromstring(argv[2], &addr, false))
            return false;
        if(argc > 3)
        {
            duint value;
            if(!valfromstring(argv[3], &value, false))
                return false;
            maxPtrDepth = int(value);
            if(argc > 4)
                name = argv[4];
        }
    }
    PrintVisitor visitor(addr, maxPtrDepth);
    if(!VisitType(type, name, visitor))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "VisitType failed"));
        return false;
    }
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
    if(!ParseTypes(data, owner))
        return false;
    dprintf("Parsed header: %s\n", argv[1]);
    return true;
}
