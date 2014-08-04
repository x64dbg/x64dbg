#ifndef REGISTERSVIEW_H
#define REGISTERSVIEW_H

#include <QtGui>
#include <QLabel>
#include <QMenu>
#include <QSet>
#include "Bridge.h"
#include "WordEditDialog.h"
#include "LineEditDialog.h"
#include <QAbstractScrollArea>

#define IsCharacterRegister(x) ((x>=CAX && x<CIP))

namespace Ui
{
class RegistersView;
}

class RegistersView : public QAbstractScrollArea
{
    Q_OBJECT

public:
    // all possible register ids
    enum REGISTER_NAME
    {
        CAX, CCX, CDX, CBX, CDI, CBP, CSI, CSP,
        R8, R9, R10, R11, R12, R13, R14, R15,
        CIP,
        EFLAGS, CF, PF, AF, ZF, SF, TF, IF, DF, OF,
        GS, FS, ES, DS, CS, SS,
        DR0, DR1, DR2, DR3, DR6, DR7,
        UNKNOWN
    };

    // contains viewport position of register
    struct Register_Position
    {
        int line;
        int start;
        int valuesize;
        int labelwidth;

        Register_Position(int l, int s, int w, int v)
        {
            line = l;
            start = s;
            valuesize = v;
            labelwidth = w;
        }
        Register_Position()
        {
            line = 0;
            start = 0;
            valuesize = 0;
            labelwidth = 0;
        }
    };


    explicit RegistersView(QWidget* parent = 0);
    ~RegistersView();

    QSize sizeHint() const;

public slots:
    void refreshShortcutsSlot();
    void updateRegistersSlot();
    void displayCustomContextMenuSlot(QPoint pos);
    void setRegister(REGISTER_NAME reg, uint_t value);
    void debugStateChangedSlot(DBGSTATE state);
    void repaint();
signals:
    void refresh();

protected:
    // events
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseDoubleClickEvent(QMouseEvent* event);
    virtual void paintEvent(QPaintEvent* event);
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void wheelEvent(QWheelEvent* event);

    // use-in-class-only methods
    void drawRegister(QPainter* p, REGISTER_NAME reg, uint_t value);
    void setRegisters(REGDUMP* reg);
    int_t registerValue(const REGDUMP* regd, const REGISTER_NAME reg);
    bool identifyRegister(const int y, const int x, REGISTER_NAME* clickedReg);

    void displayEditDialog();

protected slots:
    void fontsUpdatedSlot();
    void onIncrementAction();
    void onDecrementAction();
    void onZeroAction();
    void onSetToOneAction();
    void onModifyAction();
    void onToggleValueAction();
    void onCopyToClipboardAction();
    void onFollowInDisassembly();
    void onFollowInDump();
    void onFollowInStack();

private:
    int mVScrollOffset;
    int mRowsNeeded;
    int yTopSpacing;
    // holds current selected register
    REGISTER_NAME mSelected;
    // general purposes register id s (cax, ..., r8, ....)
    QSet<REGISTER_NAME> mGPR;
    // all flags
    QSet<REGISTER_NAME> mFlags;
    // contains all id's of registers if there occurs a change
    QSet<REGISTER_NAME> mRegisterUpdates;
    // registers that do not allow changes
    QSet<REGISTER_NAME> mNoChange;
    // maps from id to name
    QMap<REGISTER_NAME, QString> mRegisterMapping;
    // contains viewport positions
    QMap<REGISTER_NAME, Register_Position> mRegisterPlaces;
    // contains a dump of the current register values
    REGDUMP wRegDumpStruct;
    REGDUMP wCipRegDumpStruct;
    // font measures (TODO: create a class that calculates all thos values)
    unsigned int mRowHeight, mCharWidth;
    // context menu actions
    QAction* wCM_Increment;
    QAction* wCM_Decrement;
    QAction* wCM_Zero;
    QAction* wCM_SetToOne;
    QAction* wCM_Modify;
    QAction* wCM_ToggleValue;
    QAction* wCM_CopyToClipboard;
    QAction* wCM_FollowInDisassembly;
    QAction* wCM_FollowInDump;
    QAction* wCM_FollowInStack;
    int_t mCip;
};

#endif // REGISTERSVIEW_H
