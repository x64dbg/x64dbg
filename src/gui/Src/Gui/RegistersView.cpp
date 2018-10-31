#include <QMessageBox>
#include <QListWidget>
#include <QToolTip>
#include <stdint.h>
#include "RegistersView.h"
#include "CPUWidget.h"
#include "CPUDisassembly.h"
#include "CPUMultiDump.h"
#include "Configuration.h"
#include "WordEditDialog.h"
#include "LineEditDialog.h"
#include "EditFloatRegister.h"
#include "SelectFields.h"
#include "MiscUtil.h"
#include "ldconvert.h"

int RegistersView::getEstimateHeight()
{
    return mRowsNeeded * mRowHeight;
}

void RegistersView::SetChangeButton(QPushButton* push_button)
{
    mChangeViewButton = push_button;
    fontsUpdatedSlot();
}

void RegistersView::InitMappings()
{
    // create mapping from internal id to name
    mRegisterMapping.clear();
    mRegisterPlaces.clear();
    int offset = 0;

    /* Register_Position is a struct definition the position
     *
     * (line , start, labelwidth, valuesize )
     */
#ifdef _WIN64
    mRegisterMapping.insert(CAX, "RAX");
    mRegisterPlaces.insert(CAX, Register_Position(offset++, 0, 6, sizeof(duint) * 2));
    mRegisterMapping.insert(CBX, "RBX");
    mRegisterPlaces.insert(CBX, Register_Position(offset++, 0, 6, sizeof(duint) * 2));
    mRegisterMapping.insert(CCX, "RCX");
    mRegisterPlaces.insert(CCX, Register_Position(offset++, 0, 6, sizeof(duint) * 2));
    mRegisterMapping.insert(CDX, "RDX");
    mRegisterPlaces.insert(CDX, Register_Position(offset++, 0, 6, sizeof(duint) * 2));
    mRegisterMapping.insert(CBP, "RBP");
    mRegisterPlaces.insert(CBP, Register_Position(offset++, 0, 6, sizeof(duint) * 2));
    mRegisterMapping.insert(CSP, "RSP");
    mRegisterPlaces.insert(CSP, Register_Position(offset++, 0, 6, sizeof(duint) * 2));
    mRegisterMapping.insert(CSI, "RSI");
    mRegisterPlaces.insert(CSI, Register_Position(offset++, 0, 6, sizeof(duint) * 2));
    mRegisterMapping.insert(CDI, "RDI");
    mRegisterPlaces.insert(CDI, Register_Position(offset++, 0, 6, sizeof(duint) * 2));

    offset++;

    mRegisterMapping.insert(R8, "R8");
    mRegisterPlaces.insert(R8, Register_Position(offset++, 0, 6, sizeof(duint) * 2));
    mRegisterMapping.insert(R9, "R9");
    mRegisterPlaces.insert(R9, Register_Position(offset++, 0, 6, sizeof(duint) * 2));
    mRegisterMapping.insert(R10, "R10");
    mRegisterPlaces.insert(R10, Register_Position(offset++, 0, 6, sizeof(duint) * 2));
    mRegisterMapping.insert(R11, "R11");
    mRegisterPlaces.insert(R11, Register_Position(offset++, 0, 6, sizeof(duint) * 2));
    mRegisterMapping.insert(R12, "R12");
    mRegisterPlaces.insert(R12, Register_Position(offset++, 0, 6, sizeof(duint) * 2));
    mRegisterMapping.insert(R13, "R13");
    mRegisterPlaces.insert(R13, Register_Position(offset++, 0, 6, sizeof(duint) * 2));
    mRegisterMapping.insert(R14, "R14");
    mRegisterPlaces.insert(R14, Register_Position(offset++, 0, 6, sizeof(duint) * 2));
    mRegisterMapping.insert(R15, "R15");
    mRegisterPlaces.insert(R15, Register_Position(offset++, 0, 6, sizeof(duint) * 2));

    offset++;

    mRegisterMapping.insert(CIP, "RIP");
    mRegisterPlaces.insert(CIP, Register_Position(offset++, 0, 6, sizeof(duint) * 2));

    offset++;

    mRegisterMapping.insert(EFLAGS, "RFLAGS");
    mRegisterPlaces.insert(EFLAGS, Register_Position(offset++, 0, 9, sizeof(duint) * 2));
#else //x32
    mRegisterMapping.insert(CAX, "EAX");
    mRegisterPlaces.insert(CAX, Register_Position(offset++, 0, 6, sizeof(duint) * 2));
    mRegisterMapping.insert(CBX, "EBX");
    mRegisterPlaces.insert(CBX, Register_Position(offset++, 0, 6, sizeof(duint) * 2));
    mRegisterMapping.insert(CCX, "ECX");
    mRegisterPlaces.insert(CCX, Register_Position(offset++, 0, 6, sizeof(duint) * 2));
    mRegisterMapping.insert(CDX, "EDX");
    mRegisterPlaces.insert(CDX, Register_Position(offset++, 0, 6, sizeof(duint) * 2));
    mRegisterMapping.insert(CBP, "EBP");
    mRegisterPlaces.insert(CBP, Register_Position(offset++, 0, 6, sizeof(duint) * 2));
    mRegisterMapping.insert(CSP, "ESP");
    mRegisterPlaces.insert(CSP, Register_Position(offset++, 0, 6, sizeof(duint) * 2));
    mRegisterMapping.insert(CSI, "ESI");
    mRegisterPlaces.insert(CSI, Register_Position(offset++, 0, 6, sizeof(duint) * 2));
    mRegisterMapping.insert(CDI, "EDI");
    mRegisterPlaces.insert(CDI, Register_Position(offset++, 0, 6, sizeof(duint) * 2));

    offset++;

    mRegisterMapping.insert(CIP, "EIP");
    mRegisterPlaces.insert(CIP, Register_Position(offset++, 0, 6, sizeof(duint) * 2));

    offset++;

    mRegisterMapping.insert(EFLAGS, "EFLAGS");
    mRegisterPlaces.insert(EFLAGS, Register_Position(offset++, 0, 9, sizeof(duint) * 2));
#endif

    mRegisterMapping.insert(ZF, "ZF");
    mRegisterPlaces.insert(ZF, Register_Position(offset, 0, 3, 1));
    mRegisterMapping.insert(PF, "PF");
    mRegisterPlaces.insert(PF, Register_Position(offset, 6, 3, 1));
    mRegisterMapping.insert(AF, "AF");
    mRegisterPlaces.insert(AF, Register_Position(offset++, 12, 3, 1));

    mRegisterMapping.insert(OF, "OF");
    mRegisterPlaces.insert(OF, Register_Position(offset, 0, 3, 1));
    mRegisterMapping.insert(SF, "SF");
    mRegisterPlaces.insert(SF, Register_Position(offset, 6, 3, 1));
    mRegisterMapping.insert(DF, "DF");
    mRegisterPlaces.insert(DF, Register_Position(offset++, 12, 3, 1));

    mRegisterMapping.insert(CF, "CF");
    mRegisterPlaces.insert(CF, Register_Position(offset, 0, 3, 1));
    mRegisterMapping.insert(TF, "TF");
    mRegisterPlaces.insert(TF, Register_Position(offset, 6, 3, 1));
    mRegisterMapping.insert(IF, "IF");
    mRegisterPlaces.insert(IF, Register_Position(offset++, 12, 3, 1));

    offset++;

    mRegisterMapping.insert(LastError, "LastError");
    mRegisterPlaces.insert(LastError, Register_Position(offset++, 0, 11, 20));
    mMODIFYDISPLAY.insert(LastError);
    mRegisterMapping.insert(LastStatus, "LastStatus");
    mRegisterPlaces.insert(LastStatus, Register_Position(offset++, 0, 11, 20));
    mMODIFYDISPLAY.insert(LastStatus);

    offset++;

    mRegisterMapping.insert(GS, "GS");
    mRegisterPlaces.insert(GS, Register_Position(offset, 0, 3, 4));
    mRegisterMapping.insert(FS, "FS");
    mRegisterPlaces.insert(FS, Register_Position(offset++, 9, 3, 4));
    mRegisterMapping.insert(ES, "ES");
    mRegisterPlaces.insert(ES, Register_Position(offset, 0, 3, 4));
    mRegisterMapping.insert(DS, "DS");
    mRegisterPlaces.insert(DS, Register_Position(offset++, 9, 3, 4));
    mRegisterMapping.insert(CS, "CS");
    mRegisterPlaces.insert(CS, Register_Position(offset, 0, 3, 4));
    mRegisterMapping.insert(SS, "SS");
    mRegisterPlaces.insert(SS, Register_Position(offset++, 9, 3, 4));

    if(mShowFpu)
    {
        offset++;

        if(mFpuMode)
        {
            mRegisterMapping.insert(x87r0, "x87r0");
            mRegisterPlaces.insert(x87r0, Register_Position(offset++, 0, 6, 10 * 2));
            mRegisterMapping.insert(x87r1, "x87r1");
            mRegisterPlaces.insert(x87r1, Register_Position(offset++, 0, 6, 10 * 2));
            mRegisterMapping.insert(x87r2, "x87r2");
            mRegisterPlaces.insert(x87r2, Register_Position(offset++, 0, 6, 10 * 2));
            mRegisterMapping.insert(x87r3, "x87r3");
            mRegisterPlaces.insert(x87r3, Register_Position(offset++, 0, 6, 10 * 2));
            mRegisterMapping.insert(x87r4, "x87r4");
            mRegisterPlaces.insert(x87r4, Register_Position(offset++, 0, 6, 10 * 2));
            mRegisterMapping.insert(x87r5, "x87r5");
            mRegisterPlaces.insert(x87r5, Register_Position(offset++, 0, 6, 10 * 2));
            mRegisterMapping.insert(x87r6, "x87r6");
            mRegisterPlaces.insert(x87r6, Register_Position(offset++, 0, 6, 10 * 2));
            mRegisterMapping.insert(x87r7, "x87r7");
            mRegisterPlaces.insert(x87r7, Register_Position(offset++, 0, 6, 10 * 2));
        }
        else
        {
            mRegisterMapping.insert(x87st0, "ST(0)");
            mRegisterPlaces.insert(x87st0, Register_Position(offset++, 0, 6, 10 * 2));
            mRegisterMapping.insert(x87st1, "ST(1)");
            mRegisterPlaces.insert(x87st1, Register_Position(offset++, 0, 6, 10 * 2));
            mRegisterMapping.insert(x87st2, "ST(2)");
            mRegisterPlaces.insert(x87st2, Register_Position(offset++, 0, 6, 10 * 2));
            mRegisterMapping.insert(x87st3, "ST(3)");
            mRegisterPlaces.insert(x87st3, Register_Position(offset++, 0, 6, 10 * 2));
            mRegisterMapping.insert(x87st4, "ST(4)");
            mRegisterPlaces.insert(x87st4, Register_Position(offset++, 0, 6, 10 * 2));
            mRegisterMapping.insert(x87st5, "ST(5)");
            mRegisterPlaces.insert(x87st5, Register_Position(offset++, 0, 6, 10 * 2));
            mRegisterMapping.insert(x87st6, "ST(6)");
            mRegisterPlaces.insert(x87st6, Register_Position(offset++, 0, 6, 10 * 2));
            mRegisterMapping.insert(x87st7, "ST(7)");
            mRegisterPlaces.insert(x87st7, Register_Position(offset++, 0, 6, 10 * 2));
        }

        offset++;

        mRegisterMapping.insert(x87TagWord, "x87TagWord");
        mRegisterPlaces.insert(x87TagWord, Register_Position(offset++, 0, 11, sizeof(WORD) * 2));
        //Special treatment of long internationalized string
        int NextColumnPosition = 20;
        int temp;
        QFontMetrics metrics(font());
        //13 = 20 - strlen("Nonzero")
        temp = metrics.width(QApplication::translate("RegistersView_ConstantsOfRegisters", "Nonzero")) / mCharWidth + 13;
        NextColumnPosition = std::max(NextColumnPosition, temp);
        temp = metrics.width(QApplication::translate("RegistersView_ConstantsOfRegisters", "Zero")) / mCharWidth + 13;
        NextColumnPosition = std::max(NextColumnPosition, temp);
        temp = metrics.width(QApplication::translate("RegistersView_ConstantsOfRegisters", "Special")) / mCharWidth + 13;
        NextColumnPosition = std::max(NextColumnPosition, temp);
        temp = metrics.width(QApplication::translate("RegistersView_ConstantsOfRegisters", "Empty")) / mCharWidth + 13;
        NextColumnPosition = std::max(NextColumnPosition, temp);
        mRegisterMapping.insert(x87TW_0, "x87TW_0");
        mRegisterPlaces.insert(x87TW_0, Register_Position(offset, 0, 8, 10));
        mRegisterMapping.insert(x87TW_1, "x87TW_1");
        mRegisterPlaces.insert(x87TW_1, Register_Position(offset++, NextColumnPosition, 8, 10));

        mRegisterMapping.insert(x87TW_2, "x87TW_2");
        mRegisterPlaces.insert(x87TW_2, Register_Position(offset, 0, 8, 10));
        mRegisterMapping.insert(x87TW_3, "x87TW_3");
        mRegisterPlaces.insert(x87TW_3, Register_Position(offset++, NextColumnPosition, 8, 10));

        mRegisterMapping.insert(x87TW_4, "x87TW_4");
        mRegisterPlaces.insert(x87TW_4, Register_Position(offset, 0, 8, 10));
        mRegisterMapping.insert(x87TW_5, "x87TW_5");
        mRegisterPlaces.insert(x87TW_5, Register_Position(offset++, NextColumnPosition, 8, 10));

        mRegisterMapping.insert(x87TW_6, "x87TW_6");
        mRegisterPlaces.insert(x87TW_6, Register_Position(offset, 0, 8, 10));
        mRegisterMapping.insert(x87TW_7, "x87TW_7");
        mRegisterPlaces.insert(x87TW_7, Register_Position(offset++, NextColumnPosition, 8, 10));

        offset++;

        mRegisterMapping.insert(x87StatusWord, "x87StatusWord");
        mRegisterPlaces.insert(x87StatusWord, Register_Position(offset++, 0, 14, sizeof(WORD) * 2));

        mRegisterMapping.insert(x87SW_B, "x87SW_B");
        mRegisterPlaces.insert(x87SW_B, Register_Position(offset, 0, 9, 1));
        mRegisterMapping.insert(x87SW_C3, "x87SW_C3");
        mRegisterPlaces.insert(x87SW_C3, Register_Position(offset, 12, 10, 1));
        mRegisterMapping.insert(x87SW_C2, "x87SW_C2");
        mRegisterPlaces.insert(x87SW_C2, Register_Position(offset++, 25, 10, 1));

        mRegisterMapping.insert(x87SW_C1, "x87SW_C1");
        mRegisterPlaces.insert(x87SW_C1, Register_Position(offset, 0, 9, 1));
        mRegisterMapping.insert(x87SW_C0, "x87SW_C0");
        mRegisterPlaces.insert(x87SW_C0, Register_Position(offset, 12, 10, 1));
        mRegisterMapping.insert(x87SW_ES, "x87SW_ES");
        mRegisterPlaces.insert(x87SW_ES, Register_Position(offset++, 25, 10, 1));

        mRegisterMapping.insert(x87SW_SF, "x87SW_SF");
        mRegisterPlaces.insert(x87SW_SF, Register_Position(offset, 0, 9, 1));
        mRegisterMapping.insert(x87SW_P, "x87SW_P");
        mRegisterPlaces.insert(x87SW_P, Register_Position(offset, 12, 10, 1));
        mRegisterMapping.insert(x87SW_U, "x87SW_U");
        mRegisterPlaces.insert(x87SW_U, Register_Position(offset++, 25, 10, 1));

        mRegisterMapping.insert(x87SW_O, "x87SW_O");
        mRegisterPlaces.insert(x87SW_O, Register_Position(offset, 0, 9, 1));
        mRegisterMapping.insert(x87SW_Z, "x87SW_Z");
        mRegisterPlaces.insert(x87SW_Z, Register_Position(offset, 12, 10, 1));
        mRegisterMapping.insert(x87SW_D, "x87SW_D");
        mRegisterPlaces.insert(x87SW_D, Register_Position(offset++, 25, 10, 1));

        mRegisterMapping.insert(x87SW_I, "x87SW_I");
        mRegisterPlaces.insert(x87SW_I, Register_Position(offset, 0, 9, 1));
        mRegisterMapping.insert(x87SW_TOP, "x87SW_TOP");
        mRegisterPlaces.insert(x87SW_TOP, Register_Position(offset++, 12, 10, 13));

        offset++;

        mRegisterMapping.insert(x87ControlWord, "x87ControlWord");
        mRegisterPlaces.insert(x87ControlWord, Register_Position(offset++, 0, 15, sizeof(WORD) * 2));

        mRegisterMapping.insert(x87CW_IC, "x87CW_IC");
        mRegisterPlaces.insert(x87CW_IC, Register_Position(offset, 0, 9, 1));
        mRegisterMapping.insert(x87CW_ZM, "x87CW_ZM");
        mRegisterPlaces.insert(x87CW_ZM, Register_Position(offset, 12, 10, 1));
        mRegisterMapping.insert(x87CW_PM, "x87CW_PM");
        mRegisterPlaces.insert(x87CW_PM, Register_Position(offset++, 25, 10, 1));

        mRegisterMapping.insert(x87CW_UM, "x87CW_UM");
        mRegisterPlaces.insert(x87CW_UM, Register_Position(offset, 0, 9, 1));
        mRegisterMapping.insert(x87CW_OM, "x87CW_OM");
        mRegisterPlaces.insert(x87CW_OM, Register_Position(offset, 12, 10, 1));
        mRegisterMapping.insert(x87CW_PC, "x87CW_PC");
        mRegisterPlaces.insert(x87CW_PC, Register_Position(offset++, 25, 10, 14));

        mRegisterMapping.insert(x87CW_DM, "x87CW_DM");
        mRegisterPlaces.insert(x87CW_DM, Register_Position(offset, 0, 9, 1));
        mRegisterMapping.insert(x87CW_IM, "x87CW_IM");
        mRegisterPlaces.insert(x87CW_IM, Register_Position(offset, 12, 10, 1));
        mRegisterMapping.insert(x87CW_RC, "x87CW_RC");
        mRegisterPlaces.insert(x87CW_RC, Register_Position(offset++, 25, 10, 14));

        offset++;

        mRegisterMapping.insert(MxCsr, "MxCsr");
        mRegisterPlaces.insert(MxCsr, Register_Position(offset++, 0, 6, sizeof(DWORD) * 2));

        mRegisterMapping.insert(MxCsr_FZ, "MxCsr_FZ");
        mRegisterPlaces.insert(MxCsr_FZ, Register_Position(offset, 0, 9, 1));
        mRegisterMapping.insert(MxCsr_PM, "MxCsr_PM");
        mRegisterPlaces.insert(MxCsr_PM, Register_Position(offset, 12, 10, 1));
        mRegisterMapping.insert(MxCsr_UM, "MxCsr_UM");
        mRegisterPlaces.insert(MxCsr_UM, Register_Position(offset++, 25, 10, 1));

        mRegisterMapping.insert(MxCsr_OM, "MxCsr_OM");
        mRegisterPlaces.insert(MxCsr_OM, Register_Position(offset, 0, 9, 1));
        mRegisterMapping.insert(MxCsr_ZM, "MxCsr_ZM");
        mRegisterPlaces.insert(MxCsr_ZM, Register_Position(offset, 12, 10, 1));
        mRegisterMapping.insert(MxCsr_IM, "MxCsr_IM");
        mRegisterPlaces.insert(MxCsr_IM, Register_Position(offset++, 25, 10, 1));

        mRegisterMapping.insert(MxCsr_UE, "MxCsr_UE");
        mRegisterPlaces.insert(MxCsr_UE, Register_Position(offset, 0, 9, 1));
        mRegisterMapping.insert(MxCsr_PE, "MxCsr_PE");
        mRegisterPlaces.insert(MxCsr_PE, Register_Position(offset, 12, 10, 1));
        mRegisterMapping.insert(MxCsr_DAZ, "MxCsr_DAZ");
        mRegisterPlaces.insert(MxCsr_DAZ, Register_Position(offset++, 25, 10, 1));

        mRegisterMapping.insert(MxCsr_OE, "MxCsr_OE");
        mRegisterPlaces.insert(MxCsr_OE, Register_Position(offset, 0, 9, 1));
        mRegisterMapping.insert(MxCsr_ZE, "MxCsr_ZE");
        mRegisterPlaces.insert(MxCsr_ZE, Register_Position(offset, 12, 10, 1));
        mRegisterMapping.insert(MxCsr_DE, "MxCsr_DE");
        mRegisterPlaces.insert(MxCsr_DE, Register_Position(offset++, 25, 10, 1));

        mRegisterMapping.insert(MxCsr_IE, "MxCsr_IE");
        mRegisterPlaces.insert(MxCsr_IE, Register_Position(offset, 0, 9, 1));
        mRegisterMapping.insert(MxCsr_DM, "MxCsr_DM");
        mRegisterPlaces.insert(MxCsr_DM, Register_Position(offset, 12, 10, 1));
        mRegisterMapping.insert(MxCsr_RC, "MxCsr_RC");
        mRegisterPlaces.insert(MxCsr_RC, Register_Position(offset++, 25, 10, 19));

        offset++;

        mRegisterMapping.insert(MM0, "MM0");
        mRegisterPlaces.insert(MM0, Register_Position(offset++, 0, 4, 8 * 2));
        mRegisterMapping.insert(MM1, "MM1");
        mRegisterPlaces.insert(MM1, Register_Position(offset++, 0, 4, 8 * 2));
        mRegisterMapping.insert(MM2, "MM2");
        mRegisterPlaces.insert(MM2, Register_Position(offset++, 0, 4, 8 * 2));
        mRegisterMapping.insert(MM3, "MM3");
        mRegisterPlaces.insert(MM3, Register_Position(offset++, 0, 4, 8 * 2));
        mRegisterMapping.insert(MM4, "MM4");
        mRegisterPlaces.insert(MM4, Register_Position(offset++, 0, 4, 8 * 2));
        mRegisterMapping.insert(MM5, "MM5");
        mRegisterPlaces.insert(MM5, Register_Position(offset++, 0, 4, 8 * 2));
        mRegisterMapping.insert(MM6, "MM6");
        mRegisterPlaces.insert(MM6, Register_Position(offset++, 0, 4, 8 * 2));
        mRegisterMapping.insert(MM7, "MM7");
        mRegisterPlaces.insert(MM7, Register_Position(offset++, 0, 4, 8 * 2));

        offset++;

        mRegisterMapping.insert(XMM0, "XMM0");
        mRegisterPlaces.insert(XMM0, Register_Position(offset++, 0, 6, 16 * 2));
        mRegisterMapping.insert(XMM1, "XMM1");
        mRegisterPlaces.insert(XMM1, Register_Position(offset++, 0, 6, 16 * 2));
        mRegisterMapping.insert(XMM2, "XMM2");
        mRegisterPlaces.insert(XMM2, Register_Position(offset++, 0, 6, 16 * 2));
        mRegisterMapping.insert(XMM3, "XMM3");
        mRegisterPlaces.insert(XMM3, Register_Position(offset++, 0, 6, 16 * 2));
        mRegisterMapping.insert(XMM4, "XMM4");
        mRegisterPlaces.insert(XMM4, Register_Position(offset++, 0, 6, 16 * 2));
        mRegisterMapping.insert(XMM5, "XMM5");
        mRegisterPlaces.insert(XMM5, Register_Position(offset++, 0, 6, 16 * 2));
        mRegisterMapping.insert(XMM6, "XMM6");
        mRegisterPlaces.insert(XMM6, Register_Position(offset++, 0, 6, 16 * 2));
        mRegisterMapping.insert(XMM7, "XMM7");
        mRegisterPlaces.insert(XMM7, Register_Position(offset++, 0, 6, 16 * 2));
#ifdef _WIN64
        mRegisterMapping.insert(XMM8, "XMM8");
        mRegisterPlaces.insert(XMM8, Register_Position(offset++, 0, 6, 16 * 2));
        mRegisterMapping.insert(XMM9, "XMM9");
        mRegisterPlaces.insert(XMM9, Register_Position(offset++, 0, 6, 16 * 2));
        mRegisterMapping.insert(XMM10, "XMM10");
        mRegisterPlaces.insert(XMM10, Register_Position(offset++, 0, 6, 16 * 2));
        mRegisterMapping.insert(XMM11, "XMM11");
        mRegisterPlaces.insert(XMM11, Register_Position(offset++, 0, 6, 16 * 2));
        mRegisterMapping.insert(XMM12, "XMM12");
        mRegisterPlaces.insert(XMM12, Register_Position(offset++, 0, 6, 16 * 2));
        mRegisterMapping.insert(XMM13, "XMM13");
        mRegisterPlaces.insert(XMM13, Register_Position(offset++, 0, 6, 16 * 2));
        mRegisterMapping.insert(XMM14, "XMM14");
        mRegisterPlaces.insert(XMM14, Register_Position(offset++, 0, 6, 16 * 2));
        mRegisterMapping.insert(XMM15, "XMM15");
        mRegisterPlaces.insert(XMM15, Register_Position(offset++, 0, 6, 16 * 2));
#endif

        offset++;

        mRegisterMapping.insert(YMM0, "YMM0");
        mRegisterPlaces.insert(YMM0, Register_Position(offset++, 0, 6, 32 * 2));
        mRegisterMapping.insert(YMM1, "YMM1");
        mRegisterPlaces.insert(YMM1, Register_Position(offset++, 0, 6, 32 * 2));
        mRegisterMapping.insert(YMM2, "YMM2");
        mRegisterPlaces.insert(YMM2, Register_Position(offset++, 0, 6, 32 * 2));
        mRegisterMapping.insert(YMM3, "YMM3");
        mRegisterPlaces.insert(YMM3, Register_Position(offset++, 0, 6, 32 * 2));
        mRegisterMapping.insert(YMM4, "YMM4");
        mRegisterPlaces.insert(YMM4, Register_Position(offset++, 0, 6, 32 * 2));
        mRegisterMapping.insert(YMM5, "YMM5");
        mRegisterPlaces.insert(YMM5, Register_Position(offset++, 0, 6, 32 * 2));
        mRegisterMapping.insert(YMM6, "YMM6");
        mRegisterPlaces.insert(YMM6, Register_Position(offset++, 0, 6, 32 * 2));
        mRegisterMapping.insert(YMM7, "YMM7");
        mRegisterPlaces.insert(YMM7, Register_Position(offset++, 0, 6, 32 * 2));
#ifdef _WIN64
        mRegisterMapping.insert(YMM8, "YMM8");
        mRegisterPlaces.insert(YMM8, Register_Position(offset++, 0, 6, 32 * 2));
        mRegisterMapping.insert(YMM9, "YMM9");
        mRegisterPlaces.insert(YMM9, Register_Position(offset++, 0, 6, 32 * 2));
        mRegisterMapping.insert(YMM10, "YMM10");
        mRegisterPlaces.insert(YMM10, Register_Position(offset++, 0, 6, 32 * 2));
        mRegisterMapping.insert(YMM11, "YMM11");
        mRegisterPlaces.insert(YMM11, Register_Position(offset++, 0, 6, 32 * 2));
        mRegisterMapping.insert(YMM12, "YMM12");
        mRegisterPlaces.insert(YMM12, Register_Position(offset++, 0, 6, 32 * 2));
        mRegisterMapping.insert(YMM13, "YMM13");
        mRegisterPlaces.insert(YMM13, Register_Position(offset++, 0, 6, 32 * 2));
        mRegisterMapping.insert(YMM14, "YMM14");
        mRegisterPlaces.insert(YMM14, Register_Position(offset++, 0, 6, 32 * 2));
        mRegisterMapping.insert(YMM15, "YMM15");
        mRegisterPlaces.insert(YMM15, Register_Position(offset++, 0, 6, 32 * 2));
#endif
    }

    offset++;

    mRegisterMapping.insert(DR0, "DR0");
    mRegisterPlaces.insert(DR0, Register_Position(offset++, 0, 4, sizeof(duint) * 2));
    mRegisterMapping.insert(DR1, "DR1");
    mRegisterPlaces.insert(DR1, Register_Position(offset++, 0, 4, sizeof(duint) * 2));
    mRegisterMapping.insert(DR2, "DR2");
    mRegisterPlaces.insert(DR2, Register_Position(offset++, 0, 4, sizeof(duint) * 2));
    mRegisterMapping.insert(DR3, "DR3");
    mRegisterPlaces.insert(DR3, Register_Position(offset++, 0, 4, sizeof(duint) * 2));
    mRegisterMapping.insert(DR6, "DR6");
    mRegisterPlaces.insert(DR6, Register_Position(offset++, 0, 4, sizeof(duint) * 2));
    mRegisterMapping.insert(DR7, "DR7");
    mRegisterPlaces.insert(DR7, Register_Position(offset++, 0, 4, sizeof(duint) * 2));

    mRowsNeeded = offset + 1;
}

