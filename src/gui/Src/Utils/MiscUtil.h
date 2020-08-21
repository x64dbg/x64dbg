#ifndef MISCUTIL_H
#define MISCUTIL_H

#include <QIcon>
#include <functional>
#include "Imports.h"

class QWidget;
class QByteArray;

void SetApplicationIcon(WId winId);
QByteArray & ByteReverse(QByteArray & array);
QByteArray ByteReverse(QByteArray && array);
bool SimpleInputBox(QWidget* parent, const QString & title, QString defaultValue, QString & output, const QString & placeholderText, QIcon* icon = nullptr);
bool SimpleChoiceBox(QWidget* parent, const QString & title, QString defaultValue, const QStringList & choices, QString & output, bool editable, const QString & placeholderText, QIcon* icon = nullptr, int minimumContentsLength = -1);
void SimpleErrorBox(QWidget* parent, const QString & title, const QString & text);
void SimpleWarningBox(QWidget* parent, const QString & title, const QString & text);
void SimpleInfoBox(QWidget* parent, const QString & title, const QString & text);
QString getSymbolicName(duint addr);
QString getSymbolicNameStr(duint addr);
bool ExportCSV(dsint rows, dsint columns, std::vector<QString> headers, std::function<QString(dsint, dsint)> getCellContent);
bool isEaster();
QString couldItBeSeasonal(QString icon);

#define DIcon(file) QIcon(QString(":/icons/images/").append(couldItBeSeasonal(file)))
#endif // MISCUTIL_H
