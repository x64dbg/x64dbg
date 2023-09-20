#pragma once

#include "AbstractStdTable.h"

class StdTable : public AbstractStdTable
{
    Q_OBJECT
public:
    explicit StdTable(QWidget* parent = nullptr);

    // Sorting
    struct SortBy
    {
        typedef std::function<bool(const QString &, const QString &)> t;
        static bool AsText(const QString & a, const QString & b);
        static bool AsInt(const QString & a, const QString & b);
        static bool AsHex(const QString & a, const QString & b);
    };

    // Data Management
    void addColumnAt(int width, QString title, bool isClickable, QString copyTitle = "", SortBy::t sortFn = SortBy::AsText);
    void deleteAllColumns() override;
    void setRowCount(duint count) override;
    void setCellContent(duint r, duint c, QString s);
    void setCellContent(duint r, duint c, QString s, duint userdata);
    QString getCellContent(duint r, duint c) override;
    void setCellUserdata(duint r, duint c, duint userdata);
    duint getCellUserdata(duint r, duint c);
    bool isValidIndex(duint r, duint c) override;
    void sortRows(duint column, bool ascending) override;

protected:
    struct CellData
    {
        QString text;
        duint userdata = 0;
    };

    std::vector<std::vector<CellData>> mData; //listof(row) where row = (listof(col) where col = CellData)
    std::vector<SortBy::t> mColumnSortFunctions;
};
