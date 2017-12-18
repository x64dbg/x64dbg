#pragma once

#include "PDBDiaTypes.h"

#include <vector>
#include <map>
#include <string>
#include <set>
#include <unordered_set>
#include <unordered_map>

struct IDiaDataSource;
struct IDiaSession;
struct IDiaSymbol;

class PDBDiaFile
{
private:
    static volatile long m_sbInitialized;

private:
    IDiaDataSource* m_dataSource;
    IDiaSession* m_session;

public:
    PDBDiaFile();
    ~PDBDiaFile();

    static bool initLibrary();

    static bool shutdownLibrary();

    bool open(const char* file, uint64_t loadAddress = 0, DiaValidationData_t* validationData = nullptr);

    bool open(const wchar_t* file, uint64_t loadAddress = 0, DiaValidationData_t* validationData = nullptr);

    bool isOpen() const;

    bool close();

    bool getFunctionLineNumbers(DWORD rva, ULONGLONG size, uint64_t imageBase, std::map<uint64_t, DiaLineInfo_t> & lines);

    bool enumerateLexicalHierarchy(std::function<bool(DiaSymbol_t &)> callback, const bool collectUndecoratedNames);

    bool findSymbolRVA(uint64_t address, DiaSymbol_t & sym, DiaSymbolType symType = DiaSymbolType::ANY);

private:
    bool testError(HRESULT hr);

    std::string getSymbolNameString(IDiaSymbol* sym);
    std::string getSymbolUndecoratedNameString(IDiaSymbol* sym);

    bool enumerateCompilandScope(IDiaSymbol* compiland, std::function<bool(DiaSymbol_t &)> & callback, std::unordered_set<uint32_t> & visited, const bool collectUndecoratedNames);
    bool processFunctionSymbol(IDiaSymbol* profilerFunction, std::function<bool(DiaSymbol_t &)> & callback, std::unordered_set<uint32_t> & visited, const bool collectUndecoratedNames);

    bool resolveSymbolSize(IDiaSymbol* symbol, uint64_t & size, uint32_t symTag);
    bool convertSymbolInfo(IDiaSymbol* symbol, DiaSymbol_t & symbolInfo, const bool collectUndecoratedNames);
};

