#include "LogView.h"
#include "Configuration.h"
#include "Bridge.h"

#include <QRegularExpression>
#include <QDesktopServices>
#include <QClipboard>
#include <QMimeData>
#include <QTimer>
#include <QApplication>
#include <QContextMenuEvent>

#include "BrowseDialog.h"
#include "MiscUtil.h"
#include "StringUtil.h"

/**
 * @brief LogView::LogView The constructor constructs a rich text browser
 * @param parent The parent
 */
LogView::LogView(QWidget* parent) : QTextBrowser(parent), logRedirection(NULL)
{
    updateStyle();
    this->setUndoRedoEnabled(false);
    this->setReadOnly(true);
    this->setOpenExternalLinks(false);
    this->setOpenLinks(false);
    this->setLoggingEnabled(true);
    autoScroll = true;

    flushTimer = new QTimer(this);
    flushTimer->setInterval(500);
    connect(flushTimer, SIGNAL(timeout()), this, SLOT(flushTimerSlot()));
    connect(Bridge::getBridge(), SIGNAL(closeApplication()), flushTimer, SLOT(stop()));

    connect(Config(), SIGNAL(colorsUpdated()), this, SLOT(updateStyle()));
    connect(Config(), SIGNAL(fontsUpdated()), this, SLOT(updateStyle()));
    connect(Bridge::getBridge(), SIGNAL(addMsgToLog(QByteArray)), this, SLOT(addMsgToLogSlot(QByteArray)));
    connect(Bridge::getBridge(), SIGNAL(addMsgToLogHtml(QByteArray)), this, SLOT(addMsgToLogHtmlSlot(QByteArray)));
    connect(Bridge::getBridge(), SIGNAL(clearLog()), this, SLOT(clearLogSlot()));
    connect(Bridge::getBridge(), SIGNAL(saveLog()), this, SLOT(saveSlot()));
    connect(Bridge::getBridge(), SIGNAL(saveLogToFile(QString)), this, SLOT(saveToFileSlot(QString)));
    connect(Bridge::getBridge(), SIGNAL(redirectLogToFile(QString)), this, SLOT(redirectLogToFileSlot(QString)));
    connect(Bridge::getBridge(), SIGNAL(redirectLogStop()), this, SLOT(stopRedirectLogSlot()));
    connect(Bridge::getBridge(), SIGNAL(setLogEnabled(bool)), this, SLOT(setLoggingEnabled(bool)));
    connect(Bridge::getBridge(), SIGNAL(flushLog()), this, SLOT(flushLogSlot()));
    connect(this, SIGNAL(anchorClicked(QUrl)), this, SLOT(onAnchorClicked(QUrl)));
    dialogFindInLog = new LineEditDialog(this);
    dialogFindInLog->setWindowTitle(tr("Find For"));

    duint setting;
    if(BridgeSettingGetUint("Misc", "Utf16LogRedirect", &setting))
        utf16Redirect = !!setting;

    setupContextMenu();
}

/**
 * @brief LogView::~LogView The destructor
 */
LogView::~LogView()
{
    if(logRedirection != nullptr)
        fclose(logRedirection);
    logRedirection = nullptr;
}

void LogView::updateStyle()
{
    setFont(ConfigFont("Log"));
    setStyleSheet(QString("QTextEdit { color: %1; background-color: %2 }").arg(ConfigColor("LogColor").name(), ConfigColor("LogBackgroundColor").name()));
    QColor LogLinkBackgroundColor = ConfigColor("LogLinkBackgroundColor");

    this->document()->setDefaultStyleSheet(QString("a {color: %1; background-color: %2 }").arg(ConfigColor("LogLinkColor").name(), LogLinkBackgroundColor == Qt::transparent ? "transparent" : LogLinkBackgroundColor.name()));
}

template<class T> static QAction* setupAction(const QIcon & icon, const QString & text, LogView* this_object, T slot)
{
    QAction* action = new QAction(icon, text, this_object);
    action->setShortcutContext(Qt::WidgetShortcut);
    this_object->addAction(action);
    this_object->connect(action, SIGNAL(triggered()), this_object, slot);
    return action;
}

template<class T> static QAction* setupAction(const QString & text, LogView* this_object, T slot)
{
    QAction* action = new QAction(text, this_object);
    action->setShortcutContext(Qt::WidgetShortcut);
    this_object->addAction(action);
    this_object->connect(action, SIGNAL(triggered()), this_object, slot);
    return action;
}

