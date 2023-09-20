#pragma once

#include <QScrollArea>

class QHexEditPrivate;

class QHexEdit : public QScrollArea
{
    Q_OBJECT
public:
    QHexEdit(QWidget* parent = nullptr);

    //data management
    void setData(const QByteArray & data, const QByteArray & mask);
    void setData(const QByteArray & data);
    void setData(const QString & pattern);
    QByteArray applyMaskedData(const QByteArray & data);
    QByteArray data();
    QByteArray mask();
    QString pattern(bool space = false);
    void insert(int i, const QByteArray & ba, const QByteArray & mask);
    void insert(int i, char ch, char mask);
    void remove(int pos, int len = 1);
    void replace(int pos, int len, const QByteArray & after, const QByteArray & mask);
    void fill(int index, const QString & pattern);

    //properties
    void setCursorPosition(int cusorPos);
    int cursorPosition();
    void setOverwriteMode(bool overwriteMode);
    bool overwriteMode();
    void setWildcardEnabled(bool enabled);
    bool wildcardEnabled();
    void setKeepSize(bool enabled);
    bool keepSize();
    void setHorizontalSpacing(int x);
    int horizontalSpacing();
    void setTextColor(QColor color);
    QColor textColor();
    void setWildcardColor(QColor color);
    QColor wildcardColor();
    void setBackgroundColor(QColor color);
    QColor backgroundColor();
    void setSelectionColor(QColor color);
    QColor selectionColor();
    void setEditFont(const QFont & font);

public slots:
    void redo();
    void undo();

signals:
    void currentAddressChanged(int address);
    void currentSizeChanged(int size);
    void dataChanged();
    void dataEdited();
    void overwriteModeChanged(bool state);

private:
    QHexEditPrivate* qHexEdit_p;
};
