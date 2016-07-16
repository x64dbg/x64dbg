#include "LogView.h"
#include "Configuration.h"
#include "Bridge.h"
#include "BrowseDialog.h"

LogView::LogView(QWidget* parent) : QTextEdit(parent), logRedirection(INVALID_HANDLE_VALUE)
{
    updateStyle();
    this->setUndoRedoEnabled(false);
    this->setReadOnly(true);
    this->setLoggingEnabled(true);

    connect(Config(), SIGNAL(colorsUpdated()), this, SLOT(updateStyle()));
    connect(Config(), SIGNAL(fontsUpdated()), this, SLOT(updateStyle()));
    connect(Bridge::getBridge(), SIGNAL(addMsgToLog(QString)), this, SLOT(addMsgToLogSlot(QString)));
    connect(Bridge::getBridge(), SIGNAL(clearLog()), this, SLOT(clearLogSlot()));

    setupContextMenu();
}

LogView::~LogView()
{
    if(logRedirection != INVALID_HANDLE_VALUE)
        CloseHandle(logRedirection);
    logRedirection = INVALID_HANDLE_VALUE;
}

void LogView::updateStyle()
{
    setFont(ConfigFont("Log"));
    setStyleSheet(QString("QTextEdit { color: %1; background-color: %2 }").arg(ConfigColor("AbstractTableViewTextColor").name(), ConfigColor("AbstractTableViewBackgroundColor").name()));
}

void LogView::setupContextMenu()
{
    actionClear = new QAction(tr("Clea&r"), this);
    connect(actionClear, SIGNAL(triggered()), this, SLOT(clearLogSlot()));
    actionClear->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(actionClear);
    actionCopy = new QAction(tr("&Copy"), this);
    connect(actionCopy, SIGNAL(triggered()), this, SLOT(copy()));
    actionCopy->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(actionCopy);
    actionSelectAll = new QAction(tr("Select &All"), this);
    connect(actionSelectAll, SIGNAL(triggered()), this, SLOT(selectAll()));
    actionSelectAll->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(actionSelectAll);
    actionSave = new QAction(tr("&Save"), this);
    actionSave->setShortcutContext(Qt::WidgetShortcut);
    connect(actionSave, SIGNAL(triggered()), this, SLOT(saveSlot()));
    this->addAction(actionSave);
    actionToggleLogging = new QAction(tr("Disable &Logging"), this);
    actionToggleLogging->setShortcutContext(Qt::WidgetShortcut);
    connect(actionToggleLogging, SIGNAL(triggered()), this, SLOT(toggleLoggingSlot()));
    this->addAction(actionToggleLogging);
    actionRedirectLog = new QAction(tr("&Redirect Log..."), this);
    connect(actionRedirectLog, SIGNAL(triggered()), this, SLOT(redirectLogSlot()));
    this->addAction(actionRedirectLog);

    refreshShortcutsSlot();
    connect(Config(), SIGNAL(shortcutsUpdated()), this, SLOT(refreshShortcutsSlot()));
}

void LogView::refreshShortcutsSlot()
{
    actionCopy->setShortcut(ConfigShortcut("ActionCopy"));
    actionToggleLogging->setShortcut(ConfigShortcut("ActionToggleLogging"));
}

void LogView::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu wMenu(this);
    wMenu.addAction(actionClear);
    wMenu.addAction(actionSelectAll);
    wMenu.addAction(actionCopy);
    wMenu.addAction(actionSave);
    if(getLoggingEnabled())
        actionToggleLogging->setText(tr("Disable &Logging"));
    else
        actionToggleLogging->setText(tr("Enable &Logging"));
    wMenu.addAction(actionToggleLogging);
    wMenu.addAction(actionRedirectLog);

    wMenu.exec(event->globalPos());
}

void LogView::addMsgToLogSlot(QString msg)
{
    // fix Unix-style line endings.
    msg.replace(QString("\r\n"), QString("\n"));
    msg.replace(QChar('\n'), QString("\r\n"));
    // redirect the log
    if(logRedirection != INVALID_HANDLE_VALUE)
    {
        DWORD written = 0;
        SetLastError(ERROR_SUCCESS);
        if(!WriteFile(logRedirection, msg.data_ptr()->data(), msg.size() * 2, &written, nullptr))
        {
            CloseHandle(logRedirection);
            logRedirection = INVALID_HANDLE_VALUE;
            msg += tr("WriteFile() failed (GetLastError()= %1 ). Log redirection stopped.\r\n").arg(GetLastError());
        }
    }
    if(!loggingEnabled)
        return;
    if(this->document()->characterCount() > 10000 * 100) //limit the log to ~100mb
        this->clear();
    this->insertPlainText(msg);
    this->moveCursor(QTextCursor::End);
}

void LogView::clearLogSlot()
{
    this->clear();
}

void LogView::redirectLogSlot()
{
    if(logRedirection != INVALID_HANDLE_VALUE)
        CloseHandle(logRedirection);
    logRedirection = INVALID_HANDLE_VALUE;
    BrowseDialog browse(this, tr("Redirect log to file"), tr("Enter the file to which you want to redirect log messages."), tr("Log files(*.txt);;All files(*.*)"), QCoreApplication::applicationDirPath(), true);
    if(browse.exec() == QDialog::Accepted)
    {
        logRedirection = CreateFile(browse.path.toStdWString().c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
        if(logRedirection == INVALID_HANDLE_VALUE && GetLastError() == ERROR_FILE_EXISTS) // File already exists. Append to it.
            logRedirection = CreateFile(browse.path.toStdWString().c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if(logRedirection == INVALID_HANDLE_VALUE)
            addMsgToLogSlot(tr("CreateFile() failed. Log will not be redirected to %1.\n").arg(browse.path));
        else
        {
            unsigned short BOM = 0xfeff;
            DWORD written = 0;
            WriteFile(logRedirection, &BOM, sizeof(BOM), &written, nullptr);
            addMsgToLogSlot(tr("Log will be redirected to %1.\n").arg(browse.path));
        }
    }
}

void LogView::setLoggingEnabled(bool enabled)
{
    loggingEnabled = enabled;
}

bool LogView::getLoggingEnabled()
{
    return loggingEnabled;
}

void LogView::saveSlot()
{
    QString fileName;
    fileName = QString("log-%1.txt").arg(QDateTime::currentDateTime().toString().replace(QChar(':'), QChar('-')));
    QFile savedLog(fileName);
    savedLog.open(QIODevice::Append | QIODevice::Text);
    if(savedLog.error() != QFile::NoError)
    {
        addMsgToLogSlot(tr("Error, log have not been saved.\n"));
    }
    else
    {
        savedLog.write(this->document()->toPlainText().toUtf8().constData());
        savedLog.close();
        addMsgToLogSlot(tr("Log have been saved as %1\n").arg(fileName));
    }
}

void LogView::toggleLoggingSlot()
{
    setLoggingEnabled(!getLoggingEnabled());
}
