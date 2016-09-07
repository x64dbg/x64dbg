#include "symcache.h"
#include "addrinfo.h"
#include "threading.h"

static std::map<Range, SymbolInfo, RangeCompare> symbolRange;
static std::unordered_map<duint, duint> symbolName;

bool SymbolFromAddr(duint addr, SymbolInfo & symbol)
{
    SHARED_ACQUIRE(LockSymbolCache);
    auto found = symbolRange.find(Range(addr, addr));
    if(found == symbolRange.end())
        return false;
    symbol = found->second;
    return true;
}

bool SymbolFromName(const char* name, SymbolInfo & symbol)
{
    if(!name)
        return false;
    auto hash = ModHashFromName(name);
    SHARED_ACQUIRE(LockSymbolCache);
    auto found = symbolName.find(hash);
    if(found == symbolName.end())
        return false;
    return SymbolFromAddr(found->second, symbol);
}

bool SymbolAdd(const SymbolInfo & symbol)
{
    EXCLUSIVE_ACQUIRE(LockSymbolCache);
    auto found = symbolRange.find(Range(symbol.addr, symbol.addr));
    if(found != symbolRange.end())
        return false;
    auto dec = symbol.size ? 1 : 0;
    symbolRange.insert({ Range(symbol.addr, symbol.addr + symbol.size - dec), symbol });
    auto hash = ModHashFromName(symbol.decoratedName.c_str());
    symbolName.insert({ hash, symbol.addr });
    return true;
}

void SymbolDelRange(duint start, duint end)
{
}

static std::map<Range, LineInfo, RangeCompare> lineRange;
static std::unordered_map<duint, duint> lineName;

bool LineFromAddr(duint addr, LineInfo & line)
{
    SHARED_ACQUIRE(LockLineCache);
    auto found = lineRange.find(Range(addr, addr));
    if(found == lineRange.end())
        return false;
    line = found->second;
    return true;
}

bool LineFromName(const char* sourceFile, int lineNumber, LineInfo & line)
{
    if(!sourceFile)
        return false;
    auto hash = ModHashFromName(sourceFile) + lineNumber;
    SHARED_ACQUIRE(LockLineCache);
    auto found = lineName.find(hash);
    if(found == lineName.end())
        return false;
    return LineFromAddr(found->second, line);
}

bool LineAdd(const LineInfo & line)
{
    EXCLUSIVE_ACQUIRE(LockLineCache);
    auto found = lineRange.find(Range(line.addr, line.addr));
    if(found != lineRange.end())
        return false;
    auto dec = line.addr ? 1 : 0;
    lineRange.insert({ Range(line.addr, line.addr + line.size - dec), line });
    auto hash = ModHashFromName(line.sourceFile.c_str()) + line.lineNumber;
    lineName.insert({ hash, line.addr });
    return true;
}

void LineDelRange(duint start, duint end)
{
}