static QAction* setupAction(const QIcon & icon, const QString & text, RegistersView* this_object)
{
    QAction* action = new QAction(icon, text, this_object);
    action->setShortcutContext(Qt::WidgetShortcut);
    this_object->addAction(action);
    return action;
}

static QAction* setupAction(const QString & text, RegistersView* this_object)
{
    QAction* action = new QAction(text, this_object);
    action->setShortcutContext(Qt::WidgetShortcut);
    this_object->addAction(action);
    return action;
}

RegistersView::RegistersView(CPUWidget* parent) : QScrollArea(parent), mVScrollOffset(0), mParent(parent)
{
    setWindowTitle("Registers");
    mChangeViewButton = NULL;
    connect(Bridge::getBridge(), SIGNAL(close()), this, SLOT(onClose()));
    switch(ConfigUint("Gui", "SIMDRegistersDisplayMode"))
    {
    default:
    case 0:
        wSIMDRegDispMode = SIMD_REG_DISP_HEX;
        break;
    case 1:
        wSIMDRegDispMode = SIMD_REG_DISP_FLOAT;
        break;
    case 2:
        wSIMDRegDispMode = SIMD_REG_DISP_DOUBLE;
        break;
    case 3:
        wSIMDRegDispMode = SIMD_REG_DISP_WORD_SIGNED;
        break;
    case 4:
        wSIMDRegDispMode = SIMD_REG_DISP_DWORD_SIGNED;
        break;
    case 5:
        wSIMDRegDispMode = SIMD_REG_DISP_QWORD_SIGNED;
        break;
    case 6:
        wSIMDRegDispMode = SIMD_REG_DISP_WORD_UNSIGNED;
        break;
    case 7:
        wSIMDRegDispMode = SIMD_REG_DISP_DWORD_UNSIGNED;
        break;
    case 8:
        wSIMDRegDispMode = SIMD_REG_DISP_QWORD_UNSIGNED;
        break;
    case 9:
        wSIMDRegDispMode = SIMD_REG_DISP_WORD_HEX;
        break;
    case 10:
        wSIMDRegDispMode = SIMD_REG_DISP_DWORD_HEX;
        break;
    case 11:
        wSIMDRegDispMode = SIMD_REG_DISP_QWORD_HEX;
        break;
    }
    mFpuMode = false;

    // precreate ContextMenu Actions
    wCM_Increment = setupAction(DIcon("register_inc.png"), tr("Increment"), this);
    wCM_Decrement = setupAction(DIcon("register_dec.png"), tr("Decrement"), this);
    wCM_Zero = setupAction(DIcon("register_zero.png"), tr("Zero"), this);
    wCM_SetToOne = setupAction(DIcon("register_one.png"), tr("Set to 1"), this);
    wCM_Modify = new QAction(DIcon("register_edit.png"), tr("Modify value"), this);
    wCM_Modify->setShortcut(QKeySequence(Qt::Key_Enter));
    wCM_ToggleValue = setupAction(DIcon("register_toggle.png"), tr("Toggle"), this);
    wCM_Undo = setupAction(DIcon("undo.png"), tr("Undo"), this);
    wCM_CopyToClipboard = setupAction(DIcon("copy.png"), tr("Copy value"), this);
    wCM_CopyFloatingPointValueToClipboard = setupAction(DIcon("copy.png"), tr("Copy floating point value"), this);
    wCM_CopySymbolToClipboard = setupAction(DIcon("pdb.png"), tr("Copy Symbol Value"), this);
    wCM_CopyAll = setupAction(DIcon("copy-alt.png"), tr("Copy all registers"), this);
    wCM_FollowInDisassembly = new QAction(DIcon(QString("processor%1.png").arg(ArchValue("32", "64"))), tr("Follow in Disassembler"), this);
    wCM_FollowInDump = new QAction(DIcon("dump.png"), tr("Follow in Dump"), this);
    wCM_FollowInStack = new QAction(DIcon("stack.png"), tr("Follow in Stack"), this);
    wCM_FollowInMemoryMap = new QAction(DIcon("memmap_find_address_page"), tr("Follow in Memory Map"), this);
    wCM_Incrementx87Stack = setupAction(DIcon("arrow-small-down.png"), tr("Increment x87 Stack"), this);
    wCM_Decrementx87Stack = setupAction(DIcon("arrow-small-up.png"), tr("Decrement x87 Stack"), this);
    wCM_ChangeFPUView = new QAction(DIcon("change-view.png"), tr("Change view"), this);
    wCM_IncrementPtrSize = setupAction(DIcon("register_inc.png"), ArchValue(tr("Increase 4"), tr("Increase 8")), this);
    wCM_DecrementPtrSize = setupAction(DIcon("register_dec.png"), ArchValue(tr("Decrease 4"), tr("Decrease 8")), this);
    wCM_Push = setupAction(DIcon("arrow-small-down.png"), tr("Push"), this);
    wCM_Pop = setupAction(DIcon("arrow-small-up.png"), tr("Pop"), this);
    wCM_Highlight = setupAction(DIcon("highlight.png"), tr("Highlight"), this);
    mSwitchSIMDDispMode = new QMenu(tr("Change SIMD Register Display Mode"), this);
    mSwitchSIMDDispMode->setIcon(DIcon("simdmode.png"));
    mSwitchFPUDispMode = new QAction(tr("Display ST(x)"), this);
    mSwitchFPUDispMode->setCheckable(true);
    SIMDHex = new QAction(tr("Hexadecimal"), mSwitchSIMDDispMode);
    SIMDFloat = new QAction(tr("Float"), mSwitchSIMDDispMode);
    SIMDDouble = new QAction(tr("Double"), mSwitchSIMDDispMode);
    SIMDSWord = new QAction(tr("Signed Word"), mSwitchSIMDDispMode);
    SIMDSDWord = new QAction(tr("Signed DWord"), mSwitchSIMDDispMode);
    SIMDSQWord = new QAction(tr("Signed QWord"), mSwitchSIMDDispMode);
    SIMDUWord = new QAction(tr("Unsigned Word"), mSwitchSIMDDispMode);
    SIMDUDWord = new QAction(tr("Unsigned DWord"), mSwitchSIMDDispMode);
    SIMDUQWord = new QAction(tr("Unsigned QWord"), mSwitchSIMDDispMode);
    SIMDHWord = new QAction(tr("Hexadecimal Word"), mSwitchSIMDDispMode);
    SIMDHDWord = new QAction(tr("Hexadecimal DWord"), mSwitchSIMDDispMode);
    SIMDHQWord = new QAction(tr("Hexadecimal QWord"), mSwitchSIMDDispMode);
    SIMDHex->setData(QVariant(SIMD_REG_DISP_HEX));
    SIMDFloat->setData(QVariant(SIMD_REG_DISP_FLOAT));
    SIMDDouble->setData(QVariant(SIMD_REG_DISP_DOUBLE));
    SIMDSWord->setData(QVariant(SIMD_REG_DISP_WORD_SIGNED));
    SIMDUWord->setData(QVariant(SIMD_REG_DISP_WORD_UNSIGNED));
    SIMDHWord->setData(QVariant(SIMD_REG_DISP_WORD_HEX));
    SIMDSDWord->setData(QVariant(SIMD_REG_DISP_DWORD_SIGNED));
    SIMDUDWord->setData(QVariant(SIMD_REG_DISP_DWORD_UNSIGNED));
    SIMDHDWord->setData(QVariant(SIMD_REG_DISP_DWORD_HEX));
    SIMDSQWord->setData(QVariant(SIMD_REG_DISP_QWORD_SIGNED));
    SIMDUQWord->setData(QVariant(SIMD_REG_DISP_QWORD_UNSIGNED));
    SIMDHQWord->setData(QVariant(SIMD_REG_DISP_QWORD_HEX));
    connect(SIMDHex, SIGNAL(triggered()), this, SLOT(onSIMDMode()));
    connect(SIMDFloat, SIGNAL(triggered()), this, SLOT(onSIMDMode()));
    connect(SIMDDouble, SIGNAL(triggered()), this, SLOT(onSIMDMode()));
    connect(SIMDSWord, SIGNAL(triggered()), this, SLOT(onSIMDMode()));
    connect(SIMDUWord, SIGNAL(triggered()), this, SLOT(onSIMDMode()));
    connect(SIMDHWord, SIGNAL(triggered()), this, SLOT(onSIMDMode()));
    connect(SIMDSDWord, SIGNAL(triggered()), this, SLOT(onSIMDMode()));
    connect(SIMDUDWord, SIGNAL(triggered()), this, SLOT(onSIMDMode()));
    connect(SIMDHDWord, SIGNAL(triggered()), this, SLOT(onSIMDMode()));
    connect(SIMDSQWord, SIGNAL(triggered()), this, SLOT(onSIMDMode()));
    connect(SIMDUQWord, SIGNAL(triggered()), this, SLOT(onSIMDMode()));
    connect(SIMDHQWord, SIGNAL(triggered()), this, SLOT(onSIMDMode()));
    connect(mSwitchFPUDispMode, SIGNAL(triggered()), this, SLOT(onFpuMode()));
    SIMDHex->setCheckable(true);
    SIMDFloat->setCheckable(true);
    SIMDDouble->setCheckable(true);
    SIMDSWord->setCheckable(true);
    SIMDUWord->setCheckable(true);
    SIMDHWord->setCheckable(true);
    SIMDSDWord->setCheckable(true);
    SIMDUDWord->setCheckable(true);
    SIMDHDWord->setCheckable(true);
    SIMDSQWord->setCheckable(true);
    SIMDUQWord->setCheckable(true);
    SIMDHQWord->setCheckable(true);
    SIMDHex->setChecked(true);
    SIMDFloat->setChecked(false);
    SIMDDouble->setChecked(false);
    SIMDSWord->setChecked(false);
    SIMDUWord->setChecked(false);
    SIMDHWord->setChecked(false);
    SIMDSDWord->setChecked(false);
    SIMDUDWord->setChecked(false);
    SIMDHDWord->setChecked(false);
    SIMDSQWord->setChecked(false);
    SIMDUQWord->setChecked(false);
    SIMDHQWord->setChecked(false);
    mSwitchSIMDDispMode->addAction(SIMDHex);
    mSwitchSIMDDispMode->addAction(SIMDFloat);
    mSwitchSIMDDispMode->addAction(SIMDDouble);
    mSwitchSIMDDispMode->addAction(SIMDSWord);
    mSwitchSIMDDispMode->addAction(SIMDSDWord);
    mSwitchSIMDDispMode->addAction(SIMDSQWord);
    mSwitchSIMDDispMode->addAction(SIMDUWord);
    mSwitchSIMDDispMode->addAction(SIMDUDWord);
    mSwitchSIMDDispMode->addAction(SIMDUQWord);
    mSwitchSIMDDispMode->addAction(SIMDHWord);
    mSwitchSIMDDispMode->addAction(SIMDHDWord);
    mSwitchSIMDDispMode->addAction(SIMDHQWord);

    // general purposes register (we allow the user to modify the value)
    mGPR.insert(CAX);
    mCANSTOREADDRESS.insert(CAX);
    mUINTDISPLAY.insert(CAX);
    mLABELDISPLAY.insert(CAX);
    mMODIFYDISPLAY.insert(CAX);
    mUNDODISPLAY.insert(CAX);
    mINCREMENTDECREMET.insert(CAX);
    mSETONEZEROTOGGLE.insert(CAX);

    mSETONEZEROTOGGLE.insert(CBX);
    mINCREMENTDECREMET.insert(CBX);
    mGPR.insert(CBX);
    mUINTDISPLAY.insert(CBX);
    mLABELDISPLAY.insert(CBX);
    mMODIFYDISPLAY.insert(CBX);
    mUNDODISPLAY.insert(CBX);
    mCANSTOREADDRESS.insert(CBX);

    mSETONEZEROTOGGLE.insert(CCX);
    mINCREMENTDECREMET.insert(CCX);
    mGPR.insert(CCX);
    mUINTDISPLAY.insert(CCX);
    mLABELDISPLAY.insert(CCX);
    mMODIFYDISPLAY.insert(CCX);
    mUNDODISPLAY.insert(CCX);
    mCANSTOREADDRESS.insert(CCX);

    mSETONEZEROTOGGLE.insert(CDX);
    mINCREMENTDECREMET.insert(CDX);
    mGPR.insert(CDX);
    mUINTDISPLAY.insert(CDX);
    mLABELDISPLAY.insert(CDX);
    mMODIFYDISPLAY.insert(CDX);
    mUNDODISPLAY.insert(CDX);
    mCANSTOREADDRESS.insert(CDX);

    mSETONEZEROTOGGLE.insert(CBP);
    mINCREMENTDECREMET.insert(CBP);
    mCANSTOREADDRESS.insert(CBP);
    mGPR.insert(CBP);
    mUINTDISPLAY.insert(CBP);
    mLABELDISPLAY.insert(CBP);
    mMODIFYDISPLAY.insert(CBP);
    mUNDODISPLAY.insert(CBP);

    mSETONEZEROTOGGLE.insert(CSP);
    mINCREMENTDECREMET.insert(CSP);
    mCANSTOREADDRESS.insert(CSP);
    mGPR.insert(CSP);
    mUINTDISPLAY.insert(CSP);
    mLABELDISPLAY.insert(CSP);
    mMODIFYDISPLAY.insert(CSP);
    mUNDODISPLAY.insert(CSP);

    mSETONEZEROTOGGLE.insert(CSI);
    mINCREMENTDECREMET.insert(CSI);
    mCANSTOREADDRESS.insert(CSI);
    mGPR.insert(CSI);
    mUINTDISPLAY.insert(CSI);
    mLABELDISPLAY.insert(CSI);
    mMODIFYDISPLAY.insert(CSI);
    mUNDODISPLAY.insert(CSI);

    mSETONEZEROTOGGLE.insert(CDI);
    mINCREMENTDECREMET.insert(CDI);
    mCANSTOREADDRESS.insert(CDI);
    mGPR.insert(CDI);
    mUINTDISPLAY.insert(CDI);
    mLABELDISPLAY.insert(CDI);
    mMODIFYDISPLAY.insert(CDI);
    mUNDODISPLAY.insert(CDI);
#ifdef _WIN64
    for(REGISTER_NAME i = R8; i <= R15; i = (REGISTER_NAME)(i + 1))
    {
        mSETONEZEROTOGGLE.insert(i);
        mINCREMENTDECREMET.insert(i);
        mCANSTOREADDRESS.insert(i);
        mGPR.insert(i);
        mLABELDISPLAY.insert(i);
        mUINTDISPLAY.insert(i);
        mMODIFYDISPLAY.insert(i);
        mUNDODISPLAY.insert(i);
    }
#endif //_WIN64

    mSETONEZEROTOGGLE.insert(EFLAGS);
    mGPR.insert(EFLAGS);
    mMODIFYDISPLAY.insert(EFLAGS);
    mUNDODISPLAY.insert(EFLAGS);
    mUINTDISPLAY.insert(EFLAGS);

    // flags (we allow the user to toggle them)
    mFlags.insert(CF);
    mBOOLDISPLAY.insert(CF);
    mSETONEZEROTOGGLE.insert(CF);

    mSETONEZEROTOGGLE.insert(PF);
    mFlags.insert(PF);
    mBOOLDISPLAY.insert(PF);

    mSETONEZEROTOGGLE.insert(AF);
    mFlags.insert(AF);
    mBOOLDISPLAY.insert(AF);

    mSETONEZEROTOGGLE.insert(ZF);
    mFlags.insert(ZF);
    mBOOLDISPLAY.insert(ZF);

    mSETONEZEROTOGGLE.insert(SF);
    mFlags.insert(SF);
    mBOOLDISPLAY.insert(SF);

    mSETONEZEROTOGGLE.insert(TF);
    mFlags.insert(TF);
    mBOOLDISPLAY.insert(TF);

    mFlags.insert(IF);
    mBOOLDISPLAY.insert(IF);

    mSETONEZEROTOGGLE.insert(DF);
    mFlags.insert(DF);
    mBOOLDISPLAY.insert(DF);

    mSETONEZEROTOGGLE.insert(OF);
    mFlags.insert(OF);
    mBOOLDISPLAY.insert(OF);

    // FPU: XMM, x87 and MMX registers
    mSETONEZEROTOGGLE.insert(MxCsr);
    mDWORDDISPLAY.insert(MxCsr);
    mMODIFYDISPLAY.insert(MxCsr);
    mUNDODISPLAY.insert(MxCsr);
    mFPU.insert(MxCsr);

    for(REGISTER_NAME i = x87r0; i <= x87r7; i = (REGISTER_NAME)(i + 1))
    {
        mMODIFYDISPLAY.insert(i);
        mUNDODISPLAY.insert(i);
        mFPUx87.insert(i);
        mFPUx87_80BITSDISPLAY.insert(i);
        mFPU.insert(i);
        mSETONEZEROTOGGLE.insert(i);
    }

    for(REGISTER_NAME i = x87st0; i <= x87st7; i = (REGISTER_NAME)(i + 1))
    {
        mMODIFYDISPLAY.insert(i);
        mUNDODISPLAY.insert(i);
        mFPUx87.insert(i);
        mFPUx87_80BITSDISPLAY.insert(i);
        mFPU.insert(i);
        mSETONEZEROTOGGLE.insert(i);
    }

    mSETONEZEROTOGGLE.insert(x87TagWord);
    mFPUx87.insert(x87TagWord);
    mMODIFYDISPLAY.insert(x87TagWord);
    mUNDODISPLAY.insert(x87TagWord);
    mUSHORTDISPLAY.insert(x87TagWord);
    mFPU.insert(x87TagWord);

    mSETONEZEROTOGGLE.insert(x87StatusWord);
    mUSHORTDISPLAY.insert(x87StatusWord);
    mMODIFYDISPLAY.insert(x87StatusWord);
    mUNDODISPLAY.insert(x87StatusWord);
    mFPUx87.insert(x87StatusWord);
    mFPU.insert(x87StatusWord);

    mSETONEZEROTOGGLE.insert(x87ControlWord);
    mFPUx87.insert(x87ControlWord);
    mMODIFYDISPLAY.insert(x87ControlWord);
    mUNDODISPLAY.insert(x87ControlWord);
    mUSHORTDISPLAY.insert(x87ControlWord);
    mFPU.insert(x87ControlWord);

    mSETONEZEROTOGGLE.insert(x87SW_B);
    mFPUx87.insert(x87SW_B);
    mBOOLDISPLAY.insert(x87SW_B);
    mFPU.insert(x87SW_B);

    mSETONEZEROTOGGLE.insert(x87SW_C3);
    mFPUx87.insert(x87SW_C3);
    mBOOLDISPLAY.insert(x87SW_C3);
    mFPU.insert(x87SW_C3);

    mFPUx87.insert(x87SW_TOP);
    mFIELDVALUE.insert(x87SW_TOP);
    mFPU.insert(x87SW_TOP);
    mMODIFYDISPLAY.insert(x87SW_TOP);
    mUNDODISPLAY.insert(x87SW_TOP);

    mFPUx87.insert(x87SW_C2);
    mBOOLDISPLAY.insert(x87SW_C2);
    mSETONEZEROTOGGLE.insert(x87SW_C2);
    mFPU.insert(x87SW_C2);

    mSETONEZEROTOGGLE.insert(x87SW_C1);
    mFPUx87.insert(x87SW_C1);
    mBOOLDISPLAY.insert(x87SW_C1);
    mFPU.insert(x87SW_C1);

    mSETONEZEROTOGGLE.insert(x87SW_C0);
    mFPUx87.insert(x87SW_C0);
    mBOOLDISPLAY.insert(x87SW_C0);
    mFPU.insert(x87SW_C0);

    mSETONEZEROTOGGLE.insert(x87SW_ES);
    mFPUx87.insert(x87SW_ES);
    mBOOLDISPLAY.insert(x87SW_ES);
    mFPU.insert(x87SW_ES);

    mSETONEZEROTOGGLE.insert(x87SW_SF);
    mFPUx87.insert(x87SW_SF);
    mBOOLDISPLAY.insert(x87SW_SF);
    mFPU.insert(x87SW_SF);

    mSETONEZEROTOGGLE.insert(x87SW_P);
    mFPUx87.insert(x87SW_P);
    mBOOLDISPLAY.insert(x87SW_P);
    mFPU.insert(x87SW_P);

    mSETONEZEROTOGGLE.insert(x87SW_U);
    mFPUx87.insert(x87SW_U);
    mBOOLDISPLAY.insert(x87SW_U);
    mFPU.insert(x87SW_U);

    mSETONEZEROTOGGLE.insert(x87SW_O);
    mFPUx87.insert(x87SW_O);
    mBOOLDISPLAY.insert(x87SW_O);
    mFPU.insert(x87SW_O);

    mSETONEZEROTOGGLE.insert(x87SW_Z);
    mFPUx87.insert(x87SW_Z);
    mBOOLDISPLAY.insert(x87SW_Z);
    mFPU.insert(x87SW_Z);

    mSETONEZEROTOGGLE.insert(x87SW_D);
    mFPUx87.insert(x87SW_D);
    mBOOLDISPLAY.insert(x87SW_D);
    mFPU.insert(x87SW_D);

    mSETONEZEROTOGGLE.insert(x87SW_I);
    mFPUx87.insert(x87SW_I);
    mBOOLDISPLAY.insert(x87SW_I);
    mFPU.insert(x87SW_I);

    mSETONEZEROTOGGLE.insert(x87CW_IC);
    mFPUx87.insert(x87CW_IC);
    mBOOLDISPLAY.insert(x87CW_IC);
    mFPU.insert(x87CW_IC);

    mFPUx87.insert(x87CW_RC);
    mFIELDVALUE.insert(x87CW_RC);
    mFPU.insert(x87CW_RC);
    mMODIFYDISPLAY.insert(x87CW_RC);
    mUNDODISPLAY.insert(x87CW_RC);

    for(REGISTER_NAME i = x87TW_0; i <= x87TW_7; i = (REGISTER_NAME)(i + 1))
    {
        mFPUx87.insert(i);
        mFIELDVALUE.insert(i);
        mTAGWORD.insert(i);
        mFPU.insert(i);
        mMODIFYDISPLAY.insert(i);
        mUNDODISPLAY.insert(i);
    }

    mFPUx87.insert(x87CW_PC);
    mFIELDVALUE.insert(x87CW_PC);
    mFPU.insert(x87CW_PC);
    mMODIFYDISPLAY.insert(x87CW_PC);
    mUNDODISPLAY.insert(x87CW_PC);

    mSETONEZEROTOGGLE.insert(x87CW_PM);
    mFPUx87.insert(x87CW_PM);
    mBOOLDISPLAY.insert(x87CW_PM);
    mFPU.insert(x87CW_PM);

    mSETONEZEROTOGGLE.insert(x87CW_UM);
    mFPUx87.insert(x87CW_UM);
    mBOOLDISPLAY.insert(x87CW_UM);
    mFPU.insert(x87CW_UM);

    mSETONEZEROTOGGLE.insert(x87CW_OM);
    mFPUx87.insert(x87CW_OM);
    mBOOLDISPLAY.insert(x87CW_OM);
    mFPU.insert(x87CW_OM);

    mSETONEZEROTOGGLE.insert(x87CW_ZM);
    mFPUx87.insert(x87CW_ZM);
    mBOOLDISPLAY.insert(x87CW_ZM);
    mFPU.insert(x87CW_ZM);

    mSETONEZEROTOGGLE.insert(x87CW_DM);
    mFPUx87.insert(x87CW_DM);
    mBOOLDISPLAY.insert(x87CW_DM);
    mFPU.insert(x87CW_DM);

    mSETONEZEROTOGGLE.insert(x87CW_IM);
    mFPUx87.insert(x87CW_IM);
    mBOOLDISPLAY.insert(x87CW_IM);
    mFPU.insert(x87CW_IM);

    mSETONEZEROTOGGLE.insert(MxCsr_FZ);
    mBOOLDISPLAY.insert(MxCsr_FZ);
    mFPU.insert(MxCsr_FZ);

    mSETONEZEROTOGGLE.insert(MxCsr_PM);
    mBOOLDISPLAY.insert(MxCsr_PM);
    mFPU.insert(MxCsr_PM);

    mSETONEZEROTOGGLE.insert(MxCsr_UM);
    mBOOLDISPLAY.insert(MxCsr_UM);
    mFPU.insert(MxCsr_UM);

    mSETONEZEROTOGGLE.insert(MxCsr_OM);
    mBOOLDISPLAY.insert(MxCsr_OM);
    mFPU.insert(MxCsr_OM);

    mSETONEZEROTOGGLE.insert(MxCsr_ZM);
    mBOOLDISPLAY.insert(MxCsr_ZM);
    mFPU.insert(MxCsr_ZM);

    mSETONEZEROTOGGLE.insert(MxCsr_IM);
    mBOOLDISPLAY.insert(MxCsr_IM);
    mFPU.insert(MxCsr_IM);

    mSETONEZEROTOGGLE.insert(MxCsr_DM);
    mBOOLDISPLAY.insert(MxCsr_DM);
    mFPU.insert(MxCsr_DM);

    mSETONEZEROTOGGLE.insert(MxCsr_DAZ);
    mBOOLDISPLAY.insert(MxCsr_DAZ);
    mFPU.insert(MxCsr_DAZ);

    mSETONEZEROTOGGLE.insert(MxCsr_PE);
    mBOOLDISPLAY.insert(MxCsr_PE);
    mFPU.insert(MxCsr_PE);

    mSETONEZEROTOGGLE.insert(MxCsr_UE);
    mBOOLDISPLAY.insert(MxCsr_UE);
    mFPU.insert(MxCsr_UE);

    mSETONEZEROTOGGLE.insert(MxCsr_OE);
    mBOOLDISPLAY.insert(MxCsr_OE);
    mFPU.insert(MxCsr_OE);

    mSETONEZEROTOGGLE.insert(MxCsr_ZE);
    mBOOLDISPLAY.insert(MxCsr_ZE);
    mFPU.insert(MxCsr_ZE);

    mSETONEZEROTOGGLE.insert(MxCsr_DE);
    mBOOLDISPLAY.insert(MxCsr_DE);
    mFPU.insert(MxCsr_DE);

    mSETONEZEROTOGGLE.insert(MxCsr_IE);
    mBOOLDISPLAY.insert(MxCsr_IE);
    mFPU.insert(MxCsr_IE);

    mFIELDVALUE.insert(MxCsr_RC);
    mFPU.insert(MxCsr_RC);
    mMODIFYDISPLAY.insert(MxCsr_RC);
    mUNDODISPLAY.insert(MxCsr_RC);

    for(REGISTER_NAME i = MM0; i <= MM7; i = (REGISTER_NAME)(i + 1))
    {
        mMODIFYDISPLAY.insert(i);
        mUNDODISPLAY.insert(i);
        mFPUMMX.insert(i);
        mFPU.insert(i);
    }

    for(REGISTER_NAME i = XMM0; i <= ArchValue(XMM7, XMM15); i = (REGISTER_NAME)(i + 1))
    {
        mFPUXMM.insert(i);
        mMODIFYDISPLAY.insert(i);
        mUNDODISPLAY.insert(i);
        mFPU.insert(i);
    }

    for(REGISTER_NAME i = YMM0; i <= ArchValue(YMM7, YMM15); i = (REGISTER_NAME)(i + 1))
    {
        mFPUYMM.insert(i);
        mMODIFYDISPLAY.insert(i);
        mUNDODISPLAY.insert(i);
        mFPU.insert(i);
    }

    //registers that should not be changed
    mNoChange.insert(LastError);
    mNoChange.insert(LastStatus);

    mNoChange.insert(GS);
    mUSHORTDISPLAY.insert(GS);
    mSEGMENTREGISTER.insert(GS);

    mNoChange.insert(FS);
    mUSHORTDISPLAY.insert(FS);
    mSEGMENTREGISTER.insert(FS);

    mNoChange.insert(ES);
    mUSHORTDISPLAY.insert(ES);
    mSEGMENTREGISTER.insert(ES);

    mNoChange.insert(DS);
    mUSHORTDISPLAY.insert(DS);
    mSEGMENTREGISTER.insert(DS);

    mNoChange.insert(CS);
    mUSHORTDISPLAY.insert(CS);
    mSEGMENTREGISTER.insert(CS);

    mNoChange.insert(SS);
    mUSHORTDISPLAY.insert(SS);
    mSEGMENTREGISTER.insert(SS);

    mNoChange.insert(DR0);
    mUINTDISPLAY.insert(DR0);
    mLABELDISPLAY.insert(DR0);
    mONLYMODULEANDLABELDISPLAY.insert(DR0);
    mCANSTOREADDRESS.insert(DR0);

    mNoChange.insert(DR1);
    mONLYMODULEANDLABELDISPLAY.insert(DR1);
    mUINTDISPLAY.insert(DR1);
    mCANSTOREADDRESS.insert(DR1);

    mLABELDISPLAY.insert(DR2);
    mONLYMODULEANDLABELDISPLAY.insert(DR2);
    mNoChange.insert(DR2);
    mUINTDISPLAY.insert(DR2);
    mCANSTOREADDRESS.insert(DR2);

    mNoChange.insert(DR3);
    mONLYMODULEANDLABELDISPLAY.insert(DR3);
    mLABELDISPLAY.insert(DR3);
    mUINTDISPLAY.insert(DR3);
    mCANSTOREADDRESS.insert(DR3);

    mNoChange.insert(DR6);
    mUINTDISPLAY.insert(DR6);

    mNoChange.insert(DR7);
    mUINTDISPLAY.insert(DR7);

    mNoChange.insert(CIP);
    mUINTDISPLAY.insert(CIP);
    mLABELDISPLAY.insert(CIP);
    mONLYMODULEANDLABELDISPLAY.insert(CIP);
    mCANSTOREADDRESS.insert(CIP);
    mMODIFYDISPLAY.insert(CIP);
    mUNDODISPLAY.insert(CIP);

    fontsUpdatedSlot();
    connect(Config(), SIGNAL(fontsUpdated()), this, SLOT(fontsUpdatedSlot()));

    InitMappings();

    memset(&wRegDumpStruct, 0, sizeof(REGDUMP));
    memset(&wCipRegDumpStruct, 0, sizeof(REGDUMP));
    mCip = 0;
    mRegisterUpdates.clear();

    mButtonHeight = 0;
    yTopSpacing = 4; //set top spacing (in pixels)

    this->setMouseTracking(true);

    // Context Menu
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    // foreign messages
    connect(Bridge::getBridge(), SIGNAL(updateRegisters()), this, SLOT(updateRegistersSlot()));
    connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(displayCustomContextMenuSlot(QPoint)));
    connect(Bridge::getBridge(), SIGNAL(dbgStateChanged(DBGSTATE)), this, SLOT(debugStateChangedSlot(DBGSTATE)));
    connect(parent->getDisasmWidget(), SIGNAL(selectionChanged(dsint)), this, SLOT(disasmSelectionChangedSlot(dsint)));
    // self communication for repainting (maybe some other widgets needs this information, too)
    connect(this, SIGNAL(refresh()), this, SLOT(reload()));
    // context menu actions
    connect(wCM_Increment, SIGNAL(triggered()), this, SLOT(onIncrementAction()));
    connect(wCM_ChangeFPUView, SIGNAL(triggered()), this, SLOT(onChangeFPUViewAction()));
    connect(wCM_Decrement, SIGNAL(triggered()), this, SLOT(onDecrementAction()));
    connect(wCM_Incrementx87Stack, SIGNAL(triggered()), this, SLOT(onIncrementx87StackAction()));
    connect(wCM_Decrementx87Stack, SIGNAL(triggered()), this, SLOT(onDecrementx87StackAction()));
    connect(wCM_Zero, SIGNAL(triggered()), this, SLOT(onZeroAction()));
    connect(wCM_SetToOne, SIGNAL(triggered()), this, SLOT(onSetToOneAction()));
    connect(wCM_Modify, SIGNAL(triggered()), this, SLOT(onModifyAction()));
    connect(wCM_ToggleValue, SIGNAL(triggered()), this, SLOT(onToggleValueAction()));
    connect(wCM_Undo, SIGNAL(triggered()), this, SLOT(onUndoAction()));
    connect(wCM_CopyToClipboard, SIGNAL(triggered()), this, SLOT(onCopyToClipboardAction()));
    connect(wCM_CopyFloatingPointValueToClipboard, SIGNAL(triggered()), this, SLOT(onCopyFloatingPointToClipboardAction()));
    connect(wCM_CopySymbolToClipboard, SIGNAL(triggered()), this, SLOT(onCopySymbolToClipboardAction()));
    connect(wCM_CopyAll, SIGNAL(triggered()), this, SLOT(onCopyAllAction()));
    connect(wCM_FollowInDisassembly, SIGNAL(triggered()), this, SLOT(onFollowInDisassembly()));
    connect(wCM_FollowInDump, SIGNAL(triggered()), this, SLOT(onFollowInDump()));
    connect(wCM_FollowInStack, SIGNAL(triggered()), this, SLOT(onFollowInStack()));
    connect(wCM_FollowInMemoryMap, SIGNAL(triggered()), this, SLOT(onFollowInMemoryMap()));
    connect(wCM_IncrementPtrSize, SIGNAL(triggered()), this, SLOT(onIncrementPtrSize()));
    connect(wCM_DecrementPtrSize, SIGNAL(triggered()), this, SLOT(onDecrementPtrSize()));
    connect(wCM_Push, SIGNAL(triggered()), this, SLOT(onPushAction()));
    connect(wCM_Pop, SIGNAL(triggered()), this, SLOT(onPopAction()));
    connect(wCM_Highlight, SIGNAL(triggered()), this, SLOT(onHighlightSlot()));

    refreshShortcutsSlot();
    connect(Config(), SIGNAL(shortcutsUpdated()), this, SLOT(refreshShortcutsSlot()));
}

