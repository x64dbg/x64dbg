#include "UpdateChecker.h"
#include <QUrl>
#include <QNetworkRequest>
#include <QMessageBox>
#include <QNetworkReply>
#include <QIcon>
#include <QDateTime>
#include "Bridge.h"
#include "StringUtil.h"
#include "MiscUtil.h"

UpdateChecker::UpdateChecker(QWidget* parent)
    : QNetworkAccessManager(parent),
      mParent(parent)
{
    connect(this, SIGNAL(finished(QNetworkReply*)), this, SLOT(finishedSlot(QNetworkReply*)));
}

void UpdateChecker::checkForUpdates()
{
    get(QNetworkRequest(QUrl("https://api.github.com/repos/x64dbg/x64dbg/releases/latest")));
}

void UpdateChecker::finishedSlot(QNetworkReply* reply)
{
    if(reply->error() != QNetworkReply::NoError) //error
    {
        SimpleErrorBox(mParent, tr("Network Error!"), reply->errorString());
        return;
    }
    QString json = QString(reply->readAll());
    reply->close();
    QRegExp regExp("\"published_at\": ?\"([^\"]+)\"");
    QDateTime serverTime;
    if(regExp.indexIn(json) >= 0)
        serverTime = QDateTime::fromString(regExp.cap(1), Qt::ISODate);
    if(!serverTime.isValid())
    {
        SimpleErrorBox(mParent, tr("Error!"), tr("File on server could not be parsed..."));
        return;
    }
    QRegExp regUrl("\"browser_download_url\": ?\"([^\"]+)\"");
    auto url = regUrl.indexIn(json) >= 0 ? regUrl.cap(1) : "http://releases.x64dbg.com";
    auto server = serverTime.date();
    auto build = GetCompileDate();
    QString info;
    if(server > build)
        info = QString(tr("New build %1 available!<br>Download <a href=\"%2\">here</a><br><br>You are now on build %3")).arg(ToDateString(server)).arg(url).arg(ToDateString(build));
    else if(server < build)
        info = QString(tr("You have a development build (%1) of x64dbg!")).arg(ToDateString(build));
    else
        info = QString(tr("You have the latest build (%1) of x64dbg!")).arg(ToDateString(build));
    SimpleInfoBox(mParent, tr("Information"), info);
}
