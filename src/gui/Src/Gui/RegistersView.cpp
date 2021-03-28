#include <QMessageBox>
#include <QListWidget>
#include <QToolTip>
#include <stdint.h>
#include "RegistersView.h"
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
    mRegisterRelativePlaces.clear();
    int offset = 0;

    /* Register_Position is a struct definition the position
     *
     * (line , start, labelwidth, valuesize )
     */
#ifdef _WIN64
    mRegisterMapping.insert(CAX, "RAX");
    mRegisterPlaces.insert(CAX, Register_Position(offset++, 0, 6, sizeof(duint) * 2));
    mRegisterRelativePlaces.insert(CAX, Register_Relative_Position(UNKNOWN, CBX));
    mRegisterMapping.insert(CBX, "RBX");
    mRegisterPlaces.insert(CBX, Register_Position(offset++, 0, 6, sizeof(duint) * 2));
    mRegisterRelativePlaces.insert(CBX, Register_Relative_Position(CAX, CCX));
    mRegisterMapping.insert(CCX, "RCX");
    mRegisterPlaces.insert(CCX, Register_Position(offset++, 0, 6, sizeof(duint) * 2));
    mRegisterRelativePlaces.insert(CCX, Register_Relative_Position(CBX, CDX));
    mRegisterMapping.insert(CDX, "RDX");
    mRegisterPlaces.insert(CDX, Register_Position(offset++, 0, 6, sizeof(duint) * 2));
    mRegisterRelativePlaces.insert(CDX, Register_Relative_Position(CCX, CBP));
    mRegisterMapping.insert(CBP, "RBP");
    mRegisterPlaces.insert(CBP, Register_Position(offset++, 0, 6, sizeof(duint) * 2));
    mRegisterRelativePlaces.insert(CBP, Register_Relative_Position(CDX, CSP));
    mRegisterMapping.insert(CSP, "RSP");
    mRegisterPlaces.insert(CSP, Register_Position(offset++, 0, 6, sizeof(duint) * 2));
    mRegisterRelativePlaces.insert(CSP, Register_Relative_Position(CBP, CSI));
    mRegisterMapping.insert(CSI, "RSI");
    mRegisterPlaces.insert(CSI, Register_Position(offset++, 0, 6, sizeof(duint) * 2));
    mRegisterRelativePlaces.insert(CSI, Register_Relative_Position(CSP, CDI));
    mRegisterMapping.insert(CDI, "RDI");
    mRegisterPlaces.insert(CDI, Register_Position(offset++, 0, 6, sizeof(duint) * 2));
    mRegisterRelativePlaces.insert(CDI, Register_Relative_Position(CSI, R8));

    offset++;

    mRegisterMapping.insert(R8, "R8");
    mRegisterPlaces.insert(R8, Register_Position(offset++, 0, 6, sizeof(duint) * 2));
    mRegisterRelativePlaces.insert(R8, Register_Relative_Position(CDI, R9));
    mRegisterMapping.insert(R9, "R9");
    mRegisterPlaces.insert(R9, Register_Position(offset++, 0, 6, sizeof(duint) * 2));
    mRegisterRelativePlaces.insert(R9, Register_Relative_Position(R8, R10));
    mRegisterMapping.insert(R10, "R10");
    mRegisterPlaces.insert(R10, Register_Position(offset++, 0, 6, sizeof(duint) * 2));
    mRegisterRelativePlaces.insert(R10, Register_Relative_Position(R9, R11));
    mRegisterMapping.insert(R11, "R11");
    mRegisterPlaces.insert(R11, Register_Position(offset++, 0, 6, sizeof(duint) * 2));
    mRegisterRelativePlaces.insert(R11, Register_Relative_Position(R10, R12));
    mRegisterMapping.insert(R12, "R12");
    mRegisterPlaces.insert(R12, Register_Position(offset++, 0, 6, sizeof(duint) * 2));
    mRegisterRelativePlaces.insert(R12, Register_Relative_Position(R11, R13));
    mRegisterMapping.insert(R13, "R13");
    mRegisterPlaces.insert(R13, Register_Position(offset++, 0, 6, sizeof(duint) * 2));
    mRegisterRelativePlaces.insert(R13, Register_Relative_Position(R12, R14));
    mRegisterMapping.insert(R14, "R14");
    mRegisterPlaces.insert(R14, Register_Position(offset++, 0, 6, sizeof(duint) * 2));
    mRegisterRelativePlaces.insert(R14, Register_Relative_Position(R13, R15));
    mRegisterMapping.insert(R15, "R15");
    mRegisterPlaces.insert(R15, Register_Position(offset++, 0, 6, sizeof(duint) * 2));
    mRegisterRelativePlaces.insert(R15, Register_Relative_Position(R14, CIP));

    offset++;

    mRegisterMapping.insert(CIP, "RIP");
    mRegisterPlaces.insert(CIP, Register_Position(offset++, 0, 6, sizeof(duint) * 2));
    mRegisterRelativePlaces.insert(CIP, Register_Relative_Position(R15, EFLAGS));

    offset++;

    mRegisterMapping.insert(EFLAGS, "RFLAGS");
    mRegisterPlaces.insert(EFLAGS, Register_Position(offset++, 0, 9, sizeof(duint) * 2));
    mRegisterRelativePlaces.insert(EFLAGS, Register_Relative_Position(CIP, ZF));
#else //x32
    mRegisterMapping.insert(CAX, "EAX");
    mRegisterPlaces.insert(CAX, Register_Position(offset++, 0, 6, sizeof(duint) * 2));
    mRegisterRelativePlaces.insert(CAX, Register_Relative_Position(UNKNOWN, CBX));
    mRegisterMapping.insert(CBX, "EBX");
    mRegisterPlaces.insert(CBX, Register_Position(offset++, 0, 6, sizeof(duint) * 2));
    mRegisterRelativePlaces.insert(CBX, Register_Relative_Position(CAX, CCX));
    mRegisterMapping.insert(CCX, "ECX");
    mRegisterPlaces.insert(CCX, Register_Position(offset++, 0, 6, sizeof(duint) * 2));
    mRegisterRelativePlaces.insert(CCX, Register_Relative_Position(CBX, CDX));
    mRegisterMapping.insert(CDX, "EDX");
    mRegisterPlaces.insert(CDX, Register_Position(offset++, 0, 6, sizeof(duint) * 2));
    mRegisterRelativePlaces.insert(CDX, Register_Relative_Position(CCX, CBP));
    mRegisterMapping.insert(CBP, "EBP");
    mRegisterPlaces.insert(CBP, Register_Position(offset++, 0, 6, sizeof(duint) * 2));
    mRegisterRelativePlaces.insert(CBP, Register_Relative_Position(CDX, CSP));
    mRegisterMapping.insert(CSP, "ESP");
    mRegisterPlaces.insert(CSP, Register_Position(offset++, 0, 6, sizeof(duint) * 2));
    mRegisterRelativePlaces.insert(CSP, Register_Relative_Position(CBP, CSI));
    mRegisterMapping.insert(CSI, "ESI");
    mRegisterPlaces.insert(CSI, Register_Position(offset++, 0, 6, sizeof(duint) * 2));
    mRegisterRelativePlaces.insert(CSI, Register_Relative_Position(CSP, CDI));
    mRegisterMapping.insert(CDI, "EDI");
    mRegisterPlaces.insert(CDI, Register_Position(offset++, 0, 6, sizeof(duint) * 2));
    mRegisterRelativePlaces.insert(CDI, Register_Relative_Position(CSI, CIP));

    offset++;

    mRegisterMapping.insert(CIP, "EIP");
    mRegisterPlaces.insert(CIP, Register_Position(offset++, 0, 6, sizeof(duint) * 2));
    mRegisterRelativePlaces.insert(CIP, Register_Relative_Position(CDI, EFLAGS));

    offset++;

    mRegisterMapping.insert(EFLAGS, "EFLAGS");
    mRegisterPlaces.insert(EFLAGS, Register_Position(offset++, 0, 9, sizeof(duint) * 2));
    mRegisterRelativePlaces.insert(EFLAGS, Register_Relative_Position(CIP, ZF));
