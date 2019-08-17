#ifndef _SYMBOLSOURCEBASE_H_
#define _SYMBOLSOURCEBASE_H_

#include "_global.h"

#include <stdint.h>
#include <vector>
#include <functional>
#include <map>
#include <algorithm>

struct SymbolInfoGui
{
    virtual void convertToGuiSymbol(duint base, SYMBOLINFO* info) const = 0;
};

struct SymbolInfo : SymbolInfoGui
{
    duint rva = 0;
    duint size = 0;
    int32 disp = 0;
    String decoratedName;
    String undecoratedName;
    bool publicSymbol = false;

    void convertToGuiSymbol(duint modbase, SYMBOLINFO* info) const override
    {
        info->addr = modbase + this->rva;
        info->decoratedSymbol = (char*)this->decoratedName.c_str();
        info->undecoratedSymbol = (char*)this->undecoratedName.c_str();
        info->type = sym_symbol;
        info->freeDecorated = info->freeUndecorated = false;
        info->ordinal = 0;
    }
};

struct LineInfo
{
    duint rva = 0;
    duint size = 0;
    duint disp = 0;
    int lineNumber = 0;
    String sourceFile;
};

struct NameIndex
{
    const char* name;
    size_t index;

    bool operator<(const NameIndex & b) const
    {
        return cmp(*this, b, false) < 0;
    }

    static int cmp(const NameIndex & a, const NameIndex & b, bool caseSensitive)
    {
        return (caseSensitive ? strcmp : StringUtils::hackicmp)(a.name, b.name);
    }

    static bool findByPrefix(const std::vector<NameIndex> & byName, const std::string & prefix, const std::function<bool(const NameIndex &)> & cbFound, bool caseSensitive);
    static bool findByName(const std::vector<NameIndex> & byName, const std::string & name, NameIndex & foundIndex, bool caseSensitive);
};

using CbEnumSymbol = std::function<bool(const SymbolInfo &)>;

class SymbolSourceBase
{
private:
    std::vector<uint8_t> _symbolBitmap; // TODO: what is the maximum size for this?
    std::map<std::string, std::string> _sourceFileMapPdbToDisk; // pdb source path -> disk source path
    std::map<std::string, std::string> _sourceFileMapDiskToPdb; // disk source path -> pdb source path

public:
    virtual ~SymbolSourceBase() = default;

    void resizeSymbolBitmap(size_t imageSize)
    {
        if(!isOpen())
            return;

        _symbolBitmap.resize(imageSize);
        std::fill(_symbolBitmap.begin(), _symbolBitmap.end(), false);
    }

    void markAdressInvalid(duint rva)
    {
        if(_symbolBitmap.empty())
            return;

        _symbolBitmap[rva] = true;
    }

    bool isAddressInvalid(duint rva) const
    {
        if(_symbolBitmap.empty())
            return false;

        return !!_symbolBitmap[rva];
    }

    // Tells us if the symbols are loaded for this module.
    virtual bool isOpen() const
    {
        return false; // Stub
    }

    virtual bool isLoading() const
    {
        return false; // Stub
    }

    virtual bool cancelLoading()
    {
        return false;
    }

    // Get the symbol at the specified address, will return false if not found.
    virtual bool findSymbolExact(duint rva, SymbolInfo & symInfo)
    {
        return false; // Stub
    }

    // Get the symbol at the address or the closest behind, in case it got the closest it will set disp to non-zero, false on nothing.
    virtual bool findSymbolExactOrLower(duint rva, SymbolInfo & symInfo)
    {
        return false; // Stub
    }

    // only call if isOpen && !isLoading
    virtual void enumSymbols(const CbEnumSymbol & cbEnum)
    {
        // Stub
    }

    virtual bool findSourceLineInfo(duint rva, LineInfo & lineInfo)
    {
        return false; // Stub
    }

    virtual bool findSourceLineInfo(const std::string & file, int line, LineInfo & lineInfo)
    {
        return false; // Stub
    }

    virtual bool findSymbolByName(const std::string & name, SymbolInfo & symInfo, bool caseSensitive)
    {
        return false; // Stub
    }

    virtual bool findSymbolsByPrefix(const std::string & prefix, const std::function<bool(const SymbolInfo &)> & cbSymbol, bool caseSensitive)
    {
        return false; // Stub
    }

    virtual std::string loadedSymbolPath() const
    {
        return ""; // Stub
    }

    bool mapSourceFilePdbToDisk(const std::string & pdb, const std::string & disk);
    bool getSourceFilePdbToDisk(const std::string & pdb, std::string & disk) const;
    bool getSourceFileDiskToPdb(const std::string & disk, std::string & pdb) const;
};

static SymbolSourceBase EmptySymbolSource;

#endif // _SYMBOLSOURCEBASE_H_