void LogView::setupContextMenu()
{
    actionClear = setupAction(DIcon("eraser"), tr("Clea&r"), this, SLOT(clearLogSlot()));
    actionCopy = setupAction(DIcon("copy"), tr("&Copy"), this, SLOT(copy()));
    actionPaste = setupAction(DIcon("binary_paste"), tr("&Paste"), this, SLOT(pasteSlot()));
    actionSelectAll = setupAction(DIcon("copy_full_table"), tr("Select &All"), this, SLOT(selectAll()));
    actionSave = setupAction(DIcon("binary_save"), tr("&Save"), this, SLOT(saveSlot()));
    actionToggleLogging = setupAction(DIcon("lock"), tr("Disable &Logging"), this, SLOT(toggleLoggingSlot()));
    actionRedirectLog = setupAction(DIcon("database-export"), tr("&Redirect Log..."), this, SLOT(redirectLogSlot()));
    actionAutoScroll = setupAction(tr("Auto Scrolling"), this, SLOT(autoScrollSlot()));
    menuCopyToNotes = new QMenu(tr("Copy To Notes"), this);
    menuCopyToNotes->setIcon(DIcon("notes"));
    actionCopyToGlobalNotes = new QAction(tr("&Global"), menuCopyToNotes);
    actionCopyToDebuggeeNotes = new QAction(tr("&Debuggee"), menuCopyToNotes);
    connect(actionCopyToGlobalNotes, SIGNAL(triggered()), this, SLOT(copyToGlobalNotes()));
    connect(actionCopyToDebuggeeNotes, SIGNAL(triggered()), this, SLOT(copyToDebuggeeNotes()));
    menuCopyToNotes->addAction(actionCopyToGlobalNotes);
    menuCopyToNotes->addAction(actionCopyToDebuggeeNotes);
    actionAutoScroll->setCheckable(true);
    actionAutoScroll->setChecked(autoScroll);
    actionFindInLog = setupAction(tr("Find"), this, SLOT(findInLogSlot()));
    actionFindNext = setupAction(tr("Find Next Occurence"), this, SLOT(findNextInLogSlot()));;
    actionFindPrevious = setupAction(tr("Find Previous Occurence"), this, SLOT(findPreviousInLogSlot()));

    refreshShortcutsSlot();
    connect(Config(), SIGNAL(shortcutsUpdated()), this, SLOT(refreshShortcutsSlot()));
}

void LogView::refreshShortcutsSlot()
{
    actionClear->setShortcut(ConfigShortcut("ActionClear"));
    actionCopy->setShortcut(ConfigShortcut("ActionCopy"));
    actionToggleLogging->setShortcut(ConfigShortcut("ActionToggleLogging"));
    actionRedirectLog->setShortcut(ConfigShortcut("ActionRedirectLog"));
    actionFindInLog->setShortcut(ConfigShortcut("ActionFind"));
    actionFindNext->setShortcut(ConfigShortcut("ActionGotoNext"));
    actionFindPrevious->setShortcut(ConfigShortcut("ActionGotoPrevious"));
}

void LogView::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu menu(this);
    menu.addAction(actionClear);
    menu.addAction(actionSelectAll);
    menu.addAction(actionCopy);
    if(QApplication::clipboard()->mimeData()->hasText())
        menu.addAction(actionPaste);
    menu.addAction(actionSave);
    if(getLoggingEnabled())
        actionToggleLogging->setText(tr("Disable &Logging"));
    else
        actionToggleLogging->setText(tr("Enable &Logging"));
    actionCopyToDebuggeeNotes->setEnabled(DbgIsDebugging());
    menu.addMenu(menuCopyToNotes);
    menu.addAction(actionToggleLogging);
    actionAutoScroll->setChecked(autoScroll);
    menu.addAction(actionAutoScroll);
    menu.addAction(actionFindInLog);
    menu.addAction(actionFindNext);
    menu.addAction(actionFindPrevious);
    if(logRedirection == nullptr)
        actionRedirectLog->setText(tr("&Redirect Log..."));
    else
        actionRedirectLog->setText(tr("Stop &Redirection"));
    menu.addAction(actionRedirectLog);

    menu.exec(event->globalPos());
}

void LogView::showEvent(QShowEvent* event)
{
    flushTimerSlot();
    flushTimer->start();
    QTextBrowser::showEvent(event);
}

void LogView::hideEvent(QHideEvent* event)
{
    flushTimer->stop();
    QTextBrowser::hideEvent(event);
}

