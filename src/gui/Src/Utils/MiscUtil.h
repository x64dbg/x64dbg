#pragma once

#include <QIcon>
#include <functional>
#include "Imports.h"

class QWidget;
class QByteArray;

void SetApplicationIcon(WId winId);
QByteArray & ByteReverse(QByteArray & array);
QByteArray ByteReverse(QByteArray && array);
bool SimpleInputBox(QWidget* parent, const QString & title, QString defaultValue, QString & output, const QString & placeholderText, const QIcon* icon = nullptr);
bool SimpleChoiceBox(QWidget* parent, const QString & title, QString defaultValue, const QStringList & choices, QString & output, bool editable, const QString & placeholderText, const QIcon* icon = nullptr, int minimumContentsLength = -1);
void SimpleErrorBox(QWidget* parent, const QString & title, const QString & text);
void SimpleWarningBox(QWidget* parent, const QString & title, const QString & text);
void SimpleInfoBox(QWidget* parent, const QString & title, const QString & text);
QString getSymbolicName(duint addr);
QString getSymbolicNameStr(duint addr);
bool ExportCSV(dsint rows, dsint columns, std::vector<QString> headers, std::function<QString(dsint, dsint)> getCellContent);
bool isEaster();
bool isSeasonal();
QString couldItBeSeasonal(QString icon);
QIcon getFileIcon(QString file);

template<int>
static const QIcon & DIconHelper(const QString & file)
{
    static QIcon icon(QString(":/icons/images/").append(couldItBeSeasonal(file)));
    return icon;
}

#define DIcon(file) DIconHelper<__LINE__>(file)
