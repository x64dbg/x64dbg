#pragma once
#include "../_global.h"
#include <map>
#include "Meta.h"
#include "StackEmulator.h"
#include "RegisterEmulator.h"
#include "ApiDB.h"
#include "FlowGraph.h"
#include <set>

namespace fa
{


class AnalysisRunner
{
	// we will place all VA here that should be a start address for disassembling
	std::set<UInt64> disasmRoot;
	// all known disassemling should be cached
    std::map<UInt64, Instruction_t> instructionBuffer;
	// baseaddress for current thread
    duint baseAddress;
	// size of code for security while disassembling
    duint codeSize;
	// copy of all instructions bytes
    unsigned char* codeBuffer;
	// temporal value of EIP
    UIntPtr currentEIP;
	// temporal value of virtual address
    UInt64 currentVirtualAddr;
	// information about the CIP
	const UInt64 OEP;
	// whole application as a graph
	FlowGraph *Grph;
	// flag for correct initialisation of the code memory
	bool codeWasCopied;

protected:
    bool initialise();

private:
    void buildGraph();
	bool disasmChilds(duint addr);

public:

    AnalysisRunner(const duint CIP,const duint BaseAddress,const duint Size);
    ~AnalysisRunner(void);

    void start();
	std::map<UInt64, Instruction_t>::const_iterator instruction(UInt64 va) const;
	std::map<UInt64, Instruction_t>::const_iterator lastInstruction() const;

	duint base() const;


};

};