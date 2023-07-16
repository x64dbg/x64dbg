#include "MiscUtil.h"
#include <QtWin>
#include <QApplication>
#include <QMessageBox>
#include <QDir>
#include "LineEditDialog.h"
#include "ComboBoxDialog.h"
#include "StringUtil.h"
#include "BrowseDialog.h"
#include <thread>

void SetApplicationIcon(WId winId)
{
    std::thread([winId]
    {
        HICON hIcon = LoadIcon(GetModuleHandleW(0), MAKEINTRESOURCE(100));
        SendMessageW((HWND)winId, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        DestroyIcon(hIcon);
    }).detach();
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

bool SimpleInputBox(QWidget* parent, const QString & title, QString defaultValue, QString & output, const QString & placeholderText, const QIcon* icon)
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

bool SimpleChoiceBox(QWidget* parent, const QString & title, QString defaultValue, const QStringList & choices, QString & output, bool editable, const QString & placeholderText, const QIcon* icon, int minimumContentsLength)
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
    msg.setWindowIcon(DIcon("fatal-error"));
    msg.setParent(parent, Qt::Dialog);
    msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
    msg.exec();
}

void SimpleWarningBox(QWidget* parent, const QString & title, const QString & text)
{
    QMessageBox msg(QMessageBox::Warning, title, text, QMessageBox::NoButton, parent);
    msg.setWindowIcon(DIcon("exclamation"));
    msg.setParent(parent, Qt::Dialog);
    msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
    msg.exec();
}

void SimpleInfoBox(QWidget* parent, const QString & title, const QString & text)
{
    QMessageBox msg(QMessageBox::Information, title, text, QMessageBox::NoButton, parent);
    msg.setWindowIcon(DIcon("information"));
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

QIcon getFileIcon(QString file)
{
    SHFILEINFO info;
    if(SHGetFileInfoW((const wchar_t*)file.utf16(), 0, &info, sizeof(info), SHGFI_ICON) == 0)
        return QIcon(); //API error
    QIcon result = QIcon(QtWin::fromHICON(info.hIcon));
    DestroyIcon(info.hIcon);
    return result;
}

//Export table in CSV. TODO: Display a dialog where the user choose what column to export and in which encoding
bool ExportCSV(dsint rows, dsint columns, std::vector<QString> headers, std::function<QString(dsint, dsint)> getCellContent)
{
    BrowseDialog browse(
        nullptr,
        QApplication::translate("ExportCSV", "Export data in CSV format"),
        QApplication::translate("ExportCSV", "Enter the CSV file name to export"),
        QApplication::translate("ExportCSV", "CSV files (*.csv);;All files (*.*)"),
        getDbPath("export.csv", true),
        true
    );
    browse.setWindowIcon(DIcon("database-export"));
    if(browse.exec() == QDialog::Accepted)
    {
        FILE* csv;
        bool utf16;
        csv = _wfopen(browse.path.toStdWString().c_str(), L"wb");
        if(csv == NULL)
        {
            GuiAddLogMessage(QApplication::translate("ExportCSV", "CSV export error\n").toUtf8().constData());
            return false;
        }
        else
        {
            duint setting;
            if(BridgeSettingGetUint("Misc", "Utf16LogRedirect", &setting))
                utf16 = !!setting;
            else
                utf16 = false;
            if(utf16 && ftell(csv) == 0)
            {
                unsigned short BOM = 0xfeff;
                fwrite(&BOM, 2, 1, csv);
            }
            dsint row, column;
            QString text;
            QString cell;
            if(headers.size() > 0)
            {
                for(column = 0; column < columns; column++)
                {
                    cell = headers.at(column);
                    if(cell.contains('"') || cell.contains(',') || cell.contains('\r') || cell.contains('\n'))
                    {
                        if(cell.contains('"'))
                            cell = cell.replace("\"", "\"\"");
                        cell = "\"" + cell + "\"";
                    }
                    if(column != columns - 1)
                        cell = cell + ",";
                    text = text + cell;
                }
                if(utf16)
                {
                    text = text + "\r\n";
                    if(!fwrite(text.utf16(), text.length(), 2, csv))
                    {
                        fclose(csv);
                        GuiAddLogMessage(QApplication::translate("ExportCSV", "CSV export error\n").toUtf8().constData());
                        return false;
                    }
                }
                else
                {
                    text = text + "\n";
                    QByteArray utf8;
                    utf8 = text.toUtf8();
                    if(!fwrite(utf8.constData(), utf8.size(), 1, csv))
                    {
                        fclose(csv);
                        GuiAddLogMessage(QApplication::translate("ExportCSV", "CSV export error\n").toUtf8().constData());
                        return false;
                    }
                }
            }
            for(row = 0; row < rows; row++)
            {
                text.clear();
                for(column = 0; column < columns; column++)
                {
                    cell = getCellContent(row, column);
                    if(cell.contains('"') || cell.contains(',') || cell.contains('\r') || cell.contains('\n'))
                    {
                        if(cell.contains('"'))
                            cell = cell.replace("\"", "\"\"");
                        cell = "\"" + cell + "\"";
                    }
                    if(column != columns - 1)
                        cell = cell + ",";
                    text = text + cell;
                }
                if(utf16)
                {
                    text = text + "\r\n";
                    if(!fwrite(text.utf16(), text.length(), 2, csv))
                    {
                        fclose(csv);
                        GuiAddLogMessage(QApplication::translate("ExportCSV", "CSV export error\n").toUtf8().constData());
                        return false;
                    }
                }
                else
                {
                    text = text + "\n";
                    QByteArray utf8;
                    utf8 = text.toUtf8();
                    if(!fwrite(utf8.constData(), utf8.size(), 1, csv))
                    {
                        fclose(csv);
                        GuiAddLogMessage(QApplication::translate("ExportCSV", "CSV export error\n").toUtf8().constData());
                        return false;
                    }
                }
            }
            fclose(csv);
            GuiAddLogMessage(QApplication::translate("ExportCSV", "Saved CSV data at %1\n").arg(browse.path).toUtf8().constData());
            return true;
        }
    }
    else
        return false;
}

static bool allowIcons()
{
    duint setting = 0;
    return !BridgeSettingGetUint("Gui", "NoIcons", &setting) || !setting;
}

static bool allowSeasons()
{
    srand(GetTickCount());
    duint setting = 0;
    return !BridgeSettingGetUint("Gui", "NoSeasons", &setting) || !setting;
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

bool isSeasonal()
{
    return (isChristmas() || isEaster());
}

QIcon DIconHelper(QString name)
{
    if(name.endsWith(".png"))
        name = name.left(name.length() - 4);
    static bool icons = allowIcons();
    if(!icons)
        return QIcon();
    static bool seasons = allowSeasons();
    static bool christmas = isChristmas();
    static bool easter = isEaster();
    if(seasons)
    {
        if(christmas)
            name = QString("christmas%1").arg(rand() % 8 + 1);
        else if(easter)
            name = QString("easter%1").arg(rand() % 8 + 1);
    }
    return QIcon::fromTheme(name);
}

QString getDbPath(const QString & filename, bool addDateTimeSuffix)
{
    auto path = QString("%1/db").arg(QString::fromWCharArray(BridgeUserDirectory()));
    if(!filename.isEmpty())
    {
        path += '/';
        path += filename;
        // Add a date suffix before the extension
        if(addDateTimeSuffix)
        {
            auto extensionIdx = path.lastIndexOf('.');
            if(extensionIdx == -1)
            {
                extensionIdx = path.length();
            }
            auto now = QDateTime::currentDateTime();
            auto suffix = QString().sprintf("-%04d%02d%02d-%02d%02d%02d",
                                            now.date().year(),
                                            now.date().month(),
                                            now.date().day(),
                                            now.time().hour(),
                                            now.time().minute(),
                                            now.time().second()
                                           );
            path.insert(extensionIdx, suffix);
        }
    }
    return QDir::toNativeSeparators(path);
}

QString mainModuleName(bool extension)
{
    auto base = DbgEval("mod.main()");
    char name[MAX_MODULE_SIZE] = "";
    if(base && DbgFunctions()->ModNameFromAddr(base, name, extension))
    {
        return name;
    }
    return QString();
}
