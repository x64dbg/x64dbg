#pragma once

#include <QMainWindow>
#include "SimpleFunctionThread.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();
    void loadFile(const QString & path);

private slots:
    void on_action_Load_file_triggered();

private:
    void setupNavigation();
    void setupWidgets();

private:
    Ui::MainWindow* ui = nullptr;
    SimpleFunctionThread* mIdaThread = nullptr;
};
