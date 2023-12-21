#pragma once

#include "AbstractStdTable.h"
#include <QMutex>

class ZehSymbolTable : public AbstractStdTable
{
    Q_OBJECT
public:
    ZehSymbolTable(QWidget* parent = nullptr);

    QString getCellContent(duint row, duint column) override;
    duint getCellUserdata(duint row, duint column) override;
    bool isValidIndex(duint row, duint column) override;
    void sortRows(duint column, bool ascending) override;

    friend class SymbolView;
    friend class SearchListViewSymbols;
    friend class SymbolSearchList;

private:
    std::vector<duint> mModules;
    std::vector<SYMBOLPTR> mData;
    QMutex mMutex;

    //Caching of translations to fix a bottleneck
    QString trImport;
    QString trExport;
    QString trSymbol;

    enum
    {
        ColAddr,
        ColType,
        ColOrdinal,
        ColDecorated,
        ColUndecorated
    };

    QString symbolInfoString(const SYMBOLINFO* info, duint c);
};
