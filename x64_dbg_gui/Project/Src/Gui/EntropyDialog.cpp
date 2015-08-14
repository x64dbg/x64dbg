#include "EntropyDialog.h"
#include "ui_EntropyDialog.h"

EntropyDialog::EntropyDialog(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::EntropyDialog)
{
    ui->setupUi(this);
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
    setWindowFlags(Qt::Dialog | Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::MSWindowsFixedSizeDialogHint);
#endif
    setFixedSize(this->size()); //fixed size

    mBlockSize = 128;
    mPointCount = 300;
    mInitialized = false;
}

EntropyDialog::~EntropyDialog()
{
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