#endif

    mRegisterMapping.insert(ZF, "ZF");
    mRegisterPlaces.insert(ZF, Register_Position(offset, 0, 3, 1));
    mRegisterRelativePlaces.insert(ZF, Register_Relative_Position(EFLAGS, PF, EFLAGS, OF));
    mRegisterMapping.insert(PF, "PF");
    mRegisterPlaces.insert(PF, Register_Position(offset, 6, 3, 1));
    mRegisterRelativePlaces.insert(PF, Register_Relative_Position(ZF, AF, EFLAGS, SF));
    mRegisterMapping.insert(AF, "AF");
    mRegisterPlaces.insert(AF, Register_Position(offset++, 12, 3, 1));
    mRegisterRelativePlaces.insert(AF, Register_Relative_Position(PF, OF, EFLAGS, DF));

    mRegisterMapping.insert(OF, "OF");
    mRegisterPlaces.insert(OF, Register_Position(offset, 0, 3, 1));
    mRegisterRelativePlaces.insert(OF, Register_Relative_Position(AF, SF, ZF, CF));
    mRegisterMapping.insert(SF, "SF");
    mRegisterPlaces.insert(SF, Register_Position(offset, 6, 3, 1));
    mRegisterRelativePlaces.insert(SF, Register_Relative_Position(OF, DF, PF, TF));
    mRegisterMapping.insert(DF, "DF");
    mRegisterPlaces.insert(DF, Register_Position(offset++, 12, 3, 1));
    mRegisterRelativePlaces.insert(DF, Register_Relative_Position(SF, CF, AF, IF));

    mRegisterMapping.insert(CF, "CF");
    mRegisterPlaces.insert(CF, Register_Position(offset, 0, 3, 1));
    mRegisterRelativePlaces.insert(CF, Register_Relative_Position(DF, TF, OF, LastError));
    mRegisterMapping.insert(TF, "TF");
    mRegisterPlaces.insert(TF, Register_Position(offset, 6, 3, 1));
    mRegisterRelativePlaces.insert(TF, Register_Relative_Position(CF, IF, SF, LastError));
    mRegisterMapping.insert(IF, "IF");
    mRegisterPlaces.insert(IF, Register_Position(offset++, 12, 3, 1));
    mRegisterRelativePlaces.insert(IF, Register_Relative_Position(TF, LastError, DF, LastError));

    offset++;

    mRegisterMapping.insert(LastError, "LastError");
    mRegisterPlaces.insert(LastError, Register_Position(offset++, 0, 11, 20));
    mRegisterRelativePlaces.insert(LastError, Register_Relative_Position(IF, LastStatus));
    mMODIFYDISPLAY.insert(LastError);
    mRegisterMapping.insert(LastStatus, "LastStatus");
    mRegisterPlaces.insert(LastStatus, Register_Position(offset++, 0, 11, 20));
    mRegisterRelativePlaces.insert(LastStatus, Register_Relative_Position(LastError, GS));
    mMODIFYDISPLAY.insert(LastStatus);

    offset++;

    mRegisterMapping.insert(GS, "GS");
    mRegisterPlaces.insert(GS, Register_Position(offset, 0, 3, 4));
    mRegisterRelativePlaces.insert(GS, Register_Relative_Position(LastStatus, FS, LastStatus, ES));
    mRegisterMapping.insert(FS, "FS");
    mRegisterPlaces.insert(FS, Register_Position(offset++, 9, 3, 4));
    mRegisterRelativePlaces.insert(FS, Register_Relative_Position(GS, ES, LastStatus, DS));
    mRegisterMapping.insert(ES, "ES");
    mRegisterPlaces.insert(ES, Register_Position(offset, 0, 3, 4));
    mRegisterRelativePlaces.insert(ES, Register_Relative_Position(FS, DS, GS, CS));
    mRegisterMapping.insert(DS, "DS");
    mRegisterPlaces.insert(DS, Register_Position(offset++, 9, 3, 4));
    mRegisterRelativePlaces.insert(DS, Register_Relative_Position(ES, CS, FS, SS));
    mRegisterMapping.insert(CS, "CS");
    mRegisterPlaces.insert(CS, Register_Position(offset, 0, 3, 4));
    mRegisterMapping.insert(SS, "SS");
    mRegisterPlaces.insert(SS, Register_Position(offset++, 9, 3, 4));


    if(mShowFpu)
    {
        REGISTER_NAME tempRegisterName;
        offset++;

        if(mFpuMode == 1)
        {
            tempRegisterName = x87r0;

            mRegisterMapping.insert(x87r0, "x87r0");
            mRegisterPlaces.insert(x87r0, Register_Position(offset++, 0, 6, 10 * 2));
            mRegisterRelativePlaces.insert(x87r0, Register_Relative_Position(SS, x87r1));
            mRegisterMapping.insert(x87r1, "x87r1");
            mRegisterPlaces.insert(x87r1, Register_Position(offset++, 0, 6, 10 * 2));
            mRegisterRelativePlaces.insert(x87r1, Register_Relative_Position(x87r0, x87r2));
            mRegisterMapping.insert(x87r2, "x87r2");
            mRegisterPlaces.insert(x87r2, Register_Position(offset++, 0, 6, 10 * 2));
            mRegisterRelativePlaces.insert(x87r2, Register_Relative_Position(x87r1, x87r3));
            mRegisterMapping.insert(x87r3, "x87r3");
            mRegisterPlaces.insert(x87r3, Register_Position(offset++, 0, 6, 10 * 2));
            mRegisterRelativePlaces.insert(x87r3, Register_Relative_Position(x87r2, x87r4));
            mRegisterMapping.insert(x87r4, "x87r4");
            mRegisterPlaces.insert(x87r4, Register_Position(offset++, 0, 6, 10 * 2));
            mRegisterRelativePlaces.insert(x87r4, Register_Relative_Position(x87r3, x87r5));
            mRegisterMapping.insert(x87r5, "x87r5");
            mRegisterPlaces.insert(x87r5, Register_Position(offset++, 0, 6, 10 * 2));
            mRegisterRelativePlaces.insert(x87r5, Register_Relative_Position(x87r4, x87r6));
            mRegisterMapping.insert(x87r6, "x87r6");
            mRegisterPlaces.insert(x87r6, Register_Position(offset++, 0, 6, 10 * 2));
            mRegisterRelativePlaces.insert(x87r6, Register_Relative_Position(x87r5, x87r7));
            mRegisterMapping.insert(x87r7, "x87r7");
            mRegisterPlaces.insert(x87r7, Register_Position(offset++, 0, 6, 10 * 2));
            mRegisterRelativePlaces.insert(x87r7, Register_Relative_Position(x87r6, x87TagWord));

        }
        else if(mFpuMode == 0)
        {
            tempRegisterName = x87st0;

            mRegisterMapping.insert(x87st0, "ST(0)");
            mRegisterPlaces.insert(x87st0, Register_Position(offset++, 0, 6, 10 * 2));
            mRegisterRelativePlaces.insert(x87st0, Register_Relative_Position(SS, x87st1));
            mRegisterMapping.insert(x87st1, "ST(1)");
            mRegisterPlaces.insert(x87st1, Register_Position(offset++, 0, 6, 10 * 2));
            mRegisterRelativePlaces.insert(x87st1, Register_Relative_Position(x87st0, x87st2));
            mRegisterMapping.insert(x87st2, "ST(2)");
            mRegisterPlaces.insert(x87st2, Register_Position(offset++, 0, 6, 10 * 2));
            mRegisterRelativePlaces.insert(x87st2, Register_Relative_Position(x87st1, x87st3));
            mRegisterMapping.insert(x87st3, "ST(3)");
            mRegisterPlaces.insert(x87st3, Register_Position(offset++, 0, 6, 10 * 2));
            mRegisterRelativePlaces.insert(x87st3, Register_Relative_Position(x87st2, x87st4));
            mRegisterMapping.insert(x87st4, "ST(4)");
            mRegisterPlaces.insert(x87st4, Register_Position(offset++, 0, 6, 10 * 2));
            mRegisterRelativePlaces.insert(x87st4, Register_Relative_Position(x87st3, x87st5));
            mRegisterMapping.insert(x87st5, "ST(5)");
            mRegisterPlaces.insert(x87st5, Register_Position(offset++, 0, 6, 10 * 2));
            mRegisterRelativePlaces.insert(x87st5, Register_Relative_Position(x87st4, x87st6));
            mRegisterMapping.insert(x87st6, "ST(6)");
            mRegisterPlaces.insert(x87st6, Register_Position(offset++, 0, 6, 10 * 2));
            mRegisterRelativePlaces.insert(x87st6, Register_Relative_Position(x87st5, x87st7));
            mRegisterMapping.insert(x87st7, "ST(7)");
            mRegisterPlaces.insert(x87st7, Register_Position(offset++, 0, 6, 10 * 2));
            mRegisterRelativePlaces.insert(x87st7, Register_Relative_Position(x87st6, x87TagWord));

        }
        else if(mFpuMode == 2)
        {
            tempRegisterName = MM0;

            mRegisterMapping.insert(MM0, "MM0");
            mRegisterPlaces.insert(MM0, Register_Position(offset++, 0, 4, 8 * 2));
            mRegisterRelativePlaces.insert(MM0, Register_Relative_Position(SS, MM1));
            mRegisterMapping.insert(MM1, "MM1");
            mRegisterPlaces.insert(MM1, Register_Position(offset++, 0, 4, 8 * 2));
            mRegisterRelativePlaces.insert(MM1, Register_Relative_Position(MM0, MM2));
            mRegisterMapping.insert(MM2, "MM2");
            mRegisterPlaces.insert(MM2, Register_Position(offset++, 0, 4, 8 * 2));
            mRegisterRelativePlaces.insert(MM2, Register_Relative_Position(MM1, MM3));
            mRegisterMapping.insert(MM3, "MM3");
            mRegisterPlaces.insert(MM3, Register_Position(offset++, 0, 4, 8 * 2));
            mRegisterRelativePlaces.insert(MM3, Register_Relative_Position(MM2, MM4));
            mRegisterMapping.insert(MM4, "MM4");
            mRegisterPlaces.insert(MM4, Register_Position(offset++, 0, 4, 8 * 2));
            mRegisterRelativePlaces.insert(MM4, Register_Relative_Position(MM3, MM5));
            mRegisterMapping.insert(MM5, "MM5");
            mRegisterPlaces.insert(MM5, Register_Position(offset++, 0, 4, 8 * 2));
            mRegisterRelativePlaces.insert(MM5, Register_Relative_Position(MM4, MM6));
            mRegisterMapping.insert(MM6, "MM6");
            mRegisterPlaces.insert(MM6, Register_Position(offset++, 0, 4, 8 * 2));
            mRegisterRelativePlaces.insert(MM6, Register_Relative_Position(MM5, MM7));
            mRegisterMapping.insert(MM7, "MM7");
            mRegisterPlaces.insert(MM7, Register_Position(offset++, 0, 4, 8 * 2));
            mRegisterRelativePlaces.insert(MM7, Register_Relative_Position(MM6, x87TagWord));
        }
        mRegisterRelativePlaces.insert(CS, Register_Relative_Position(DS, SS, ES, tempRegisterName));
        mRegisterRelativePlaces.insert(SS, Register_Relative_Position(CS, tempRegisterName, DS, tempRegisterName));

        offset++;

        mRegisterMapping.insert(x87TagWord, "x87TagWord");
        mRegisterPlaces.insert(x87TagWord, Register_Position(offset++, 0, 11, sizeof(WORD) * 2));

        switch(mFpuMode)
        {
        case 0:
            tempRegisterName = x87st7;
            break;
        case 1:
            tempRegisterName = x87r7;
            break;
        case 2:
            tempRegisterName = MM7;
            break;
        }
        mRegisterRelativePlaces.insert(x87TagWord, Register_Relative_Position(tempRegisterName, x87TW_0));

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
        mRegisterRelativePlaces.insert(x87TW_0, Register_Relative_Position(x87TagWord, x87TW_1, x87TagWord, x87TW_2));
        mRegisterMapping.insert(x87TW_1, "x87TW_1");
        mRegisterPlaces.insert(x87TW_1, Register_Position(offset++, NextColumnPosition, 8, 10));
        mRegisterRelativePlaces.insert(x87TW_1, Register_Relative_Position(x87TW_0, x87TW_2, x87TagWord, x87TW_3));

        mRegisterMapping.insert(x87TW_2, "x87TW_2");
        mRegisterPlaces.insert(x87TW_2, Register_Position(offset, 0, 8, 10));
        mRegisterRelativePlaces.insert(x87TW_2, Register_Relative_Position(x87TW_1, x87TW_3, x87TW_0, x87TW_4));
        mRegisterMapping.insert(x87TW_3, "x87TW_3");
        mRegisterPlaces.insert(x87TW_3, Register_Position(offset++, NextColumnPosition, 8, 10));
        mRegisterRelativePlaces.insert(x87TW_3, Register_Relative_Position(x87TW_2, x87TW_4, x87TW_1, x87TW_5));

        mRegisterMapping.insert(x87TW_4, "x87TW_4");
        mRegisterPlaces.insert(x87TW_4, Register_Position(offset, 0, 8, 10));
        mRegisterRelativePlaces.insert(x87TW_4, Register_Relative_Position(x87TW_3, x87TW_5, x87TW_2, x87TW_6));
        mRegisterMapping.insert(x87TW_5, "x87TW_5");
        mRegisterPlaces.insert(x87TW_5, Register_Position(offset++, NextColumnPosition, 8, 10));
        mRegisterRelativePlaces.insert(x87TW_5, Register_Relative_Position(x87TW_4, x87TW_6, x87TW_3, x87TW_7));

        mRegisterMapping.insert(x87TW_6, "x87TW_6");
        mRegisterPlaces.insert(x87TW_6, Register_Position(offset, 0, 8, 10));
        mRegisterRelativePlaces.insert(x87TW_6, Register_Relative_Position(x87TW_5, x87TW_7, x87TW_4, x87StatusWord));
        mRegisterMapping.insert(x87TW_7, "x87TW_7");
        mRegisterPlaces.insert(x87TW_7, Register_Position(offset++, NextColumnPosition, 8, 10));
        mRegisterRelativePlaces.insert(x87TW_7, Register_Relative_Position(x87TW_6, x87StatusWord, x87TW_5, x87StatusWord));


        offset++;

        mRegisterMapping.insert(x87StatusWord, "x87StatusWord");
        mRegisterPlaces.insert(x87StatusWord, Register_Position(offset++, 0, 14, sizeof(WORD) * 2));
        mRegisterRelativePlaces.insert(x87StatusWord, Register_Relative_Position(x87TW_7, x87SW_B));

        mRegisterMapping.insert(x87SW_B, "x87SW_B");
        mRegisterPlaces.insert(x87SW_B, Register_Position(offset, 0, 9, 1));
        mRegisterRelativePlaces.insert(x87SW_B, Register_Relative_Position(x87StatusWord, x87SW_C3, x87StatusWord, x87SW_C1));
        mRegisterMapping.insert(x87SW_C3, "x87SW_C3");
        mRegisterPlaces.insert(x87SW_C3, Register_Position(offset, 12, 10, 1));
        mRegisterRelativePlaces.insert(x87SW_C3, Register_Relative_Position(x87SW_B, x87SW_C2, x87StatusWord, x87SW_C0));
        mRegisterMapping.insert(x87SW_C2, "x87SW_C2");
        mRegisterPlaces.insert(x87SW_C2, Register_Position(offset++, 25, 10, 1));
        mRegisterRelativePlaces.insert(x87SW_C2, Register_Relative_Position(x87SW_C3, x87SW_C1, x87StatusWord, x87SW_ES));

        mRegisterMapping.insert(x87SW_C1, "x87SW_C1");
        mRegisterPlaces.insert(x87SW_C1, Register_Position(offset, 0, 9, 1));
        mRegisterRelativePlaces.insert(x87SW_C1, Register_Relative_Position(x87SW_C2, x87SW_C0, x87SW_B, x87SW_SF));
        mRegisterMapping.insert(x87SW_C0, "x87SW_C0");
        mRegisterPlaces.insert(x87SW_C0, Register_Position(offset, 12, 10, 1));
        mRegisterRelativePlaces.insert(x87SW_C0, Register_Relative_Position(x87SW_C1, x87SW_ES, x87SW_C3, x87SW_P));
        mRegisterMapping.insert(x87SW_ES, "x87SW_ES");
        mRegisterPlaces.insert(x87SW_ES, Register_Position(offset++, 25, 10, 1));
        mRegisterRelativePlaces.insert(x87SW_ES, Register_Relative_Position(x87SW_C0, x87SW_SF, x87SW_C2, x87SW_U));

        mRegisterMapping.insert(x87SW_SF, "x87SW_SF");
        mRegisterPlaces.insert(x87SW_SF, Register_Position(offset, 0, 9, 1));
        mRegisterRelativePlaces.insert(x87SW_SF, Register_Relative_Position(x87SW_ES, x87SW_P, x87SW_C1, x87SW_O));
        mRegisterMapping.insert(x87SW_P, "x87SW_P");
        mRegisterPlaces.insert(x87SW_P, Register_Position(offset, 12, 10, 1));
        mRegisterRelativePlaces.insert(x87SW_P, Register_Relative_Position(x87SW_SF, x87SW_U, x87SW_C0, x87SW_Z));
        mRegisterMapping.insert(x87SW_U, "x87SW_U");
        mRegisterPlaces.insert(x87SW_U, Register_Position(offset++, 25, 10, 1));
        mRegisterRelativePlaces.insert(x87SW_U, Register_Relative_Position(x87SW_P, x87SW_O, x87SW_ES, x87SW_D));

        mRegisterMapping.insert(x87SW_O, "x87SW_O");
        mRegisterPlaces.insert(x87SW_O, Register_Position(offset, 0, 9, 1));
        mRegisterRelativePlaces.insert(x87SW_O, Register_Relative_Position(x87SW_U, x87SW_Z, x87SW_SF, x87SW_I));
        mRegisterMapping.insert(x87SW_Z, "x87SW_Z");
        mRegisterPlaces.insert(x87SW_Z, Register_Position(offset, 12, 10, 1));
        mRegisterRelativePlaces.insert(x87SW_Z, Register_Relative_Position(x87SW_O, x87SW_D, x87SW_P, x87SW_TOP));
        mRegisterMapping.insert(x87SW_D, "x87SW_D");
        mRegisterPlaces.insert(x87SW_D, Register_Position(offset++, 25, 10, 1));
        mRegisterRelativePlaces.insert(x87SW_D, Register_Relative_Position(x87SW_Z, x87SW_I, x87SW_U, x87SW_TOP));

        mRegisterMapping.insert(x87SW_I, "x87SW_I");
        mRegisterPlaces.insert(x87SW_I, Register_Position(offset, 0, 9, 1));
        mRegisterRelativePlaces.insert(x87SW_I, Register_Relative_Position(x87SW_D, x87SW_TOP, x87SW_O, x87ControlWord));
        mRegisterMapping.insert(x87SW_TOP, "x87SW_TOP");
        mRegisterPlaces.insert(x87SW_TOP, Register_Position(offset++, 12, 10, 13));
        mRegisterRelativePlaces.insert(x87SW_TOP, Register_Relative_Position(x87SW_I, x87ControlWord, x87SW_Z, x87ControlWord));

        offset++;

        mRegisterMapping.insert(x87ControlWord, "x87ControlWord");
        mRegisterPlaces.insert(x87ControlWord, Register_Position(offset++, 0, 15, sizeof(WORD) * 2));
        mRegisterRelativePlaces.insert(x87ControlWord, Register_Relative_Position(x87SW_TOP, x87CW_IC));

        mRegisterMapping.insert(x87CW_IC, "x87CW_IC");
        mRegisterPlaces.insert(x87CW_IC, Register_Position(offset, 0, 9, 1));
        mRegisterRelativePlaces.insert(x87CW_IC, Register_Relative_Position(x87ControlWord, x87CW_ZM, x87ControlWord, x87CW_UM));
        mRegisterMapping.insert(x87CW_ZM, "x87CW_ZM");
        mRegisterPlaces.insert(x87CW_ZM, Register_Position(offset, 12, 10, 1));
        mRegisterRelativePlaces.insert(x87CW_ZM, Register_Relative_Position(x87CW_IC, x87CW_PM, x87ControlWord, x87CW_OM));
        mRegisterMapping.insert(x87CW_PM, "x87CW_PM");
        mRegisterPlaces.insert(x87CW_PM, Register_Position(offset++, 25, 10, 1));
        mRegisterRelativePlaces.insert(x87CW_PM, Register_Relative_Position(x87CW_ZM, x87CW_UM, x87ControlWord, x87CW_PC));

        mRegisterMapping.insert(x87CW_UM, "x87CW_UM");
        mRegisterPlaces.insert(x87CW_UM, Register_Position(offset, 0, 9, 1));
        mRegisterRelativePlaces.insert(x87CW_UM, Register_Relative_Position(x87CW_PM, x87CW_OM, x87CW_IC, x87CW_DM));
        mRegisterMapping.insert(x87CW_OM, "x87CW_OM");
        mRegisterPlaces.insert(x87CW_OM, Register_Position(offset, 12, 10, 1));
        mRegisterRelativePlaces.insert(x87CW_OM, Register_Relative_Position(x87CW_UM, x87CW_PC, x87CW_ZM, x87CW_IM));
        mRegisterMapping.insert(x87CW_PC, "x87CW_PC");
        mRegisterPlaces.insert(x87CW_PC, Register_Position(offset++, 25, 10, 14));
        mRegisterRelativePlaces.insert(x87CW_PC, Register_Relative_Position(x87CW_OM, x87CW_DM, x87CW_PM, x87CW_RC));

        mRegisterMapping.insert(x87CW_DM, "x87CW_DM");
        mRegisterPlaces.insert(x87CW_DM, Register_Position(offset, 0, 9, 1));
        mRegisterRelativePlaces.insert(x87CW_DM, Register_Relative_Position(x87CW_PC, x87CW_IM, x87CW_UM, MxCsr));
        mRegisterMapping.insert(x87CW_IM, "x87CW_IM");
        mRegisterPlaces.insert(x87CW_IM, Register_Position(offset, 12, 10, 1));
        mRegisterRelativePlaces.insert(x87CW_IM, Register_Relative_Position(x87CW_DM, x87CW_RC, x87CW_OM, MxCsr));
        mRegisterMapping.insert(x87CW_RC, "x87CW_RC");
        mRegisterPlaces.insert(x87CW_RC, Register_Position(offset++, 25, 10, 14));
        mRegisterRelativePlaces.insert(x87CW_RC, Register_Relative_Position(x87CW_IM, MxCsr, x87CW_PC, MxCsr));

        offset++;

        mRegisterMapping.insert(MxCsr, "MxCsr");
        mRegisterPlaces.insert(MxCsr, Register_Position(offset++, 0, 6, sizeof(DWORD) * 2));
        mRegisterRelativePlaces.insert(MxCsr, Register_Relative_Position(x87CW_RC, MxCsr_FZ));

        mRegisterMapping.insert(MxCsr_FZ, "MxCsr_FZ");
        mRegisterPlaces.insert(MxCsr_FZ, Register_Position(offset, 0, 9, 1));
        mRegisterRelativePlaces.insert(MxCsr_FZ, Register_Relative_Position(MxCsr, MxCsr_PM, MxCsr, MxCsr_OM));
        mRegisterMapping.insert(MxCsr_PM, "MxCsr_PM");
        mRegisterPlaces.insert(MxCsr_PM, Register_Position(offset, 12, 10, 1));
        mRegisterRelativePlaces.insert(MxCsr_PM, Register_Relative_Position(MxCsr_FZ, MxCsr_UM, MxCsr, MxCsr_ZM));
        mRegisterMapping.insert(MxCsr_UM, "MxCsr_UM");
        mRegisterPlaces.insert(MxCsr_UM, Register_Position(offset++, 25, 10, 1));
        mRegisterRelativePlaces.insert(MxCsr_UM, Register_Relative_Position(MxCsr_PM, MxCsr_OM, MxCsr, MxCsr_IM));

        mRegisterMapping.insert(MxCsr_OM, "MxCsr_OM");
        mRegisterPlaces.insert(MxCsr_OM, Register_Position(offset, 0, 9, 1));
        mRegisterRelativePlaces.insert(MxCsr_OM, Register_Relative_Position(MxCsr_UM, MxCsr_ZM, MxCsr_FZ, MxCsr_UE));
        mRegisterMapping.insert(MxCsr_ZM, "MxCsr_ZM");
        mRegisterPlaces.insert(MxCsr_ZM, Register_Position(offset, 12, 10, 1));
        mRegisterRelativePlaces.insert(MxCsr_ZM, Register_Relative_Position(MxCsr_OM, MxCsr_IM, MxCsr_PM, MxCsr_PE));
        mRegisterMapping.insert(MxCsr_IM, "MxCsr_IM");
        mRegisterPlaces.insert(MxCsr_IM, Register_Position(offset++, 25, 10, 1));
        mRegisterRelativePlaces.insert(MxCsr_IM, Register_Relative_Position(MxCsr_ZM, MxCsr_UE, MxCsr_UM, MxCsr_DAZ));

        mRegisterMapping.insert(MxCsr_UE, "MxCsr_UE");
        mRegisterPlaces.insert(MxCsr_UE, Register_Position(offset, 0, 9, 1));
        mRegisterRelativePlaces.insert(MxCsr_UE, Register_Relative_Position(MxCsr_IM, MxCsr_PE, MxCsr_OM, MxCsr_OE));
        mRegisterMapping.insert(MxCsr_PE, "MxCsr_PE");
        mRegisterPlaces.insert(MxCsr_PE, Register_Position(offset, 12, 10, 1));
        mRegisterRelativePlaces.insert(MxCsr_PE, Register_Relative_Position(MxCsr_UE, MxCsr_DAZ, MxCsr_ZM, MxCsr_ZE));
        mRegisterMapping.insert(MxCsr_DAZ, "MxCsr_DAZ");
        mRegisterPlaces.insert(MxCsr_DAZ, Register_Position(offset++, 25, 10, 1));
        mRegisterRelativePlaces.insert(MxCsr_DAZ, Register_Relative_Position(MxCsr_PE, MxCsr_OE, MxCsr_IM, MxCsr_DE));

        mRegisterMapping.insert(MxCsr_OE, "MxCsr_OE");
        mRegisterPlaces.insert(MxCsr_OE, Register_Position(offset, 0, 9, 1));
        mRegisterRelativePlaces.insert(MxCsr_OE, Register_Relative_Position(MxCsr_DAZ, MxCsr_ZE, MxCsr_UE, MxCsr_IE));
        mRegisterMapping.insert(MxCsr_ZE, "MxCsr_ZE");
        mRegisterPlaces.insert(MxCsr_ZE, Register_Position(offset, 12, 10, 1));
        mRegisterRelativePlaces.insert(MxCsr_ZE, Register_Relative_Position(MxCsr_OE, MxCsr_DE, MxCsr_PE, MxCsr_DM));
        mRegisterMapping.insert(MxCsr_DE, "MxCsr_DE");
        mRegisterPlaces.insert(MxCsr_DE, Register_Position(offset++, 25, 10, 1));
        mRegisterRelativePlaces.insert(MxCsr_DE, Register_Relative_Position(MxCsr_ZE, MxCsr_IE, MxCsr_DAZ, MxCsr_RC));

        mRegisterMapping.insert(MxCsr_IE, "MxCsr_IE");
        mRegisterPlaces.insert(MxCsr_IE, Register_Position(offset, 0, 9, 1));
        mRegisterRelativePlaces.insert(MxCsr_IE, Register_Relative_Position(MxCsr_DE, MxCsr_DM, MxCsr_OE, XMM0));
        mRegisterMapping.insert(MxCsr_DM, "MxCsr_DM");
        mRegisterPlaces.insert(MxCsr_DM, Register_Position(offset, 12, 10, 1));
        mRegisterRelativePlaces.insert(MxCsr_DM, Register_Relative_Position(MxCsr_IE, MxCsr_RC, MxCsr_ZE, XMM0));
        mRegisterMapping.insert(MxCsr_RC, "MxCsr_RC");
        mRegisterPlaces.insert(MxCsr_RC, Register_Position(offset++, 25, 10, 19));
        mRegisterRelativePlaces.insert(MxCsr_RC, Register_Relative_Position(MxCsr_DM, XMM0, MxCsr_DE, XMM0));

        offset++;

        mRegisterMapping.insert(XMM0, "XMM0");
        mRegisterPlaces.insert(XMM0, Register_Position(offset++, 0, 6, 16 * 2));
        mRegisterRelativePlaces.insert(XMM0, Register_Relative_Position(MxCsr_RC, XMM1));
        mRegisterMapping.insert(XMM1, "XMM1");
        mRegisterPlaces.insert(XMM1, Register_Position(offset++, 0, 6, 16 * 2));
        mRegisterRelativePlaces.insert(XMM1, Register_Relative_Position(XMM0, XMM2));
        mRegisterMapping.insert(XMM2, "XMM2");
        mRegisterPlaces.insert(XMM2, Register_Position(offset++, 0, 6, 16 * 2));
        mRegisterRelativePlaces.insert(XMM2, Register_Relative_Position(XMM1, XMM3));
        mRegisterMapping.insert(XMM3, "XMM3");
        mRegisterPlaces.insert(XMM3, Register_Position(offset++, 0, 6, 16 * 2));
        mRegisterRelativePlaces.insert(XMM3, Register_Relative_Position(XMM2, XMM4));
        mRegisterMapping.insert(XMM4, "XMM4");
        mRegisterPlaces.insert(XMM4, Register_Position(offset++, 0, 6, 16 * 2));
        mRegisterRelativePlaces.insert(XMM4, Register_Relative_Position(XMM3, XMM5));
        mRegisterMapping.insert(XMM5, "XMM5");
        mRegisterPlaces.insert(XMM5, Register_Position(offset++, 0, 6, 16 * 2));
        mRegisterRelativePlaces.insert(XMM5, Register_Relative_Position(XMM4, XMM6));
        mRegisterMapping.insert(XMM6, "XMM6");
        mRegisterPlaces.insert(XMM6, Register_Position(offset++, 0, 6, 16 * 2));
        mRegisterRelativePlaces.insert(XMM6, Register_Relative_Position(XMM5, XMM7));
#ifdef _WIN64
        mRegisterMapping.insert(XMM7, "XMM7");
        mRegisterPlaces.insert(XMM7, Register_Position(offset++, 0, 6, 16 * 2));
        mRegisterRelativePlaces.insert(XMM7, Register_Relative_Position(XMM6, XMM8));
        mRegisterMapping.insert(XMM8, "XMM8");
        mRegisterPlaces.insert(XMM8, Register_Position(offset++, 0, 6, 16 * 2));
        mRegisterRelativePlaces.insert(XMM8, Register_Relative_Position(XMM7, XMM9));
        mRegisterMapping.insert(XMM9, "XMM9");
        mRegisterPlaces.insert(XMM9, Register_Position(offset++, 0, 6, 16 * 2));
        mRegisterRelativePlaces.insert(XMM9, Register_Relative_Position(XMM8, XMM10));
        mRegisterMapping.insert(XMM10, "XMM10");
        mRegisterPlaces.insert(XMM10, Register_Position(offset++, 0, 6, 16 * 2));
        mRegisterRelativePlaces.insert(XMM10, Register_Relative_Position(XMM9, XMM11));
        mRegisterMapping.insert(XMM11, "XMM11");
        mRegisterPlaces.insert(XMM11, Register_Position(offset++, 0, 6, 16 * 2));
        mRegisterRelativePlaces.insert(XMM11, Register_Relative_Position(XMM10, XMM12));
        mRegisterMapping.insert(XMM12, "XMM12");
        mRegisterPlaces.insert(XMM12, Register_Position(offset++, 0, 6, 16 * 2));
        mRegisterRelativePlaces.insert(XMM12, Register_Relative_Position(XMM11, XMM13));
        mRegisterMapping.insert(XMM13, "XMM13");
        mRegisterPlaces.insert(XMM13, Register_Position(offset++, 0, 6, 16 * 2));
        mRegisterRelativePlaces.insert(XMM13, Register_Relative_Position(XMM12, XMM14));
        mRegisterMapping.insert(XMM14, "XMM14");
        mRegisterPlaces.insert(XMM14, Register_Position(offset++, 0, 6, 16 * 2));
        mRegisterRelativePlaces.insert(XMM14, Register_Relative_Position(XMM13, XMM15));
        mRegisterMapping.insert(XMM15, "XMM15");
        mRegisterPlaces.insert(XMM15, Register_Position(offset++, 0, 6, 16 * 2));
        mRegisterRelativePlaces.insert(XMM15, Register_Relative_Position(XMM14, YMM0));
#else
        mRegisterMapping.insert(XMM7, "XMM7");
        mRegisterPlaces.insert(XMM7, Register_Position(offset++, 0, 6, 16 * 2));
        mRegisterRelativePlaces.insert(XMM7, Register_Relative_Position(XMM6, YMM0));
#endif

        offset++;

#ifdef _WIN64
        mRegisterMapping.insert(YMM0, "YMM0");
        mRegisterPlaces.insert(YMM0, Register_Position(offset++, 0, 6, 32 * 2));
        mRegisterRelativePlaces.insert(YMM0, Register_Relative_Position(XMM15, YMM1));
#else
        mRegisterMapping.insert(YMM0, "YMM0");
        mRegisterPlaces.insert(YMM0, Register_Position(offset++, 0, 6, 32 * 2));
        mRegisterRelativePlaces.insert(YMM0, Register_Relative_Position(XMM7, YMM1));
#endif
        mRegisterMapping.insert(YMM1, "YMM1");
        mRegisterPlaces.insert(YMM1, Register_Position(offset++, 0, 6, 32 * 2));
        mRegisterRelativePlaces.insert(YMM1, Register_Relative_Position(YMM0, YMM2));
        mRegisterMapping.insert(YMM2, "YMM2");
        mRegisterPlaces.insert(YMM2, Register_Position(offset++, 0, 6, 32 * 2));
        mRegisterRelativePlaces.insert(YMM2, Register_Relative_Position(YMM1, YMM3));
        mRegisterMapping.insert(YMM3, "YMM3");
        mRegisterPlaces.insert(YMM3, Register_Position(offset++, 0, 6, 32 * 2));
        mRegisterRelativePlaces.insert(YMM3, Register_Relative_Position(YMM2, YMM4));
        mRegisterMapping.insert(YMM4, "YMM4");
        mRegisterPlaces.insert(YMM4, Register_Position(offset++, 0, 6, 32 * 2));
        mRegisterRelativePlaces.insert(YMM4, Register_Relative_Position(YMM3, YMM5));
        mRegisterMapping.insert(YMM5, "YMM5");
        mRegisterPlaces.insert(YMM5, Register_Position(offset++, 0, 6, 32 * 2));
        mRegisterRelativePlaces.insert(YMM5, Register_Relative_Position(YMM4, YMM6));
        mRegisterMapping.insert(YMM6, "YMM6");
        mRegisterPlaces.insert(YMM6, Register_Position(offset++, 0, 6, 32 * 2));
        mRegisterRelativePlaces.insert(YMM6, Register_Relative_Position(YMM5, YMM7));
#ifdef _WIN64
        mRegisterMapping.insert(YMM7, "YMM7");
        mRegisterPlaces.insert(YMM7, Register_Position(offset++, 0, 6, 32 * 2));
        mRegisterRelativePlaces.insert(YMM7, Register_Relative_Position(YMM6, YMM8));
        mRegisterMapping.insert(YMM8, "YMM8");
        mRegisterPlaces.insert(YMM8, Register_Position(offset++, 0, 6, 32 * 2));
        mRegisterRelativePlaces.insert(YMM8, Register_Relative_Position(YMM7, YMM9));
        mRegisterMapping.insert(YMM9, "YMM9");
        mRegisterPlaces.insert(YMM9, Register_Position(offset++, 0, 6, 32 * 2));
        mRegisterRelativePlaces.insert(YMM9, Register_Relative_Position(YMM8, YMM10));
        mRegisterMapping.insert(YMM10, "YMM10");
        mRegisterPlaces.insert(YMM10, Register_Position(offset++, 0, 6, 32 * 2));
        mRegisterRelativePlaces.insert(YMM10, Register_Relative_Position(YMM9, YMM11));
        mRegisterMapping.insert(YMM11, "YMM11");
        mRegisterPlaces.insert(YMM11, Register_Position(offset++, 0, 6, 32 * 2));
        mRegisterRelativePlaces.insert(YMM11, Register_Relative_Position(YMM10, YMM12));
        mRegisterMapping.insert(YMM12, "YMM12");
        mRegisterPlaces.insert(YMM12, Register_Position(offset++, 0, 6, 32 * 2));
        mRegisterRelativePlaces.insert(YMM12, Register_Relative_Position(YMM11, YMM13));
        mRegisterMapping.insert(YMM13, "YMM13");
        mRegisterPlaces.insert(YMM13, Register_Position(offset++, 0, 6, 32 * 2));
        mRegisterRelativePlaces.insert(YMM13, Register_Relative_Position(YMM12, YMM14));
        mRegisterMapping.insert(YMM14, "YMM14");
        mRegisterPlaces.insert(YMM14, Register_Position(offset++, 0, 6, 32 * 2));
        mRegisterRelativePlaces.insert(YMM14, Register_Relative_Position(YMM13, YMM15));
        mRegisterMapping.insert(YMM15, "YMM15");
        mRegisterPlaces.insert(YMM15, Register_Position(offset++, 0, 6, 32 * 2));
        mRegisterRelativePlaces.insert(YMM15, Register_Relative_Position(YMM14, DR0));
#else
        mRegisterMapping.insert(YMM7, "YMM7");
        mRegisterPlaces.insert(YMM7, Register_Position(offset++, 0, 6, 32 * 2));
        mRegisterRelativePlaces.insert(YMM7, Register_Relative_Position(YMM6, DR0));
#endif
    }
    else
    {
        mRegisterRelativePlaces.insert(CS, Register_Relative_Position(DS, SS, ES, DR0));
        mRegisterRelativePlaces.insert(SS, Register_Relative_Position(CS, DR0, DS, DR0));
    }

    offset++;

    if(mShowFpu)
    {
#ifdef _WIN64
        mRegisterRelativePlaces.insert(DR0, Register_Relative_Position(YMM15, DR1));
#else
        mRegisterRelativePlaces.insert(DR0, Register_Relative_Position(YMM7, DR1));
#endif
    }
    else
    {
        mRegisterRelativePlaces.insert(DR0, Register_Relative_Position(SS, DR1));
    }

    mRegisterMapping.insert(DR0, "DR0");
    mRegisterPlaces.insert(DR0, Register_Position(offset++, 0, 4, sizeof(duint) * 2));
    mRegisterMapping.insert(DR1, "DR1");
    mRegisterPlaces.insert(DR1, Register_Position(offset++, 0, 4, sizeof(duint) * 2));
    mRegisterRelativePlaces.insert(DR1, Register_Relative_Position(DR0, DR2));
    mRegisterMapping.insert(DR2, "DR2");
    mRegisterPlaces.insert(DR2, Register_Position(offset++, 0, 4, sizeof(duint) * 2));
    mRegisterRelativePlaces.insert(DR2, Register_Relative_Position(DR1, DR3));
    mRegisterMapping.insert(DR3, "DR3");
    mRegisterPlaces.insert(DR3, Register_Position(offset++, 0, 4, sizeof(duint) * 2));
    mRegisterRelativePlaces.insert(DR3, Register_Relative_Position(DR2, DR6));
    mRegisterMapping.insert(DR6, "DR6");
    mRegisterPlaces.insert(DR6, Register_Position(offset++, 0, 4, sizeof(duint) * 2));
    mRegisterRelativePlaces.insert(DR6, Register_Relative_Position(DR3, DR7));
    mRegisterMapping.insert(DR7, "DR7");
    mRegisterPlaces.insert(DR7, Register_Position(offset++, 0, 4, sizeof(duint) * 2));
    mRegisterRelativePlaces.insert(DR7, Register_Relative_Position(DR6, UNKNOWN));

    mRowsNeeded = offset + 1;
}

