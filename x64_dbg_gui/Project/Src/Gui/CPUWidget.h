#ifndef CPUWIDGET_H
#define CPUWIDGET_H

#include <QtGui>
#include <QVBoxLayout>
#include "CPUDisassembly.h"
#include "CPUDump.h"
#include "RegistersView.h"
#include "InfoBox.h"

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
    QVBoxLayout* getTopLeftUpperWidget(void);
    QVBoxLayout* getTopLeftLowerWidget(void);
    QVBoxLayout* getTopRightWidget(void);
    QVBoxLayout* getBotLeftWidget(void);
    QVBoxLayout* getBotRightWidget(void);

signals:

public slots:
    void runSelection();

private:
    Ui::CPUWidget *ui;
    Disassembly* mDisas;
    RegistersView* mRegs;
    InfoBox* mInfo;

};

#endif // CPUWIDGET_H
