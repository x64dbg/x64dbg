#include "ZehSymbolTable.h"
#include "Bridge.h"
#include "RichTextPainter.h"

class SymbolInfoWrapper
{
    SYMBOLINFO info;

public:
    SymbolInfoWrapper()
    {
        memset(&info, 0, sizeof(SYMBOLINFO));
    }

    ~SymbolInfoWrapper()
    {
        if(info.freeDecorated)
            BridgeFree(info.decoratedSymbol);
        if(info.freeUndecorated)
            BridgeFree(info.undecoratedSymbol);
    }

    SYMBOLINFO* operator&() { return &info; }
    SYMBOLINFO* operator->() { return &info; }
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
    addColumnAt(charwidth * 80, tr("Symbol"), true);
    addColumnAt(2000, tr("Symbol (undecorated)"), true);
    loadColumnFromConfig("Symbol");
    updateColors();

    trImport = tr("Import");
    trExport = tr("Export");
    trSymbol = tr("Symbol");
}

QString ZehSymbolTable::getCellContent(int r, int c)
{
    QMutexLocker lock(&mMutex);
    if(!isValidIndex(r, c))
        return QString();
    SymbolInfoWrapper info;
    DbgGetSymbolInfo(&mData.at(r), &info);
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
    case ColDecorated:
        return info->decoratedSymbol;
    case ColUndecorated:
        return info->undecoratedSymbol;
    default:
        return QString();
    }
}

bool ZehSymbolTable::isValidIndex(int r, int c)
{
    QMutexLocker lock(&mMutex);
    return r >= 0 && r < mData.size() && c >= 0 && c <= ColUndecorated;
}

void ZehSymbolTable::sortRows(int column, bool ascending)
{
    QMutexLocker lock(&mMutex);
    //TODO: invalid compare when !ascending
    std::stable_sort(mData.begin(), mData.end(), [column, ascending](const SYMBOLPTR & a, const SYMBOLPTR & b)
    {
        SymbolInfoWrapper ainfo, binfo;
        DbgGetSymbolInfo(&a, &ainfo);
        DbgGetSymbolInfo(&b, &binfo);
        bool less;
        switch(column)
        {
        case ColAddr:
            less = ainfo->addr < binfo->addr;
            break;
        case ColType:
            less = ainfo->type < binfo->type;
            break;
        case ColDecorated:
            less = strcmp(ainfo->decoratedSymbol, binfo->decoratedSymbol) < 0;
            break;
        case ColUndecorated:
            less = strcmp(ainfo->undecoratedSymbol, binfo->undecoratedSymbol) < 0;
            break;
        }
        return ascending ? less : !less;
    });
}
