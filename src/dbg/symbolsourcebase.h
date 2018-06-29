#ifndef _SYMBOLSOURCEBASE_H_
#define _SYMBOLSOURCEBASE_H_

#include "_global.h"

#include <stdint.h>
#include <vector>
#include <functional>

struct SymbolInfoGui
{
    virtual void convertToGuiSymbol(duint base, SYMBOLINFO* info) const = 0;
};

struct SymbolInfo : SymbolInfoGui
{
    duint rva;
    duint size;
    int32 disp;
    String decoratedName;
    String undecoratedName;
    bool publicSymbol;

    void convertToGuiSymbol(duint modbase, SYMBOLINFO* info) const override
    {
        info->addr = modbase + this->rva;
        info->decoratedSymbol = (char*)this->decoratedName.c_str();
        info->undecoratedSymbol = (char*)this->undecoratedName.c_str();
        info->type = sym_symbol;
        info->freeDecorated = info->freeUndecorated = false;
    }
};

struct LineInfo
{
    duint rva;
    duint size;
    duint disp;
    int lineNumber;
    String sourceFile;
};

using CbEnumSymbol = std::function<bool(const SymbolInfo &)>;

class SymbolSourceBase
{
private:
    std::vector<uint8_t> _symbolBitmap; // TODO: what is the maximum size for this?

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
};

static SymbolSourceBase EmptySymbolSource;

#endif // _SYMBOLSOURCEBASE_H_
