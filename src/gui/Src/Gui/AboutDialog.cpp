#include "AboutDialog.h"
#include "UpdateChecker.h"
#include "ui_AboutDialog.h"
#include <QDesktopServices>
#include <QUrl>

AboutDialog::AboutDialog(QString title, QString logo_icon, QString compile_date, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    //set window flags
    setModal(true);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint | Qt::MSWindowsFixedSizeDialogHint);

    this->setWindowTitle(title);
    ui->lblVersion->setText(compile_date);
    ui->lblLogoImage->setPixmap(logo_icon);

    // Create updatechecker
    mUpdateChecker = new UpdateChecker(this);
}

AboutDialog::~AboutDialog()
{
    delete ui;
}

void AboutDialog::on_lblWebsite_linkActivated(const QString &link)
{
    QDesktopServices::openUrl(QUrl(link));
}

void AboutDialog::on_lblVersion_7_linkActivated(const QString &link)
{
    QDesktopServices::openUrl(QUrl(link));
}

void AboutDialog::on_lblAbout_2_linkActivated(const QString &link)
{
    QDesktopServices::openUrl(QUrl(link));
}

void AboutDialog::on_btnCheckUpdates_clicked()
{
    mUpdateChecker->checkForUpdates();
}
