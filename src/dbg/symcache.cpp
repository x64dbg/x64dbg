#include "symcache.h"
#include "dbghelp_safe.h"
#include "addrinfo.h"
#include "threading.h"
#include "sortedlru.h"

template<typename T>
using RangeMap = std::map<Range, T, RangeCompare>;

static RangeMap<RangeMap<SymbolInfo>> symbolRange;
static std::unordered_map<duint, duint> symbolName;

static SortedLRU<duint, SymbolInfo> symbolCache;

bool SymbolFromAddrCached(HANDLE hProcess, duint address, SymbolInfo& symInfo)
{
	if (address == 0)
		return false;

	MODINFO* modInfo = ModInfoFromAddr(address);
	if (modInfo)
	{
		uint64_t rva = address - modInfo->base;
		if (modInfo->invalidSymbols[rva] == true)
			return false;

		auto it = symbolCache.find(address);
		if (symbolCache.acquire(it))
		{
			symInfo = (*it).second;
			return symInfo.valid;
		}

		if (modInfo->pdb.isOpen())
		{
			DiaSymbol_t diaSym;
			diaSym.disp = 0;

			if (modInfo->pdb.findSymbolExactRVA(rva, diaSym) && diaSym.disp == 0)
			{
				symInfo.addr = diaSym.virtualAddress;
				symInfo.decoratedName = diaSym.name;
				symInfo.undecoratedName = diaSym.name;
				symInfo.size = diaSym.size;
				symInfo.valid = true;

				symbolCache.insert(address, symInfo);

				return symInfo.valid;
			}
			else
			{
				// Flag as invalid.
				modInfo->invalidSymbols[rva] = true;

				return false;
			}
		}
	}

	return false;

	// Non-Module?
	auto it = symbolCache.find(address);
	if (symbolCache.acquire(it))
	{
		symInfo = (*it).second;
		return symInfo.valid;
	}

	DWORD64 disp = 0;

	char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(char)];
	PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;
	pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
	pSymbol->MaxNameLen = MAX_LABEL_SIZE;

	// For performance reasons we map invalid symbols as invalid, for now.
	if (SafeSymFromAddr(hProcess, address, &disp, pSymbol) == TRUE && disp == 0)
	{
		symInfo.addr = pSymbol->Address;
		symInfo.decoratedName = pSymbol->Name;
		symInfo.undecoratedName = pSymbol->Name;
		symInfo.size = pSymbol->Size;
		symInfo.valid = true;
	}
	else
	{
		symInfo.valid = false;
	}

	symbolCache.insert(address, symInfo);

	return symInfo.valid;
}

bool SymbolFromAddr(duint addr, SymbolInfo & symbol)
{
    SHARED_ACQUIRE(LockSymbolCache);
    auto foundR = symbolRange.find(Range(addr, addr));
    if(foundR == symbolRange.end())
        return false;
    auto foundS = foundR->second.find(Range(addr, addr));
    if(foundS == foundR->second.end())
        return false;
    symbol = foundS->second;
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
    auto foundR = symbolRange.find(Range(symbol.addr, symbol.addr));
    if(foundR == symbolRange.end())
        return false;
    auto foundS = foundR->second.find(Range(symbol.addr, symbol.addr));
    if(foundS != foundR->second.end())
        return false;
    auto dec = symbol.size ? 1 : 0;
    foundR->second.insert({ Range(symbol.addr, symbol.addr + symbol.size - dec), symbol });
    auto hash = ModHashFromName(symbol.decoratedName.c_str());
    symbolName.insert({ hash, symbol.addr });
    return true;
}

bool SymbolAddRange(duint start, duint size)
{
    EXCLUSIVE_ACQUIRE(LockSymbolCache);
    auto foundR = symbolRange.find(Range(start, start + size - 1));
    if(foundR != symbolRange.end())
        return false;
    symbolRange.insert({ Range(start, start + size - 1), RangeMap<SymbolInfo>() });
    return true;
}

bool SymbolDelRange(duint addr)
{
    EXCLUSIVE_ACQUIRE(LockSymbolCache);
    auto foundR = symbolRange.find(Range(addr, addr));
    if(foundR == symbolRange.end())
        return false;
    symbolRange.erase(foundR);
    return true;
}

static RangeMap<RangeMap<LineInfo>> lineRange;
static std::unordered_map<duint, duint> lineName;

bool LineFromAddr(duint addr, LineInfo & line)
{
    SHARED_ACQUIRE(LockLineCache);
    auto foundR = lineRange.find(Range(addr, addr));
    if(foundR == lineRange.end())
        return false;
    auto foundL = foundR->second.find(Range(addr, addr));
    if(foundL == foundR->second.end())
        return false;
    line = foundL->second;
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
    auto foundR = lineRange.find(Range(line.addr, line.addr));
    if(foundR == lineRange.end())
        return false;
    auto foundL = foundR->second.find(Range(line.addr, line.addr));
    if(foundL != foundR->second.end())
        return false;
    auto dec = line.size ? 1 : 0;
    foundR->second.insert({ Range(line.addr, line.addr + line.size - dec), line });
    auto hash = ModHashFromName(line.sourceFile.c_str()) + line.lineNumber;
    lineName.insert({ hash, line.addr });
    return true;
}

bool LineAddRange(duint start, duint size)
{
    EXCLUSIVE_ACQUIRE(LockLineCache);
    auto foundR = lineRange.find(Range(start, start + size - 1));
    if(foundR != lineRange.end())
        return false;
    lineRange.insert({ Range(start, start + size - 1), RangeMap<LineInfo>() });
    return true;
}

bool LineDelRange(duint addr)
{
    EXCLUSIVE_ACQUIRE(LockLineCache);
    auto foundR = lineRange.find(Range(addr, addr));
    if(foundR == lineRange.end())
        return false;
    lineRange.erase(foundR);
    return true;
}
