#include "ICommand.h"
#include "AnalysisRunner.h"

namespace tr4ce
{

ICommand::ICommand(AnalysisRunner* parent) : mParent(parent)
{
}


ICommand::~ICommand(void)
{
}

void ICommand::initialise(const uint Base, const uint Size)
{
    mBase = Base;
    mSize = Size;
}



};