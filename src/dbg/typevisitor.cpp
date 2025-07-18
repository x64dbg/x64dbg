#include "typevisitor.h"

#include "memory.h"
#include "stringutils.h"
#include "types.h"
#include "label.h"
#include <algorithm>

int gDefaultMaxPtrDepth = 2;
int gDefaultMaxExpandDepth = INT_MAX;
int gDefaultMaxExpandArray = 5;

using namespace Types;

template <typename T>
static std::string basicPrint(void* data, const char* format)
{
    return StringUtils::sprintf(format, *(T*)data);
}

template <typename T, typename U>
static std::string basicPrint(void* data, const char* format)
{
    return StringUtils::sprintf(format, *(T*)data, *(U*)data);
}

static bool cbPrintPrimitive(const TYPEDESCRIPTOR* type, char* dest, size_t* destCount)
{
    if(type->addr == 0 || !type->sizeBits)
    {
        *dest = '\0';
        return true;
    }
    std::string valueStr;

    auto readStart = type->addr + type->offset + type->bitOffset / 8;
    auto readSize = (type->sizeBits + 7) / 8;

    auto memoryReadEnd = readSize;
    auto lastSignificantByte = (type->bitOffset % 8 + type->sizeBits + 7) / 8;

    bool crossByteRead = memoryReadEnd != lastSignificantByte;
    if(crossByteRead)
        readSize += 1;

    Memory<unsigned char*> readBuffer(readSize);
    if(MemRead(readStart, readBuffer(), readSize))
    {
        if(type->reverse)
            std::reverse(readBuffer(), readBuffer() + readBuffer.size());

        size_t bitsFromStart = type->bitOffset % 8;
        if(bitsFromStart != 0)  // have to shift the data over
        {
            uint8_t currentCarry = 0;
            for(size_t i = readBuffer.size(); i > 0; i--)
            {
                uint8_t newCarry = readBuffer()[i - 1] << (8 - bitsFromStart);
                readBuffer()[i - 1] = readBuffer()[i - 1] >> bitsFromStart | currentCarry;

                currentCarry = newCarry;
            }

            // pop the last byte
            // since it will be empty after a cross read
            if(crossByteRead)
                readBuffer()[readBuffer.size() - 1] = 0;
        }

        if(type->sizeBits % 8 != 0)
        {
            uint8_t mask = (1 << type->sizeBits % 8) - 1;
            readBuffer()[(type->sizeBits + 7) / 8 - 1] &= mask;
        }

        Memory<unsigned char*> data(sizeof(unsigned long long));
        memcpy(data(), readBuffer(), std::min(data.size(), readBuffer.size()));

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
    if(type->addr == 0)
    {
        *dest = '\0';
        return true;
    }

    std::string valueStr;
    auto enumTypeIdData = LookupTypeByName(type->typeName);

    auto readStart = type->addr + type->offset + type->bitOffset / 8;
    auto readSize = (type->sizeBits + 7) / 8;

    auto memoryReadEnd = readSize;
    auto lastSignificantByte = (type->bitOffset % 8 + type->sizeBits + 7) / 8;

    bool crossByteRead = memoryReadEnd != lastSignificantByte;
    if(crossByteRead)
        readSize += 1;

    Memory<unsigned char*> readBuffer(readSize);
    if(MemRead(readStart, readBuffer(), readSize))
    {
        if(type->reverse)
            std::reverse(readBuffer(), readBuffer() + readBuffer.size());

        size_t bitsFromStart = type->bitOffset % 8;
        if(bitsFromStart != 0)  // have to shift the data over
        {
            uint8_t currentCarry = 0;
            for(size_t i = readBuffer.size(); i > 0; i--)
            {
                uint8_t newCarry = readBuffer()[i - 1] << (8 - bitsFromStart);
                readBuffer()[i - 1] = readBuffer()[i - 1] >> bitsFromStart | currentCarry;

                currentCarry = newCarry;
            }

            // pop the last byte
            // since it will be empty after a cross read
            if(crossByteRead)
                readBuffer()[readBuffer.size() - 1] = 0;
        }

        if(type->sizeBits % 8 != 0)
        {
            uint8_t mask = (1 << type->sizeBits % 8) - 1;
            readBuffer()[(type->sizeBits + 7) / 8 - 1] &= mask;
        }

        uint64_t extractedValue = 0;
        memcpy(&extractedValue, readBuffer(), std::min(sizeof(uint64_t), readBuffer.size()));

        if(enumTypeIdData)
        {
            auto & enumData = *(Enum*)enumTypeIdData;
            if(enumData.isFlags)
            {
                bool first = true;
                uint64_t remainingBits = extractedValue;

                for(const auto & member : enumData.members)
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
                auto it = std::find_if(enumData.members.begin(), enumData.members.end(),
                                       [extractedValue](const std::pair<uint64_t, std::string> & member)
                {
                    return member.first == extractedValue;
                });

                if(it != enumData.members.end())
                    valueStr = it->second;
                else
                    valueStr = StringUtils::sprintf("0x%llX", extractedValue);
            }
        }
        else
        {
            valueStr = StringUtils::sprintf("0x%llX", extractedValue);
        }
    }
    else
    {
        valueStr = "???";
    }

    if(*destCount <= valueStr.size())
    {
        *destCount = valueStr.size() + 1;
        return false;
    }

    strcpy_s(dest, *destCount, valueStr.c_str());
    return true;
}

static bool cbPrintArray(const TYPEDESCRIPTOR* type, char* dest, size_t* destCount)
{
    if(type->addr == 0 || type->typeName == nullptr || *type->typeName == '\0')
    {
        *dest = '\0';
        return true;
    }

    // set in NodeVisitor::visitArray
    auto arrayAddr = type->addr + type->offset;
    auto entrySizeBits = (int)(duint)type->userdata;
    auto characterCount = std::min(type->sizeBits / entrySizeBits, MAX_STRING_SIZE);

    std::string valueStr;
    if(entrySizeBits == 8)
    {
        std::vector<char> str(characterCount + 1);
        if(!MemRead(arrayAddr, str.data(), str.size() - 1))
        {
            return false;
        }

        if(str[0] != '\0')
        {
            valueStr += '\"';
            valueStr += StringUtils::Escape(str.data());
            valueStr += '\"';
        }
    }
    else if(entrySizeBits == 16)
    {
        std::vector<wchar_t> str(characterCount + 1);
        if(!MemRead(arrayAddr, str.data(), (str.size() - 1) * 2) && str[0] != L'\0')
        {
            return false;
        }

        auto utf8 = StringUtils::Utf16ToUtf8(str.data());
        if(!utf8.empty())
        {
            valueStr += "L\"";
            valueStr += StringUtils::Escape(utf8);
            valueStr += '\"';
        }
    }
    else
    {
        return false;
    }

    if(*destCount <= valueStr.size())
    {
        *destCount = valueStr.size() + 1;
        return false;
    }

    strcpy_s(dest, *destCount, valueStr.c_str());
    return true;
}

static const char* LookupTypePrefix(const std::string & typeName)
{
    auto type = LookupTypeByName(typeName);
    auto td = dynamic_cast<Typedef*>(type);
    if(td != nullptr && td->primitive == Alias)
    {
        return LookupTypePrefix(td->alias);
    }
    return type->prefix();
}

bool NodeVisitor::visitType(const Member & member, const Typedef & type, const std::string & prettyType)
{
    if(!mParents.empty() && parent().type == Parent::Union)
    {
        mOffset = parent().offset;
        mBitOffset = 0;
    }

    std::string tname;
    auto isArrayMember = !mParents.empty() && parent().type == Parent::Type::Array;
    if(isArrayMember)
    {
        tname += '[';
        tname += std::to_string(parent().arrayIndex++);
        tname += ']';
        tname += ' ';
    }

    if(type.primitive == Pointer && !type.alias.empty())
    {
        auto ptrPrefix = LookupTypePrefix(type.alias);
        if(*ptrPrefix != '\0')
        {
            tname += ptrPrefix;
            tname += ' ';
        }
    }

    if(!isArrayMember)
    {
        tname += prettyType;
        tname += ' ';
        tname += member.name;

        if(member.isBitfield)
        {
            tname += " : ";
            tname += std::to_string(member.sizeBits);
        }
    }

    std::string path;
    for(size_t i = 0; i < mPath.size(); i++)
    {
        if(isArrayMember && i + 1 == mPath.size())
            break;
        path.append(mPath[i]);
    }
    path.append(member.name);

    auto ptr = mAddr + mOffset;
    if(mCreateLabels && MemIsValidReadPtr(ptr))
    {
        if(!LabelGet(ptr, nullptr) && (!mParents.empty() && (parent().arrayIndex == 1 || !isArrayMember)))
            LabelSet(ptr, path.c_str(), false, true);
    }

    TYPEDESCRIPTOR td = { };
    td.expanded = mParents.size() < mMaxExpandDepth;
    td.reverse = false;
    td.magic = TYPEDESCRIPTOR_MAGIC;
    td.name = tname.c_str();
    td.addr = mAddr;
    td.offset = mOffset;
    td.id = type.primitive;
    td.sizeBits = member.isBitfield ? member.sizeBits : type.sizeBits;
    if(td.sizeBits < 0)
        __debugbreak();

    td.callback = cbPrintPrimitive;
    td.userdata = nullptr;
    td.bitOffset = mBitOffset;
    td.typeName = member.type.c_str();
    mNode = mAddNode(parentNode(), &td);

    if(!member.isBitfield)
    {
        mBitOffset = 0;
        mOffset += td.sizeBits / 8;
    }
    else
    {
        mBitOffset += member.sizeBits;
    }

    return true;
}

bool NodeVisitor::visitStructUnion(const Member & member, const StructUnion & type, const std::string & prettyType)
{
    if(!mParents.empty() && parent().type == Parent::Type::Union)
    {
        mOffset = parent().offset;
        mBitOffset = 0;
    }

    std::string tname;
    auto isArrayMember = !mParents.empty() && parent().type == Parent::Type::Array;
    if(isArrayMember)
    {
        tname += '[';
        tname += std::to_string(parent().arrayIndex++);
        tname += ']';
        tname += ' ';
    }

    tname += type.isUnion ? "union" : "struct";

    if(prettyType.find("__anonymous") != 0)
    {
        tname += ' ';
        tname += prettyType;
    }

    if(!isArrayMember && member.name.find("__anonymous") != 0)
    {
        tname += ' ';
        tname += member.name;
    }

    TYPEDESCRIPTOR td = { };
    td.expanded = mParents.size() < mMaxExpandDepth;
    td.reverse = false;
    td.magic = TYPEDESCRIPTOR_MAGIC;
    td.name = tname.c_str();
    td.addr = mAddr;
    td.offset = mOffset;
    td.id = type.typeId;
    td.sizeBits = type.sizeBits;
    td.callback = nullptr;
    td.userdata = nullptr;
    td.typeName = type.name.c_str();
    auto node = mAddNode(parentNode(), &td);

    mPath.push_back((member.name.empty() ? type.name : prettyType) + ".");
    mParents.emplace_back(type.isUnion ? Parent::Union : Parent::Struct);
    parent().node = node;
    parent().size = td.sizeBits / 8;
    parent().offset = mOffset;
    return true;
}

bool NodeVisitor::visitEnum(const Member & member, const Enum & num, const std::string & prettyType)
{
    if(!mParents.empty() && parent().type == Parent::Type::Union)
    {
        mOffset = parent().offset;
        mBitOffset = 0;
    }

    std::string tname;
    auto isArrayMember = !mParents.empty() && parent().type == Parent::Type::Array;
    if(isArrayMember)
    {
        tname += '[';
        tname += std::to_string(parent().arrayIndex++);
        tname += ']';
        tname += ' ';
    }

    tname += "enum";
    if(prettyType.find("__anonymous") != 0)
    {
        tname += ' ';
        tname += prettyType;
    }

    if(!isArrayMember && member.name.find("__anonymous") != 0)
    {
        tname += ' ';
        tname += member.name;
    }

    TYPEDESCRIPTOR td = { };
    td.expanded = mParents.size() < mMaxExpandDepth;
    td.reverse = false;
    td.magic = TYPEDESCRIPTOR_MAGIC;
    td.name = tname.c_str();
    td.addr = mAddr;
    td.offset = mOffset;
    td.id = num.typeId;
    td.sizeBits = num.sizeBits;
    td.callback = cbPrintEnum;
    td.userdata = nullptr;
    td.typeName = member.type.c_str();

    mNode = mAddNode(parentNode(), &td);
    mOffset += td.sizeBits / 8;

    return true;
}

bool NodeVisitor::visitArray(const Member & member, const std::string & prettyType)
{
    // TODO: support array of arrays?

    auto typePrefix = LookupTypePrefix(member.type);
    std::string tname = typePrefix;
    if(!tname.empty())
    {
        tname += ' ';
    }
    tname += prettyType;
    tname += ' ';
    tname += member.name;
    tname += '[';
    tname += std::to_string(member.arraySize);
    tname += ']';

    TYPEDESCRIPTOR td = { };
    auto isPadding = member.name.find("padding") == 0;
    td.expanded = mParents.size() < mMaxExpandDepth && member.arraySize <= mMaxExpandArray && !isPadding;
    td.reverse = false;
    td.magic = TYPEDESCRIPTOR_MAGIC;
    td.name = tname.c_str();
    td.addr = mAddr;
    td.offset = mOffset;
    td.id = Alias;
    auto memberSizeBits = SizeofType(member.type);
    td.sizeBits = member.arraySize * memberSizeBits;
    if(*typePrefix == '\0' && (memberSizeBits == 8 || memberSizeBits == 16))
    {
        td.callback = cbPrintArray;
    }
    else
    {
        td.callback = nullptr;
    }
    td.userdata = (void*)(duint)memberSizeBits;
    td.typeName = member.type.c_str();
    auto node = mAddNode(parentNode(), &td);

    mPath.push_back(member.name + ".");
    mParents.emplace_back(Parent::Array);
    parent().node = node;
    parent().size = td.sizeBits / 8;
    return true;
}

bool NodeVisitor::visitPtr(const Member & member, const Typedef & type, const std::string & prettyType)
{
    // TODO: support array of pointers?

    auto offset = mOffset;
    auto res = visitType(member, type, prettyType); //print the pointer value
    if(mPtrDepth >= mMaxPtrDepth)
        return false;

    duint value = 0;
    if(!mAddr || !MemRead(mAddr + offset, &value, sizeof(value)))
        return false;

    mPath.push_back(prettyType + "->");
    mParents.emplace_back(Parent::Pointer);
    parent().offset = mOffset;
    parent().addr = mAddr;
    parent().node = mNode;
    parent().size = type.sizeBits / 8;
    mOffset = 0;
    mAddr = value;
    mPtrDepth++;
    return res;
}

bool NodeVisitor::visitBack(const Member & member)
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
