#include "CommandLineEdit.h"
#include "Bridge.h"
#include "Configuration.h"

CommandLineEdit::CommandLineEdit(QWidget* parent)
    : HistoryLineEdit(parent),
      mCurrentScriptIndex(-1)
{
    // QComboBox
    mCmdScriptType = new QComboBox(this);
    mCmdScriptType->setSizeAdjustPolicy(QComboBox::AdjustToContents);

    //Initialize QCompleter
    mCompleter = new QCompleter(QStringList(), this);
    mCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    mCompleter->setCompletionMode(QCompleter::PopupCompletion);
    mCompleterModel = (QStringListModel*)mCompleter->model();
    this->setCompleter(mCompleter);

    loadSettings("CommandLine");

    //Setup signals & slots
    connect(mCompleter, SIGNAL(activated(const QString &)), this, SLOT(clear()), Qt::QueuedConnection);
    connect(this, SIGNAL(textEdited(QString)), this, SLOT(autoCompleteUpdate(QString)));
    connect(Bridge::getBridge(), SIGNAL(autoCompleteAddCmd(QString)), this, SLOT(autoCompleteAddCmd(QString)));
    connect(Bridge::getBridge(), SIGNAL(autoCompleteDelCmd(QString)), this, SLOT(autoCompleteDelCmd(QString)));
    connect(Bridge::getBridge(), SIGNAL(autoCompleteClearAll()), this, SLOT(autoCompleteClearAll()));
    connect(Bridge::getBridge(), SIGNAL(registerScriptLang(SCRIPTTYPEINFO*)), this, SLOT(registerScriptType(SCRIPTTYPEINFO*)));
    connect(Bridge::getBridge(), SIGNAL(unregisterScriptLang(int)), this, SLOT(unregisterScriptType(int)));
    connect(mCmdScriptType, SIGNAL(currentIndexChanged(int)), this, SLOT(scriptTypeChanged(int)));
    connect(mCmdScriptType, SIGNAL(activated(int)), this, SLOT(scriptTypeActivated(int)));
    connect(Config(), SIGNAL(fontsUpdated()), this, SLOT(fontsUpdated()));

    fontsUpdated();
}

CommandLineEdit::~CommandLineEdit()
{
    saveSettings("CommandLine");
}

void CommandLineEdit::keyPressEvent(QKeyEvent* event)
{
    if(event->type() == QEvent::KeyPress && event->key() == Qt::Key_Tab)
    {
        // TAB autocompletes the command
        QStringList stringList = mCompleterModel->stringList();

        if(stringList.size())
        {
            QAbstractItemView* popup = mCompleter->popup();
            QModelIndex currentModelIndex = popup->currentIndex();

            // If not item selected, select first one in the list
            if(currentModelIndex.row() < 0)
                currentModelIndex = mCompleter->currentIndex();

            // If popup list is not visible, selected next suggested command
            if(!popup->isVisible())
            {
                for(int row = 0; row < popup->model()->rowCount(); row++)
                {
                    QModelIndex modelIndex = popup->model()->index(row, 0);

                    // If the lineedit contains a suggested command, get the next suggested one
                    if(popup->model()->data(modelIndex) == this->text())
                    {
                        int nextModelIndexRow = (currentModelIndex.row() + 1) % popup->model()->rowCount();
                        currentModelIndex = popup->model()->index(nextModelIndexRow, 0);
                        break;
                    }
                }
            }

            popup->setCurrentIndex(currentModelIndex);
            popup->hide();
        }
    }
    else if(event->type() == QEvent::KeyPress && event->modifiers() == Qt::ControlModifier)
    {
        int index = mCmdScriptType->currentIndex(), count = mCmdScriptType->count();
        if(event->key() == Qt::Key_Up)
        {
            // Ctrl + Up selects the previous language
            if(index > 0)
                index--;
            else
                index = count - 1;
        }
        else if(event->key() == Qt::Key_Down)
        {
            // Ctrl + Down selects the next language
            index = (index + 1) % count;
        }
        else
            HistoryLineEdit::keyPressEvent(event);
        mCmdScriptType->setCurrentIndex(index);
        scriptTypeActivated(index);
    }
    else
        HistoryLineEdit::keyPressEvent(event);
}

// Disables moving to Prev/Next child when pressing tab
bool CommandLineEdit::focusNextPrevChild(bool next)
{
    Q_UNUSED(next);
    return false;
}

void CommandLineEdit::execute()
{
    if(mCurrentScriptIndex == -1)
        return;
    GUISCRIPTEXECUTE exec = mScriptInfo[mCurrentScriptIndex].execute;
    QString cmd = text();

    if(exec)
    {
        if(cmd.trimmed().isEmpty())
            if(Config()->getBool("Gui", "AutoRepeatOnEnter"))
                cmd = getLineFromHistory();
        // Clear the status bar
        GuiAddStatusBarMessage("\n");
        // Send this string directly to the user
        exec(cmd.toUtf8().constData());
    }

    // Add this line to the history and clear text, regardless if it was executed
    addLineToHistory(cmd);
    emit textEdited("");
    setText("");
}

