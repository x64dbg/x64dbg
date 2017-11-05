#include "MiscUtil.h"
#include <windows.h>
#include "LineEditDialog.h"
#include "ComboBoxDialog.h"
#include <QMessageBox>
#include "StringUtil.h"

void SetApplicationIcon(WId winId)
{
    HICON hIcon = LoadIcon(GetModuleHandleW(0), MAKEINTRESOURCE(100));
    SendMessageW((HWND)winId, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    DestroyIcon(hIcon);
}

QByteArray & ByteReverse(QByteArray & array)
{
    int length = array.length();
    for(int i = 0; i < length / 2; i++)
    {
        char temp = array[i];
        array[i] = array[length - i - 1];
        array[length - i - 1] = temp;
    }
    return array;
}

QByteArray ByteReverse(QByteArray && array)
{
    int length = array.length();
    for(int i = 0; i < length / 2; i++)
    {
        char temp = array[i];
        array[i] = array[length - i - 1];
        array[length - i - 1] = temp;
    }
    return array;
}

bool SimpleInputBox(QWidget* parent, const QString & title, QString defaultValue, QString & output, const QString & placeholderText, QIcon* icon)
{
    LineEditDialog mEdit(parent);
    mEdit.setWindowIcon(icon ? *icon : parent->windowIcon());
    mEdit.setText(defaultValue);
    mEdit.setPlaceholderText(placeholderText);
    mEdit.setWindowTitle(title);
    mEdit.setCheckBox(false);
    if(mEdit.exec() == QDialog::Accepted)
    {
        output = mEdit.editText;
        return true;
    }
    else
        return false;
}

bool SimpleChoiceBox(QWidget* parent, const QString & title, QString defaultValue, const QStringList & choices, QString & output, bool editable, const QString & placeholderText, QIcon* icon, int minimumContentsLength)
{
    ComboBoxDialog mChoice(parent);
    mChoice.setWindowIcon(icon ? *icon : parent->windowIcon());
    mChoice.setEditable(editable);
    mChoice.setItems(choices);
    mChoice.setText(defaultValue);
    mChoice.setPlaceholderText(placeholderText);
    mChoice.setWindowTitle(title);
    mChoice.setCheckBox(false);
    if(minimumContentsLength >= 0)
        mChoice.setMinimumContentsLength(minimumContentsLength);
    if(mChoice.exec() == QDialog::Accepted)
    {
        output = mChoice.currentText();
        return true;
    }
    else
        return false;
}

void SimpleErrorBox(QWidget* parent, const QString & title, const QString & text)
{
    QMessageBox msg(QMessageBox::Critical, title, text, QMessageBox::NoButton, parent);
    msg.setWindowIcon(DIcon("compile-error.png"));
    msg.setParent(parent, Qt::Dialog);
    msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
    msg.exec();
}

void SimpleWarningBox(QWidget* parent, const QString & title, const QString & text)
{
    QMessageBox msg(QMessageBox::Warning, title, text, QMessageBox::NoButton, parent);
    msg.setWindowIcon(DIcon("compile-warning.png"));
    msg.setParent(parent, Qt::Dialog);
    msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
    msg.exec();
}

void SimpleInfoBox(QWidget* parent, const QString & title, const QString & text)
{
    QMessageBox msg(QMessageBox::Information, title, text, QMessageBox::NoButton, parent);
    msg.setWindowIcon(DIcon("information.png"));
    msg.setParent(parent, Qt::Dialog);
    msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
    msg.exec();
}

QString getSymbolicName(duint addr)
{
    char labelText[MAX_LABEL_SIZE]   = "";
    char moduleText[MAX_MODULE_SIZE] = "";
    bool bHasLabel = DbgGetLabelAt(addr, SEG_DEFAULT, labelText);
    bool bHasModule = (DbgGetModuleAt(addr, moduleText) && !QString(labelText).startsWith("JMP.&"));
    QString addrText = ToPtrString(addr);

    if(bHasLabel && bHasModule) // <module.label>
        return QString("%1 <%2.%3>").arg(addrText).arg(moduleText).arg(labelText);
    else if(bHasModule) // module.addr
        return QString("%1.%2").arg(moduleText).arg(addrText);
    else if(bHasLabel) // <label>
        return QString("<%1>").arg(labelText);
    else
        return addrText;
}

QString getSymbolicNameStr(duint addr)
{
    char labelText[MAX_LABEL_SIZE] = "";
    char moduleText[MAX_MODULE_SIZE] = "";
    char string[MAX_STRING_SIZE] = "";
    bool bHasString = DbgGetStringAt(addr, string);
    bool bHasLabel = DbgGetLabelAt(addr, SEG_DEFAULT, labelText);
    bool bHasModule = (DbgGetModuleAt(addr, moduleText) && !QString(labelText).startsWith("JMP.&"));
    QString addrText = DbgMemIsValidReadPtr(addr) ? ToPtrString(addr) : ToHexString(addr);
    QString finalText;
    if(bHasString)
        finalText = addrText + " " + QString(string);
    else if(bHasLabel && bHasModule) //<module.label>
        finalText = QString("<%1.%2>").arg(moduleText).arg(labelText);
    else if(bHasModule) //module.addr
        finalText = QString("%1.%2").arg(moduleText).arg(addrText);
    else if(bHasLabel) //<label>
        finalText = QString("<%1>").arg(labelText);
    else
    {
        finalText = addrText;
        if(addr == (addr & 0xFF))
        {
            QChar c = QChar((char)addr);
            if(c.isPrint() || c.isSpace())
                finalText += QString(" '%1'").arg(EscapeCh(c));
        }
        else if(addr == (addr & 0xFFF)) //UNICODE?
        {
            QChar c = QChar((ushort)addr);
            if(c.isPrint() || c.isSpace())
                finalText += QString(" L'%1'").arg(EscapeCh(c));
        }
    }
    return finalText;
}

static bool allowSeasons()
{
    srand(GetTickCount());
    duint setting = 0;
    return !BridgeSettingGetUint("Misc", "NoSeasons", &setting) || !setting;
}

static bool isChristmas()
{
    auto date = QDateTime::currentDateTime().date();
    return date.month() == 12 && date.day() >= 23 && date.day() <= 26;
}

//https://www.daniweb.com/programming/software-development/threads/463261/c-easter-day-calculation
bool isEaster()
{
    auto date = QDateTime::currentDateTime().date();
    int K, M, S, A, D, R, OG, SZ, OE, X = date.year();
    K  = X / 100;                                   // Secular number
    M  = 15 + (3 * K + 3) / 4 - (8 * K + 13) / 25;  // Secular Moon shift
    S  = 2 - (3 * K + 3) / 4;                       // Secular sun shift
    A  = X % 19;                                    // Moon parameter
    D  = (19 * A + M) % 30;                         // Seed for 1st full Moon in spring
    R  = D / 29 + (D / 28 - D / 29) * (A / 11);     // Calendarian correction quantity
    OG = 21 + D - R;                                // Easter limit
    SZ = 7 - (X + X / 4 + S) % 7;                   // 1st sunday in March
    OE = 7 - (OG - SZ) % 7;                         // Distance Easter sunday from Easter limit in days
    int MM = ((OG + OE) > 31) ? 4 : 3;
    int DD = (((OG + OE) % 31) == 0) ? 31 : ((OG + OE) % 31);
    return date.month() == MM && date.day() >= DD - 2 && date.day() <= DD + 1;
}

QString couldItBeSeasonal(QString icon)
{
    static bool seasons = allowSeasons();
    static bool christmas = isChristmas();
    static bool easter = isEaster();
    if(!seasons)
        return icon;
    if(christmas)
        return QString("christmas%1.png").arg(rand() % 8 + 1);
    else if(easter)
        return QString("easter%1.png").arg(rand() % 8 + 1);
    return icon;
}
