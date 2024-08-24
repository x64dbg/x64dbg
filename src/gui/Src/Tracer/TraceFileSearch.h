#pragma once

#include "Bridge.h"
#include "TraceFileReader.h"

int TraceFileSearchConstantRange(TraceFileReader* file, duint start, duint end);
int TraceFileSearchMemReference(TraceFileReader* file, duint address);
TRACEINDEX TraceFileSearchFuncReturn(TraceFileReader* file, TRACEINDEX start);
int TraceFileSearchMemPattern(TraceFileReader* file, const QString & pattern);
