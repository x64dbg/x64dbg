#ifndef SYMBOLVIEW_H
#define SYMBOLVIEW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QList>
#include "StdTable.h"
#include "Bridge.h"

namespace Ui {
class SymbolView;
}

class SymbolView : public QWidget
{
    Q_OBJECT

public:
    explicit SymbolView(QWidget *parent = 0);
    ~SymbolView();

public slots:
    void addMsgToSymbolLogSlot(QString msg);
    void clearSymbolLogSlot();
    void moduleSelectionChanged(int index);
    void updateSymbolList(int module_count, SYMBOLMODULEINFO* modules);

private:
    Ui::SymbolView *ui;
    QVBoxLayout* mainLayout;

    StdTable* mModuleList;
    StdTable* mSymbolList;

    struct SymbolInfo_t
    {
        uint_t addr;
        QString decoratedSymbol;
        QString undecoratedSymbol;
    };

    struct ModuleInfo_t
    {
        uint_t base;
        QString name;
        QList<SymbolInfo_t> symbols;
    };

    QList<ModuleInfo_t> moduleList;

};

#endif // SYMBOLVIEW_H
