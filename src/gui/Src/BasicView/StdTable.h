#ifndef STDTABLE_H
#define STDTABLE_H

#include "AbstractStdTable.h"

class StdTable : public AbstractStdTable
{
    Q_OBJECT
public:
    explicit StdTable(QWidget* parent = 0);

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
    void setRowCount(dsint count) override;
    void setCellContent(int r, int c, QString s);
    QString getCellContent(int r, int c) override;
    void setCellUserdata(int r, int c, duint userdata);
    duint getCellUserdata(int r, int c);
    bool isValidIndex(int r, int c) override;
    void sortRows(int column, bool ascending) override;

protected:
    struct CellData
    {
        QString text;
        duint userdata = 0;
    };

    std::vector<std::vector<CellData>> mData; //listof(row) where row = (listof(col) where col = CellData)
    std::vector<SortBy::t> mColumnSortFunctions;
};

#endif // STDTABLE_H