void RegistersView::refreshShortcutsSlot()
{
    wCM_Increment->setShortcut(ConfigShortcut("ActionIncreaseRegister"));
    wCM_Decrement->setShortcut(ConfigShortcut("ActionDecreaseRegister"));
    wCM_Zero->setShortcut(ConfigShortcut("ActionZeroRegister"));
    wCM_SetToOne->setShortcut(ConfigShortcut("ActionSetOneRegister"));
    wCM_ToggleValue->setShortcut(ConfigShortcut("ActionToggleRegisterValue"));
    wCM_CopyToClipboard->setShortcut(ConfigShortcut("ActionCopy"));
    wCM_CopySymbolToClipboard->setShortcut(ConfigShortcut("ActionCopySymbol"));
    wCM_CopyAll->setShortcut(ConfigShortcut("ActionCopyAllRegisters"));
    wCM_Highlight->setShortcut(ConfigShortcut("ActionHighlightingMode"));
    wCM_IncrementPtrSize->setShortcut(ConfigShortcut("ActionIncreaseRegisterPtrSize"));
    wCM_DecrementPtrSize->setShortcut(ConfigShortcut("ActionDecreaseRegisterPtrSize"));
    wCM_Incrementx87Stack->setShortcut(ConfigShortcut("ActionIncrementx87Stack"));
    wCM_Decrementx87Stack->setShortcut(ConfigShortcut("ActionDecrementx87Stack"));
    wCM_Push->setShortcut(ConfigShortcut("ActionPush"));
    wCM_Pop->setShortcut(ConfigShortcut("ActionPop"));
}