void LogView::handleLink(QWidget* parent, const QUrl & link)
{
    if(link.scheme() == "x64dbg")
    {
        // x64dbg:path#fragment
        auto path = link.path();
        auto fragment = link.fragment(QUrl::FullyDecoded);
        if(path == "address" || path == "/address32" || path == "/address64")
        {
            if(DbgIsDebugging())
            {
                bool ok = false;
                auto address = duint(fragment.toULongLong(&ok, 16));
                if(ok && DbgMemIsValidReadPtr(address))
                {
                    if(DbgFunctions()->MemIsCodePage(address, true))
                        DbgCmdExec(QString("disasm %1").arg(link.fragment()));
                    else
                    {
                        DbgCmdExecDirect(QString("dump %1").arg(link.fragment()));
                        emit Bridge::getBridge()->getDumpAttention();
                    }
                }
                else
                    SimpleErrorBox(parent, tr("Invalid address!"), tr("The address %1 is not a valid memory location...").arg(ToPtrString(address)));
            }
        }
        else if(path == "command")
        {
            DbgCmdExec(fragment.toUtf8().constData());
        }
        else
            SimpleErrorBox(parent, tr("Url is not valid!"), tr("The Url %1 is not supported").arg(link.toString()));
    }
    else
        QDesktopServices::openUrl(link); // external Url
}

/**
 * @brief linkify Add hyperlink HTML to the message where applicable.
 * @param msg The message passed by reference.
 * Url format:
 * x64dbg:// localhost                                                                                          /  address64 # address
 * ^fixed    ^host(probably will be changed to PID + Host when remote debugging and child debugging are supported) ^token      ^parameter
 */
#ifdef _WIN64
static QRegularExpression addressRegExp("([0-9A-Fa-f]{16})");
#else //x86
static QRegularExpression addressRegExp("([0-9A-Fa-f]{8})");
#endif //_WIN64
void LogView::linkify(QString & msg)
{
#ifdef _WIN64
    msg.replace(addressRegExp, "<a href=\"x64dbg://localhost/address64#\\1\">\\1</a>");
#else //x86
    msg.replace(addressRegExp, "<a href=\"x64dbg://localhost/address32#\\1\">\\1</a>");
#endif //_WIN64
}

/**
* @brief LogView::addMsgToLogHtmlSlot Adds a HTML message to the log view. This function is a slot for Bridge::addMsgToLogHtml.
* @param msg The log message (Which is assumed to contain HTML)
*/
void LogView::addMsgToLogHtmlSlot(QByteArray msg)
{
    LogView::addMsgToLogSlotRaw(msg, false);
}

/**
* @brief LogView::addMsgToLogSlot Adds a message to the log view. This function is a slot for Bridge::addMsgToLog.
* @param msg The log message
*/
void LogView::addMsgToLogSlot(QByteArray msg)
{
    LogView::addMsgToLogSlotRaw(msg, true);
}

/**
 * @brief LogView::addMsgToLogSlotRaw Adds a message to the log view.
 * @param msg The log message
 * @param encodeHTML HTML-encode the log message or not
 */
