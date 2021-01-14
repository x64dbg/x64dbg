#ifndef TRACEFILESEARCH_H
#define TRACEFILESEARCH_H
#include "Bridge.h"
class TraceFileReader;

int TraceFileSearchConstantRange(TraceFileReader* file, duint start, duint end);
int TraceFileSearchMemReference(TraceFileReader* file, duint address);
unsigned long long TraceFileSearchFuncReturn(TraceFileReader* file, unsigned long long start);
#endif //TRACEFILESEARCH_H
