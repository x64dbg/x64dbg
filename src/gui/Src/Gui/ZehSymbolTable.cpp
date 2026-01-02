#include "ZehSymbolTable.h"
#include "Bridge.h"
#include "RichTextPainter.h"

class SymbolInfoWrapper
{
    void free()
    {
        if(info.freeDecorated)
            BridgeFree(info.decoratedSymbol);
        if(info.freeUndecorated)
            BridgeFree(info.undecoratedSymbol);
    }

    SYMBOLINFO info{};
    bool cached = false;

public:
    SymbolInfoWrapper() = default;
    ~SymbolInfoWrapper() { free(); }

    SymbolInfoWrapper(const SymbolInfoWrapper &) = delete;
    SymbolInfoWrapper & operator=(const SymbolInfoWrapper &) = delete;

    SYMBOLINFO* put()
    {
        free();
        memset(&info, 0, sizeof(info));
        cached = false;
        return &info;
    }

    SYMBOLINFO* get() 
    { 
        if(!cached)
        {
            cached = true;
        }
        return &info; 
    }
    
    const SYMBOLINFO* get() const 
    { 
        return &info; 
    }

    SYMBOLINFO* operator->() { return get(); }
    const SYMBOLINFO* operator->() const { return get(); }
    
    bool isCached() const { return cached; }
};

ZehSymbolTable::ZehSymbolTable(QWidget* parent)
    : AbstractStdTable(parent),
      mMutex(QMutex::Recursive)
{
    auto charwidth = getCharWidth();
    enableMultiSelection(true);
    setAddressColumn(0);
    addColumnAt(charwidth * 2 * sizeof(dsint) + 8, tr("Address"), true);
    addColumnAt(charwidth * 6 + 8, tr("Type"), true);
    addColumnAt(charwidth * 7 + 8, tr("Ordinal"), true);
    addColumnAt(charwidth * 80, tr("Symbol"), true);
    addColumnAt(2000, tr("Symbol (undecorated)"), true);
    loadColumnFromConfig("Symbol");

    trImport = tr("Import");
    trExport = tr("Export");
    trSymbol = tr("Symbol");

    Initialize();
}

QString ZehSymbolTable::getCellContent(duint row, duint column)
{
    QMutexLocker lock(&mMutex);
    if(!isValidIndex(row, column))
        return QString();
        
    // Use cached symbol info if available, otherwise fetch and cache it
    if(row >= mSymbolCache.size() || mSymbolCache[row].addr == 0)
    {
        ensureCacheSize(row + 1);
        SymbolInfoWrapper info;
        DbgGetSymbolInfo(&mData.at(row), info.put());
        mSymbolCache[row] = *info.get();
    }
    
    return symbolInfoString(&mSymbolCache[row], column);
}

duint ZehSymbolTable::getCellUserdata(duint row, duint column)
{
    QMutexLocker lock(&mMutex);
    if(!isValidIndex(row, column))
        return 0;
        
    // Use cached symbol info if available, otherwise fetch and cache it
    if(row >= mSymbolCache.size() || mSymbolCache[row].addr == 0)
    {
        ensureCacheSize(row + 1);
        SymbolInfoWrapper info;
        DbgGetSymbolInfo(&mData.at(row), info.put());
        mSymbolCache[row] = *info.get();
    }
    
    switch(column)
    {
    case ColAddr:
        return mSymbolCache[row].addr;
    case ColOrdinal:
        return mSymbolCache[row].ordinal;
    case ColType:
        return mSymbolCache[row].type;
    default:
        return 0;
    }
}

bool ZehSymbolTable::isValidIndex(duint row, duint column)
{
    QMutexLocker lock(&mMutex);
    return row >= 0 && row < (int)mData.size() && column >= 0 && column <= ColUndecorated;
}

void ZehSymbolTable::sortRows(duint column, bool ascending)
{
    QMutexLocker lock(&mMutex);
    
    // Clear cache before sorting to maintain consistency
    mSymbolCache.clear();
    
    std::stable_sort(mData.begin(), mData.end(), [this, column, ascending](const SYMBOLPTR & a, const SYMBOLPTR & b)
    {
        SymbolInfoWrapper ainfo, binfo;
        DbgGetSymbolInfo(&a, ainfo.put());
        DbgGetSymbolInfo(&b, binfo.put());
        switch(column)
        {
        case ColAddr:
            return ascending ? ainfo->addr < binfo->addr : ainfo->addr > binfo->addr;

        case ColType:
            return ascending ? ainfo->type < binfo->type : ainfo->type > binfo->type;

        case ColOrdinal:
            // If we are sorting by ordinal make the exports the first entries
            if(ainfo->type == sym_export && binfo->type != sym_export)
                return ascending;
            else if(ainfo->type != sym_export && binfo->type == sym_export)
                return !ascending;
            else
                return ascending ? ainfo->ordinal < binfo->ordinal : ainfo->ordinal > binfo->ordinal;

        case ColDecorated:
        {
            auto acell = symbolInfoString(ainfo.get(), ColDecorated);
            auto bcell = symbolInfoString(binfo.get(), ColDecorated);
            int result = QString::compare(acell, bcell);
            return ascending ? result < 0 : result > 0;
        }

        case ColUndecorated:
        {
            auto acell = symbolInfoString(ainfo.get(), ColUndecorated);
            auto bcell = symbolInfoString(binfo.get(), ColUndecorated);
            int result = QString::compare(acell, bcell);
            return ascending ? result < 0 : result > 0;
        }

        default:
            return false;
        }
    });
}

QString ZehSymbolTable::symbolInfoString(const SYMBOLINFO* info, duint c)
{
    switch(c)
    {
    case ColAddr:
        return ToPtrString(info->addr);

    case ColType:
        switch(info->type)
        {
        case sym_import:
            return trImport;
        case sym_export:
            return trExport;
        case sym_symbol:
            return trSymbol;
        default:
            __debugbreak();
        }

    case ColOrdinal:
        if(info->type == sym_export)
            return QString::number(info->ordinal);
        else
            return QString();

    case ColDecorated:
    {
        char modname[MAX_MODULE_SIZE];
        // Get module name for import symbols
        if(info->type == sym_import)
        {
            duint va = 0;
            if(DbgMemRead(info->addr, &va, sizeof(duint)))
                if(DbgGetModuleAt(va, modname))
                    return QString(modname).append('.').append(info->decoratedSymbol);
        }
        return info->decoratedSymbol;
    }

    case ColUndecorated:
    {
        if(*info->undecoratedSymbol == '\0' && strstr(info->decoratedSymbol, "Ordinal") == info->decoratedSymbol)
        {
            char label[MAX_LABEL_SIZE] = "";
            switch(info->type)
            {
            case sym_import:
            {
                duint va = 0;
                if(DbgMemRead(info->addr, &va, sizeof(duint)))
                {
                    DbgGetLabelAt(va, SEG_DEFAULT, label);
                    return label;
                }
            }
            break;

            case sym_export:
            {
                DbgGetLabelAt(info->addr, SEG_DEFAULT, label);
                return label;
            }
            break;

            default:
                break;
            }
        }
        return info->undecoratedSymbol;
    }

    default:
        return QString();
    }
}

void ZehSymbolTable::ensureCacheSize(size_t size)
{
    QMutexLocker lock(&mMutex);
    if(mSymbolCache.size() < size)
    {
        mSymbolCache.resize(size);
        // Initialize new cache entries with zeroed memory
        for(size_t i = mSymbolCache.size(); i < size; ++i)
        {
            memset(&mSymbolCache[i], 0, sizeof(SYMBOLINFO));
        }
    }
}
