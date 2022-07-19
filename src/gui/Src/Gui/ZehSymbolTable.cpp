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
    addColumnAt(charwidth * 7 + 8, tr("Ordinal"), true);
    addColumnAt(charwidth * 80, tr("Symbol"), true);
    addColumnAt(2000, tr("Symbol (undecorated)"), true);
    loadColumnFromConfig("Symbol");

    trImport = tr("Import");
    trExport = tr("Export");
    trSymbol = tr("Symbol");

    Initialize();
}

QString ZehSymbolTable::getCellContent(int r, int c)
{
    QMutexLocker lock(&mMutex);
    if(!isValidIndex(r, c))
        return QString();
    SymbolInfoWrapper info;
    DbgGetSymbolInfo(&mData.at(r), &info);
    return symbolInfoString(&info, c);
}

bool ZehSymbolTable::isValidIndex(int r, int c)
{
    QMutexLocker lock(&mMutex);
    return r >= 0 && r < (int)mData.size() && c >= 0 && c <= ColUndecorated;
}

void ZehSymbolTable::sortRows(int column, bool ascending)
{
    QMutexLocker lock(&mMutex);
    std::stable_sort(mData.begin(), mData.end(), [this, column, ascending](const SYMBOLPTR & a, const SYMBOLPTR & b)
    {
        SymbolInfoWrapper ainfo, binfo;
        DbgGetSymbolInfo(&a, &ainfo);
        DbgGetSymbolInfo(&b, &binfo);
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
            auto acell = symbolInfoString(&ainfo, ColDecorated);
            auto bcell = symbolInfoString(&binfo, ColDecorated);
            int result = QString::compare(acell, bcell);
            return ascending ? result < 0 : result > 0;
        }

        case ColUndecorated:
        {
            auto acell = symbolInfoString(&ainfo, ColUndecorated);
            auto bcell = symbolInfoString(&binfo, ColUndecorated);
            int result = QString::compare(acell, bcell);
            return ascending ? result < 0 : result > 0;
        }

        default:
            return false;
        }
    });
}

QString ZehSymbolTable::symbolInfoString(const SYMBOLINFO* info, int c)
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
            duint wVA;
            if(DbgMemRead(info->addr, &wVA, sizeof(duint)))
                if(DbgGetModuleAt(wVA, modname))
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
                duint wVA;
                if(DbgMemRead(info->addr, &wVA, sizeof(duint)))
                {
                    DbgGetLabelAt(wVA, SEG_DEFAULT, label);
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
