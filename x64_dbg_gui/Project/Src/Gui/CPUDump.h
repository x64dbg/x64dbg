#ifndef CPUDUMP_H
#define CPUDUMP_H

#include <QtGui>
#include <QtDebug>
#include <QAction>
#include <QMenu>
#include "NewTypes.h"
#include "HexDump.h"
#include "Bridge.h"
#include "GotoDialog.h"

class CPUDump : public HexDump
{
    Q_OBJECT
public:
    explicit CPUDump(QWidget *parent = 0);
    QString paintContent(QPainter* painter, int_t rowBase, int rowOffset, int col, int x, int y, int w, int h);
    void setupContextMenu();
    void contextMenuEvent(QContextMenuEvent* event);

public slots:
    void gotoExpressionSlot();

    void hexAsciiSlot();
    void hexUnicodeSlot();

    void textAsciiSlot();
    void textUnicodeSlot();

    void integerSignedShortSlot();
    void integerSignedLongSlot();
    void integerSignedLongLongSlot();
    void integerUnsignedShortSlot();
    void integerUnsignedLongSlot();
    void integerUnsignedLongLongSlot();
    void integerHexShortSlot();
    void integerHexLongSlot();
    void integerHexLongLongSlot();

    void floatFloatSlot();
    void floatDoubleSlot();
    void floatLongDoubleSlot();

    void addressSlot();
    void disassemblySlot();

    void selectionGet(SELECTIONDATA* selection);
    void selectionSet(const SELECTIONDATA* selection);

private:
    QMenu* mGotoMenu;
    QAction* mGotoExpression;

    QMenu* mHexMenu;
    QAction* mHexAsciiAction;
    QAction* mHexUnicodeAction;

    QMenu* mTextMenu;
    QAction* mTextAsciiAction;
    QAction* mTextUnicodeAction;

    QMenu* mIntegerMenu;
    QAction* mIntegerSignedShortAction;
    QAction* mIntegerSignedLongAction;
#ifdef _WIN64
    QAction* mIntegerSignedLongLongAction;
#endif //_WIN64
    QAction* mIntegerUnsignedShortAction;
    QAction* mIntegerUnsignedLongAction;
#ifdef _WIN64
    QAction* mIntegerUnsignedLongLongAction;
#endif //_WIN64
    QAction* mIntegerHexShortAction;
    QAction* mIntegerHexLongAction;
#ifdef _WIN64
    QAction* mIntegerHexLongLongAction;
#endif //_WIN64

    QMenu* mFloatMenu;
    QAction* mFloatFloatAction;
    QAction* mFloatDoubleAction;
    QAction* mFloatLongDoubleAction;

    QAction* mAddressAction;
    QAction* mDisassemblyAction;

    QMenu* mSpecialMenu;
    QMenu* mCustomMenu;

    GotoDialog* mGoto;
};

#endif // CPUDUMP_H
