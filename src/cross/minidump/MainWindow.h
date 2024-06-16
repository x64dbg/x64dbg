#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <memory>

#include <QMainWindow>

#include "MiniMemoryMap.h"
#include "MiniDisassembly.h"
#include "MiniHexDump.h"
#include "REToolSync.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void on_actionLoad_DMP_triggered();

private:
    void loadMinidump(const QString & path);
    void setupNavigation();
    void setupWidgets();
    void setupToolSync();

private:
    Ui::MainWindow* ui = nullptr;
    std::vector<uint8_t> mDumpData;
    std::unique_ptr<MiniDump::AbstractParser> mParser;
    MiniMemoryMap* mMemoryMap = nullptr;
    MiniHexDump* mHexDump = nullptr;
    MiniDisassembly* mDisassembly = nullptr;
    REToolSync* mToolSync = nullptr;
    Navigation* mNavigation = nullptr;
};
#endif // MAINWINDOW_H
