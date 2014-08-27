#pragma once
#include "../BeaEngine/BeaEngine.h"
#include "../_global.h"
#include "../disasm_fast.h"
#include <vector>
#include <string>
#include <map>
#include <set>
#include <cstring>

#define DEBUG_PRINT_ENABLED

#ifdef DEBUG_PRINT_ENABLED
#define tDebug dprintf
#else
#define tDebug ((void)0)
#endif







namespace fa
{

enum EdgeType {RET, CALL, CONDJMP, UNCONDJMP, EXTERNJMP, INF, UNKOWN};




typedef struct unknownRegion
{
    duint startAddress;
    duint headerAddress;


    bool operator==(const unknownRegion & rhs) const
    {
        return static_cast<bool>(startAddress == rhs.startAddress);
    }
    bool operator<(const unknownRegion & rhs) const
    {
        return static_cast<bool>(startAddress < rhs.startAddress);
    }
} unknownRegion ;

// every edge in the application flow graph is an instruction that active modifies the EIP

typedef struct Instruction_t
{
    mutable DISASM BeaStruct;
    mutable BASIC_INSTRUCTION_INFO BasicInfo;
    unsigned int Length;

    Instruction_t(DISASM* dis, unsigned int len)
    {
        BeaStruct = *dis;
        fillbasicinfo(dis, &BasicInfo);
        BasicInfo.size = len;
        Length = len;
    }

    Instruction_t()
    {
        Length = UNKNOWN_OPCODE;
    }
} Instruction_t;

struct ArgumentInfo_t
{
    std::string Type;
    std::string Name;

    ArgumentInfo_t(std::string t, std::string n)
    {
        Type = t;
        Name = n;
    }

    ArgumentInfo_t() {}
};

typedef std::vector<ArgumentInfo_t> ArgumentInfoList;

struct FunctionInfo_t
{
    std::string DLLName;
    std::string ReturnType;
    std::string Name;
    ArgumentInfoList Arguments;
    bool invalid;

    FunctionInfo_t()
    {
        invalid = false;
    }

    FunctionInfo_t(std::string dll, std::string ret, std::string name, ArgumentInfoList args)
    {
        DLLName = dll;
        ReturnType = ret;
        Name = name;
        Arguments = args;
        invalid = false;
    }
    // !! suppresses the warning (ugly solution)
    bool operator==(const FunctionInfo_t & rhs) const
    {
        return static_cast<bool>(!!(_strcmpi(Name.c_str(), rhs.Name.c_str()) < 0));
    }
    // !! suppresses the warning (ugly solution)
    bool operator<(const FunctionInfo_t & rhs) const
    {
        return static_cast<bool>(!!_strcmpi(Name.c_str(), rhs.Name.c_str()));
    }

    ArgumentInfo_t arg(int i)
    {
        return Arguments.at(i);
    }


};

template<typename T, typename D>
bool contains(std::map<T, D> s, T key)
{
    std::map<T, D>::iterator it = s.find(key);
    return (it != s.end());
}






#ifdef _WIN64
#define REGISTER_SIZE 8
#else
#define REGISTER_SIZE 4
#endif

};
