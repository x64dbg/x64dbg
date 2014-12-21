#ifndef REGISTERSVIEW_H
#define REGISTERSVIEW_H

#include <QScrollArea>
#include <QSet>
#include <QMap>
#include "Bridge.h"

#define IsCharacterRegister(x) ((x>=CAX && x<CIP))

typedef struct
{
    QString string;
    unsigned int value;
} STRING_VALUE_TABLE_t;

#define SIZE_TABLE(table) (sizeof(table) / sizeof(*table))

namespace Ui
{
class RegistersView;
}

class RegistersView : public QScrollArea
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
        // x87 stuff
        x87r0, x87r1, x87r2, x87r3, x87r4, x87r5, x87r6, x87r7,
        x87TagWord, x87ControlWord, x87StatusWord,
        // x87 Tag Word fields
        x87TW_0, x87TW_1, x87TW_2, x87TW_3, x87TW_4, x87TW_5,
        x87TW_6, x87TW_7,
        // x87 Status Word fields
        x87SW_B, x87SW_C3, x87SW_TOP, x87SW_C2, x87SW_C1, x87SW_O,
        x87SW_IR, x87SW_SF, x87SW_P, x87SW_U, x87SW_Z,
        x87SW_D, x87SW_I, x87SW_C0,
        // x87 Control Word fields
        x87CW_IC, x87CW_RC, x87CW_PC, x87CW_IEM, x87CW_PM,
        x87CW_UM, x87CW_OM, x87CW_ZM, x87CW_DM, x87CW_IM,
        //MxCsr
        MxCsr, MxCsr_FZ, MxCsr_PM, MxCsr_UM, MxCsr_OM, MxCsr_ZM,
        MxCsr_IM, MxCsr_DM, MxCsr_DAZ, MxCsr_PE, MxCsr_UE, MxCsr_OE,
        MxCsr_ZE, MxCsr_DE, MxCsr_IE, MxCsr_RC,
        // MMX and XMM
        MM0, MM1, MM2, MM3, MM4, MM5, MM6, MM7,
        XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7,
        XMM8, XMM9, XMM10, XMM11, XMM12, XMM13, XMM14, XMM15,
        // YMM
        YMM0, YMM1, YMM2, YMM3, YMM4, YMM5, YMM6, YMM7, YMM8,
        YMM9, YMM10, YMM11, YMM12, YMM13, YMM14, YMM15,

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
    void ShowFPU(bool set_showfpu);
    void onChangeFPUViewAction();
    void SetChangeButton(QPushButton* push_button);
signals:
    void refresh();

protected:
    // events
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseDoubleClickEvent(QMouseEvent* event);
    virtual void paintEvent(QPaintEvent* event);
    virtual void keyPressEvent(QKeyEvent* event);

    // use-in-class-only methods
    void drawRegister(QPainter* p, REGISTER_NAME reg, char* value);
    void setRegisters(REGDUMP* reg);
    char* registerValue(const REGDUMP* regd, const REGISTER_NAME reg);
    bool identifyRegister(const int y, const int x, REGISTER_NAME* clickedReg);

    void displayEditDialog();

protected slots:
    void fontsUpdatedSlot();
    void onIncrementAction();
    void onDecrementAction();
    void onIncrementx87StackAction();
    void onDecrementx87StackAction();
    void onZeroAction();
    void onSetToOneAction();
    void onModifyAction();
    void onToggleValueAction();
    void onCopyToClipboardAction();
    void onCopySymbolToClipboardAction();
    void onFollowInDisassembly();
    void onFollowInDump();
    void onFollowInStack();
    void InitMappings();
    QString getRegisterLabel(REGISTER_NAME);
    int CompareRegisters(const REGISTER_NAME reg_name, REGDUMP* regdump1, REGDUMP* regdump2);
    SIZE_T GetSizeRegister(const REGISTER_NAME reg_name);
    QString GetRegStringValueFromValue(REGISTER_NAME reg , char* value);
    QString GetTagWordStateString(unsigned short);
    unsigned int GetTagWordValueFromString(QString string);
    QString GetControlWordPCStateString(unsigned short);
    unsigned int GetControlWordPCValueFromString(QString string);
    QString GetControlWordRCStateString(unsigned short);
    unsigned int GetControlWordRCValueFromString(QString string);
    QString GetMxCsrRCStateString(unsigned short);
    unsigned int GetMxCsrRCValueFromString(QString string);
    void ModifyFields(QString title, STRING_VALUE_TABLE_t* table, SIZE_T size);
    unsigned int GetStatusWordTOPValueFromString(QString string);
    QString GetStatusWordTOPStateString(unsigned short state);

private:
    QPushButton* mChangeViewButton;
    bool mShowFpu;
    int mVScrollOffset;
    int mRowsNeeded;
    int yTopSpacing;
    int mButtonHeight;
    QSet<REGISTER_NAME> mUINTDISPLAY;
    QSet<REGISTER_NAME> mUSHORTDISPLAY;
    QSet<REGISTER_NAME> mDWORDDISPLAY;
    QSet<REGISTER_NAME> mBOOLDISPLAY;
    QSet<REGISTER_NAME> mLABELDISPLAY;
    QSet<REGISTER_NAME> mONLYMODULEANDLABELDISPLAY;
    QSet<REGISTER_NAME> mSETONEZEROTOGGLE;
    QSet<REGISTER_NAME> mMODIFYDISPLAY;
    QSet<REGISTER_NAME> mFIELDVALUE;
    QSet<REGISTER_NAME> mTAGWORD;
    QSet<REGISTER_NAME> mCANSTOREADDRESS;
    QSet<REGISTER_NAME> mINCREMENTDECREMET;
    QSet<REGISTER_NAME> mFPUx87_80BITSDISPLAY;
    QSet<REGISTER_NAME> mFPU;
    // holds current selected register
    REGISTER_NAME mSelected;
    // general purposes register id s (cax, ..., r8, ....)
    QSet<REGISTER_NAME> mGPR;
    // all flags
    QSet<REGISTER_NAME> mFlags;
    // FPU x87, XMM and MMX registers
    QSet<REGISTER_NAME> mFPUx87;
    QSet<REGISTER_NAME> mFPUMMX;
    QSet<REGISTER_NAME> mFPUXMM;
    QSet<REGISTER_NAME> mFPUYMM;
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
    QAction* wCM_CopySymbolToClipboard;
    QAction* wCM_FollowInDisassembly;
    QAction* wCM_FollowInDump;
    QAction* wCM_FollowInStack;
    QAction* wCM_Incrementx87Stack;
    QAction* wCM_Decrementx87Stack;
    QAction* wCM_ChangeFPUView;
    int_t mCip;
};

#endif // REGISTERSVIEW_H
