#pragma once

#include <QScrollArea>
#include <QSet>
#include <QMap>
#include "Bridge.h"

class CPUWidget;
class CPUMultiDump;
class QPushButton;

typedef struct
{
    const char* string;
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
    enum REGISTER_NAME : int
    {
        CAX, CCX, CDX, CBX, CDI, CBP, CSI, CSP,
#ifdef _WIN64
        R8, R9, R10, R11, R12, R13, R14, R15,
#endif //_WIN64
        CIP,
        EFLAGS, CF, PF, AF, ZF, SF, TF, IF, DF, OF,
        GS, FS, ES, DS, CS, SS,
        LastError, LastStatus,
        DR0, DR1, DR2, DR3, DR6, DR7,
        // x87 stuff
        x87r0, x87r1, x87r2, x87r3, x87r4, x87r5, x87r6, x87r7,
        x87st0, x87st1, x87st2, x87st3, x87st4, x87st5, x87st6, x87st7,
        x87TagWord, x87ControlWord, x87StatusWord,
        // x87 Tag Word fields
        x87TW_0, x87TW_1, x87TW_2, x87TW_3, x87TW_4, x87TW_5,
        x87TW_6, x87TW_7,
        // x87 Status Word fields
        x87SW_B, x87SW_C3, x87SW_TOP, x87SW_C2, x87SW_C1, x87SW_O,
        x87SW_ES, x87SW_SF, x87SW_P, x87SW_U, x87SW_Z,
        x87SW_D, x87SW_I, x87SW_C0,
        // x87 Control Word fields
        x87CW_IC, x87CW_RC, x87CW_PC, x87CW_PM,
        x87CW_UM, x87CW_OM, x87CW_ZM, x87CW_DM, x87CW_IM,
        //MxCsr
        MxCsr, MxCsr_FZ, MxCsr_PM, MxCsr_UM, MxCsr_OM, MxCsr_ZM,
        MxCsr_IM, MxCsr_DM, MxCsr_DAZ, MxCsr_PE, MxCsr_UE, MxCsr_OE,
        MxCsr_ZE, MxCsr_DE, MxCsr_IE, MxCsr_RC,
        // MMX
        MM0, MM1, MM2, MM3, MM4, MM5, MM6, MM7,
        // shared XMM, YMM, ZMM
        XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7,
#ifdef _WIN64
        XMM8, XMM9, XMM10, XMM11, XMM12, XMM13, XMM14, XMM15,
        // The following registers are part of AVX-512
        XMM16, XMM17, XMM18, XMM19, XMM20, XMM21, XMM22, XMM23,
        XMM24, XMM25, XMM26, XMM27, XMM28, XMM29, XMM30, XMM31,
#endif //_WIN64
        K0, K1, K2, K3, K4, K5, K6, K7,
        UNKNOWN
    };
protected:
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

    struct REGDUMP_EXTENDED
    {
        REGISTERCONTEXT_AVX512 regcontext;
        FLAGS flags;
        X87FPUREGISTER x87FPURegisters[8];
        MXCSRFIELDS MxCsrFields;
        X87STATUSWORDFIELDS x87StatusWordFields;
        X87CONTROLWORDFIELDS x87ControlWordFields;
        LASTERROR lastError;
        LASTSTATUS lastStatus;
    };

    // tracks position of a register relative to other registers
    struct Register_Relative_Position
    {
        REGISTER_NAME left;
        REGISTER_NAME right;
        REGISTER_NAME up;
        REGISTER_NAME down;

        Register_Relative_Position(REGISTER_NAME l, REGISTER_NAME r)
        {
            left = l;
            right = r;
            up = left;
            down = right;
        }
        Register_Relative_Position(REGISTER_NAME l, REGISTER_NAME r, REGISTER_NAME u, REGISTER_NAME d)
        {
            left = l;
            right = r;
            up = u;
            down = d;
        }
        Register_Relative_Position()
        {
            left = UNKNOWN;
            right = UNKNOWN;
            up = UNKNOWN;
            down = UNKNOWN;
        }
    };
public:
    explicit RegistersView(QWidget* parent);
    ~RegistersView();

    //QSize sizeHint() const;

    static void* operator new(size_t size);
    static void operator delete(void* p);
    int getEstimateHeight();

    static bool isAVX512Supported();

public slots:
    virtual void refreshShortcutsSlot();
    virtual void displayCustomContextMenuSlot(QPoint pos);
    virtual void debugStateChangedSlot(DBGSTATE state);
    void reload();
    void ShowFPU(bool set_showfpu);
    void onChangeFPUViewAction();
    void SetChangeButton(QPushButton* push_button);

signals:
    void refresh();

protected:
    QAction* setupAction(const QIcon & icon, const QString & text);
    QAction* setupAction(const QString & text);
    // events
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseDoubleClickEvent(QMouseEvent* event);
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void paintEvent(QPaintEvent* event);
    virtual void keyPressEvent(QKeyEvent* event);

    // use-in-class-only methods
    void drawRegister(QPainter* p, REGISTER_NAME reg, char* value);
    char* registerValue(const REGDUMP_EXTENDED* regd, const REGISTER_NAME reg);
    bool identifyRegister(const int y, const int x, REGISTER_NAME* clickedReg);
    QString helpRegister(REGISTER_NAME reg);

    void ensureRegisterVisible(REGISTER_NAME reg);

protected slots:
    void InitMappings();
    void fontsUpdatedSlot();
    void shutdownSlot();
    QString getRegisterLabel(REGISTER_NAME);
    int CompareRegisters(const REGISTER_NAME reg_name, REGDUMP_EXTENDED* regdump);
    SIZE_T GetSizeRegister(const REGISTER_NAME reg_name);
    QString GetRegStringValueFromValue(REGISTER_NAME reg, const char* value);
    QString GetTagWordStateString(unsigned short);
    //unsigned int GetTagWordValueFromString(const char* string);
    QString GetControlWordPCStateString(unsigned short);
    //unsigned int GetControlWordPCValueFromString(const char* string);
    QString GetControlWordRCStateString(unsigned short);
    //unsigned int GetControlWordRCValueFromString(const char* string);
    QString GetMxCsrRCStateString(unsigned short);
    //unsigned int GetMxCsrRCValueFromString(const char* string);
    //unsigned int GetStatusWordTOPValueFromString(const char* string);
    QString GetStatusWordTOPStateString(unsigned short state);
    void setRegisters(REGDUMP* reg);
    void setRegisters(REGDUMP_AVX512* reg);
    void appendRegister(QString & text, REGISTER_NAME reg, const char* name64, const char* name32);

    void onCopyToClipboardAction();
    void onCopyFloatingPointToClipboardAction();
    void onCopySymbolToClipboardAction();
    // switch SIMD display modes
    void onSIMDMode();
    void onXMMSizeAutoClicked();
    void onAlwaysShowAVX512Clicked();
    void onFpuMode();
    void onCopyAllAction();
protected:
    bool isActive;
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
    QSet<REGISTER_NAME> mUNDODISPLAY;
    QSet<REGISTER_NAME> mMODIFYDISPLAY;
    QSet<REGISTER_NAME> mFIELDVALUE;
    QSet<REGISTER_NAME> mTAGWORD;
    QSet<REGISTER_NAME> mCANSTOREADDRESS;
    QSet<REGISTER_NAME> mSEGMENTREGISTER;
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
    QSet<REGISTER_NAME> mFPUOpmask;
    // contains all id's of registers if there occurs a change
    QSet<REGISTER_NAME> mRegisterUpdates;
    // registers that do not allow changes
    QSet<REGISTER_NAME> mNoChange;
    // maps from id to name
    QMap<REGISTER_NAME, QString> mRegisterMapping;
    // contains viewport positions
    QMap<REGISTER_NAME, Register_Position> mRegisterPlaces;
    // contains names of closest registers in view
    QMap<REGISTER_NAME, Register_Relative_Position> mRegisterRelativePlaces;
    // contains a dump of the current register values
    REGDUMP_EXTENDED mRegDumpStruct;
    REGDUMP_EXTENDED mCipRegDumpStruct;
    REGDUMP_EXTENDED expandContext(const REGDUMP* reg);
    REGDUMP_EXTENDED expandContext(const REGDUMP_AVX512* reg);
    // font measures (TODO: create a class that calculates all thos values)
    unsigned int mRowHeight, mCharWidth;
    // SIMD registers display mode
    char mFpuMode; //0 = order by ST(X), 1 = order by x87rX, 2 = MMX registers
    char mXMMMode; //0 = XMM, 1 = YMM, 2 = ZMM
    bool mXMMModeAuto; //true = automatically switch on and off YMM/ZMM display
    bool mAlwaysShowAVX512Registers; //true = always show AVX512 registers, false = auto
    bool mAVX512RegistersShown;
    void autoUpdateXMMModesAndRefresh();
    dsint mCip;
    std::vector<std::pair<const char*, uint8_t>> mHighlightRegs;
    // menu actions
    QAction* mDisplaySTX;
    QAction* mDisplayx87rX;
    QAction* mDisplayMMX;
    QAction* wCM_CopyToClipboard;
    QAction* wCM_CopyFloatingPointValueToClipboard;
    QAction* wCM_CopySymbolToClipboard;
    QAction* wCM_CopyAll;
    QAction* wCM_ChangeFPUView;
    QMenu* mSwitchSIMDDispMode;
    void setupSIMDModeMenu();
    QAction* SIMDHex;
    QAction* SIMDFloat;
    QAction* SIMDDouble;
    QAction* SIMDSWord;
    QAction* SIMDUWord;
    QAction* SIMDHWord;
    QAction* SIMDSDWord;
    QAction* SIMDUDWord;
    QAction* SIMDHDWord;
    QAction* SIMDSQWord;
    QAction* SIMDUQWord;
    QAction* SIMDHQWord;
    QAction* SIMDXMMSizeAuto;
    QAction* SIMDAlwaysShowAVX512;
};