QAction* RegistersView::setupAction(const QIcon & icon, const QString & text)
{
    QAction* action = new QAction(icon, text, this);
    action->setShortcutContext(Qt::WidgetShortcut);
    addAction(action);
    return action;
}

QAction* RegistersView::setupAction(const QString & text)
{
    QAction* action = new QAction(text, this);
    action->setShortcutContext(Qt::WidgetShortcut);
    addAction(action);
    return action;
}

RegistersView::RegistersView(QWidget* parent) : QScrollArea(parent), mVScrollOffset(0)
{
    setWindowTitle("Registers");
    mChangeViewButton = NULL;
    mFpuMode = 0;
    isActive = false;

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
    // self communication for repainting (maybe some other widgets needs this information, too)
    connect(this, SIGNAL(refresh()), this, SLOT(reload()));

    InitMappings();

    // Context Menu
    this->setContextMenuPolicy(Qt::CustomContextMenu);

    wCM_CopyToClipboard = setupAction(DIcon("copy.png"), tr("Copy value"));
    wCM_CopyFloatingPointValueToClipboard = setupAction(DIcon("copy.png"), tr("Copy floating point value"));
    wCM_CopySymbolToClipboard = setupAction(DIcon("pdb.png"), tr("Copy Symbol Value"));
    wCM_CopyAll = setupAction(DIcon("copy-alt.png"), tr("Copy all registers"));
    wCM_ChangeFPUView = new QAction(DIcon("change-view.png"), tr("Change view"), this);
    mSwitchSIMDDispMode = new QMenu(tr("Change SIMD Register Display Mode"), this);
    mSwitchSIMDDispMode->setIcon(DIcon("simdmode.png"));
    mDisplaySTX = new QAction(tr("Display ST(x)"), this);
    mDisplayx87rX = new QAction(tr("Display x87rX"), this);
    mDisplayMMX = new QAction(tr("Display MMX"), this);

    // Create the SIMD display mode actions
    SIMDHex = new QAction(tr("Hexadecimal"), mSwitchSIMDDispMode);
    SIMDFloat = new QAction(tr("Float"), mSwitchSIMDDispMode);
    SIMDDouble = new QAction(tr("Double"), mSwitchSIMDDispMode);
    SIMDSWord = new QAction(tr("Signed Word"), mSwitchSIMDDispMode);
    SIMDSDWord = new QAction(tr("Signed Dword"), mSwitchSIMDDispMode);
    SIMDSQWord = new QAction(tr("Signed Qword"), mSwitchSIMDDispMode);
    SIMDUWord = new QAction(tr("Unsigned Word"), mSwitchSIMDDispMode);
    SIMDUDWord = new QAction(tr("Unsigned Dword"), mSwitchSIMDDispMode);
    SIMDUQWord = new QAction(tr("Unsigned Qword"), mSwitchSIMDDispMode);
    SIMDHWord = new QAction(tr("Hexadecimal Word"), mSwitchSIMDDispMode);
    SIMDHDWord = new QAction(tr("Hexadecimal Dword"), mSwitchSIMDDispMode);
    SIMDHQWord = new QAction(tr("Hexadecimal Qword"), mSwitchSIMDDispMode);
    // Set the user data of these actions to the config value
    SIMDHex->setData(QVariant(0));
    SIMDFloat->setData(QVariant(1));
    SIMDDouble->setData(QVariant(2));
    SIMDSWord->setData(QVariant(3));
    SIMDUWord->setData(QVariant(6));
    SIMDHWord->setData(QVariant(9));
    SIMDSDWord->setData(QVariant(4));
    SIMDUDWord->setData(QVariant(7));
    SIMDHDWord->setData(QVariant(10));
    SIMDSQWord->setData(QVariant(5));
    SIMDUQWord->setData(QVariant(8));
    SIMDHQWord->setData(QVariant(11));
    mDisplaySTX->setData(QVariant(0));
    mDisplayx87rX->setData(QVariant(1));
    mDisplayMMX->setData(QVariant(2));
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
    connect(mDisplaySTX, SIGNAL(triggered()), this, SLOT(onFpuMode()));
    connect(mDisplayx87rX, SIGNAL(triggered()), this, SLOT(onFpuMode()));
    connect(mDisplayMMX, SIGNAL(triggered()), this, SLOT(onFpuMode()));
    // Make SIMD display mode actions checkable and unchecked
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

    connect(wCM_CopyToClipboard, SIGNAL(triggered()), this, SLOT(onCopyToClipboardAction()));
    connect(wCM_CopyFloatingPointValueToClipboard, SIGNAL(triggered()), this, SLOT(onCopyFloatingPointToClipboardAction()));
    connect(wCM_CopySymbolToClipboard, SIGNAL(triggered()), this, SLOT(onCopySymbolToClipboardAction()));
    connect(wCM_CopyAll, SIGNAL(triggered()), this, SLOT(onCopyAllAction()));
    connect(wCM_ChangeFPUView, SIGNAL(triggered()), this, SLOT(onChangeFPUViewAction()));

    memset(&wRegDumpStruct, 0, sizeof(REGDUMP));
    memset(&wCipRegDumpStruct, 0, sizeof(REGDUMP));
    mCip = 0;
    mRegisterUpdates.clear();

    mButtonHeight = 0;
    yTopSpacing = 4; //set top spacing (in pixels)

    this->setMouseTracking(true);
}

