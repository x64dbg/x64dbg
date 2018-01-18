#include "EntropyDialog.h"
#include "ui_EntropyDialog.h"
#include <Configuration.h>

EntropyDialog::EntropyDialog(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::EntropyDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint | Qt::MSWindowsFixedSizeDialogHint);

    mBlockSize = 128;
    mPointCount = 300;
    mInitialized = false;
    Config()->setupWindowPos(this);
}

EntropyDialog::~EntropyDialog()
{
    Config()->saveWindowPos(this);
    delete ui;
}

void EntropyDialog::GraphMemory(const unsigned char* data, int dataSize, QColor color)
{
    initializeGraph();
    ui->entropyView->GraphMemory(data, dataSize, mBlockSize, mPointCount, color);
}

void EntropyDialog::GraphFile(const QString & fileName, QColor color)
{
    initializeGraph();
    ui->entropyView->GraphFile(fileName, mBlockSize, mPointCount, color);
}

void EntropyDialog::initializeGraph()
{
    if(mInitialized)
        return;
    mInitialized = true;
    ui->entropyView->InitializeGraph();
}
