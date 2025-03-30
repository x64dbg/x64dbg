#ifndef _SYMBOLSOURCEDIA_H_
#define _SYMBOLSOURCEDIA_H_

#include "_global.h"

#include "pdbdiafile.h"
#include "symbolsourcebase.h"

#include <thread>
#include <atomic>
#include <mutex>

class SymbolSourceDIA : public SymbolSourceBase
{
    struct CachedLineInfo
    {
        uint32 rva;
        uint32 lineNumber;
        uint32 sourceFileIndex;
    };

    struct ScopedDecrement
    {
    private:
        std::atomic<duint> & _counter;
    public:
        ScopedDecrement(const ScopedDecrement &) = delete;
        ScopedDecrement & operator=(const ScopedDecrement &) = delete;
        ScopedDecrement(std::atomic<duint> & counter) : _counter(counter) {}
        ~ScopedDecrement() { _counter--; }
    };

private: //symbols
    std::vector<SymbolInfo> _symData;

    struct AddrIndex
    {
        duint rva;
        size_t index;

        bool operator<(const AddrIndex & b) const
        {
            return rva < b.rva;
        }
    };

    std::vector<AddrIndex> _symAddrMap; //rva -> data index (sorted on rva)
    std::vector<NameIndex> _symNameMap; //name -> data index (sorted on name)

private: //line info
    std::vector<CachedLineInfo> _linesData;
    std::vector<AddrIndex> _lineAddrMap; //addr -> line
    std::vector<String> _sourceFiles; // uniqueId + name

    struct LineIndex
    {
        uint32_t line;
        uint32_t index;

        bool operator<(const LineIndex & b) const
        {
            return line < b.line;
        }
    };

    std::vector<std::vector<LineIndex>> _sourceLines; //uses index in _sourceFiles

private: //general
    HANDLE _symbolsThread = nullptr;
    HANDLE _sourceLinesThread = nullptr;
    std::atomic<bool> _requiresShutdown;
    std::atomic<duint> _loadCounter;

    bool _isOpen;
    std::string _path;
    std::string _modname;
    duint _imageBase;
    duint _imageSize;
    std::atomic<bool> _symbolsLoaded;
    std::atomic<bool> _linesLoaded;

private:
    static int hackicmp(const char* s1, const char* s2)
    {
        unsigned char c1, c2;
        while((c1 = *s1++) == (c2 = *s2++))
            if(c1 == '\0')
                return 0;
        s1--, s2--;
        while((c1 = StringUtils::ToLower(*s1++)) == (c2 = StringUtils::ToLower(*s2++)))
            if(c1 == '\0')
                return 0;
        return c1 - c2;
    }

public:
    static bool isLibraryAvailable()
    {
        return PDBDiaFile::initLibrary();
    }

public:
    SymbolSourceDIA();

    virtual ~SymbolSourceDIA() override;

    virtual bool isOpen() const override;

    virtual bool isLoading() const override;

    virtual bool cancelLoading() override;

    virtual void waitUntilLoaded() override;

    virtual bool findSymbolExact(duint rva, SymbolInfo & symInfo) override;

    virtual bool findSymbolExactOrLower(duint rva, SymbolInfo & symInfo) override;

    virtual void enumSymbols(const CbEnumSymbol & cbEnum, duint beginRva, duint endRva) override;

    virtual bool findSourceLineInfo(duint rva, LineInfo & lineInfo) override;

    virtual bool findSourceLineInfo(const std::string & file, int line, LineInfo & lineInfo) override;

    virtual bool findSymbolByName(const std::string & name, SymbolInfo & symInfo, bool caseSensitive) override;

    virtual bool findSymbolsByPrefix(const std::string & prefix, const std::function<bool(const SymbolInfo &)> & cbSymbol, bool caseSensitive) override;

    virtual std::string loadedSymbolPath() const override;

public:
    bool loadPDB(const std::string & path, const std::string & modname, duint imageBase, duint imageSize, DiaValidationData_t* validationData);

private:
    bool loadSymbolsAsync();
    bool loadSourceLinesAsync();
    uint32_t findSourceFile(const std::string & fileName) const;

    static DWORD WINAPI SymbolsThread(void* parameter);
    static DWORD WINAPI SourceLinesThread(void* parameter);
};

#endif // _SYMBOLSOURCEPDB_H_
