#ifndef CPUWIDGET_H
#define CPUWIDGET_H

#include <QtGui>
#include <QVBoxLayout>
#include "Disassembly.h"
#include "HexDump.h"
#include "RegistersView.h"


namespace Ui {
class CPUWidget;
}

class CPUWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit CPUWidget(QWidget *parent = 0);
    ~CPUWidget();
    void setDefaultDisposition(void);
    QVBoxLayout* getTopLeftWidget(void);
    QVBoxLayout* getTopRightWidget(void);
    QVBoxLayout* getBotLeftWidget(void);
    QVBoxLayout* getBotRightWidget(void);

signals:

public slots:
    void stepOverSlot();

private:
    Ui::CPUWidget *ui;
    Disassembly* mDisas;
    RegistersView* mRegs;

};

#endif // CPUWIDGET_H
