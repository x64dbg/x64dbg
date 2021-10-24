#pragma once

#include "Bridge.h"
class TraceFileReader;

int TraceFileSearchConstantRange(TraceFileReader* file, duint start, duint end);
int TraceFileSearchMemReference(TraceFileReader* file, duint address);
unsigned long long TraceFileSearchFuncReturn(TraceFileReader* file, unsigned long long start);
