#pragma once
#include "../_global.h"
#include <map>
#include "Meta.h"
#include <set>
#include "ClientInterface.h"
#include "FunctionDB.h"

namespace fa
{

// BeaEngine uses "duint" as default size type. We will do the same here in contrast to "duint"

class StackEmulator;
class RegisterEmulator;
class FunctionInfo;
class FlowGraph;

typedef std::set<unknownRegion> UnkownRegionSet;
typedef std::map<duint, Instruction_t> InstructionMap;
typedef std::vector<ClientInterface*> ClientInterfaceList;


class AnalysisRunner
{
    // we will place all VA here that should be a start address for disassembling
    UnkownRegionSet explorationSpace;
    // all known disassemling should be cached
    InstructionMap instructionsCache;
    // baseaddress for current thread
    duint baseAddress;
    // size of code for security while disassembling
    duint codeSize;
    // copy of all instructions bytes
    unsigned char* codeBuffer;
    // temporal value of EIP
    UIntPtr currentEIP;
    // temporal value of virtual address
    duint currentVirtualAddr;
    // information about the CIP
    const duint OEP;
    // whole application as a graph
    FlowGraph* Grph;
    // flag for correct initialisation of the code memory
    bool codeWasCopied;

    FunctionDB* DB;
    ClientInterfaceList interfaces;


    StackEmulator* Stack;
    RegisterEmulator* Register;
    FunctionInfo* functionInfo;

protected:
    bool initialise();

private:
    void emulateInstructions();

    void explore();
    bool explore(const unknownRegion region);

public:

    AnalysisRunner(const duint CIP, const duint BaseAddress, const duint Size, FunctionDB* db);
    ~AnalysisRunner(void);

    void start();
    const fa::Instruction_t* instruction(duint va) const;


    duint base() const;
    duint oep() const;
    duint size() const;
    FlowGraph* graph() const;
    FunctionDB* functionDB();

};

};