void LogView::addMsgToLogSlotRaw(QByteArray msg, bool encodeHTML)
{
    /*
    * This supports the 'UTF-8 Everywhere' manifesto.
    * - UTF-8 (http://utf8everywhere.org);
    * - No BOM (http://utf8everywhere.org/#faq.boms);
    * - No carriage return (http://utf8everywhere.org/#faq.crlf).
    */

    if(msg.length() == 0 || msg.at(0) == '\0')
        return;

    // fix Unix-style line endings.
    // redirect the log
    QString msgUtf16;
    bool redirectError = false;
    if(logRedirection != nullptr)
    {
        if(utf16Redirect)
        {
            msgUtf16 = QString::fromUtf8(msg);
            msgUtf16.replace("\n", "\r\n");
            if(!fwrite(msgUtf16.utf16(), msgUtf16.length(), 2, logRedirection))
            {
                fclose(logRedirection);
                logRedirection = nullptr;
                redirectError = true;
            }
        }
        else
        {
            const char* data;
            std::string temp;
            size_t offset = 0;
            size_t buffersize = 0;
            if(strstr(msg.constData(), "\r\n") != nullptr)  // Don't replace "\r\n" to "\n" if there is none
            {
                temp = msg.constData();
                while(true)
                {
                    size_t index = temp.find("\r\n", offset);
                    if(index == std::string::npos)
                        break;
                    temp.erase(index);
                    offset = index;
                }
                data = temp.c_str();
                buffersize = temp.size();
            }
            else
            {
                data = msg.constData();
                buffersize = strlen(msg);
            }
            if(!fwrite(data, buffersize, 1, logRedirection))
            {
                fclose(logRedirection);
                logRedirection = nullptr;
                redirectError = true;
            }
            if(loggingEnabled)
                msgUtf16 = QString::fromUtf8(data, int(buffersize));
        }
    }
    else msgUtf16 = QString::fromUtf8(msg);

    if(!loggingEnabled)
        return;

    if(encodeHTML)
    {
        msgUtf16 = msgUtf16.toHtmlEscaped();
        /* Below line will break HTML tags with spaces separating the HTML tag name and attributes.
            ie <a href="aaaa"> -> <a&nbsp;href="aaaa">
            so we don't escape spaces where we deliberately passed in HTML.
        */
        msgUtf16.replace(QChar(' '), QString("&nbsp;"));
    }
    if(logRedirection)
    {
        if(utf16Redirect)
            msgUtf16.replace(QString("\r\n"), QString("<br/>\n"));
        else
            msgUtf16.replace(QChar('\n'), QString("<br/>\n"));
    }
    else
    {
        msgUtf16.replace(QString("\r\n"), QString("<br/>\n"));
        msgUtf16.replace(QChar('\n'), QString("<br/>\n"));
    }
    if(encodeHTML)
    {
        /* If we passed in non-html log string, we look for address links.
        * otherwise, if our HTML contains any address-looking word, ie in our CSS, it would be mangled
        * linking to addresses when passing in HTML log message is an exercise left to the plugin developer */
        linkify(msgUtf16);
    }

    if(redirectError)
        msgUtf16.append(tr("fwrite() failed (GetLastError()= %1 ). Log redirection stopped.\n").arg(GetLastError()));

    if(logBuffer.length() >= MAX_LOG_BUFFER_SIZE)
        logBuffer.clear();

    logBuffer.append(msgUtf16);
    if(flushLog)
    {
        flushTimerSlot();
        flushLog = false;
    }
}

/**
 * @brief LogView::onAnchorClicked Called when a hyperlink is clicked
 * @param link The clicked link
 */
void LogView::onAnchorClicked(const QUrl & link)
{
    handleLink(this, link);
}

void LogView::clearLogSlot()
{
    this->clear();
}

void LogView::stopRedirectLogSlot()
{
    if(logRedirection != nullptr)
    {
        fclose(logRedirection);
        logRedirection = nullptr;
        GuiAddLogMessage(tr("Log redirection is stopped.\n").toUtf8().constData());
    }
    else
    {
        GuiAddLogMessage(tr("Log is not redirected.\n").toUtf8().constData());
    }
}

void LogView::redirectLogToFileSlot(QString filename)
{
    if(logRedirection != nullptr)
    {
        fclose(logRedirection);
        logRedirection = nullptr;
        GuiAddLogMessage(tr("Log redirection is stopped.\n").toUtf8().constData());
    }
    logRedirection = _wfopen(filename.toStdWString().c_str(), L"ab");
    if(logRedirection == nullptr)
        GuiAddLogMessage(tr("_wfopen() failed. Log will not be redirected to %1.\n").arg(QString::fromWCharArray(BridgeUserDirectory())).toUtf8().constData());
    else
    {
        if(utf16Redirect && ftell(logRedirection) == 0)
        {
            unsigned short BOM = 0xfeff;
            fwrite(&BOM, 2, 1, logRedirection);
        }
        GuiAddLogMessage(tr("Log will be redirected to %1.\n").arg(filename).toUtf8().constData());
    }
}

void LogView::redirectLogSlot()
{
    if(logRedirection != nullptr)
    {
        fclose(logRedirection);
        logRedirection = nullptr;
        GuiAddLogMessage(tr("Log redirection is stopped.\n").toUtf8().constData());
    }
    else
    {
        BrowseDialog browse(this, tr("Redirect log to file"), tr("Enter the file to which you want to redirect log messages."), tr("Log files (*.txt);;All files (*.*)"), QString::fromWCharArray(BridgeUserDirectory()), true);
        if(browse.exec() == QDialog::Accepted)
        {
            redirectLogToFileSlot(browse.path);
        }
    }
}

void LogView::setLoggingEnabled(bool enabled)
{
    if(enabled)
    {
        loggingEnabled = true;
        GuiAddStatusBarMessage(tr("Logging will be enabled.\n").toUtf8().constData());
    }
    else
    {
        GuiAddStatusBarMessage(tr("Logging will be disabled.\n").toUtf8().constData());
        loggingEnabled = false;
    }
}

