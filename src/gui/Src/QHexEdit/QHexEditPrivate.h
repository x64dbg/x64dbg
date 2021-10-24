#pragma once

#include <QWidget>
#include <QScrollArea>
#include <QUndoStack>
#include <QKeyEvent>
#include <QTimer>
#include "XByteArray.h"

class QHexEditPrivate : public QWidget
{
    Q_OBJECT
public:
    QHexEditPrivate(QScrollArea* parent);

    //properties
    void setCursorPos(int position);
    int cursorPos();
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

    //data management
    void setData(const QByteArray & data, const QByteArray & mask);
    QByteArray data();
    QByteArray mask();
    void insert(int index, const QByteArray & ba, const QByteArray & mask);
    void insert(int index, char ch, char mask);
    void remove(int index, int len = 1);
    void replace(int index, char ch, char mask);
    void replace(int index, const QByteArray & ba, const QByteArray & mask);
    void replace(int pos, int len, const QByteArray & after, const QByteArray & mask);
    void fill(int index, const QByteArray & ba, const QByteArray & mask);
    void undo();
    void redo();

signals:
    void currentAddressChanged(int address);
    void currentSizeChanged(int size);
    void dataChanged();
    void dataEdited();
    void overwriteModeChanged(bool state);

protected:
    void focusInEvent(QFocusEvent* event);
    void resizeEvent(QResizeEvent* event);
    void keyPressEvent(QKeyEvent* event);
    void mouseMoveEvent(QMouseEvent* event);
    void mousePressEvent(QMouseEvent* event);

    void paintEvent(QPaintEvent* event);

    int cursorPos(QPoint pos);          // calc cursorpos from graphics position. DOES NOT STORE POSITION

    void resetSelection(int pos);       // set selectionStart && selectionEnd to pos
    void resetSelection();              // set selectionEnd to selectionStart
    void setSelection(int pos);         // set min (if below init) || max (if greater init)
    int getSelectionBegin();
    int getSelectionEnd();

private slots:
    void updateCursor();

private:
    void adjust();
    void ensureVisible();

    QScrollArea* _scrollArea;
    QTimer _cursorTimer;
    QUndoStack* _undoDataStack;
    QUndoStack* _undoMaskStack;

    XByteArray _xData;
    XByteArray _xMask;

    QColor _textColor;
    QColor _wildcardColor;
    QColor _backgroundColor;
    QColor _selectionColor;

    bool _blink;                            // true: then cursor blinks
    bool _overwriteMode;
    bool _wildcardEnabled;
    bool _keepSize;

    int _charWidth, _charHeight;            // char dimensions (dpendend on font)
    int _cursorX, _cursorY;                 // graphics position of the cursor
    int _cursorPosition;                    // character positioin in stream (on byte ends in to steps)
    int _xPosHex;                           // graphics x-position of the areas

    int _selectionBegin;                    // First selected char
    int _selectionEnd;                      // Last selected char
    int _selectionInit;                     // That's, where we pressed the mouse button

    int _size;
    int _initSize;
    int _horizonalSpacing;
};
