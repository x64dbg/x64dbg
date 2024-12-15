#include "MainWindow.h"
#include "./ui_MainWindow.h"
#include <QDebug>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QFileDialog>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setupWidgets();

    // Load the dump provided on the command line
    auto args = qApp->arguments();
    if(args.length() > 1)
    {
        loadFile(args.at(1));
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::loadFile(const QString & path)
{
}

void MainWindow::setupWidgets()
{
    mStructWidget = new StructWidget();
    ui->tabWidget->addTab(mStructWidget, mStructWidget->windowTitle());
}

void MainWindow::on_action_Load_file_triggered()
{

}