void RegistersView::refreshShortcutsSlot()
{
    wCM_CopyToClipboard->setShortcut(ConfigShortcut("ActionCopy"));
    wCM_CopySymbolToClipboard->setShortcut(ConfigShortcut("ActionCopySymbol"));
    wCM_CopyAll->setShortcut(ConfigShortcut("ActionCopyAllRegisters"));
}

/**
 * @brief RegistersView::~RegistersView The destructor.
 */
RegistersView::~RegistersView()
{
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

void RegistersView::displayCustomContextMenuSlot(QPoint pos)
{

}

void RegistersView::ShowFPU(bool set_showfpu)
{
    mShowFpu = set_showfpu;
    InitMappings();
    setFixedHeight(getEstimateHeight());
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

void RegistersView::mousePressEvent(QMouseEvent* event)
{
    if(!isActive)
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
            mSelected = r;
            emit refresh();
        }
        else
            mSelected = UNKNOWN;
    }
}

void RegistersView::mouseMoveEvent(QMouseEvent* event)
{
    if(!isActive)
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
    wPainter.setFont(font());
    wPainter.fillRect(wPainter.viewport(), QBrush(ConfigColor("RegistersBackgroundColor")));

    // Don't draw the registers if a program isn't actually running
    if(!isActive)
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
    if(isActive)
    {
        int key = event->key();
        REGISTER_NAME newRegister = UNKNOWN;

        switch(key)
        {
        case Qt::Key_Left:
            newRegister = mRegisterRelativePlaces[mSelected].left;
            break;
        case Qt::Key_Right:
            newRegister = mRegisterRelativePlaces[mSelected].right;
            break;
        case Qt::Key_Up:
            newRegister = mRegisterRelativePlaces[mSelected].up;
            break;
        case Qt::Key_Down:
            newRegister = mRegisterRelativePlaces[mSelected].down;
            break;
        }

        if(newRegister != UNKNOWN)
        {
            mSelected = newRegister;
            ensureRegisterVisible(newRegister);
            emit refresh();
        }
    }
    QScrollArea::keyPressEvent(event);
}

