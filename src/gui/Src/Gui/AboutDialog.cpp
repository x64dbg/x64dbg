#include "AboutDialog.h"
#include "UpdateChecker.h"
#include "ui_AboutDialog.h"
#include <StringUtil.h>
#include <QDesktopServices>
#include <QUrl>

AboutDialog::AboutDialog(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    //set window flags
    setModal(true);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint | Qt::MSWindowsFixedSizeDialogHint);

    ui->lblVersion->setText(ToDateString(GetCompileDate()) + ", " __TIME__);

    // Create updatechecker
    mUpdateChecker = new UpdateChecker(this);
}

AboutDialog::~AboutDialog()
{
    delete ui;
}

void AboutDialog::on_btnCheckUpdates_clicked()
{
    mUpdateChecker->checkForUpdates();
}
