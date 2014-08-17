#pragma once

#include "ICommand.h"
#include <list>
#include <set>
#include <map>

#include "Meta.h"
#include "StackEmulator.h"
#include "RegisterEmulator.h"

namespace tr4ce
{

	class CallDetector : public ICommand
	{
		

		unsigned int numberOfFunctions;
		std::set<Call_t> mCalls;

	public:

		CallDetector(AnalysisRunner* parent);

		void clear();
		void see(const Instruction_t* disasm, const StackEmulator* stack, const RegisterEmulator* regState);
		bool think();
		void unknownOpCode(const DISASM* disasm);
	};

};