#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <memory>

#include <QMainWindow>

#include "MiniHexDump.h"
#include "Navigation.h"
#include "File.h"
#include "CodeEditor.h"
#include "PatternHighlighter.h"
#include "PatternLanguage.h"
#include "StructWidget.h"
#include <QTextBrowser>
#include "DataTable.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onButtonRunPressed();
    void onLogAnchorClicked(const QUrl & url);
    void gotoHexDump(duint address, duint size);

    void on_action_Load_file_triggered();

private:
    void loadFile(const QString & path);
    void setupNavigation();
    void setupWidgets();
    void logHandler(LogLevel level, const char* message);
    void compileError(const CompileError & error);
    void evalError(const EvalError & error);

private:
    Ui::MainWindow* ui = nullptr;
    Navigation* mNavigation = nullptr;
    MiniHexDump* mHexDump = nullptr;
    File* mFile = nullptr;
    CodeEditor* mCodeEditor = nullptr;
    PatternHighlighter* mHighlighter = nullptr;
    QTextBrowser* mLogBrowser = nullptr;
    StructWidget* mStructWidget = nullptr;
    DataTable* mDataTable = nullptr;
    class PatternVisitor* mVisitor = nullptr;
    QTabWidget* mStructTabs = nullptr;
};
#endif // MAINWINDOW_H
