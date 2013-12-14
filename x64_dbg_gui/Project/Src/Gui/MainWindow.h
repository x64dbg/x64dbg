#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtGui>
#include <QFileDialog>
#include <QMdiArea>
#include <QMdiSubWindow>
#include "CPUWidget.h"
#include "CommandLineEdit.h"
#include "MemoryMapView.h"
#include "LogView.h"
#include "GotoDialog.h"
#include "StatusLabel.h"
#include "BreakpointsView.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void executeCommand();
    void execStepOver();
    void execStepInto();
    void setFocusToCommandBar();
    void displayMemMapWidget();
    void displayLogWidget();
    void displayAboutWidget();
    void execClose();
    void execRun();
    void execRtr();
    void openFile();
    void execPause();
    void startScylla();
    void restartDebugging();
    void displayBreakpointWidget();
    void updateWindowTitleSlot(QString filename);
    void updateCPUTitleSlot(QString modname);
    
private slots:
    void on_actionGoto_triggered();

private:
    Ui::MainWindow *ui;
    QMdiArea* mdiArea;
    CPUWidget* mCpuWin;

    CommandLineEdit* mCmdLineEdit;

    QMdiSubWindow* mSubWindow;
    QMdiSubWindow* mMemMapView;
    QMdiSubWindow* mLogView;
    QMdiSubWindow* mBreakpointsView;

    StatusLabel* mStatusLabel;
    StatusLabel* mLastLogLabel;

    const char* mWindowMainTitle;

protected:
    void dragEnterEvent(QDragEnterEvent* pEvent);
    void dropEvent(QDropEvent* pEvent);
};

#endif // MAINWINDOW_H