//QSize RegistersView::sizeHint() const
//{
//    // 32 character width
//    return QSize(32 * mCharWidth, this->viewport()->height());
//}

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
                valueText = GetDataTypeString(value, size, enc_xmmword);
            else if(mFPUYMM.contains(reg))
                valueText = GetDataTypeString(value, size, enc_ymmword);
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
            if(!ZydisTokenizer::tokenTextPoolEquals(regName, reg.first))
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
        if(isActive && mRegisterUpdates.contains(reg))
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

        if(mFPUx87_80BITSDISPLAY.contains(reg) && isActive)
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

            if(isActive && mRegisterUpdates.contains(reg))
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

void RegistersView::setupSIMDModeMenu()
{
    const QAction* selectedAction = nullptr;
    // Find out which mode is selected by the user
    switch(ConfigUint("Gui", "SIMDRegistersDisplayMode"))
    {
    case 0:
        selectedAction = SIMDHex;
        break;
    case 1:
        selectedAction = SIMDFloat;
        break;
    case 2:
        selectedAction = SIMDDouble;
        break;
    case 3:
        selectedAction = SIMDSWord;
        break;
    case 6:
        selectedAction = SIMDUWord;
        break;
    case 9:
        selectedAction = SIMDHWord;
        break;
    case 4:
        selectedAction = SIMDSDWord;
        break;
    case 7:
        selectedAction = SIMDUDWord;
        break;
    case 10:
        selectedAction = SIMDHDWord;
        break;
    case 5:
        selectedAction = SIMDSQWord;
        break;
    case 8:
        selectedAction = SIMDUQWord;
        break;
    case 11:
        selectedAction = SIMDHQWord;
        break;
    }
    // Check that action if it is selected, uncheck otherwise
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
}