/**
 * @brief RegistersView::~RegistersView The destructor.
 */
RegistersView::~RegistersView()
{
}

void RegistersView::onClose()
{
    duint cfg = 0;
    switch(wSIMDRegDispMode)
    {
    case SIMD_REG_DISP_HEX:
        cfg = 0;
        break;
    case SIMD_REG_DISP_FLOAT:
        cfg = 1;
        break;
    case SIMD_REG_DISP_DOUBLE:
        cfg = 2;
        break;
    case SIMD_REG_DISP_WORD_SIGNED:
        cfg = 3;
        break;
    case SIMD_REG_DISP_DWORD_SIGNED:
        cfg = 4;
        break;
    case SIMD_REG_DISP_QWORD_SIGNED:
        cfg = 5;
        break;
    case SIMD_REG_DISP_WORD_UNSIGNED:
        cfg = 6;
        break;
    case SIMD_REG_DISP_DWORD_UNSIGNED:
        cfg = 7;
        break;
    case SIMD_REG_DISP_QWORD_UNSIGNED:
        cfg = 8;
        break;
    case SIMD_REG_DISP_WORD_HEX:
        cfg = 9;
        break;
    case SIMD_REG_DISP_DWORD_HEX:
        cfg = 10;
        break;
    case SIMD_REG_DISP_QWORD_HEX:
        cfg = 11;
        break;
    }
    Config()->setUint("Gui", "SIMDRegistersDisplayMode", cfg);
}

void RegistersView::fontsUpdatedSlot()
{
    auto font = ConfigFont("Registers");
    setFont(font);
    if(mChangeViewButton)
        mChangeViewButton->setFont(font);
    //update metrics information
    int wRowsHeight = QFontMetrics(this->font()).height();
    wRowsHeight = (wRowsHeight * 105) / 100;
    wRowsHeight = (wRowsHeight % 2) == 0 ? wRowsHeight : wRowsHeight + 1;
    mRowHeight = wRowsHeight;
    mCharWidth = QFontMetrics(this->font()).averageCharWidth();

    //reload layout because the layout is dependent on the font.
    InitMappings();
    //adjust the height of the area.
    setFixedHeight(getEstimateHeight());
    reload();
}

void RegistersView::ShowFPU(bool set_showfpu)
{
    mShowFpu = set_showfpu;
    InitMappings();
    reload();
}


/**
 * @brief retrieves the register id from given corrdinates of the viewport
 * @param line
 * @param offset (x-coord)
 * @param resulting register-id
 * @return true if register found
 */
bool RegistersView::identifyRegister(const int line, const int offset, REGISTER_NAME* clickedReg)
{
    // we start by an unknown register id
    if(clickedReg)
        *clickedReg = UNKNOWN;
    bool found_flag = false;
    QMap<REGISTER_NAME, Register_Position>::const_iterator it = mRegisterPlaces.begin();
    // iterate all registers that being displayed
    while(it != mRegisterPlaces.end())
    {
        if((it.value().line == (line - mVScrollOffset))    /* same line ? */
                && ((1 + it.value().start) <= offset)   /* between start ... ? */
                && (offset <= (1 + it.value().start + it.value().labelwidth + it.value().valuesize)) /* ... and end ? */
          )
        {
            // we found a matching register in the viewport
            if(clickedReg)
                *clickedReg = (REGISTER_NAME)it.key();
            found_flag = true;
            break;

        }
        ++it;
    }
    return found_flag;
}

/**
 * @brief RegistersView::helpRegister Get the help information of a register. The help information is from Intel's manual.
 * @param reg The register name
 * @return The help information, possibly translated to the native language, or empty if the help information is not available yet.
 */
QString RegistersView::helpRegister(REGISTER_NAME reg)
{
    switch(reg)
    {
    //We don't display help messages for general purpose registers as they are very well-known.
    case CF:
        return tr("CF (bit 0) : Carry flag - Set if an arithmetic operation generates a carry or a borrow out of the mostsignificant bit of the result; cleared otherwise.\n"
                  "This flag indicates an overflow condition for unsigned-integer arithmetic. It is also used in multiple-precision arithmetic.");
    case PF:
        return tr("PF (bit 2) : Parity flag - Set if the least-significant byte of the result contains an even number of 1 bits; cleared otherwise.");
    case AF:
        return tr("AF (bit 4) : Auxiliary Carry flag - Set if an arithmetic operation generates a carry or a borrow out of bit\n"
                  "3 of the result; cleared otherwise. This flag is used in binary-coded decimal (BCD) arithmetic.");
    case ZF:
        return tr("ZF (bit 6) : Zero flag - Set if the result is zero; cleared otherwise.");
    case SF:
        return tr("SF (bit 7) : Sign flag - Set equal to the most-significant bit of the result, which is the sign bit of a signed\n"
                  "integer. (0 indicates a positive value and 1 indicates a negative value.)");
    case OF:
        return tr("OF (bit 11) : Overflow flag - Set if the integer result is too large a positive number or too small a negative\n"
                  "number (excluding the sign-bit) to fit in the destination operand; cleared otherwise. This flag indicates an overflow\n"
                  "condition for signed-integer (two’s complement) arithmetic.");
    case DF:
        return tr("DF (bit 10) : The direction flag controls string instructions (MOVS, CMPS, SCAS, LODS, and STOS). Setting the DF flag causes the string instructions\n"
                  "to auto-decrement (to process strings from high addresses to low addresses). Clearing the DF flag causes the string instructions to auto-increment\n"
                  "(process strings from low addresses to high addresses).");
    case TF:
        return tr("TF (bit 8) : Trap flag - Set to enable single-step mode for debugging; clear to disable single-step mode.");
    case IF:
        return tr("IF (bit 9) : Interrupt enable flag - Controls the response of the processor to maskable interrupt requests. Set to respond to maskable interrupts; cleared to inhibit maskable interrupts.");
    case x87ControlWord:
        return tr("The 16-bit x87 FPU control word controls the precision of the x87 FPU and rounding method used. It also contains the x87 FPU floating-point exception mask bits.");
    case x87StatusWord:
        return tr("The 16-bit x87 FPU status register indicates the current state of the x87 FPU.");
    case x87TagWord:
        return tr("The 16-bit tag word indicates the contents of each the 8 registers in the x87 FPU data-register stack (one 2-bit tag per register).");

    case x87CW_PC:
        return tr("The precision-control (PC) field (bits 8 and 9 of the x87 FPU control word) determines the precision (64, 53, or 24 bits) of floating-point calculations made by the x87 FPU");
    case x87CW_RC:
        return tr("The rounding-control (RC) field of the x87 FPU control register (bits 10 and 11) controls how the results of x87 FPU floating-point instructions are rounded.");
    case x87CW_IC:
        return tr("The infinity control flag (bit 12 of the x87 FPU control word) is provided for compatibility with the Intel 287 Math Coprocessor;\n"
                  "it is not meaningful for later version x87 FPU coprocessors or IA-32 processors.");
    case x87CW_IM:
        return tr("The invalid operation exception mask (bit 0). When the mask bit is set, its corresponding exception is blocked from being generated.");
    case x87CW_DM:
        return tr("The denormal-operand exception mask (bit 2). When the mask bit is set, its corresponding exception is blocked from being generated.");
    case x87CW_ZM:
        return tr("The floating-point divide-by-zero exception mask (bit 3). When the mask bit is set, its corresponding exception is blocked from being generated.");
    case x87CW_OM:
        return tr("The floating-point numeric overflow exception mask (bit 4). When the mask bit is set, its corresponding exception is blocked from being generated.");
    case x87CW_UM:
        return tr("The potential floating-point numeric underflow condition mask (bit 5). When the mask bit is set, its corresponding exception is blocked from being generated.");
    case x87CW_PM:
        return tr("The inexact-result/precision exception mask (bit 6). When the mask bit is set, its corresponding exception is blocked from being generated.");

    case x87SW_B:
        return tr("The busy flag (bit 15) indicates if the FPU is busy (B=1) while executing an instruction, or is idle (B=0).\n"
                  "The B-bit (bit 15) is included for 8087 compatibility only. It reflects the contents of the ES flag.");
    case x87SW_C0:
        return tr("The C%1 condition code flag (bit %2) is used to indicate the results of floating-point comparison and arithmetic operations.").arg(0).arg(8);
    case x87SW_C1:
        return tr("The C%1 condition code flag (bit %2) is used to indicate the results of floating-point comparison and arithmetic operations.").arg(1).arg(9);
    case x87SW_C2:
        return tr("The C%1 condition code flag (bit %2) is used to indicate the results of floating-point comparison and arithmetic operations.").arg(2).arg(10);
    case x87SW_C3:
        return tr("The C%1 condition code flag (bit %2) is used to indicate the results of floating-point comparison and arithmetic operations.").arg(3).arg(14);
    case x87SW_ES:
        return tr("The error/exception summary status flag (bit 7) is set when any of the unmasked exception flags are set.");
    case x87SW_SF:
        return tr("The stack fault flag (bit 6 of the x87 FPU status word) indicates that stack overflow or stack underflow has occurred with data\nin the x87 FPU data register stack.");
    case x87SW_TOP:
        return tr("A pointer to the x87 FPU data register that is currently at the top of the x87 FPU register stack is contained in bits 11 through 13\n"
                  "of the x87 FPU status word. This pointer, which is commonly referred to as TOP (for top-of-stack), is a binary value from 0 to 7.");
    case x87SW_I:
        return tr("The processor reports an invalid operation exception (bit 0) in response to one or more invalid arithmetic operands.");
    case x87SW_D:
        return tr("The processor reports the denormal-operand exception (bit 2) if an arithmetic instruction attempts to operate on a denormal operand.");
    case x87SW_Z:
        return tr("The processor reports the floating-point divide-by-zero exception (bit 3) whenever an instruction attempts to divide a finite non-zero operand by 0.");
    case x87SW_O:
        return tr("The processor reports a floating-point numeric overflow exception (bit 4) whenever the rounded result of an instruction exceeds the largest allowable finite value that will fit into the destination operand.");
    case x87SW_U:
        return tr("The processor detects a potential floating-point numeric underflow condition (bit 5) whenever the result of rounding with unbounded exponent is non-zero and tiny.");
    case x87SW_P:
        return tr("The inexact-result/precision exception (bit 6) occurs if the result of an operation is not exactly representable in the destination format.");

    case MxCsr:
        return tr("The 32-bit MXCSR register contains control and status information for SIMD floating-point operations.");
    case MxCsr_IE:
        return tr("Bit 0 (IE) : Invalid Operation Flag; indicate whether a SIMD floating-point exception has been detected.");
    case MxCsr_DE:
        return tr("Bit 1 (DE) : Denormal Flag; indicate whether a SIMD floating-point exception has been detected.");
    case MxCsr_ZE:
        return tr("Bit 2 (ZE) : Divide-by-Zero Flag; indicate whether a SIMD floating-point exception has been detected.");
    case MxCsr_OE:
        return tr("Bit 3 (OE) : Overflow Flag; indicate whether a SIMD floating-point exception has been detected.");
    case MxCsr_UE:
        return tr("Bit 4 (UE) : Underflow Flag; indicate whether a SIMD floating-point exception has been detected.");
    case MxCsr_PE:
        return tr("Bit 5 (PE) : Precision Flag; indicate whether a SIMD floating-point exception has been detected.");
    case MxCsr_IM:
        return tr("Bit 7 (IM) : Invalid Operation Mask. When the mask bit is set, its corresponding exception is blocked from being generated.");
    case MxCsr_DM:
        return tr("Bit 8 (DM) : Denormal Mask. When the mask bit is set, its corresponding exception is blocked from being generated.");
    case MxCsr_ZM:
        return tr("Bit 9 (ZM) : Divide-by-Zero Mask. When the mask bit is set, its corresponding exception is blocked from being generated.");
    case MxCsr_OM:
        return tr("Bit 10 (OM) : Overflow Mask. When the mask bit is set, its corresponding exception is blocked from being generated.");
    case MxCsr_UM:
        return tr("Bit 11 (UM) : Underflow Mask. When the mask bit is set, its corresponding exception is blocked from being generated.");
    case MxCsr_PM:
        return tr("Bit 12 (PM) : Precision Mask. When the mask bit is set, its corresponding exception is blocked from being generated.");
    case MxCsr_FZ:
        return tr("Bit 15 (FZ) of the MXCSR register enables the flush-to-zero mode, which controls the masked response to a SIMD floating-point underflow condition.");
    case MxCsr_DAZ:
        return tr("Bit 6 (DAZ) of the MXCSR register enables the denormals-are-zeros mode, which controls the processor’s response to a SIMD floating-point\n"
                  "denormal operand condition.");
    case MxCsr_RC:
        return tr("Bits 13 and 14 of the MXCSR register (the rounding control [RC] field) control how the results of SIMD floating-point instructions are rounded.");
    case LastError:
    {
        char dat[1024];
        LASTERROR* error;
        error = (LASTERROR*)registerValue(&wRegDumpStruct, LastError);
        if(DbgFunctions()->StringFormatInline(QString().sprintf("{winerror@%X}", error->code).toUtf8().constData(), sizeof(dat), dat))
            return dat;
        else
            return tr("The value of GetLastError(). This value is stored in the TEB.");
    }
    case LastStatus:
    {
        char dat[1024];
        LASTSTATUS* error;
        error = (LASTSTATUS*)registerValue(&wRegDumpStruct, LastStatus);
        if(DbgFunctions()->StringFormatInline(QString().sprintf("{ntstatus@%X}", error->code).toUtf8().constData(), sizeof(dat), dat))
            return dat;
        else
            return tr("The NTSTATUS in the LastStatusValue field of the TEB.");
    }
#ifdef _WIN64
    case GS:
        return tr("The TEB of the current thread can be accessed as an offset of segment register GS (x64).\nThe TEB can be used to get a lot of information on the process without calling Win32 API.");
#else //x86
    case FS:
        return tr("The TEB of the current thread can be accessed as an offset of segment register FS (x86).\nThe TEB can be used to get a lot of information on the process without calling Win32 API.");
#endif //_WIN64
    default:
        return QString();
    }
}

void RegistersView::CreateDumpNMenu(QMenu* dumpMenu)
{
    QList<QString> names;
    CPUMultiDump* multiDump = mParent->getDumpWidget();
    dumpMenu->setIcon(DIcon("dump.png"));
    int maxDumps = multiDump->getMaxCPUTabs();
    multiDump->getTabNames(names);
    for(int i = 0; i < maxDumps; i++)
    {
        QAction* action = new QAction(names.at(i), this);
        connect(action, SIGNAL(triggered()), this, SLOT(onFollowInDumpN()));
        dumpMenu->addAction(action);
        action->setData(i + 1);
    }
}

void RegistersView::mousePressEvent(QMouseEvent* event)
{
    if(!DbgIsDebugging())
        return;

    if(event->y() < yTopSpacing - mButtonHeight)
    {
        onChangeFPUViewAction();
    }
    else
    {
        // get mouse position
        const int y = (event->y() - yTopSpacing) / (double)mRowHeight;
        const int x = event->x() / (double)mCharWidth;

        REGISTER_NAME r;
        // do we find a corresponding register?
        if(identifyRegister(y, x, &r))
        {
            Disassembly* CPUDisassemblyView = mParent->getDisasmWidget();
            if(CPUDisassemblyView->isHighlightMode())
            {
                if(mGPR.contains(r) && r != REGISTER_NAME::EFLAGS)
                    CPUDisassemblyView->hightlightToken(CapstoneTokenizer::SingleToken(CapstoneTokenizer::TokenType::GeneralRegister, mRegisterMapping.constFind(r).value()));
                else if(mFPUMMX.contains(r))
                    CPUDisassemblyView->hightlightToken(CapstoneTokenizer::SingleToken(CapstoneTokenizer::TokenType::MmxRegister, mRegisterMapping.constFind(r).value()));
                else if(mFPUXMM.contains(r))
                    CPUDisassemblyView->hightlightToken(CapstoneTokenizer::SingleToken(CapstoneTokenizer::TokenType::XmmRegister, mRegisterMapping.constFind(r).value()));
                else if(mFPUYMM.contains(r))
                    CPUDisassemblyView->hightlightToken(CapstoneTokenizer::SingleToken(CapstoneTokenizer::TokenType::YmmRegister, mRegisterMapping.constFind(r).value()));
                else if(mSEGMENTREGISTER.contains(r))
                    CPUDisassemblyView->hightlightToken(CapstoneTokenizer::SingleToken(CapstoneTokenizer::TokenType::MemorySegment, mRegisterMapping.constFind(r).value()));
                else
                    mSelected = r;
            }
            else
                mSelected = r;
            emit refresh();
        }
        else
            mSelected = UNKNOWN;
    }
}

void RegistersView::mouseMoveEvent(QMouseEvent* event)
{
    if(!DbgIsDebugging())
    {
        QScrollArea::mouseMoveEvent(event);
        setCursor(QCursor(Qt::ArrowCursor));
        return;
    }

    REGISTER_NAME r = REGISTER_NAME::UNKNOWN;
    QString registerHelpInformation;
    // do we find a corresponding register?
    if(identifyRegister((event->y() - yTopSpacing) / (double)mRowHeight, event->x() / (double)mCharWidth, &r))
    {
        registerHelpInformation = helpRegister(r).replace(" : ", ": ");
        setCursor(QCursor(Qt::PointingHandCursor));
    }
    else
    {
        registerHelpInformation = "";
        setCursor(QCursor(Qt::ArrowCursor));
    }
    if(!registerHelpInformation.isEmpty())
        QToolTip::showText(event->globalPos(), registerHelpInformation);
    else
        QToolTip::hideText();
    QScrollArea::mouseMoveEvent(event);
}

void RegistersView::mouseDoubleClickEvent(QMouseEvent* event)
{
    if(!DbgIsDebugging() || event->button() != Qt::LeftButton)
        return;
    // get mouse position
    const int y = (event->y() - yTopSpacing) / (double)mRowHeight;
    const int x = event->x() / (double)mCharWidth;

    // do we find a corresponding register?
    if(!identifyRegister(y, x, 0))
        return;
    if(mSelected == CIP) //double clicked on CIP register
        DbgCmdExec("disasm cip");
    // is current register general purposes register or FPU register?
    else if(mMODIFYDISPLAY.contains(mSelected))
        wCM_Modify->trigger();
    else if(mBOOLDISPLAY.contains(mSelected)) // is flag ?
        wCM_ToggleValue->trigger();
    else if(mCANSTOREADDRESS.contains(mSelected))
        wCM_FollowInDisassembly->trigger();
}

void RegistersView::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    if(mChangeViewButton != NULL)
    {
        if(mShowFpu)
            mChangeViewButton->setText(tr("Hide FPU"));
        else
            mChangeViewButton->setText(tr("Show FPU"));
    }

    QPainter wPainter(this->viewport());
    wPainter.fillRect(wPainter.viewport(), QBrush(ConfigColor("RegistersBackgroundColor")));

    // Don't draw the registers if a program isn't actually running
    if(!DbgIsDebugging())
        return;

    // Iterate all registers
    for(auto itr = mRegisterMapping.begin(); itr != mRegisterMapping.end(); itr++)
    {
        // Paint register at given position
        drawRegister(&wPainter, itr.key(), registerValue(&wRegDumpStruct, itr.key()));
    }
}

void RegistersView::keyPressEvent(QKeyEvent* event)
{
    if(DbgIsDebugging())
    {
        int key = event->key();
        if(key == Qt::Key_Enter || key == Qt::Key_Return)
            wCM_Modify->trigger();
    }
    QScrollArea::keyPressEvent(event);
}

QSize RegistersView::sizeHint() const
{
    // 32 character width
    return QSize(32 * mCharWidth, this->viewport()->height());
}

void* RegistersView::operator new(size_t size)
{
    return _aligned_malloc(size, 16);
}

void RegistersView::operator delete(void* p)
{
    _aligned_free(p);
}

/**
 * @brief                   Get the label associated with the register
 * @param register_selected the register
 * @return                  the label
 */
QString RegistersView::getRegisterLabel(REGISTER_NAME register_selected)
{
    char label_text[MAX_LABEL_SIZE] = "";
    char module_text[MAX_MODULE_SIZE] = "";
    char string_text[MAX_STRING_SIZE] = "";

    QString valueText = QString("%1").arg((* ((duint*) registerValue(&wRegDumpStruct, register_selected))), mRegisterPlaces[register_selected].valuesize, 16, QChar('0')).toUpper();
    duint register_value = (* ((duint*) registerValue(&wRegDumpStruct, register_selected)));
    QString newText = QString("");

    bool hasString = DbgGetStringAt(register_value, string_text);
    bool hasLabel = DbgGetLabelAt(register_value, SEG_DEFAULT, label_text);
    bool hasModule = DbgGetModuleAt(register_value, module_text);

    if(hasString && !mONLYMODULEANDLABELDISPLAY.contains(register_selected))
    {
        newText = string_text;
    }
    else if(hasLabel && hasModule)
    {
        newText = "<" + QString(module_text) + "." + QString(label_text) + ">";
    }
    else if(hasModule)
    {
        newText = QString(module_text) + "." + valueText;
    }
    else if(hasLabel)
    {
        newText = "<" + QString(label_text) + ">";
    }
    else if(!mONLYMODULEANDLABELDISPLAY.contains(register_selected))
    {
        if(register_value == (register_value & 0xFF))
        {
            QChar c = QChar((char)register_value);
            if(c.isPrint())
            {
                newText = QString("'%1'").arg((char)register_value);
            }
        }
        else if(register_value == (register_value & 0xFFF)) //UNICODE?
        {
            QChar c = QChar((wchar_t)register_value);
            if(c.isPrint())
            {
                newText = "L'" + QString(c) + "'";
            }
        }
    }

    return std::move(newText);
}

