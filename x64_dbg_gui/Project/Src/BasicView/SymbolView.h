#ifndef SYMBOLVIEW_H
#define SYMBOLVIEW_H

#include <QWidget>
#include <QVBoxLayout>
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

private:
    Ui::SymbolView *ui;
    QVBoxLayout* mainLayout;

    StdTable* mModuleList;
    StdTable* mSymbolList;

};

#endif // SYMBOLVIEW_H
