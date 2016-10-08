#pragma once

#include "_global.h"

struct SymbolInfo
{
    duint addr;
    duint size;
    String decoratedName;
    String undecoratedName;
};

struct LineInfo
{
    duint addr;
    duint size;
    int lineNumber;
    String sourceFile;
};

bool SymbolFromAddr(duint addr, SymbolInfo & symbol);
bool SymbolFromName(const char* name, SymbolInfo & symbol);
bool SymbolAdd(const SymbolInfo & symbol);
void SymbolDelRange(duint start, duint end);

bool LineFromAddr(duint addr, LineInfo & line);
bool LineFromName(const char* sourceFile, int lineNumber, LineInfo & line);
bool LineAdd(const LineInfo & line);
void LineDelRange(duint start, duint end);