bool LogView::getLoggingEnabled()
{
    return loggingEnabled;
}

void LogView::autoScrollSlot()
{
    autoScroll = !autoScroll;
}

void LogView::saveToFileSlot(QString fileName)
{
    QFile savedLog(fileName);
    savedLog.open(QIODevice::Append | QIODevice::Text);
    if(savedLog.error() != QFile::NoError)
    {
        GuiAddLogMessage(tr("Error, log have not been saved.\n").toUtf8().constData());
    }
    else
    {
        savedLog.write(this->document()->toPlainText().toUtf8().constData());
        savedLog.close();
        GuiAddLogMessage(tr("Log have been saved as %1\n").arg(fileName).toUtf8().constData());
    }
}

/**
 * @brief LogView::saveSlot Called by "save" action
 */
void LogView::saveSlot()
{
    QString fileName;
    fileName = QString("log-%1.txt").arg(isoDateTime());
    saveToFileSlot(fileName);
}

void LogView::toggleLoggingSlot()
{
    setLoggingEnabled(!getLoggingEnabled());
}

void LogView::copyToGlobalNotes()
{
    char* NotesBuffer;
    emit Bridge::getBridge()->getGlobalNotes(&NotesBuffer);
    QString Notes = QString::fromUtf8(NotesBuffer);
    BridgeFree(NotesBuffer);
    Notes.append(this->textCursor().selectedText());
    emit Bridge::getBridge()->setGlobalNotes(Notes);
}

void LogView::copyToDebuggeeNotes()
{
    char* NotesBuffer;
    emit Bridge::getBridge()->getDebuggeeNotes(&NotesBuffer);
    QString Notes = QString::fromUtf8(NotesBuffer);
    BridgeFree(NotesBuffer);
    Notes.append(this->textCursor().selectedText());
    emit Bridge::getBridge()->setDebuggeeNotes(Notes);
}

void LogView::findNextInLogSlot()
{
    find(QRegExp(lastFindText));
}

void LogView::findPreviousInLogSlot()
{
    find(QRegExp(lastFindText), QTextDocument::FindBackward);
}

void LogView::findInLogSlot()
{
    dialogFindInLog->show();

    if(dialogFindInLog->exec() == QDialog::Accepted)
    {
        QList<QTextEdit::ExtraSelection> extraSelections;
        QColor highlight = ConfigColor("SearchListViewHighlightColor");
        QColor background = ConfigColor("SearchListViewHighlightBackgroundColor");
        // capturing the current location, so that we can reset it once capturing all the
        // extra selections
        QTextCursor resetCursorLoc = textCursor();

        moveCursor(QTextCursor::Start);

        // finding all occurances matching the regex given from start
        while(find(QRegExp(dialogFindInLog->editText)))
        {
            QTextEdit::ExtraSelection extra;
            extra.format.setForeground(highlight);
            extra.format.setBackground(background);
            extra.cursor = textCursor();
            extraSelections.append(extra);
        }
        // highlighting all those selections
        setExtraSelections(extraSelections);
        setTextCursor(resetCursorLoc); // resetting the cursor location
        find(QRegExp(dialogFindInLog->editText));
        lastFindText = dialogFindInLog->editText;
    }
}

void LogView::pasteSlot()
{
    QString clipboardText = QApplication::clipboard()->text();
    if(clipboardText.isEmpty())
        return;
    if(!clipboardText.endsWith('\n'))
        clipboardText.append('\n');
    addMsgToLogSlot(clipboardText.toUtf8());
}

void LogView::flushTimerSlot()
{
    if(logBuffer.isEmpty())
        return;
    setUpdatesEnabled(false);
    static unsigned char counter = 100;
    counter--;
    if(counter == 0)
    {
        if(document()->characterCount() > MAX_LOG_BUFFER_SIZE)
            clear();
        counter = 100;
    }
    QTextCursor cursor(document());
    cursor.movePosition(QTextCursor::End);
    cursor.beginEditBlock();
    cursor.insertBlock();
    // hack to not insert too many newlines: https://lists.qt-project.org/pipermail/qt-interest-old/2011-July/034725.html
    cursor.deletePreviousChar();
    cursor.insertHtml(logBuffer);
    cursor.endEditBlock();
    if(autoScroll)
        moveCursor(QTextCursor::End);
    setUpdatesEnabled(true);
    logBuffer.clear();
}

void LogView::flushLogSlot()
{
    flushLog = true;
    if(flushTimer->isActive())
        flushTimerSlot();
}
