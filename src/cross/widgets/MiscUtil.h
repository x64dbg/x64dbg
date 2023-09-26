#pragma once

#include <QIcon>
#include <functional>
#include "Types.h"

class QWidget;
class QByteArray;

//void SetApplicationIcon(WId winId);
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
QIcon getFileIcon(QString file);
QIcon DIconHelper(QString name);
QString getDbPath(const QString & filename = QString(), bool addDateTimeSuffix = false);
QString mainModuleName(bool extension = false);

#define DIcon(name) [](QString arg) { static QIcon icon(DIconHelper(std::move(arg))); return icon; }(name)
