#include "LogView.h"
#include "Configuration.h"
#include "Bridge.h"
#include "BrowseDialog.h"

LogView::LogView(QWidget* parent) : QTextEdit(parent), logRedirection(NULL)
{
    updateStyle();
    this->setUndoRedoEnabled(false);
    this->setReadOnly(true);
    this->setLoggingEnabled(true);

    connect(Config(), SIGNAL(colorsUpdated()), this, SLOT(updateStyle()));
    connect(Config(), SIGNAL(fontsUpdated()), this, SLOT(updateStyle()));
    connect(Bridge::getBridge(), SIGNAL(addMsgToLog(QString)), this, SLOT(addMsgToLogSlot(QString)));
    connect(Bridge::getBridge(), SIGNAL(clearLog()), this, SLOT(clearLogSlot()));
    connect(Bridge::getBridge(), SIGNAL(setLogEnabled(bool)), this, SLOT(setLoggingEnabled(bool)));

    setupContextMenu();
}

LogView::~LogView()
{
    if(logRedirection != NULL)
        fclose(logRedirection);
    logRedirection = NULL;
}

void LogView::updateStyle()
{
    setFont(ConfigFont("Log"));
    setStyleSheet(QString("QTextEdit { color: %1; background-color: %2 }").arg(ConfigColor("AbstractTableViewTextColor").name(), ConfigColor("AbstractTableViewBackgroundColor").name()));
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
    actionClear = setupAction(tr("Clea&r"), this, SLOT(clearLogSlot()));
    actionCopy = setupAction(tr("&Copy"), this, SLOT(copy()));
    actionSelectAll = setupAction(tr("Select &All"), this, SLOT(selectAll()));
    actionSave = setupAction(tr("&Save"), this, SLOT(saveSlot()));
    actionToggleLogging = setupAction(tr("Disable &Logging"), this, SLOT(toggleLoggingSlot()));
    actionRedirectLog = setupAction(tr("&Redirect Log..."), this, SLOT(redirectLogSlot()));

    refreshShortcutsSlot();
    connect(Config(), SIGNAL(shortcutsUpdated()), this, SLOT(refreshShortcutsSlot()));
}

void LogView::refreshShortcutsSlot()
{
    actionCopy->setShortcut(ConfigShortcut("ActionCopy"));
    actionToggleLogging->setShortcut(ConfigShortcut("ActionToggleLogging"));
    actionRedirectLog->setShortcut(ConfigShortcut("ActionRedirectLog"));
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
    if(logRedirection == NULL)
        actionRedirectLog->setText(tr("&Redirect Log..."));
    else
        actionRedirectLog->setText(tr("Stop &Redirection"));
    wMenu.addAction(actionRedirectLog);

    wMenu.exec(event->globalPos());
}

void LogView::addMsgToLogSlot(QString msg)
{
    // fix Unix-style line endings.
    msg.replace(QString("\r\n"), QString("\n"));
    msg.replace(QChar('\n'), QString("\r\n"));
    // redirect the log
    if(logRedirection != NULL)
    {
        if(!fwrite(msg.data_ptr()->data(), msg.size() * 2, 1, logRedirection))
        {
            fclose(logRedirection);
            logRedirection = NULL;
            msg += tr("fwrite() failed (GetLastError()= %1 ). Log redirection stopped.\r\n").arg(GetLastError());
        }
    }
    if(!loggingEnabled)
        return;
    if(this->document()->characterCount() > 10000 * 100) //limit the log to ~100mb
        this->clear();
    // This sets the cursor to the end for the next insert
    this->moveCursor(QTextCursor::End);
    this->insertPlainText(msg);
    // This sets the cursor to the end to display the new text
    this->moveCursor(QTextCursor::End);
}

void LogView::clearLogSlot()
{
    this->clear();
}

void LogView::redirectLogSlot()
{
    if(logRedirection != NULL)
    {
        fclose(logRedirection);
        logRedirection = NULL;
    }
    else
    {
        BrowseDialog browse(this, tr("Redirect log to file"), tr("Enter the file to which you want to redirect log messages."), tr("Log files(*.txt);;All files(*.*)"), QCoreApplication::applicationDirPath(), true);
        if(browse.exec() == QDialog::Accepted)
        {
            logRedirection = _wfopen(browse.path.toStdWString().c_str(), L"ab");
            if(logRedirection == NULL)
                GuiAddLogMessage(tr("_wfopen() failed. Log will not be redirected to %1.\n").arg(browse.path).toUtf8().constData());
            else
            {
                if(ftell(logRedirection) == 0)
                {
                    unsigned short BOM = 0xfeff;
                    fwrite(&BOM, 2, 1, logRedirection);
                }
                GuiAddLogMessage(tr("Log will be redirected to %1.\n").arg(browse.path).toUtf8().constData());
            }
        }
    }
}

void LogView::setLoggingEnabled(bool enabled)
{
    if(enabled)
    {
        loggingEnabled = true;
        GuiAddLogMessage(tr("Logging will be enabled.\n").toUtf8().constData());
    }
    else
    {
        GuiAddLogMessage(tr("Logging will be disabled.\n").toUtf8().constData());
        loggingEnabled = false;
    }
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
        GuiAddLogMessage(tr("Error, log have not been saved.\n").toUtf8().constData());
    }
    else
    {
        savedLog.write(this->document()->toPlainText().toUtf8().constData());
        savedLog.close();
        GuiAddLogMessage(tr("Log have been saved as %1\n").arg(fileName).toUtf8().constData());
    }
}

void LogView::toggleLoggingSlot()
{
    setLoggingEnabled(!getLoggingEnabled());
}
