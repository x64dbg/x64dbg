#include "LogView.h"
#include "Configuration.h"
#include "Bridge.h"

LogView::LogView(QWidget* parent) : QTextEdit(parent)
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

    refreshShortcutsSlot();
    connect(Config(), SIGNAL(shortcutsUpdated()), this, SLOT(refreshShortcutsSlot()));
}

void LogView::refreshShortcutsSlot()
{
    actionCopy->setShortcut(ConfigShortcut("ActionCopy"));
    // More shortcuts?
}

void LogView::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu* wMenu = new QMenu(this);
    wMenu->addAction(actionClear);
    wMenu->addAction(actionSelectAll);
    wMenu->addAction(actionCopy);
    wMenu->addAction(actionSave);
    if(getLoggingEnabled())
        actionToggleLogging->setText(tr("Disable &Logging"));
    else
        actionToggleLogging->setText(tr("Enable &Logging"));
    wMenu->addAction(actionToggleLogging);

    wMenu->exec(event->globalPos());
    delete wMenu;
}

void LogView::addMsgToLogSlot(QString msg)
{
    if(!loggingEnabled)
        return;
    if(this->document()->characterCount() > 10000 * 100) //limit the log to ~100mb
        this->clear();
    this->moveCursor(QTextCursor::End);
    this->insertPlainText(msg);
}

void LogView::clearLogSlot()
{
    this->clear();
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
    fileName=QString("log-%1.txt").arg(QDateTime::currentDateTime().toString().replace(QChar(':'), QChar('-')));
    QFile savedLog(fileName);
    savedLog.open(QIODevice::Append | QIODevice::Text);
    if(savedLog.error()!=QFile::NoError)
    {
        addMsgToLogSlot(tr("Error, log have not been saved.\n"));
    }
    else
    {
        savedLog.write(this->document()->toPlainText().toUtf8().constData());
        savedLog.close();
        addMsgToLogSlot(tr("Log have been saved to %1\n").arg(fileName));
    }
}

void LogView::toggleLoggingSlot()
{
    setLoggingEnabled(!getLoggingEnabled());
}
