#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <memory>

#include <QMainWindow>

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
    void on_action_Load_file_triggered();

private:
    void loadFile(const QString & path);
    void setupWidgets();

private:
    Ui::MainWindow* ui = nullptr;
};
#endif // MAINWINDOW_H
