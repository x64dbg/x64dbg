#include "MiscUtil.h"
#include <windows.h>
#include "LineEditDialog.h"
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

static bool isChristmas()
{
    srand(GetTickCount());
    duint setting = 0;
    if(BridgeSettingGetUint("Misc", "NoChristmas", &setting) && setting)
        return false;
    auto date = QDateTime::currentDateTime().date();
    return date.month() == 12 && date.day() >= 23 && date.day() <= 26;
}

QString couldItBeChristmas(QString icon)
{
    static bool christmas = isChristmas();
    return christmas ? QString("christmas%0.png").arg(rand() % 8 + 1) : icon;
}
