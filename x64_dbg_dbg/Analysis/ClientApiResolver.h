#pragma once
#include "../_global.h"
#include "Meta.h"
#include "ClientInterface.h"


namespace fa
{
	class ClientApiResolver : public ClientInterface{
	public:

		void see(const Instruction_t Instr,const RegisterEmulator *reg,const StackEmulator *stack);

	};

};