#include "UpdateChecker.h"
#include <QUrl>
#include <QNetworkRequest>
#include <QMessageBox>
#include <QNetworkReply>
#include <QIcon>
#include <QDateTime>
#include "Bridge.h"
#include "StringUtil.h"

UpdateChecker::UpdateChecker(QWidget* parent)
    : QNetworkAccessManager(parent),
      mParent(parent)
{
    connect(this, SIGNAL(finished(QNetworkReply*)), this, SLOT(finishedSlot(QNetworkReply*)));
}

void UpdateChecker::checkForUpdates()
{
    get(QNetworkRequest(QUrl("http://jenkins.x64dbg.com/job/vs13/lastSuccessfulBuild/api/json")));
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
    QString json = QString(reply->readAll());
    QRegExp regExp("\"timestamp\":([0-9]+)");
    qulonglong timestamp;
    if(regExp.indexIn(json) >= 0)
        timestamp = regExp.cap(1).toULongLong(&ok) / 1000;
    reply->close();
    if(!ok)
    {
        QMessageBox msg(QMessageBox::Critical, "Error!", "File on server could not be parsed...");
        msg.setParent(mParent, Qt::Dialog);
        msg.setWindowIcon(QIcon(":/icons/images/compile-error.png"));
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        msg.exec();
        return;
    }
    auto server = QDateTime::fromTime_t(timestamp).date();
    auto build = GetCompileDate();
    QString info;
    if(server > build)
        info = QString("New build %1 available!<br>Download <a href=\"http://releases.x64dbg.com\">here</a><br><br>You are now on build %2").arg(ToDateString(server), ToDateString(build));
    else if(server < build)
        info = QString("You have a development build (%1) of x64dbg!").arg(ToDateString(build));
    else
        info = QString("You have the latest build (%1) of x64dbg!").arg(ToDateString(build));
    QMessageBox msg(QMessageBox::Information, "Information", info);
    msg.setWindowIcon(QIcon(":/icons/images/information.png"));
    msg.setParent(mParent, Qt::Dialog);
    msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
    msg.exec();
}