static QString fillValue(const char* value, int valsize = 2, bool bFpuRegistersLittleEndian = false)
{
    if(bFpuRegistersLittleEndian)
        return QString(QByteArray(value, valsize).toHex()).toUpper();
    else // Big Endian
        return QString(ByteReverse(QByteArray(value, valsize)).toHex()).toUpper();
}

static QString composeRegTextXMM(const char* value, RegistersView::SIMD_REG_DISP_MODE wSIMDRegDispMode, bool bFpuRegistersLittleEndian)
{
    QString valueText;
    switch(wSIMDRegDispMode)
    {
    default:
    case RegistersView::SIMD_REG_DISP_HEX:
    {
        valueText = fillValue(value, 16, bFpuRegistersLittleEndian);
    }
    break;
    case RegistersView::SIMD_REG_DISP_DOUBLE:
    {
        const double* dbl_values = reinterpret_cast<const double*>(value);
        if(bFpuRegistersLittleEndian)
            valueText = ToDoubleString(&dbl_values[0]) + ' ' + ToDoubleString(&dbl_values[1]);
        else // Big Endian
            valueText = ToDoubleString(&dbl_values[1]) + ' ' + ToDoubleString(&dbl_values[0]);
    }
    break;
    case RegistersView::SIMD_REG_DISP_FLOAT:
    {
        const float* flt_values = reinterpret_cast<const float*>(value);
        if(bFpuRegistersLittleEndian)
            valueText = ToFloatString(&flt_values[0]) + ' ' + ToFloatString(&flt_values[1]) + ' '
                      + ToFloatString(&flt_values[2]) + ' ' + ToFloatString(&flt_values[3]);
        else // Big Endian
            valueText = ToFloatString(&flt_values[3]) + ' ' + ToFloatString(&flt_values[2]) + ' '
                      + ToFloatString(&flt_values[1]) + ' ' + ToFloatString(&flt_values[0]);
    }
    break;
    case RegistersView::SIMD_REG_DISP_WORD_HEX:
    {
        if(bFpuRegistersLittleEndian)
            valueText = fillValue(value) + ' ' + fillValue(value + 1 * 2) + ' ' + fillValue(value + 2 * 2) + ' ' + fillValue(value + 3 * 2)
                        + ' ' + fillValue(value + 4 * 2) + ' ' + fillValue(value + 5 * 2) + ' ' + fillValue(value + 6 * 2) + ' ' + fillValue(value + 7 * 2);
        else // Big Endian
            valueText = fillValue(value + 7 * 2) + ' ' + fillValue(value + 6 * 2) + ' ' + fillValue(value + 5 * 2) + ' ' + fillValue(value + 4 * 2)
                        + ' ' + fillValue(value + 3 * 2) + ' ' + fillValue(value + 2 * 2) + ' ' + fillValue(value + 1 * 2) + ' ' + fillValue(value);
    }
    break;
    case RegistersView::SIMD_REG_DISP_WORD_SIGNED:
    {
        const short* sword_values = reinterpret_cast<const short*>(value);
        if(bFpuRegistersLittleEndian)
            valueText = QString::number(sword_values[0]) + ' ' + QString::number(sword_values[1]) + ' ' + QString::number(sword_values[2]) + ' ' + QString::number(sword_values[3])
                        + ' ' + QString::number(sword_values[4]) + ' ' + QString::number(sword_values[5]) + ' ' + QString::number(sword_values[6]) + ' ' + QString::number(sword_values[7]);
        else // Big Endian
            valueText = QString::number(sword_values[7]) + ' ' + QString::number(sword_values[6]) + ' ' + QString::number(sword_values[5]) + ' ' + QString::number(sword_values[4])
                        + ' ' + QString::number(sword_values[3]) + ' ' + QString::number(sword_values[2]) + ' ' + QString::number(sword_values[1]) + ' ' + QString::number(sword_values[0]);
    }
    break;
    case RegistersView::SIMD_REG_DISP_WORD_UNSIGNED:
    {
        const unsigned short* uword_values = reinterpret_cast<const unsigned short*>(value);
        if(bFpuRegistersLittleEndian)
            valueText = QString::number(uword_values[0]) + ' ' + QString::number(uword_values[1]) + ' ' + QString::number(uword_values[2]) + ' ' + QString::number(uword_values[3])
                        + ' ' + QString::number(uword_values[4]) + ' ' + QString::number(uword_values[5]) + ' ' + QString::number(uword_values[6]) + ' ' + QString::number(uword_values[7]);
        else // Big Endian
            valueText = QString::number(uword_values[7]) + ' ' + QString::number(uword_values[6]) + ' ' + QString::number(uword_values[5]) + ' ' + QString::number(uword_values[4])
                        + ' ' + QString::number(uword_values[3]) + ' ' + QString::number(uword_values[2]) + ' ' + QString::number(uword_values[1]) + ' ' + QString::number(uword_values[0]);
    }
    break;
    case RegistersView::SIMD_REG_DISP_DWORD_HEX:
    {
        if(bFpuRegistersLittleEndian)
            valueText = fillValue(value, 4) + ' ' +  fillValue(value + 1 * 4, 4) + ' ' +  fillValue(value + 2 * 4, 4) + ' ' +  fillValue(value + 3 * 4, 4);
        else // Big Endian
            valueText = fillValue(value + 3 * 4, 4) + ' ' +  fillValue(value + 2 * 4, 4) + ' ' +  fillValue(value + 1 * 4, 4) + ' ' +  fillValue(value, 4);
    }
    break;
    case RegistersView::SIMD_REG_DISP_DWORD_SIGNED:
    {
        const int* sdword_values = reinterpret_cast<const int*>(value);
        if(bFpuRegistersLittleEndian)
            valueText = QString::number(sdword_values[0]) + ' ' + QString::number(sdword_values[1]) + ' ' + QString::number(sdword_values[2]) + ' ' + QString::number(sdword_values[3]);
        else // Big Endian
            valueText = QString::number(sdword_values[3]) + ' ' + QString::number(sdword_values[2]) + ' ' + QString::number(sdword_values[1]) + ' ' + QString::number(sdword_values[0]);
    }
    break;
    case RegistersView::SIMD_REG_DISP_DWORD_UNSIGNED:
    {
        const unsigned int* udword_values = reinterpret_cast<const unsigned int*>(value);
        if(bFpuRegistersLittleEndian)
            valueText = QString::number(udword_values[0]) + ' ' + QString::number(udword_values[1]) + ' ' + QString::number(udword_values[2]) + ' ' + QString::number(udword_values[3]);
        else // Big Endian
            valueText = QString::number(udword_values[3]) + ' ' + QString::number(udword_values[2]) + ' ' + QString::number(udword_values[1]) + ' ' + QString::number(udword_values[0]);
    }
    break;
    case RegistersView::SIMD_REG_DISP_QWORD_HEX:
    {
        if(bFpuRegistersLittleEndian)
            valueText = fillValue(value, 8) + ' ' + fillValue(value + 8, 8);
        else // Big Endian
            valueText = fillValue(value + 8, 8) + ' ' + fillValue(value, 8);
    }
    break;
    case RegistersView::SIMD_REG_DISP_QWORD_SIGNED:
    {
        const long long* sqword_values = reinterpret_cast<const long long*>(value);
        if(bFpuRegistersLittleEndian)
            valueText = QString::number(sqword_values[0]) + ' ' + QString::number(sqword_values[1]);
        else // Big Endian
            valueText = QString::number(sqword_values[1]) + ' ' + QString::number(sqword_values[0]);
    }
    break;
    case RegistersView::SIMD_REG_DISP_QWORD_UNSIGNED:
    {
        const unsigned long long* uqword_values = reinterpret_cast<const unsigned long long*>(value);
        if(bFpuRegistersLittleEndian)
            valueText = QString::number(uqword_values[0]) + ' ' + QString::number(uqword_values[1]);
        else // Big Endian
            valueText = QString::number(uqword_values[1]) + ' ' + QString::number(uqword_values[0]);
    }
    break;
    }
    return std::move(valueText);
}

/**
 * @brief RegistersView::GetRegStringValueFromValue Get the textual representation of the register value.
 * @param reg The name of the register
 * @param value The current value of the register
 * @return The textual representation of the register value
 *
 * This value does not return hex representation all the times for SIMD registers. The actual representation of SIMD registers depends on the user settings.
 */
QString RegistersView::GetRegStringValueFromValue(REGISTER_NAME reg, const char* value)
{
    QString valueText;

    if(mUINTDISPLAY.contains(reg))
        valueText = QString("%1").arg((* ((const duint*) value)), mRegisterPlaces[reg].valuesize, 16, QChar('0')).toUpper();
    else if(mUSHORTDISPLAY.contains(reg))
        valueText = QString("%1").arg((* ((const unsigned short*) value)), mRegisterPlaces[reg].valuesize, 16, QChar('0')).toUpper();
    else if(mDWORDDISPLAY.contains(reg))
        valueText = QString("%1").arg((* ((const DWORD*) value)), mRegisterPlaces[reg].valuesize, 16, QChar('0')).toUpper();
    else if(mBOOLDISPLAY.contains(reg))
        valueText = QString("%1").arg((* ((const bool*) value)), mRegisterPlaces[reg].valuesize, 16, QChar('0')).toUpper();
    else if(mFIELDVALUE.contains(reg))
    {
        if(mTAGWORD.contains(reg))
        {
            valueText = QString("%1").arg((* ((const unsigned short*) value)), 1, 16, QChar('0')).toUpper();
            valueText += QString(" (");
            valueText += GetTagWordStateString((* ((const unsigned short*) value)));
            valueText += QString(")");
        }
        if(reg == MxCsr_RC)
        {
            valueText = QString("%1").arg((* ((const unsigned short*) value)), 1, 16, QChar('0')).toUpper();
            valueText += QString(" (");
            valueText += GetMxCsrRCStateString((* ((const unsigned short*) value)));
            valueText += QString(")");
        }
        else if(reg == x87CW_RC)
        {
            valueText = QString("%1").arg((* ((const unsigned short*) value)), 1, 16, QChar('0')).toUpper();
            valueText += QString(" (");
            valueText += GetControlWordRCStateString((* ((const unsigned short*) value)));
            valueText += QString(")");
        }
        else if(reg == x87CW_PC)
        {
            valueText = QString("%1").arg((* ((const unsigned short*) value)), 1, 16, QChar('0')).toUpper();
            valueText += QString(" (");
            valueText += GetControlWordPCStateString((* ((const unsigned short*) value)));
            valueText += QString(")");
        }
        else if(reg == x87SW_TOP)
        {
            valueText = QString("%1").arg((* ((const unsigned short*) value)), 1, 16, QChar('0')).toUpper();
            valueText += QString(" (ST0=");
            valueText += GetStatusWordTOPStateString((* ((const unsigned short*) value)));
            valueText += QString(")");
        }
    }
    else if(reg == LastError)
    {
        LASTERROR* data = (LASTERROR*)value;
        if(*data->name)
            valueText = QString().sprintf("%08X (%s)", data->code, data->name);
        else
            valueText = QString().sprintf("%08X", data->code);
        mRegisterPlaces[LastError].valuesize = valueText.length();
    }
    else if(reg == LastStatus)
    {
        LASTSTATUS* data = (LASTSTATUS*)value;
        if(*data->name)
            valueText = QString().sprintf("%08X (%s)", data->code, data->name);
        else
            valueText = QString().sprintf("%08X", data->code);
        mRegisterPlaces[LastStatus].valuesize = valueText.length();
    }
    else
    {
        SIZE_T size = GetSizeRegister(reg);
        bool bFpuRegistersLittleEndian = ConfigBool("Gui", "FpuRegistersLittleEndian");
        if(size != 0)
        {
            if(mFPUXMM.contains(reg))
                valueText = composeRegTextXMM(value, wSIMDRegDispMode, bFpuRegistersLittleEndian);
            else if(mFPUYMM.contains(reg))
            {
                if(wSIMDRegDispMode == SIMD_REG_DISP_HEX)
                    valueText = fillValue(value, size, bFpuRegistersLittleEndian);
                else if(bFpuRegistersLittleEndian)
                    valueText = composeRegTextXMM(value, wSIMDRegDispMode, bFpuRegistersLittleEndian) + ' ' + composeRegTextXMM(value + 16, wSIMDRegDispMode, bFpuRegistersLittleEndian);
                else
                    valueText = composeRegTextXMM(value + 16, wSIMDRegDispMode, bFpuRegistersLittleEndian) + ' ' + composeRegTextXMM(value, wSIMDRegDispMode, bFpuRegistersLittleEndian);
            }
            else
                valueText = fillValue(value, size, bFpuRegistersLittleEndian);
        }
        else
            valueText = QString("???");
    }

    return std::move(valueText);
}

#define MxCsr_RC_NEAR 0
#define MxCsr_RC_NEGATIVE 1
#define MxCsr_RC_POSITIVE   2
#define MxCsr_RC_TOZERO 3

STRING_VALUE_TABLE_t MxCsrRCValueStringTable[] =
{
    {QT_TRANSLATE_NOOP("RegistersView_ConstantsOfRegisters", "Toward Zero"), MxCsr_RC_TOZERO},
    {QT_TRANSLATE_NOOP("RegistersView_ConstantsOfRegisters", "Toward Positive"), MxCsr_RC_POSITIVE},
    {QT_TRANSLATE_NOOP("RegistersView_ConstantsOfRegisters", "Toward Negative"), MxCsr_RC_NEGATIVE},
    {QT_TRANSLATE_NOOP("RegistersView_ConstantsOfRegisters", "Round Near"), MxCsr_RC_NEAR}
};
//WARNING: The following function is commented out because it is not used currently. If it is used again, it should be modified to keep working under internationized environment.
/*
unsigned int RegistersView::GetMxCsrRCValueFromString(const char* string)
{
    int i;

    for(i = 0; i < (sizeof(MxCsrRCValueStringTable) / sizeof(*MxCsrRCValueStringTable)); i++)
    {
        if(MxCsrRCValueStringTable[i].string == string)
            return MxCsrRCValueStringTable[i].value;
    }

    return i;
}
*/
QString RegistersView::GetMxCsrRCStateString(unsigned short state)
{
    int i;

    for(i = 0; i < (sizeof(MxCsrRCValueStringTable) / sizeof(*MxCsrRCValueStringTable)); i++)
    {
        if(MxCsrRCValueStringTable[i].value == state)
            return QApplication::translate("RegistersView_ConstantsOfRegisters", MxCsrRCValueStringTable[i].string);
    }

    return tr("Unknown");
}

#define x87CW_RC_NEAR 0
#define x87CW_RC_DOWN 1
#define x87CW_RC_UP   2
#define x87CW_RC_TRUNCATE 3

STRING_VALUE_TABLE_t ControlWordRCValueStringTable[] =
{
    {QT_TRANSLATE_NOOP("RegistersView_ConstantsOfRegisters", "Truncate"), x87CW_RC_TRUNCATE},
    {QT_TRANSLATE_NOOP("RegistersView_ConstantsOfRegisters", "Round Up"), x87CW_RC_UP},
    {QT_TRANSLATE_NOOP("RegistersView_ConstantsOfRegisters", "Round Down"), x87CW_RC_DOWN},
    {QT_TRANSLATE_NOOP("RegistersView_ConstantsOfRegisters", "Round Near"), x87CW_RC_NEAR}
};
//WARNING: The following function is commented out because it is not used currently. If it is used again, it should be modified to keep working under internationized environment.
/*
unsigned int RegistersView::GetControlWordRCValueFromString(const char* string)
{
    int i;

    for(i = 0; i < (sizeof(ControlWordRCValueStringTable) / sizeof(*ControlWordRCValueStringTable)); i++)
    {
        if(tr(ControlWordRCValueStringTable[i].string) == string)
            return ControlWordRCValueStringTable[i].value;
    }

    return i;
}
*/
QString RegistersView::GetControlWordRCStateString(unsigned short state)
{
    int i;

    for(i = 0; i < (sizeof(ControlWordRCValueStringTable) / sizeof(*ControlWordRCValueStringTable)); i++)
    {
        if(ControlWordRCValueStringTable[i].value == state)
            return QApplication::translate("RegistersView_ConstantsOfRegisters", ControlWordRCValueStringTable[i].string);
    }

    return tr("Unknown");
}

#define x87SW_TOP_0 0
#define x87SW_TOP_1 1
#define x87SW_TOP_2 2
#define x87SW_TOP_3 3
#define x87SW_TOP_4 4
#define x87SW_TOP_5 5
#define x87SW_TOP_6 6
#define x87SW_TOP_7 7
// This string needs not to be internationalized.
STRING_VALUE_TABLE_t StatusWordTOPValueStringTable[] =
{
    {"x87r0", x87SW_TOP_0},
    {"x87r1", x87SW_TOP_1},
    {"x87r2", x87SW_TOP_2},
    {"x87r3", x87SW_TOP_3},
    {"x87r4", x87SW_TOP_4},
    {"x87r5", x87SW_TOP_5},
    {"x87r6", x87SW_TOP_6},
    {"x87r7", x87SW_TOP_7}
};
//WARNING: The following function is commented out because it is not used currently. If it is used again, it should be modified to keep working under internationized environment.
/*
unsigned int RegistersView::GetStatusWordTOPValueFromString(const char* string)
{
    int i;

    for(i = 0; i < (sizeof(StatusWordTOPValueStringTable) / sizeof(*StatusWordTOPValueStringTable)); i++)
    {
        if(StatusWordTOPValueStringTable[i].string == string)
            return StatusWordTOPValueStringTable[i].value;
    }

    return i;
}
*/
QString RegistersView::GetStatusWordTOPStateString(unsigned short state)
{
    int i;

    for(i = 0; i < (sizeof(StatusWordTOPValueStringTable) / sizeof(*StatusWordTOPValueStringTable)); i++)
    {
        if(StatusWordTOPValueStringTable[i].value == state)
            return StatusWordTOPValueStringTable[i].string;
    }

    return tr("Unknown");
}


#define x87CW_PC_REAL4 0
#define x87CW_PC_NOTUSED 1
#define x87CW_PC_REAL8   2
#define x87CW_PC_REAL10 3

STRING_VALUE_TABLE_t ControlWordPCValueStringTable[] =
{
    {QT_TRANSLATE_NOOP("RegistersView_ConstantsOfRegisters", "Real4"), x87CW_PC_REAL4},
    {QT_TRANSLATE_NOOP("RegistersView_ConstantsOfRegisters", "Not Used"), x87CW_PC_NOTUSED},
    {QT_TRANSLATE_NOOP("RegistersView_ConstantsOfRegisters", "Real8"), x87CW_PC_REAL8},
    {QT_TRANSLATE_NOOP("RegistersView_ConstantsOfRegisters", "Real10"), x87CW_PC_REAL10}
};
/*
//WARNING: The following function is commented out because it is not used currently. If it is used again, it should be modified to keep working under internationized environment.
unsigned int RegistersView::GetControlWordPCValueFromString(const char* string)
{
    int i;

    for(i = 0; i < (sizeof(ControlWordPCValueStringTable) / sizeof(*ControlWordPCValueStringTable)); i++)
    {
        if(tr(ControlWordPCValueStringTable[i].string) == string)
            return ControlWordPCValueStringTable[i].value;
    }

    return i;
}
*/

QString RegistersView::GetControlWordPCStateString(unsigned short state)
{
    int i;

    for(i = 0; i < (sizeof(ControlWordPCValueStringTable) / sizeof(*ControlWordPCValueStringTable)); i++)
    {
        if(ControlWordPCValueStringTable[i].value == state)
            return QApplication::translate("RegistersView_ConstantsOfRegisters", ControlWordPCValueStringTable[i].string);
    }

    return tr("Unknown");
}


#define X87FPU_TAGWORD_NONZERO 0
#define X87FPU_TAGWORD_ZERO 1
#define X87FPU_TAGWORD_SPECIAL 2
#define X87FPU_TAGWORD_EMPTY 3

STRING_VALUE_TABLE_t TagWordValueStringTable[] =
{
    {QT_TRANSLATE_NOOP("RegistersView_ConstantsOfRegisters", "Nonzero"), X87FPU_TAGWORD_NONZERO},
    {QT_TRANSLATE_NOOP("RegistersView_ConstantsOfRegisters", "Zero"), X87FPU_TAGWORD_ZERO},
    {QT_TRANSLATE_NOOP("RegistersView_ConstantsOfRegisters", "Special"), X87FPU_TAGWORD_SPECIAL},
    {QT_TRANSLATE_NOOP("RegistersView_ConstantsOfRegisters", "Empty"), X87FPU_TAGWORD_EMPTY}
};
//WARNING: The following function is commented out because it is not used currently. If it is used again, it should be modified to keep working under internationized environment.
/*
unsigned int RegistersView::GetTagWordValueFromString(const char* string)
{
    int i;

    for(i = 0; i < (sizeof(TagWordValueStringTable) / sizeof(*TagWordValueStringTable)); i++)
    {
        if(tr(TagWordValueStringTable[i].string) == string)
            return TagWordValueStringTable[i].value;
    }

    return i;
}

*/
QString RegistersView::GetTagWordStateString(unsigned short state)
{
    int i;

    for(i = 0; i < (sizeof(TagWordValueStringTable) / sizeof(*TagWordValueStringTable)); i++)
    {
        if(TagWordValueStringTable[i].value == state)
            return QApplication::translate("RegistersView_ConstantsOfRegisters", TagWordValueStringTable[i].string);
    }

    return tr("Unknown");
}

