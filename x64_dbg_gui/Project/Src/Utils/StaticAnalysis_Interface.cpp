#include "StaticAnalysis_Interface.h"

StaticAnalysis_Interface::StaticAnalysis_Interface(StaticAnalysis *parent) : mParent(parent)
{
}

void StaticAnalysis_Interface::initialise(const int_t Base, const int_t Size)
{
    mBase = Base;
    mSize = Size;
}
