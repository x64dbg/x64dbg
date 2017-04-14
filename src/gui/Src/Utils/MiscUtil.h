#ifndef MISCUTIL_H
#define MISCUTIL_H

#include <QIcon>
#include "Imports.h"

class QWidget;
class QByteArray;

void SetApplicationIcon(WId winId);
QByteArray & ByteReverse(QByteArray & array);
bool SimpleInputBox(QWidget* parent, const QString & title, QString defaultValue, QString & output, const QString & placeholderText, QIcon* icon = nullptr);
void SimpleErrorBox(QWidget* parent, const QString & title, const QString & text);
void SimpleWarningBox(QWidget* parent, const QString & title, const QString & text);
QString getSymbolicName(duint addr);
bool isEaster();
QString couldItBeSeasonal(QString icon);

#define DIcon(file) QIcon(QString(":/icons/images/").append(couldItBeSeasonal(file)))
#endif // MISCUTIL_H