void RegistersView::onChangeFPUViewAction()
{
    ShowFPU(!mShowFpu);
}

void RegistersView::onSIMDMode()
{
    Config()->setUint("Gui", "SIMDRegistersDisplayMode", dynamic_cast<QAction*>(sender())->data().toInt());
    emit refresh();
    GuiUpdateDisassemblyView(); // refresh display mode for data in disassembly
}

void RegistersView::onFpuMode()
{
    mFpuMode = (char)(dynamic_cast<QAction*>(sender())->data().toInt());
    InitMappings();
    emit refresh();
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
        switch(mFpuMode)
        {
        case 0:
            appendRegister(text, REGISTER_NAME::x87st0, "ST(0) : ", "ST(0) : ");
            appendRegister(text, REGISTER_NAME::x87st1, "ST(1) : ", "ST(1) : ");
            appendRegister(text, REGISTER_NAME::x87st2, "ST(2) : ", "ST(2) : ");
            appendRegister(text, REGISTER_NAME::x87st3, "ST(3) : ", "ST(3) : ");
            appendRegister(text, REGISTER_NAME::x87st4, "ST(4) : ", "ST(4) : ");
            appendRegister(text, REGISTER_NAME::x87st5, "ST(5) : ", "ST(5) : ");
            appendRegister(text, REGISTER_NAME::x87st6, "ST(6) : ", "ST(6) : ");
            appendRegister(text, REGISTER_NAME::x87st7, "ST(7) : ", "ST(7) : ");
            break;
        case 1:
            appendRegister(text, REGISTER_NAME::x87r0, "x87r0 : ", "x87r0 : ");
            appendRegister(text, REGISTER_NAME::x87r1, "x87r1 : ", "x87r1 : ");
            appendRegister(text, REGISTER_NAME::x87r2, "x87r2 : ", "x87r2 : ");
            appendRegister(text, REGISTER_NAME::x87r3, "x87r3 : ", "x87r3 : ");
            appendRegister(text, REGISTER_NAME::x87r4, "x87r4 : ", "x87r4 : ");
            appendRegister(text, REGISTER_NAME::x87r5, "x87r5 : ", "x87r5 : ");
            appendRegister(text, REGISTER_NAME::x87r6, "x87r6 : ", "x87r6 : ");
            appendRegister(text, REGISTER_NAME::x87r7, "x87r7 : ", "x87r7 : ");
            break;
        case 2:
            appendRegister(text, REGISTER_NAME::MM0, "MM0 : ", "MM0 : ");
            appendRegister(text, REGISTER_NAME::MM1, "MM1 : ", "MM1 : ");
            appendRegister(text, REGISTER_NAME::MM2, "MM2 : ", "MM2 : ");
            appendRegister(text, REGISTER_NAME::MM3, "MM3 : ", "MM3 : ");
            appendRegister(text, REGISTER_NAME::MM4, "MM4 : ", "MM4 : ");
            appendRegister(text, REGISTER_NAME::MM5, "MM5 : ", "MM5 : ");
            appendRegister(text, REGISTER_NAME::MM6, "MM6 : ", "MM6 : ");
            appendRegister(text, REGISTER_NAME::MM7, "MM7 : ", "MM7 : ");
            break;
        }
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

void RegistersView::debugStateChangedSlot(DBGSTATE state)
{
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
    else
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

// Scroll the viewport so that the register will be visible on the screen
void RegistersView::ensureRegisterVisible(REGISTER_NAME reg)
{
    QScrollArea* upperScrollArea = (QScrollArea*)this->parentWidget()->parentWidget();

    int ySpace = yTopSpacing;
    if(mVScrollOffset != 0)
        ySpace = 0;
    int y = mRowHeight * (mRegisterPlaces[reg].line + mVScrollOffset) + ySpace;

    upperScrollArea->ensureVisible(0, y);
}
