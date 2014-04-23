#ifndef CPUWIDGET_H
#define CPUWIDGET_H

#include <QtGui>
#include <QVBoxLayout>
#include <QTableWidget>
#include "CPUDisassembly.h"
#include "CPUDump.h"
#include "CPUStack.h"
#include "RegistersView.h"
#include "InfoBox.h"

#include "tabwidget.h"

namespace Ui {
class CPUWidget;
}

class CPUWidget : public MHWorkflowWidget
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
    RegistersView* mGeneralRegs;
    InfoBox* mInfo;
    QTabWidget* mRegsTab;
};

#endif // CPUWIDGET_H
