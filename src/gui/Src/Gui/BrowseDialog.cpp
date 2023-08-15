#include "BrowseDialog.h"
#include "ui_BrowseDialog.h"
#include "MiscUtil.h"
#include <QDirModel>
#include <QCompleter>
#include <QFileDialog>
#include <Configuration.h>

BrowseDialog::BrowseDialog(QWidget* parent, const QString & title, const QString & text, const QString & filter, const QString & defaultPath, bool save) :
    QDialog(parent),
    ui(new Ui::BrowseDialog), mFilter(filter), mSave(save)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint | Qt::MSWindowsFixedSizeDialogHint);
    setWindowTitle(title);
    ui->label->setText(text);
    auto nativePath = QDir::toNativeSeparators(defaultPath);
    ui->lineEdit->setText(nativePath);
    // Select the filename when saving
    auto lastSlashIdx = nativePath.lastIndexOf(QDir::separator());
    if(save && !QFileInfo(nativePath).isDir() && lastSlashIdx != -1)
    {
        auto periodIdx = nativePath.lastIndexOf('.');
        if(periodIdx == -1)
            periodIdx = nativePath.length();
        ui->lineEdit->setSelection(lastSlashIdx + 1, periodIdx - lastSlashIdx - 1);
    }
    QCompleter* completer = new QCompleter(ui->lineEdit);
    completer->setModel(new QDirModel(completer));
    ui->lineEdit->setCompleter(completer);
    Config()->loadWindowGeometry(this);
}

BrowseDialog::~BrowseDialog()
{
    Config()->saveWindowGeometry(this);
    delete ui;
}

void BrowseDialog::on_browse_clicked()
{
    QString file;
    if(mSave)
        file = QFileDialog::getSaveFileName(this, ui->label->text(), ui->lineEdit->text(), mFilter);
    else
        file = QFileDialog::getOpenFileName(this, ui->label->text(), ui->lineEdit->text(), mFilter);
    if(file.size() != 0)
        ui->lineEdit->setText(QDir::toNativeSeparators(file));
}

void BrowseDialog::on_ok_clicked()
{
    path = ui->lineEdit->text();
    accept();
}
