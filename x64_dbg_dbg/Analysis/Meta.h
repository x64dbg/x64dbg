#pragma once
#include "../BeaEngine/BeaEngine.h"
#include "../_global.h"
#include <vector>
#include <string>
#include <map>
#include <set>
#include <cstring>





namespace fa
{

	enum EdgeType{RET,CALL,EXTERNCALL,CONDJMP,UNCONDJMP,EXTERNJMP,INF,UNKOWN};

	

	// every edge in the application flow graph is an instruction that active modifies the EIP

	typedef struct Instruction_t
	{
		DISASM BeaStruct;
		unsigned int Length;

		Instruction_t(DISASM* dis, unsigned int len)
		{
			BeaStruct = *dis;
			Length = len;
		}

		Instruction_t()
		{
			BeaStruct = DISASM();
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

	struct FunctionInfo_t
	{
		std::string DLLName;
		std::string ReturnType;
		std::string Name;
		std::vector<ArgumentInfo_t> Arguments;
		bool invalid;

		FunctionInfo_t()
		{
			invalid = false;
		}

		FunctionInfo_t(std::string dll, std::string ret, std::string name, std::vector<ArgumentInfo_t> args)
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

	typedef std::map<UInt64, Instruction_t>::const_iterator instrIter;



#ifdef _WIN64
#define REGISTER_SIZE 8
#else
#define REGISTER_SIZE 4
#endif

};
