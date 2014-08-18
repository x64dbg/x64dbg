#include "UpdateChecker.h"
#include <QUrl>
#include <QNetworkRequest>
#include <QMessageBox>
#include <QNetworkReply>
#include <QIcon>
#include "Bridge.h"

UpdateChecker::UpdateChecker(QWidget* parent)
{
    mParent = parent;
    connect(this, SIGNAL(finished(QNetworkReply*)), this, SLOT(finishedSlot(QNetworkReply*)));
}

void UpdateChecker::checkForUpdates()
{
    get(QNetworkRequest(QUrl("http://x64dbg.com/version.txt")));
}

void UpdateChecker::finishedSlot(QNetworkReply* reply)
{
    if(reply->error() != QNetworkReply::NoError) //error
    {
        QMessageBox msg(QMessageBox::Critical, "Network Error!", reply->errorString());
        msg.setParent(mParent, Qt::Dialog);
        msg.setWindowIcon(QIcon(":/icons/images/compile-error.png"));
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        msg.exec();
        return;
    }
    bool ok = false;
    int version = QString(reply->readAll()).toInt(&ok);
    reply->close();
    if(!ok)
    {
        QMessageBox msg(QMessageBox::Critical, "Error!", "File on server could not be parsed...");
        msg.setParent(mParent);
        msg.setWindowIcon(QIcon(":/icons/images/compile-error.png"));
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        msg.exec();
        return;
    }
    QString info;
    int dbgVersion = BridgeGetDbgVersion();
    if(version > dbgVersion)
        info = QString().sprintf("New version v%d available!\nDownload at http://x64dbg.com\n\nYou are now on version v%d", version, dbgVersion);
    else if(version < dbgVersion)
        info = QString().sprintf("You have a development version (v%d) of x64_dbg!", dbgVersion);
    else
        info = QString().sprintf("You have the latest version (%d) of x64_dbg!", version);
    QMessageBox msg(QMessageBox::Information, "Information", info);
    msg.setWindowIcon(QIcon(":/icons/images/information.png"));
    msg.setParent(mParent, Qt::Dialog);
    msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
    msg.exec();
}