void RegistersView::drawRegister(QPainter* p, REGISTER_NAME reg, char* value)
{
    QFontMetrics fontMetrics(font());
    // is the register-id known?
    if(mRegisterMapping.contains(reg))
    {
        // padding to the left is at least one character (looks better)
        int x = mCharWidth * (1 + mRegisterPlaces[reg].start);
        int ySpace = yTopSpacing;
        if(mVScrollOffset != 0)
            ySpace = 0;
        int y = mRowHeight * (mRegisterPlaces[reg].line + mVScrollOffset) + ySpace;

        //draw raster
        /*
        p->save();
        p->setPen(QColor("#FF0000"));
        p->drawLine(0, y, this->viewport()->width(), y);
        p->restore();
        */

        // draw name of value
        int width = fontMetrics.width(mRegisterMapping[reg]);

        // set the color of the register label
#ifdef _WIN64
        switch(reg)
        {
        case CCX: //arg1
        case CDX: //arg2
        case R8: //arg3
        case R9: //arg4
        case XMM0:
        case XMM1:
        case XMM2:
        case XMM3:
            p->setPen(ConfigColor("RegistersArgumentLabelColor"));
            break;
        default:
#endif //_WIN64
            p->setPen(ConfigColor("RegistersLabelColor"));
#ifdef _WIN64
            break;
        }
#endif //_WIN64

        //draw the register name
        auto regName = mRegisterMapping[reg];
        p->drawText(x, y, width, mRowHeight, Qt::AlignVCenter, regName);

        //highlight the register based on access
        uint8_t highlight = 0;
        for(const auto & reg : mHighlightRegs)
        {
            if(!CapstoneTokenizer::tokenTextPoolEquals(regName, reg.first))
                continue;
            highlight = reg.second;
            break;
        }
        if(highlight)
        {
            const char* name = "";
            switch(highlight & ~(Zydis::RAIImplicit | Zydis::RAIExplicit))
            {
            case Zydis::RAIRead:
                name = "RegistersHighlightReadColor";
                break;
            case Zydis::RAIWrite:
                name = "RegistersHighlightWriteColor";
                break;
            case Zydis::RAIRead | Zydis::RAIWrite:
                name = "RegistersHighlightReadWriteColor";
                break;
            }
            auto highlightColor = ConfigColor(name);
            if(highlightColor.alpha())
            {
                QPen highlightPen(highlightColor);
                highlightPen.setWidth(2);
                p->setPen(highlightPen);
                p->drawLine(x + 1, y + mRowHeight - 1, x + mCharWidth * regName.length() - 1, y + mRowHeight - 1);
            }
        }

        x += (mRegisterPlaces[reg].labelwidth) * mCharWidth;
        //p->drawText(offset,mRowHeight*(mRegisterPlaces[reg].line+1),mRegisterMapping[reg]);

        //set highlighting
        if(DbgIsDebugging() && mRegisterUpdates.contains(reg))
            p->setPen(ConfigColor("RegistersModifiedColor"));
        else
            p->setPen(ConfigColor("RegistersColor"));

        //get register value
        QString valueText = GetRegStringValueFromValue(reg, value);

        //selection
        width = fontMetrics.width(valueText);
        if(mSelected == reg)
        {
            p->fillRect(x, y, width, mRowHeight, QBrush(ConfigColor("RegistersSelectionColor")));
            //p->fillRect(QRect(x + (mRegisterPlaces[reg].labelwidth)*mCharWidth ,mRowHeight*(mRegisterPlaces[reg].line)+2, mRegisterPlaces[reg].valuesize*mCharWidth, mRowHeight), QBrush(ConfigColor("RegistersSelectionColor")));
        }

        // draw value
        p->drawText(x, y, width, mRowHeight, Qt::AlignVCenter, valueText);
        //p->drawText(x + (mRegisterPlaces[reg].labelwidth)*mCharWidth ,mRowHeight*(mRegisterPlaces[reg].line+1),QString("%1").arg(value, mRegisterPlaces[reg].valuesize, 16, QChar('0')).toUpper());

        x += width;

        if(mFPUx87_80BITSDISPLAY.contains(reg) && DbgIsDebugging())
        {
            p->setPen(ConfigColor("RegistersExtraInfoColor"));
            x += 1 * mCharWidth; //1 space
            QString newText;
            if(mRegisterUpdates.contains(x87SW_TOP))
                p->setPen(ConfigColor("RegistersModifiedColor"));

            if(reg >= x87r0 && reg <= x87r7)
            {
                newText = QString("ST%1 ").arg(((X87FPUREGISTER*) registerValue(&wRegDumpStruct, reg))->st_value);
            }
            else
            {
                newText = QString("x87r%1 ").arg((wRegDumpStruct.x87StatusWordFields.TOP + (reg - x87st0)) & 7);
            }
            width = fontMetrics.width(newText);
            p->drawText(x, y, width, mRowHeight, Qt::AlignVCenter, newText);

            x += width;

            newText = QString("");

            p->setPen(ConfigColor("RegistersExtraInfoColor"));

            if(reg >= x87r0 && reg <= x87r7 && mRegisterUpdates.contains((REGISTER_NAME)(x87TW_0 + (reg - x87r0))))
                p->setPen(ConfigColor("RegistersModifiedColor"));

            newText += GetTagWordStateString(((X87FPUREGISTER*) registerValue(&wRegDumpStruct, reg))->tag) + QString(" ");

            width = fontMetrics.width(newText);
            p->drawText(x, y, width, mRowHeight, Qt::AlignVCenter, newText);

            x += width;

            newText = QString("");

            p->setPen(ConfigColor("RegistersExtraInfoColor"));

            if(DbgIsDebugging() && mRegisterUpdates.contains(reg))
                p->setPen(ConfigColor("RegistersModifiedColor"));

            newText += ToLongDoubleString(((X87FPUREGISTER*) registerValue(&wRegDumpStruct, reg))->data);
            width = fontMetrics.width(newText);
            p->drawText(x, y, width, mRowHeight, Qt::AlignVCenter, newText);
        }

        // do we have a label ?
        if(mLABELDISPLAY.contains(reg))
        {
            x += 5 * mCharWidth; //5 spaces

            QString newText = getRegisterLabel(reg);

            // are there additional informations?
            if(newText != "")
            {
                width = fontMetrics.width(newText);
                p->setPen(ConfigColor("RegistersExtraInfoColor"));
                p->drawText(x, y, width, mRowHeight, Qt::AlignVCenter, newText);
                //p->drawText(x,mRowHeight*(mRegisterPlaces[reg].line+1),newText);
            }
        }
    }
}

void RegistersView::updateRegistersSlot()
{
    // read registers
    REGDUMP z;
    DbgGetRegDumpEx(&z, sizeof(REGDUMP));
    // update gui
    setRegisters(&z);
}

void RegistersView::ModifyFields(const QString & title, STRING_VALUE_TABLE_t* table, SIZE_T size)
{
    SelectFields mSelectFields(this);
    QListWidget* mQListWidget = mSelectFields.GetList();

    QStringList items;
    unsigned int i;

    for(i = 0; i < size; i++)
        items << QApplication::translate("RegistersView_ConstantsOfRegisters", table[i].string) + QString(" (%1)").arg(table[i].value, 0, 16);

    mQListWidget->addItems(items);

    mSelectFields.setWindowTitle(title);
    if(mSelectFields.exec() != QDialog::Accepted)
        return;

    if(mQListWidget->selectedItems().count() != 1)
        return;

    //QListWidgetItem* item = mQListWidget->takeItem(mQListWidget->currentRow());
    QString itemText = mQListWidget->item(mQListWidget->currentRow())->text();

    duint value;

    for(i = 0; i < size; i++)
    {
        if(QApplication::translate("RegistersView_ConstantsOfRegisters", table[i].string) + QString(" (%1)").arg(table[i].value, 0, 16) == itemText)
            break;
    }

    value = table[i].value;

    setRegister(mSelected, (duint)value);
    //delete item;
}

#define MODIFY_FIELDS_DISPLAY(prefix, title, table) ModifyFields(prefix + QChar(' ') + QString(title), (STRING_VALUE_TABLE_t *) & table, SIZE_TABLE(table) )

/**
 * @brief   This function displays the appropriate edit dialog according to selected register
 * @return  Nothing.
 */

void RegistersView::displayEditDialog()
{
    if(mFPU.contains(mSelected))
    {
        if(mTAGWORD.contains(mSelected))
            MODIFY_FIELDS_DISPLAY(tr("Edit"), "Tag " + mRegisterMapping.constFind(mSelected).value(), TagWordValueStringTable);
        else if(mSelected == MxCsr_RC)
            MODIFY_FIELDS_DISPLAY(tr("Edit"), "MxCsr_RC", MxCsrRCValueStringTable);
        else if(mSelected == x87CW_RC)
            MODIFY_FIELDS_DISPLAY(tr("Edit"), "x87CW_RC", ControlWordRCValueStringTable);
        else if(mSelected == x87CW_PC)
            MODIFY_FIELDS_DISPLAY(tr("Edit"), "x87CW_PC", ControlWordPCValueStringTable);
        else if(mSelected == x87SW_TOP)
        {
            MODIFY_FIELDS_DISPLAY(tr("Edit"), "x87SW_TOP", StatusWordTOPValueStringTable);
            // if(mFpuMode == false)
            updateRegistersSlot();
        }
        else if(mFPUYMM.contains(mSelected))
        {
            EditFloatRegister mEditFloat(256, this);
            mEditFloat.setWindowTitle(tr("Edit YMM register"));
            mEditFloat.loadData(registerValue(&wRegDumpStruct, mSelected));
            mEditFloat.show();
            mEditFloat.selectAllText();
            if(mEditFloat.exec() == QDialog::Accepted)
                setRegister(mSelected, (duint)mEditFloat.getData());
        }
        else if(mFPUXMM.contains(mSelected))
        {
            EditFloatRegister mEditFloat(128, this);
            mEditFloat.setWindowTitle(tr("Edit XMM register"));
            mEditFloat.loadData(registerValue(&wRegDumpStruct, mSelected));
            mEditFloat.show();
            mEditFloat.selectAllText();
            if(mEditFloat.exec() == QDialog::Accepted)
                setRegister(mSelected, (duint)mEditFloat.getData());
        }
        else if(mFPUMMX.contains(mSelected))
        {
            EditFloatRegister mEditFloat(64, this);
            mEditFloat.setWindowTitle(tr("Edit MMX register"));
            mEditFloat.loadData(registerValue(&wRegDumpStruct, mSelected));
            mEditFloat.show();
            mEditFloat.selectAllText();
            if(mEditFloat.exec() == QDialog::Accepted)
                setRegister(mSelected, (duint)mEditFloat.getData());
        }
        else
        {
            bool errorinput = false;
            LineEditDialog mLineEdit(this);

            mLineEdit.setText(GetRegStringValueFromValue(mSelected,  registerValue(&wRegDumpStruct, mSelected)));
            mLineEdit.setWindowTitle(tr("Edit FPU register"));
            mLineEdit.setWindowIcon(DIcon("log.png"));
            mLineEdit.setCursorPosition(0);
            auto sizeRegister = int(GetSizeRegister(mSelected));
            if(sizeRegister == 10)
                mLineEdit.setFpuMode();
            mLineEdit.ForceSize(sizeRegister * 2);
            do
            {
                errorinput = false;
                mLineEdit.show();
                mLineEdit.selectAllText();
                if(mLineEdit.exec() != QDialog::Accepted)
                    return; //pressed cancel
                else
                {
                    bool ok = false;
                    duint fpuvalue;

                    if(mUSHORTDISPLAY.contains(mSelected))
                        fpuvalue = (duint) mLineEdit.editText.toUShort(&ok, 16);
                    else if(mDWORDDISPLAY.contains(mSelected))
                        fpuvalue = mLineEdit.editText.toUInt(&ok, 16);
                    else if(mFPUx87_80BITSDISPLAY.contains(mSelected))
                    {
                        QString editTextLower = mLineEdit.editText.toLower();
                        if(sizeRegister == 10 && (mLineEdit.editText.contains(QChar('.')) || editTextLower == "nan" || editTextLower == "inf"
                                                  || editTextLower == "+inf" || editTextLower == "-inf"))
                        {
                            char number[10];
                            str2ld(mLineEdit.editText.toUtf8().constData(), number);
                            setRegister(mSelected, reinterpret_cast<duint>(number));
                            return;
                        }
                        else
                        {
                            QByteArray pArray =  mLineEdit.editText.toLocal8Bit();

                            if(pArray.size() == sizeRegister * 2)
                            {
                                char* pData = (char*) calloc(1, sizeof(char) * sizeRegister);

                                if(pData != NULL)
                                {
                                    ok = true;
                                    char actual_char[3];
                                    for(int i = 0; i < sizeRegister; i++)
                                    {
                                        memset(actual_char, 0, sizeof(actual_char));
                                        memcpy(actual_char, (char*) pArray.data() + (i * 2), 2);
                                        if(! isxdigit(actual_char[0]) || ! isxdigit(actual_char[1]))
                                        {
                                            ok = false;
                                            break;
                                        }
                                        pData[i] = (char)strtol(actual_char, NULL, 16);
                                    }

                                    if(ok)
                                    {
                                        if(!ConfigBool("Gui", "FpuRegistersLittleEndian")) // reverse byte order if it is big-endian
                                        {
                                            pArray = ByteReverse(QByteArray(pData, sizeRegister));
                                            setRegister(mSelected, reinterpret_cast<duint>(pArray.constData()));
                                        }
                                        else
                                            setRegister(mSelected, reinterpret_cast<duint>(pData));
                                    }

                                    free(pData);

                                    if(ok)
                                        return;
                                }
                            }
                        }
                    }
                    if(!ok)
                    {
                        errorinput = true;

                        SimpleWarningBox(this, tr("ERROR CONVERTING TO HEX"), tr("ERROR CONVERTING TO HEX"));
                    }
                    else
                        setRegister(mSelected, fpuvalue);
                }
            }
            while(errorinput);
        }
    }
    else if(mSelected == LastError)
    {
        bool errorinput = false;
        LineEditDialog mLineEdit(this);
        LASTERROR* error = (LASTERROR*)registerValue(&wRegDumpStruct, LastError);
        mLineEdit.setText(QString::number(error->code, 16));
        mLineEdit.setWindowTitle(tr("Set Last Error"));
        mLineEdit.setCursorPosition(0);
        do
        {
            errorinput = true;
            mLineEdit.show();
            mLineEdit.selectAllText();
            if(mLineEdit.exec() != QDialog::Accepted)
                return;
            if(DbgIsValidExpression(mLineEdit.editText.toUtf8().constData()))
                errorinput = false;
        }
        while(errorinput);
        setRegister(LastError, DbgValFromString(mLineEdit.editText.toUtf8().constData()));
    }
    else if(mSelected == LastStatus)
    {
        bool statusinput = false;
        LineEditDialog mLineEdit(this);
        LASTSTATUS* status = (LASTSTATUS*)registerValue(&wRegDumpStruct, LastStatus);
        mLineEdit.setText(QString::number(status->code, 16));
        mLineEdit.setWindowTitle(tr("Set Last Status"));
        mLineEdit.setCursorPosition(0);
        do
        {
            statusinput = true;
            mLineEdit.show();
            mLineEdit.selectAllText();
            if(mLineEdit.exec() != QDialog::Accepted)
                return;
            if(DbgIsValidExpression(mLineEdit.editText.toUtf8().constData()))
                statusinput = false;
        }
        while(statusinput);
        setRegister(LastStatus, DbgValFromString(mLineEdit.editText.toUtf8().constData()));
    }
    else
    {
        WordEditDialog wEditDial(this);
        wEditDial.setup(tr("Edit"), (* ((duint*) registerValue(&wRegDumpStruct, mSelected))), sizeof(dsint));
        if(wEditDial.exec() == QDialog::Accepted) //OK button clicked
            setRegister(mSelected, wEditDial.getVal());
    }
}

void RegistersView::onIncrementx87StackAction()
{
    if(mFPUx87_80BITSDISPLAY.contains(mSelected))
        setRegister(x87SW_TOP, ((* ((duint*) registerValue(&wRegDumpStruct, x87SW_TOP))) + 1) % 8);
}

void RegistersView::onDecrementx87StackAction()
{
    if(mFPUx87_80BITSDISPLAY.contains(mSelected))
        setRegister(x87SW_TOP, ((* ((duint*) registerValue(&wRegDumpStruct, x87SW_TOP))) - 1) % 8);
}

void RegistersView::onIncrementAction()
{
    if(mINCREMENTDECREMET.contains(mSelected))
        setRegister(mSelected, (* ((duint*) registerValue(&wRegDumpStruct, mSelected))) + 1);
}

void RegistersView::onDecrementAction()
{
    if(mINCREMENTDECREMET.contains(mSelected))
        setRegister(mSelected, (* ((duint*) registerValue(&wRegDumpStruct, mSelected))) - 1);
}

void RegistersView::onIncrementPtrSize()
{
    if(mINCREMENTDECREMET.contains(mSelected))
        setRegister(mSelected, (* ((duint*) registerValue(&wRegDumpStruct, mSelected))) + sizeof(void*));
}

void RegistersView::onDecrementPtrSize()
{
    if(mINCREMENTDECREMET.contains(mSelected))
        setRegister(mSelected, (* ((duint*) registerValue(&wRegDumpStruct, mSelected))) - sizeof(void*));
}

void RegistersView::onPushAction()
{
    duint csp = (* ((duint*) registerValue(&wRegDumpStruct, CSP))) - sizeof(void*);
    duint regVal = 0;
    regVal = * ((duint*) registerValue(&wRegDumpStruct, mSelected));
    setRegister(CSP, csp);
    DbgMemWrite(csp, (const unsigned char*)&regVal, sizeof(void*));
}

void RegistersView::onPopAction()
{
    duint csp = (* ((duint*) registerValue(&wRegDumpStruct, CSP)));
    duint newVal;
    DbgMemRead(csp, (unsigned char*)&newVal, sizeof(void*));
    setRegister(CSP, csp + sizeof(void*));
    setRegister(mSelected, newVal);
}

void RegistersView::onZeroAction()
{
    if(mSETONEZEROTOGGLE.contains(mSelected))
    {
        if(mSelected >= x87r0 && mSelected <= x87r7 || mSelected >= x87st0 && mSelected <= x87st7)
            setRegister(mSelected, reinterpret_cast<duint>("\0\0\0\0\0\0\0\0\0")); //9 zeros and 1 terminating zero
        else
            setRegister(mSelected, 0);
    }
}

void RegistersView::onSetToOneAction()
{
    if(mSETONEZEROTOGGLE.contains(mSelected))
    {
        if(mSelected >= x87r0 && mSelected <= x87r7 || mSelected >= x87st0 && mSelected <= x87st7)
            setRegister(mSelected, reinterpret_cast<duint>("\0\0\0\0\0\0\0\x80\xFF\x3F"));
        else
            setRegister(mSelected, 1);
    }
}

void RegistersView::onModifyAction()
{
    if(mMODIFYDISPLAY.contains(mSelected))
        displayEditDialog();
}

void RegistersView::onToggleValueAction()
{
    if(mBOOLDISPLAY.contains(mSelected))
    {
        int value = (int)(* (bool*) registerValue(&wRegDumpStruct, mSelected));
        setRegister(mSelected, value ^ 1);
    }
}

void RegistersView::onUndoAction()
{
    if(mUNDODISPLAY.contains(mSelected))
    {
        if(mFPUMMX.contains(mSelected) || mFPUXMM.contains(mSelected) || mFPUYMM.contains(mSelected) || mFPUx87_80BITSDISPLAY.contains(mSelected))
            setRegister(mSelected, (duint)registerValue(&wCipRegDumpStruct, mSelected));
        else
            setRegister(mSelected, *(duint*)registerValue(&wCipRegDumpStruct, mSelected));
    }
}

void RegistersView::onCopyToClipboardAction()
{
    Bridge::CopyToClipboard(GetRegStringValueFromValue(mSelected, registerValue(&wRegDumpStruct, mSelected)));
}

void RegistersView::onCopyFloatingPointToClipboardAction()
{
    Bridge::CopyToClipboard(ToLongDoubleString(((X87FPUREGISTER*) registerValue(&wRegDumpStruct, mSelected))->data));
}

void RegistersView::onCopySymbolToClipboardAction()
{
    if(mLABELDISPLAY.contains(mSelected))
    {
        QString symbol = getRegisterLabel(mSelected);
        if(symbol != "")
            Bridge::CopyToClipboard(symbol);
    }
}

void RegistersView::onHighlightSlot()
{
    Disassembly* CPUDisassemblyView = mParent->getDisasmWidget();
    if(mGPR.contains(mSelected) && mSelected != REGISTER_NAME::EFLAGS)
        CPUDisassemblyView->hightlightToken(CapstoneTokenizer::SingleToken(CapstoneTokenizer::TokenType::GeneralRegister, mRegisterMapping.constFind(mSelected).value()));
    else if(mSEGMENTREGISTER.contains(mSelected))
        CPUDisassemblyView->hightlightToken(CapstoneTokenizer::SingleToken(CapstoneTokenizer::TokenType::MemorySegment, mRegisterMapping.constFind(mSelected).value()));
    else if(mFPUMMX.contains(mSelected))
        CPUDisassemblyView->hightlightToken(CapstoneTokenizer::SingleToken(CapstoneTokenizer::TokenType::MmxRegister, mRegisterMapping.constFind(mSelected).value()));
    else if(mFPUXMM.contains(mSelected))
        CPUDisassemblyView->hightlightToken(CapstoneTokenizer::SingleToken(CapstoneTokenizer::TokenType::XmmRegister, mRegisterMapping.constFind(mSelected).value()));
    else if(mFPUYMM.contains(mSelected))
        CPUDisassemblyView->hightlightToken(CapstoneTokenizer::SingleToken(CapstoneTokenizer::TokenType::YmmRegister, mRegisterMapping.constFind(mSelected).value()));
    CPUDisassemblyView->reloadData();
}

