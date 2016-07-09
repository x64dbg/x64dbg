#ifndef MISCUTIL_H
#define MISCUTIL_H

#include <QWidget>
#include <QIcon>

void SetApplicationIcon(WId winId);
QByteArray & ByteReverse(QByteArray & array);
bool SimpleInputBox(QWidget* parent, const QString & title, QString defaultValue, QString & output);
void SimpleErrorBox(QWidget* parent, const QString & title, const QString & text);
void SimpleWarningBox(QWidget* parent, const QString & title, const QString & text);

struct DIcon : QIcon
{
    explicit DIcon(const QString & file);
};

#endif // MISCUTIL_H
