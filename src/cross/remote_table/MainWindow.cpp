#include "MainWindow.h"
#include "./ui_MainWindow.h"
#include <QDebug>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QFileDialog>
#include "StringUtil.h"
#include <fstream>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setupWidgets();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    QMainWindow::closeEvent(event);
}

void MainWindow::setupWidgets()
{
    mTable = new RemoteTable(this);
    ui->tabWidget->addTab(mTable, "Remote Table");
}