QWidget* CommandLineEdit::selectorWidget()
{
    return mCmdScriptType;
}

void CommandLineEdit::autoCompleteUpdate(const QString text)
{
    if(mCurrentScriptIndex == -1)
        return;
    // No command, no completer
    if(text.length() <= 0)
    {
        mCompleterModel->setStringList(QStringList());
    }
    else
    {
        // Save current index
        QModelIndex modelIndex = mCompleter->popup()->currentIndex();

        // User supplied callback
        GUISCRIPTCOMPLETER complete = mScriptInfo[mCurrentScriptIndex].completeCommand;

        if(complete)
        {
            // This will hold an array of strings allocated by BridgeAlloc
            char* completionList[32];
            int completionCount = _countof(completionList);

            complete(text.toUtf8().constData(), completionList, &completionCount);

            if(completionCount > 0)
            {
                QStringList stringList;

                // Append to the QCompleter string list and free the data
                for(int i = 0; i < completionCount; i++)
                {
                    stringList.append(completionList[i]);
                    BridgeFree(completionList[i]);
                }

                mCompleterModel->setStringList(stringList);
            }
            else
            {
                // Otherwise set the completer to nothing
                mCompleterModel->setStringList(QStringList());
            }
        }
        else
        {
            // Native auto-completion
            if(mCurrentScriptIndex == 0)
            {
                if(mDefaultCompletionsUpdated)
                {
                    mDefaultCompletions.removeDuplicates();
                    mDefaultCompletionsUpdated = false;
                }
                mCompleterModel->setStringList(mDefaultCompletions);
            }
        }

        // Restore index
        if(mCompleter->popup()->model()->rowCount() > modelIndex.row())
            mCompleter->popup()->setCurrentIndex(modelIndex);
    }
}

void CommandLineEdit::autoCompleteAddCmd(const QString cmd)
{
    mDefaultCompletions << cmd.split(QChar(','), QString::SkipEmptyParts);
    mDefaultCompletionsUpdated = true;
}

void CommandLineEdit::autoCompleteDelCmd(const QString cmd)
{
    QStringList deleteList = cmd.split(QChar(','), QString::SkipEmptyParts);

    for(int i = 0; i < deleteList.size(); i++)
        mDefaultCompletions.removeAll(deleteList.at(i));
    mDefaultCompletionsUpdated = true;
}

void CommandLineEdit::autoCompleteClearAll()
{
    // Update internal list only
    mDefaultCompletions.clear();
    mDefaultCompletionsUpdated = false;
}

void CommandLineEdit::registerScriptType(SCRIPTTYPEINFO* info)
{
    // Must be valid pointer
    if(!info)
    {
        Bridge::getBridge()->setResult(BridgeResult::RegisterScriptLang, 0);
        return;
    }

    // Insert
    info->id = mScriptInfo.size();
    mScriptInfo.push_back(*info);

    // Update
    mCmdScriptType->addItem(info->name);

    if(info->id == 0)
        mCurrentScriptIndex = 0;

    char savedType[MAX_SETTING_SIZE] = "";
    if(BridgeSettingGet("Gui", "ScriptType", savedType) && strcmp(info->name, savedType) == 0)
        mCmdScriptType->setCurrentIndex(info->id);

    Bridge::getBridge()->setResult(BridgeResult::RegisterScriptLang, 1);
}

void CommandLineEdit::unregisterScriptType(int id)
{
    // The default script type can't be unregistered
    if(id <= 0)
        return;

    // Loop through the vector and invalidate entry (validate id)
    for(int i = 0; i < mScriptInfo.size(); i++)
    {
        if(mScriptInfo[i].id == id)
        {
            mScriptInfo.removeAt(i);
            mCmdScriptType->removeItem(i);
            break;
        }
    }

    // Update selected index
    if(mCurrentScriptIndex > 0)
        mCurrentScriptIndex--;

    mCmdScriptType->setCurrentIndex(mCurrentScriptIndex);
}

void CommandLineEdit::scriptTypeChanged(int index)
{
    mCurrentScriptIndex = index;

    // Custom placeholder for the default commands
    duint timeWastedDebugging = 0;
    BridgeSettingGetUint("Engine", "TimeWastedDebugging", &timeWastedDebugging);
    if(index == 0 && timeWastedDebugging < 60 * 60 * 10)
    {
        setPlaceholderText(tr("Commands are comma separated (like assembly instructions): mov eax, ebx"));
    }
    else
    {
        setPlaceholderText(QString());
    }

    // Force reset autocompletion (blank string)
    emit textEdited("");
}

void CommandLineEdit::scriptTypeActivated(int index)
{
    if(index >= 0 && index < mScriptInfo.size())
        BridgeSettingSet("Gui", "ScriptType", mScriptInfo[index].name);
}

void CommandLineEdit::fontsUpdated()
{
    setFont(ConfigFont("Log"));
    mCompleter->popup()->setFont(ConfigFont("Log"));
}