void RegistersView::appendRegister(QString & text, REGISTER_NAME reg, const char* name64, const char* name32)
{
    QString symbol;
#ifdef _WIN64
    Q_UNUSED(name32);
    text.append(name64);
#else //x86
    Q_UNUSED(name64);
    text.append(name32);
#endif //_WIN64
    text.append(GetRegStringValueFromValue(reg, registerValue(&wRegDumpStruct, reg)));
    symbol = getRegisterLabel(reg);
    if(symbol != "")
    {
        text.append("     ");
        text.append(symbol);
    }
    text.append("\r\n");
}

void RegistersView::onCopyAllAction()
{
    QString text;
    appendRegister(text, REGISTER_NAME::CAX, "RAX : ", "EAX : ");
    appendRegister(text, REGISTER_NAME::CBX, "RBX : ", "EBX : ");
    appendRegister(text, REGISTER_NAME::CCX, "RCX : ", "ECX : ");
    appendRegister(text, REGISTER_NAME::CDX, "RDX : ", "EDX : ");
    appendRegister(text, REGISTER_NAME::CBP, "RBP : ", "EBP : ");
    appendRegister(text, REGISTER_NAME::CSP, "RSP : ", "ESP : ");
    appendRegister(text, REGISTER_NAME::CSI, "RSI : ", "ESI : ");
    appendRegister(text, REGISTER_NAME::CDI, "RDI : ", "EDI : ");
#ifdef _WIN64
    appendRegister(text, REGISTER_NAME::R8, "R8  : ", "R8  : ");
    appendRegister(text, REGISTER_NAME::R9, "R9  : ", "R9  : ");
    appendRegister(text, REGISTER_NAME::R10, "R10 : ", "R10 : ");
    appendRegister(text, REGISTER_NAME::R11, "R11 : ", "R11 : ");
    appendRegister(text, REGISTER_NAME::R12, "R12 : ", "R12 : ");
    appendRegister(text, REGISTER_NAME::R13, "R13 : ", "R13 : ");
    appendRegister(text, REGISTER_NAME::R14, "R14 : ", "R14 : ");
    appendRegister(text, REGISTER_NAME::R15, "R15 : ", "R15 : ");
#endif
    appendRegister(text, REGISTER_NAME::CIP, "RIP : ", "EIP : ");
    appendRegister(text, REGISTER_NAME::EFLAGS, "RFLAGS : ", "EFLAGS : ");
    appendRegister(text, REGISTER_NAME::ZF, "ZF : ", "ZF : ");
    appendRegister(text, REGISTER_NAME::OF, "OF : ", "OF : ");
    appendRegister(text, REGISTER_NAME::CF, "CF : ", "CF : ");
    appendRegister(text, REGISTER_NAME::PF, "PF : ", "PF : ");
    appendRegister(text, REGISTER_NAME::SF, "SF : ", "SF : ");
    appendRegister(text, REGISTER_NAME::TF, "TF : ", "TF : ");
    appendRegister(text, REGISTER_NAME::AF, "AF : ", "AF : ");
    appendRegister(text, REGISTER_NAME::DF, "DF : ", "DF : ");
    appendRegister(text, REGISTER_NAME::IF, "IF : ", "IF : ");
    appendRegister(text, REGISTER_NAME::LastError, "LastError : ", "LastError : ");
    appendRegister(text, REGISTER_NAME::LastStatus, "LastStatus : ", "LastStatus : ");
    appendRegister(text, REGISTER_NAME::GS, "GS : ", "GS : ");
    appendRegister(text, REGISTER_NAME::ES, "ES : ", "ES : ");
    appendRegister(text, REGISTER_NAME::CS, "CS : ", "CS : ");
    appendRegister(text, REGISTER_NAME::FS, "FS : ", "FS : ");
    appendRegister(text, REGISTER_NAME::DS, "DS : ", "DS : ");
    appendRegister(text, REGISTER_NAME::SS, "SS : ", "SS : ");
    if(mShowFpu)
    {

        appendRegister(text, REGISTER_NAME::x87r0, "x87r0 : ", "x87r0 : ");
        appendRegister(text, REGISTER_NAME::x87r1, "x87r1 : ", "x87r1 : ");
        appendRegister(text, REGISTER_NAME::x87r2, "x87r2 : ", "x87r2 : ");
        appendRegister(text, REGISTER_NAME::x87r3, "x87r3 : ", "x87r3 : ");
        appendRegister(text, REGISTER_NAME::x87r4, "x87r4 : ", "x87r4 : ");
        appendRegister(text, REGISTER_NAME::x87r5, "x87r5 : ", "x87r5 : ");
        appendRegister(text, REGISTER_NAME::x87r6, "x87r6 : ", "x87r6 : ");
        appendRegister(text, REGISTER_NAME::x87r7, "x87r7 : ", "x87r7 : ");
        appendRegister(text, REGISTER_NAME::x87TagWord, "x87TagWord : ", "x87TagWord : ");
        appendRegister(text, REGISTER_NAME::x87ControlWord, "x87ControlWord : ", "x87ControlWord : ");
        appendRegister(text, REGISTER_NAME::x87StatusWord, "x87StatusWord : ", "x87StatusWord : ");
        appendRegister(text, REGISTER_NAME::x87TW_0, "x87TW_0 : ", "x87TW_0 : ");
        appendRegister(text, REGISTER_NAME::x87TW_1, "x87TW_1 : ", "x87TW_1 : ");
        appendRegister(text, REGISTER_NAME::x87TW_2, "x87TW_2 : ", "x87TW_2 : ");
        appendRegister(text, REGISTER_NAME::x87TW_3, "x87TW_3 : ", "x87TW_3 : ");
        appendRegister(text, REGISTER_NAME::x87TW_4, "x87TW_4 : ", "x87TW_4 : ");
        appendRegister(text, REGISTER_NAME::x87TW_5, "x87TW_5 : ", "x87TW_5 : ");
        appendRegister(text, REGISTER_NAME::x87TW_6, "x87TW_6 : ", "x87TW_6 : ");
        appendRegister(text, REGISTER_NAME::x87TW_7, "x87TW_7 : ", "x87TW_7 : ");
        appendRegister(text, REGISTER_NAME::x87SW_B, "x87SW_B : ", "x87SW_B : ");
        appendRegister(text, REGISTER_NAME::x87SW_C3, "x87SW_C3 : ", "x87SW_C3 : ");
        appendRegister(text, REGISTER_NAME::x87SW_TOP, "x87SW_TOP : ", "x87SW_TOP : ");
        appendRegister(text, REGISTER_NAME::x87SW_C2, "x87SW_C2 : ", "x87SW_C2 : ");
        appendRegister(text, REGISTER_NAME::x87SW_C1, "x87SW_C1 : ", "x87SW_C1 : ");
        appendRegister(text, REGISTER_NAME::x87SW_O, "x87SW_O : ", "x87SW_O : ");
        appendRegister(text, REGISTER_NAME::x87SW_ES, "x87SW_ES : ", "x87SW_ES : ");
        appendRegister(text, REGISTER_NAME::x87SW_SF, "x87SW_SF : ", "x87SW_SF : ");
        appendRegister(text, REGISTER_NAME::x87SW_P, "x87SW_P : ", "x87SW_P : ");
        appendRegister(text, REGISTER_NAME::x87SW_U, "x87SW_U : ", "x87SW_U : ");
        appendRegister(text, REGISTER_NAME::x87SW_Z, "x87SW_Z : ", "x87SW_Z : ");
        appendRegister(text, REGISTER_NAME::x87SW_D, "x87SW_D : ", "x87SW_D : ");
        appendRegister(text, REGISTER_NAME::x87SW_I, "x87SW_I : ", "x87SW_I : ");
        appendRegister(text, REGISTER_NAME::x87SW_C0, "x87SW_C0 : ", "x87SW_C0 : ");
        appendRegister(text, REGISTER_NAME::x87CW_IC, "x87CW_IC : ", "x87CW_IC : ");
        appendRegister(text, REGISTER_NAME::x87CW_RC, "x87CW_RC : ", "x87CW_RC : ");
        appendRegister(text, REGISTER_NAME::x87CW_PC, "x87CW_PC : ", "x87CW_PC : ");
        appendRegister(text, REGISTER_NAME::x87CW_PM, "x87CW_PM : ", "x87CW_PM : ");
        appendRegister(text, REGISTER_NAME::x87CW_UM, "x87CW_UM : ", "x87CW_UM : ");
        appendRegister(text, REGISTER_NAME::x87CW_OM, "x87CW_OM : ", "x87CW_OM : ");
        appendRegister(text, REGISTER_NAME::x87CW_ZM, "x87CW_ZM : ", "x87CW_ZM : ");
        appendRegister(text, REGISTER_NAME::x87CW_DM, "x87CW_DM : ", "x87CW_DM : ");
        appendRegister(text, REGISTER_NAME::x87CW_IM, "x87CW_IM : ", "x87CW_IM : ");
        appendRegister(text, REGISTER_NAME::MxCsr, "MxCsr : ", "MxCsr : ");
        appendRegister(text, REGISTER_NAME::MxCsr_FZ, "MxCsr_FZ : ", "MxCsr_FZ : ");
        appendRegister(text, REGISTER_NAME::MxCsr_PM, "MxCsr_PM : ", "MxCsr_PM : ");
        appendRegister(text, REGISTER_NAME::MxCsr_UM, "MxCsr_UM : ", "MxCsr_UM : ");
        appendRegister(text, REGISTER_NAME::MxCsr_OM, "MxCsr_OM : ", "MxCsr_OM : ");
        appendRegister(text, REGISTER_NAME::MxCsr_ZM, "MxCsr_ZM : ", "MxCsr_ZM : ");
        appendRegister(text, REGISTER_NAME::MxCsr_IM, "MxCsr_IM : ", "MxCsr_IM : ");
        appendRegister(text, REGISTER_NAME::MxCsr_DM, "MxCsr_DM : ", "MxCsr_DM : ");
        appendRegister(text, REGISTER_NAME::MxCsr_DAZ, "MxCsr_DAZ : ", "MxCsr_DAZ : ");
        appendRegister(text, REGISTER_NAME::MxCsr_PE, "MxCsr_PE : ", "MxCsr_PE : ");
        appendRegister(text, REGISTER_NAME::MxCsr_UE, "MxCsr_UE : ", "MxCsr_UE : ");
        appendRegister(text, REGISTER_NAME::MxCsr_OE, "MxCsr_OE : ", "MxCsr_OE : ");
        appendRegister(text, REGISTER_NAME::MxCsr_ZE, "MxCsr_ZE : ", "MxCsr_ZE : ");
        appendRegister(text, REGISTER_NAME::MxCsr_DE, "MxCsr_DE : ", "MxCsr_DE : ");
        appendRegister(text, REGISTER_NAME::MxCsr_IE, "MxCsr_IE : ", "MxCsr_IE : ");
        appendRegister(text, REGISTER_NAME::MxCsr_RC, "MxCsr_RC : ", "MxCsr_RC : ");
        appendRegister(text, REGISTER_NAME::MM0, "MM0 : ", "MM0 : ");
        appendRegister(text, REGISTER_NAME::MM1, "MM1 : ", "MM1 : ");
        appendRegister(text, REGISTER_NAME::MM2, "MM2 : ", "MM2 : ");
        appendRegister(text, REGISTER_NAME::MM3, "MM3 : ", "MM3 : ");
        appendRegister(text, REGISTER_NAME::MM4, "MM4 : ", "MM4 : ");
        appendRegister(text, REGISTER_NAME::MM5, "MM5 : ", "MM5 : ");
        appendRegister(text, REGISTER_NAME::MM6, "MM6 : ", "MM6 : ");
        appendRegister(text, REGISTER_NAME::MM7, "MM7 : ", "MM7 : ");
        appendRegister(text, REGISTER_NAME::XMM0, "XMM0  : ", "XMM0  : ");
        appendRegister(text, REGISTER_NAME::XMM1, "XMM1  : ", "XMM1  : ");
        appendRegister(text, REGISTER_NAME::XMM2, "XMM2  : ", "XMM2  : ");
        appendRegister(text, REGISTER_NAME::XMM3, "XMM3  : ", "XMM3  : ");
        appendRegister(text, REGISTER_NAME::XMM4, "XMM4  : ", "XMM4  : ");
        appendRegister(text, REGISTER_NAME::XMM5, "XMM5  : ", "XMM5  : ");
        appendRegister(text, REGISTER_NAME::XMM6, "XMM6  : ", "XMM6  : ");
        appendRegister(text, REGISTER_NAME::XMM7, "XMM7  : ", "XMM7  : ");
#ifdef _WIN64
        appendRegister(text, REGISTER_NAME::XMM8, "XMM8  : ", "XMM8  : ");
        appendRegister(text, REGISTER_NAME::XMM9, "XMM9  : ", "XMM9  : ");
        appendRegister(text, REGISTER_NAME::XMM10, "XMM10 : ", "XMM10 : ");
        appendRegister(text, REGISTER_NAME::XMM11, "XMM11 : ", "XMM11 : ");
        appendRegister(text, REGISTER_NAME::XMM12, "XMM12 : ", "XMM12 : ");
        appendRegister(text, REGISTER_NAME::XMM13, "XMM13 : ", "XMM13 : ");
        appendRegister(text, REGISTER_NAME::XMM14, "XMM14 : ", "XMM14 : ");
        appendRegister(text, REGISTER_NAME::XMM15, "XMM15 : ", "XMM15 : ");
#endif
        appendRegister(text, REGISTER_NAME::YMM0, "YMM0  : ", "YMM0  : ");
        appendRegister(text, REGISTER_NAME::YMM1, "YMM1  : ", "YMM1  : ");
        appendRegister(text, REGISTER_NAME::YMM2, "YMM2  : ", "YMM2  : ");
        appendRegister(text, REGISTER_NAME::YMM3, "YMM3  : ", "YMM3  : ");
        appendRegister(text, REGISTER_NAME::YMM4, "YMM4  : ", "YMM4  : ");
        appendRegister(text, REGISTER_NAME::YMM5, "YMM5  : ", "YMM5  : ");
        appendRegister(text, REGISTER_NAME::YMM6, "YMM6  : ", "YMM6  : ");
        appendRegister(text, REGISTER_NAME::YMM7, "YMM7  : ", "YMM7  : ");
#ifdef _WIN64
        appendRegister(text, REGISTER_NAME::YMM8, "YMM8  : ", "YMM8  : ");
        appendRegister(text, REGISTER_NAME::YMM9, "YMM9  : ", "YMM9  : ");
        appendRegister(text, REGISTER_NAME::YMM10, "YMM10 : ", "YMM10 : ");
        appendRegister(text, REGISTER_NAME::YMM11, "YMM11 : ", "YMM11 : ");
        appendRegister(text, REGISTER_NAME::YMM12, "YMM12 : ", "YMM12 : ");
        appendRegister(text, REGISTER_NAME::YMM13, "YMM13 : ", "YMM13 : ");
        appendRegister(text, REGISTER_NAME::YMM14, "YMM14 : ", "YMM14 : ");
        appendRegister(text, REGISTER_NAME::YMM15, "YMM15 : ", "YMM15 : ");
#endif
    }
    appendRegister(text, REGISTER_NAME::DR0, "DR0 : ", "DR0 : ");
    appendRegister(text, REGISTER_NAME::DR1, "DR1 : ", "DR1 : ");
    appendRegister(text, REGISTER_NAME::DR2, "DR2 : ", "DR2 : ");
    appendRegister(text, REGISTER_NAME::DR3, "DR3 : ", "DR3 : ");
    appendRegister(text, REGISTER_NAME::DR6, "DR6 : ", "DR6 : ");
    appendRegister(text, REGISTER_NAME::DR7, "DR7 : ", "DR7 : ");

    Bridge::CopyToClipboard(text);
}

void RegistersView::onFollowInDisassembly()
{
    if(mCANSTOREADDRESS.contains(mSelected))
    {
        QString addr = QString("%1").arg((* ((duint*) registerValue(&wRegDumpStruct, mSelected))), mRegisterPlaces[mSelected].valuesize, 16, QChar('0')).toUpper();
        if(DbgMemIsValidReadPtr((* ((duint*) registerValue(&wRegDumpStruct, mSelected)))))
            DbgCmdExec(QString().sprintf("disasm \"%s\"", addr.toUtf8().constData()).toUtf8().constData());
    }
}

void RegistersView::onFollowInDump()
{
    if(mCANSTOREADDRESS.contains(mSelected))
    {
        QString addr = QString("%1").arg((* ((duint*) registerValue(&wRegDumpStruct, mSelected))), mRegisterPlaces[mSelected].valuesize, 16, QChar('0')).toUpper();
        if(DbgMemIsValidReadPtr((* ((duint*) registerValue(&wRegDumpStruct, mSelected)))))
            DbgCmdExec(QString().sprintf("dump \"%s\"", addr.toUtf8().constData()).toUtf8().constData());
    }
}

void RegistersView::onFollowInDumpN()
{
    if(mCANSTOREADDRESS.contains(mSelected))
    {
        QString addr = QString("%1").arg((* ((duint*) registerValue(&wRegDumpStruct, mSelected))), mRegisterPlaces[mSelected].valuesize, 16, QChar('0')).toUpper();
        if(DbgMemIsValidReadPtr((* ((duint*) registerValue(&wRegDumpStruct, mSelected)))))
        {
            QAction* action = qobject_cast<QAction*>(sender());
            int numDump = action->data().toInt();
            DbgCmdExec(QString("dump %1, .%2").arg(addr).arg(numDump).toUtf8().constData());
        }
    }
}

void RegistersView::onFollowInStack()
{
    if(mCANSTOREADDRESS.contains(mSelected))
    {
        QString addr = QString("%1").arg((* ((duint*) registerValue(&wRegDumpStruct, mSelected))), mRegisterPlaces[mSelected].valuesize, 16, QChar('0')).toUpper();
        if(DbgMemIsValidReadPtr((* ((duint*) registerValue(&wRegDumpStruct, mSelected)))))
            DbgCmdExec(QString().sprintf("sdump \"%s\"", addr.toUtf8().constData()).toUtf8().constData());
    }
}

void RegistersView::onFollowInMemoryMap()
{
    if(mCANSTOREADDRESS.contains(mSelected))
    {
        QString addr = QString("%1").arg((* ((duint*) registerValue(&wRegDumpStruct, mSelected))), mRegisterPlaces[mSelected].valuesize, 16, QChar('0')).toUpper();
        if(DbgMemIsValidReadPtr((* ((duint*) registerValue(&wRegDumpStruct, mSelected)))))
            DbgCmdExec(QString().sprintf("memmapdump \"%s\"", addr.toUtf8().constData()).toUtf8().constData());
    }
}

void RegistersView::onChangeFPUViewAction()
{
    if(mShowFpu == true)
        ShowFPU(false);
    else
        ShowFPU(true);
}

