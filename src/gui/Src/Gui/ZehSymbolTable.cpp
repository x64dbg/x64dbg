#include "ZehSymbolTable.h"

ZehSymbolTable::ZehSymbolTable(QWidget* parent)
    : AbstractStdTable(parent)
{
    auto charwidth = getCharWidth();
    //enableMultiSelection(true); //TODO
    addColumnAt(charwidth * 2 * sizeof(dsint) + 8, tr("Address"), true);
    addColumnAt(charwidth * 6 + 8, tr("Type"), true);
    addColumnAt(charwidth * 80, tr("Symbol"), true);
    addColumnAt(2000, tr("Symbol (undecorated)"), true);
    //loadColumnFromConfig("Symbol"); //TODO
}

QString ZehSymbolTable::getCellContent(int r, int c)
{
    if(!isValidIndex(r, c))
        return QString();
    SYMBOLINFO info = {0};
    DbgGetSymbolInfo(mData.at(r), &info);
    switch(c)
    {
    case ColAddr:
        return ToPtrString(info.addr);
    case ColType:
        return info.isImported ? tr("Import") : tr("Export");
    case ColDecorated:
        return info.decoratedSymbol;
    case ColUndecorated:
        return info.undecoratedSymbol;
    default:
        return QString();
    }
}

bool ZehSymbolTable::isValidIndex(int r, int c)
{
    return r >= 0 && r < mData.size() && c >= 0 && c <= ColUndecorated;
}

void ZehSymbolTable::sortRows(int column, bool ascending)
{
    std::stable_sort(mData.begin(), mData.end(), [column, ascending](void* a, void* b)
    {
        SYMBOLINFO ainfo, binfo;
        DbgGetSymbolInfo(a, &ainfo);
        DbgGetSymbolInfo(b, &binfo);
        bool less;
        switch(column)
        {
        case ColAddr:
            less = ainfo.addr < binfo.addr;
            break;
        case ColType:
            less = ainfo.isImported < binfo.isImported;
            break;
        case ColDecorated:
            less = strcmp(ainfo.decoratedSymbol, binfo.decoratedSymbol) < 0;
            break;
        case ColUndecorated:
            less = strcmp(ainfo.undecoratedSymbol, binfo.undecoratedSymbol) < 0;
            break;
        }
        return ascending ? less : !less;
    });
}