void RegistersView::displayCustomContextMenuSlot(QPoint pos)
{
    if(!DbgIsDebugging())
        return;
    QMenu wMenu(this);
    QMenu* followInDumpNMenu = nullptr;
    const QAction* selectedAction = nullptr;
    switch(wSIMDRegDispMode)
    {
    case SIMD_REG_DISP_HEX:
        selectedAction = SIMDHex;
        break;
    case SIMD_REG_DISP_FLOAT:
        selectedAction = SIMDFloat;
        break;
    case SIMD_REG_DISP_DOUBLE:
        selectedAction = SIMDDouble;
        break;
    case SIMD_REG_DISP_WORD_SIGNED:
        selectedAction = SIMDSWord;
        break;
    case SIMD_REG_DISP_WORD_UNSIGNED:
        selectedAction = SIMDUWord;
        break;
    case SIMD_REG_DISP_WORD_HEX:
        selectedAction = SIMDHWord;
        break;
    case SIMD_REG_DISP_DWORD_SIGNED:
        selectedAction = SIMDSDWord;
        break;
    case SIMD_REG_DISP_DWORD_UNSIGNED:
        selectedAction = SIMDUDWord;
        break;
    case SIMD_REG_DISP_DWORD_HEX:
        selectedAction = SIMDHDWord;
        break;
    case SIMD_REG_DISP_QWORD_SIGNED:
        selectedAction = SIMDSQWord;
        break;
    case SIMD_REG_DISP_QWORD_UNSIGNED:
        selectedAction = SIMDUQWord;
        break;
    case SIMD_REG_DISP_QWORD_HEX:
        selectedAction = SIMDHQWord;
        break;
    }
    SIMDHex->setChecked(SIMDHex == selectedAction);
    SIMDFloat->setChecked(SIMDFloat == selectedAction);
    SIMDDouble->setChecked(SIMDDouble == selectedAction);
    SIMDSWord->setChecked(SIMDSWord == selectedAction);
    SIMDUWord->setChecked(SIMDUWord == selectedAction);
    SIMDHWord->setChecked(SIMDHWord == selectedAction);
    SIMDSDWord->setChecked(SIMDSDWord == selectedAction);
    SIMDUDWord->setChecked(SIMDUDWord == selectedAction);
    SIMDHDWord->setChecked(SIMDHDWord == selectedAction);
    SIMDSQWord->setChecked(SIMDSQWord == selectedAction);
    SIMDUQWord->setChecked(SIMDUQWord == selectedAction);
    SIMDHQWord->setChecked(SIMDHQWord == selectedAction);
    if(mFpuMode)
        mSwitchFPUDispMode->setText(tr("Display ST(x)"));
    else
        mSwitchFPUDispMode->setText(tr("Display x87rX"));
    mSwitchFPUDispMode->setChecked(mFpuMode);

    if(mSelected != UNKNOWN)
    {
        if(mMODIFYDISPLAY.contains(mSelected))
        {
            wMenu.addAction(wCM_Modify);
        }

        if(mCANSTOREADDRESS.contains(mSelected))
        {
            duint addr = (* ((duint*) registerValue(&wRegDumpStruct, mSelected)));
            if(DbgMemIsValidReadPtr(addr))
            {
                wMenu.addAction(wCM_FollowInDump);
                followInDumpNMenu = new QMenu(tr("Follow in &Dump"), &wMenu);
                CreateDumpNMenu(followInDumpNMenu);
                wMenu.addMenu(followInDumpNMenu);
                wMenu.addAction(wCM_FollowInDisassembly);
                wMenu.addAction(wCM_FollowInMemoryMap);
                duint size = 0;
                duint base = DbgMemFindBaseAddr(DbgValFromString("csp"), &size);
                if(addr >= base && addr < base + size)
                    wMenu.addAction(wCM_FollowInStack);
            }
        }

        wMenu.addAction(wCM_CopyToClipboard);
        if(mFPUx87_80BITSDISPLAY.contains(mSelected))
        {
            wMenu.addAction(wCM_CopyFloatingPointValueToClipboard);
        }
        wMenu.addAction(wCM_CopyAll);
        if(mLABELDISPLAY.contains(mSelected))
        {
            QString symbol = getRegisterLabel(mSelected);
            if(symbol != "")
                wMenu.addAction(wCM_CopySymbolToClipboard);
        }

        if((mGPR.contains(mSelected) && mSelected != REGISTER_NAME::EFLAGS) || mSEGMENTREGISTER.contains(mSelected) || mFPUMMX.contains(mSelected) || mFPUXMM.contains(mSelected) || mFPUYMM.contains(mSelected))
        {
            wMenu.addAction(wCM_Highlight);
        }

        if(mUNDODISPLAY.contains(mSelected) && CompareRegisters(mSelected, &wRegDumpStruct, &wCipRegDumpStruct) != 0)
        {
            wMenu.addAction(wCM_Undo);
        }

        if(mSETONEZEROTOGGLE.contains(mSelected))
        {
            if(mSelected >= x87r0 && mSelected <= x87r7 || mSelected >= x87st0 && mSelected <= x87st7)
            {
                if(memcmp(registerValue(&wRegDumpStruct, mSelected), "\0\0\0\0\0\0\0\0\0", 10) != 0)
                    wMenu.addAction(wCM_Zero);
                if(memcmp(registerValue(&wRegDumpStruct, mSelected), "\0\0\0\0\0\0\0\x80\xFF\x3F", 10) != 0)
                    wMenu.addAction(wCM_SetToOne);
            }
            else
            {
                if((* ((duint*) registerValue(&wRegDumpStruct, mSelected))) != 0)
                    wMenu.addAction(wCM_Zero);
                if((* ((duint*) registerValue(&wRegDumpStruct, mSelected))) == 0)
                    wMenu.addAction(wCM_SetToOne);
            }
        }

        if(mBOOLDISPLAY.contains(mSelected))
        {
            wMenu.addAction(wCM_ToggleValue);
        }

        if(mFPUx87_80BITSDISPLAY.contains(mSelected))
        {
            wMenu.addAction(wCM_Incrementx87Stack);
            wMenu.addAction(wCM_Decrementx87Stack);
        }

        if(mINCREMENTDECREMET.contains(mSelected))
        {
            wMenu.addAction(wCM_Increment);
            wMenu.addAction(wCM_Decrement);
            wMenu.addAction(wCM_IncrementPtrSize);
            wMenu.addAction(wCM_DecrementPtrSize);
        }

        if(mGPR.contains(mSelected) || mSelected == CIP)
        {
            wMenu.addAction(wCM_Push);
            wMenu.addAction(wCM_Pop);
        }

        if(mFPUMMX.contains(mSelected) || mFPUXMM.contains(mSelected) || mFPUYMM.contains(mSelected))
        {
            wMenu.addMenu(mSwitchSIMDDispMode);
        }
        wMenu.addAction(mSwitchFPUDispMode);

        wMenu.exec(this->mapToGlobal(pos));
    }
    else
    {
        wMenu.addSeparator();
        wMenu.addAction(wCM_ChangeFPUView);
        wMenu.addAction(wCM_CopyAll);
        wMenu.addMenu(mSwitchSIMDDispMode);
        wMenu.addAction(mSwitchFPUDispMode);
        wMenu.addSeparator();
        QAction* wHwbpCsp = wMenu.addAction(DIcon("breakpoint.png"), tr("Set Hardware Breakpoint on %1").arg(ArchValue("ESP", "RSP")));
        QAction* wAction = wMenu.exec(this->mapToGlobal(pos));

        if(wAction == wHwbpCsp)
            DbgCmdExec("bphws csp,rw");
    }
}

void RegistersView::setRegister(REGISTER_NAME reg, duint value)
{
    // is register-id known?
    if(mRegisterMapping.contains(reg))
    {
        // map x87st0 to x87r0
        QString wRegName;
        if(reg >= x87st0 && reg <= x87st7)
            wRegName = QString().sprintf("st%d", reg - x87st0);
        else
            // map "cax" to "eax" or "rax"
            wRegName = mRegisterMapping.constFind(reg).value();

        // flags need to '_' infront
        if(mFlags.contains(reg))
            wRegName = "_" + wRegName;

        // we change the value (so highlight it)
        mRegisterUpdates.insert(reg);
        // tell everything the compiler
        if(mFPU.contains(reg))
            wRegName = "_" + wRegName;

        DbgValToString(wRegName.toUtf8().constData(), value);

        // force repaint
        emit refresh();
    }
}

void RegistersView::debugStateChangedSlot(DBGSTATE state)
{
    if(state == stopped)
    {
        updateRegistersSlot();
    }
}

void RegistersView::reload()
{
    this->viewport()->update();
}

SIZE_T RegistersView::GetSizeRegister(const REGISTER_NAME reg_name)
{
    SIZE_T size;

    if(mUINTDISPLAY.contains(reg_name))
        size = sizeof(duint);
    else if(mUSHORTDISPLAY.contains(reg_name) || mFIELDVALUE.contains(reg_name))
        size = sizeof(unsigned short);
    else if(mDWORDDISPLAY.contains(reg_name))
        size = sizeof(DWORD);
    else if(mBOOLDISPLAY.contains(reg_name))
        size = sizeof(bool);
    else if(mFPUx87_80BITSDISPLAY.contains(reg_name))
        size = 10;
    else if(mFPUMMX.contains(reg_name))
        size = 8;
    else if(mFPUXMM.contains(reg_name))
        size = 16;
    else if(mFPUYMM.contains(reg_name))
        size = 32;
    else if(reg_name == LastError)
        size = sizeof(DWORD);
    else if(reg_name == LastStatus)
        size = sizeof(NTSTATUS);
    else
        size = 0;

    return size;
}

int RegistersView::CompareRegisters(const REGISTER_NAME reg_name, REGDUMP* regdump1, REGDUMP* regdump2)
{
    SIZE_T size = GetSizeRegister(reg_name);
    char* reg1_data = registerValue(regdump1, reg_name);
    char* reg2_data = registerValue(regdump2, reg_name);

    if(size != 0)
        return memcmp(reg1_data, reg2_data, size);

    return -1;
}

char* RegistersView::registerValue(const REGDUMP* regd, const REGISTER_NAME reg)
{
    static int null_value = 0;
    // this is probably the most efficient general method to access the values of the struct
    // TODO: add an array with something like: return array[reg].data, this is more fast :-)

    switch(reg)
    {
    case CAX:
        return (char*) &regd->regcontext.cax;
    case CBX:
        return (char*) &regd->regcontext.cbx;
    case CCX:
        return (char*) &regd->regcontext.ccx;
    case CDX:
        return (char*) &regd->regcontext.cdx;
    case CSI:
        return (char*) &regd->regcontext.csi;
    case CDI:
        return (char*) &regd->regcontext.cdi;
    case CBP:
        return (char*) &regd->regcontext.cbp;
    case CSP:
        return (char*) &regd->regcontext.csp;

    case CIP:
        return (char*) &regd->regcontext.cip;
    case EFLAGS:
        return (char*) &regd->regcontext.eflags;
#ifdef _WIN64
    case R8:
        return (char*) &regd->regcontext.r8;
    case R9:
        return (char*) &regd->regcontext.r9;
    case R10:
        return (char*) &regd->regcontext.r10;
    case R11:
        return (char*) &regd->regcontext.r11;
    case R12:
        return (char*) &regd->regcontext.r12;
    case R13:
        return (char*) &regd->regcontext.r13;
    case R14:
        return (char*) &regd->regcontext.r14;
    case R15:
        return (char*) &regd->regcontext.r15;
#endif
    // CF,PF,AF,ZF,SF,TF,IF,DF,OF
    case CF:
        return (char*) &regd->flags.c;
    case PF:
        return (char*) &regd->flags.p;
    case AF:
        return (char*) &regd->flags.a;
    case ZF:
        return (char*) &regd->flags.z;
    case SF:
        return (char*) &regd->flags.s;
    case TF:
        return (char*) &regd->flags.t;
    case IF:
        return (char*) &regd->flags.i;
    case DF:
        return (char*) &regd->flags.d;
    case OF:
        return (char*) &regd->flags.o;

    // GS,FS,ES,DS,CS,SS
    case GS:
        return (char*) &regd->regcontext.gs;
    case FS:
        return (char*) &regd->regcontext.fs;
    case ES:
        return (char*) &regd->regcontext.es;
    case DS:
        return (char*) &regd->regcontext.ds;
    case CS:
        return (char*) &regd->regcontext.cs;
    case SS:
        return (char*) &regd->regcontext.ss;

    case LastError:
        return (char*) &regd->lastError;
    case LastStatus:
        return (char*) &regd->lastStatus;

    case DR0:
        return (char*) &regd->regcontext.dr0;
    case DR1:
        return (char*) &regd->regcontext.dr1;
    case DR2:
        return (char*) &regd->regcontext.dr2;
    case DR3:
        return (char*) &regd->regcontext.dr3;
    case DR6:
        return (char*) &regd->regcontext.dr6;
    case DR7:
        return (char*) &regd->regcontext.dr7;

    case MM0:
        return (char*) &regd->mmx[0];
    case MM1:
        return (char*) &regd->mmx[1];
    case MM2:
        return (char*) &regd->mmx[2];
    case MM3:
        return (char*) &regd->mmx[3];
    case MM4:
        return (char*) &regd->mmx[4];
    case MM5:
        return (char*) &regd->mmx[5];
    case MM6:
        return (char*) &regd->mmx[6];
    case MM7:
        return (char*) &regd->mmx[7];

    case x87r0:
        return (char*) &regd->x87FPURegisters[0];
    case x87r1:
        return (char*) &regd->x87FPURegisters[1];
    case x87r2:
        return (char*) &regd->x87FPURegisters[2];
    case x87r3:
        return (char*) &regd->x87FPURegisters[3];
    case x87r4:
        return (char*) &regd->x87FPURegisters[4];
    case x87r5:
        return (char*) &regd->x87FPURegisters[5];
    case x87r6:
        return (char*) &regd->x87FPURegisters[6];
    case x87r7:
        return (char*) &regd->x87FPURegisters[7];

    case x87st0:
        return (char*) &regd->x87FPURegisters[regd->x87StatusWordFields.TOP & 7];
    case x87st1:
        return (char*) &regd->x87FPURegisters[(regd->x87StatusWordFields.TOP + 1) & 7];
    case x87st2:
        return (char*) &regd->x87FPURegisters[(regd->x87StatusWordFields.TOP + 2) & 7];
    case x87st3:
        return (char*) &regd->x87FPURegisters[(regd->x87StatusWordFields.TOP + 3) & 7];
    case x87st4:
        return (char*) &regd->x87FPURegisters[(regd->x87StatusWordFields.TOP + 4) & 7];
    case x87st5:
        return (char*) &regd->x87FPURegisters[(regd->x87StatusWordFields.TOP + 5) & 7];
    case x87st6:
        return (char*) &regd->x87FPURegisters[(regd->x87StatusWordFields.TOP + 6) & 7];
    case x87st7:
        return (char*) &regd->x87FPURegisters[(regd->x87StatusWordFields.TOP + 7) & 7];

    case x87TagWord:
        return (char*) &regd->regcontext.x87fpu.TagWord;

    case x87ControlWord:
        return (char*) &regd->regcontext.x87fpu.ControlWord;

    case x87TW_0:
        return (char*) &regd->x87FPURegisters[0].tag;
    case x87TW_1:
        return (char*) &regd->x87FPURegisters[1].tag;
    case x87TW_2:
        return (char*) &regd->x87FPURegisters[2].tag;
    case x87TW_3:
        return (char*) &regd->x87FPURegisters[3].tag;
    case x87TW_4:
        return (char*) &regd->x87FPURegisters[4].tag;
    case x87TW_5:
        return (char*) &regd->x87FPURegisters[5].tag;
    case x87TW_6:
        return (char*) &regd->x87FPURegisters[6].tag;
    case x87TW_7:
        return (char*) &regd->x87FPURegisters[7].tag;

    case x87CW_IC:
        return (char*) &regd->x87ControlWordFields.IC;
    case x87CW_PM:
        return (char*) &regd->x87ControlWordFields.PM;
    case x87CW_UM:
        return (char*) &regd->x87ControlWordFields.UM;
    case x87CW_OM:
        return (char*) &regd->x87ControlWordFields.OM;
    case x87CW_ZM:
        return (char*) &regd->x87ControlWordFields.ZM;
    case x87CW_DM:
        return (char*) &regd->x87ControlWordFields.DM;
    case x87CW_IM:
        return (char*) &regd->x87ControlWordFields.IM;
    case x87CW_RC:
        return (char*) &regd->x87ControlWordFields.RC;
    case x87CW_PC:
        return (char*) &regd->x87ControlWordFields.PC;

    case x87StatusWord:
        return (char*) &regd->regcontext.x87fpu.StatusWord;

    case x87SW_B:
        return (char*) &regd->x87StatusWordFields.B;
    case x87SW_C3:
        return (char*) &regd->x87StatusWordFields.C3;
    case x87SW_C2:
        return (char*) &regd->x87StatusWordFields.C2;
    case x87SW_C1:
        return (char*) &regd->x87StatusWordFields.C1;
    case x87SW_O:
        return (char*) &regd->x87StatusWordFields.O;
    case x87SW_ES:
        return (char*) &regd->x87StatusWordFields.ES;
    case x87SW_SF:
        return (char*) &regd->x87StatusWordFields.SF;
    case x87SW_P:
        return (char*) &regd->x87StatusWordFields.P;
    case x87SW_U:
        return (char*) &regd->x87StatusWordFields.U;
    case x87SW_Z:
        return (char*) &regd->x87StatusWordFields.Z;
    case x87SW_D:
        return (char*) &regd->x87StatusWordFields.D;
    case x87SW_I:
        return (char*) &regd->x87StatusWordFields.I;
    case x87SW_C0:
        return (char*) &regd->x87StatusWordFields.C0;
    case x87SW_TOP:
        return (char*) &regd->x87StatusWordFields.TOP;

    case MxCsr:
        return (char*) &regd->regcontext.MxCsr;

    case MxCsr_FZ:
        return (char*) &regd->MxCsrFields.FZ;
    case MxCsr_PM:
        return (char*) &regd->MxCsrFields.PM;
    case MxCsr_UM:
        return (char*) &regd->MxCsrFields.UM;
    case MxCsr_OM:
        return (char*) &regd->MxCsrFields.OM;
    case MxCsr_ZM:
        return (char*) &regd->MxCsrFields.ZM;
    case MxCsr_IM:
        return (char*) &regd->MxCsrFields.IM;
    case MxCsr_DM:
        return (char*) &regd->MxCsrFields.DM;
    case MxCsr_DAZ:
        return (char*) &regd->MxCsrFields.DAZ;
    case MxCsr_PE:
        return (char*) &regd->MxCsrFields.PE;
    case MxCsr_UE:
        return (char*) &regd->MxCsrFields.UE;
    case MxCsr_OE:
        return (char*) &regd->MxCsrFields.OE;
    case MxCsr_ZE:
        return (char*) &regd->MxCsrFields.ZE;
    case MxCsr_DE:
        return (char*) &regd->MxCsrFields.DE;
    case MxCsr_IE:
        return (char*) &regd->MxCsrFields.IE;
    case MxCsr_RC:
        return (char*) &regd->MxCsrFields.RC;

    case XMM0:
        return (char*) &regd->regcontext.XmmRegisters[0];
    case XMM1:
        return (char*) &regd->regcontext.XmmRegisters[1];
    case XMM2:
        return (char*) &regd->regcontext.XmmRegisters[2];
    case XMM3:
        return (char*) &regd->regcontext.XmmRegisters[3];
    case XMM4:
        return (char*) &regd->regcontext.XmmRegisters[4];
    case XMM5:
        return (char*) &regd->regcontext.XmmRegisters[5];
    case XMM6:
        return (char*) &regd->regcontext.XmmRegisters[6];
    case XMM7:
        return (char*) &regd->regcontext.XmmRegisters[7];
#ifdef _WIN64
    case XMM8:
        return (char*) &regd->regcontext.XmmRegisters[8];
    case XMM9:
        return (char*) &regd->regcontext.XmmRegisters[9];
    case XMM10:
        return (char*) &regd->regcontext.XmmRegisters[10];
    case XMM11:
        return (char*) &regd->regcontext.XmmRegisters[11];
    case XMM12:
        return (char*) &regd->regcontext.XmmRegisters[12];
    case XMM13:
        return (char*) &regd->regcontext.XmmRegisters[13];
    case XMM14:
        return (char*) &regd->regcontext.XmmRegisters[14];
    case XMM15:
        return (char*) &regd->regcontext.XmmRegisters[15];
#endif //_WIN64

    case YMM0:
        return (char*) &regd->regcontext.YmmRegisters[0];
    case YMM1:
        return (char*) &regd->regcontext.YmmRegisters[1];
    case YMM2:
        return (char*) &regd->regcontext.YmmRegisters[2];
    case YMM3:
        return (char*) &regd->regcontext.YmmRegisters[3];
    case YMM4:
        return (char*) &regd->regcontext.YmmRegisters[4];
    case YMM5:
        return (char*) &regd->regcontext.YmmRegisters[5];
    case YMM6:
        return (char*) &regd->regcontext.YmmRegisters[6];
    case YMM7:
        return (char*) &regd->regcontext.YmmRegisters[7];
#ifdef _WIN64
    case YMM8:
        return (char*) &regd->regcontext.YmmRegisters[8];
    case YMM9:
        return (char*) &regd->regcontext.YmmRegisters[9];
    case YMM10:
        return (char*) &regd->regcontext.YmmRegisters[10];
    case YMM11:
        return (char*) &regd->regcontext.YmmRegisters[11];
    case YMM12:
        return (char*) &regd->regcontext.YmmRegisters[12];
    case YMM13:
        return (char*) &regd->regcontext.YmmRegisters[13];
    case YMM14:
        return (char*) &regd->regcontext.YmmRegisters[14];
    case YMM15:
        return (char*) &regd->regcontext.YmmRegisters[15];
#endif //_WIN64
    }

    return (char*) &null_value;
}

void RegistersView::setRegisters(REGDUMP* reg)
{
    // tests if new-register-value == old-register-value holds
    if(mCip != reg->regcontext.cip) //CIP changed
    {
        wCipRegDumpStruct = wRegDumpStruct;
        mRegisterUpdates.clear();
        mCip = reg->regcontext.cip;
    }

    // iterate all ids (CAX, CBX, ...)
    for(auto itr = mRegisterMapping.begin(); itr != mRegisterMapping.end(); itr++)
    {
        if(CompareRegisters(itr.key(), reg, &wCipRegDumpStruct) != 0)
            mRegisterUpdates.insert(itr.key());
        else if(mRegisterUpdates.contains(itr.key())) //registers are equal
            mRegisterUpdates.remove(itr.key());
    }

    // now we can save the values
    wRegDumpStruct = (*reg);

    if(mCip != reg->regcontext.cip)
        wCipRegDumpStruct = wRegDumpStruct;

    // force repaint
    emit refresh();
}

void RegistersView::onSIMDMode()
{
    wSIMDRegDispMode = (SIMD_REG_DISP_MODE)(dynamic_cast<QAction*>(sender())->data().toInt());
    emit refresh();
}

void RegistersView::onFpuMode()
{
    mFpuMode = !mFpuMode;
    InitMappings();
    emit refresh();
}

void RegistersView::disasmSelectionChangedSlot(dsint va)
{
    mHighlightRegs = mParent->getDisasmWidget()->DisassembleAt(va - mParent->getDisasmWidget()->getBase()).regsReferenced;
    emit refresh();
}
