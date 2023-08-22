#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QLabel>
#include <QTimer>
#include "SearchListView.h"
#include "FlickerThread.h"
#include "MethodInvoker.h"

SearchListView::SearchListView(QWidget* parent, AbstractSearchList* abstractSearchList, bool enableRegex, bool enableLock)
    : QWidget(parent), mAbstractSearchList(abstractSearchList)
{
    setContextMenuPolicy(Qt::CustomContextMenu);

    // Create the main button/bar view with QSplitter
    //
    // |- Splitter --------------------------------------------|
    // | LISTVIEW                                              |
    // | SEARCH: | SEARCH BOX | LOCK CHECKBOX | REGEX CHECKBOX |
    // |-------------------------------------------------------|
    QSplitter* barSplitter = new QSplitter(Qt::Vertical);
    {
        // Create list layout (contains both ListViews)
        {
            // Initially hide the search list
            abstractSearchList->searchList()->hide();

            // Vertical layout
            QVBoxLayout* listLayout = new QVBoxLayout();
            listLayout->setContentsMargins(0, 0, 0, 0);
            listLayout->setSpacing(0);
            listLayout->addWidget(abstractSearchList->list());
            listLayout->addWidget(abstractSearchList->searchList());

            // Add list placeholder
            QWidget* listPlaceholder = new QWidget(this);
            listPlaceholder->setLayout(listLayout);

            barSplitter->addWidget(listPlaceholder);
        }

        // Filtering elements
        {
            // Input box
            mSearchBox = new QLineEdit();
            mSearchBox->setPlaceholderText(tr("Type here to filter results..."));

            // Regex parsing checkbox
            mRegexCheckbox = new QCheckBox(tr("Regex"));
            mRegexCheckbox->setTristate(true);

            // Lock checkbox
            mLockCheckbox = new QCheckBox(tr("Lock"));

            if(!enableRegex)
                mRegexCheckbox->hide();

            if(!enableLock)
                mLockCheckbox->hide();

            // Horizontal layout
            QHBoxLayout* horzLayout = new QHBoxLayout();
            horzLayout->setContentsMargins(4, 0, (enableRegex || enableLock) ? 0 : 4, 0);
            horzLayout->setSpacing(2);
            QLabel* label = new QLabel(tr("Search: "), this);
            label->setBuddy(mSearchBox);
            horzLayout->addWidget(label);
            horzLayout->addWidget(mSearchBox);
            horzLayout->addWidget(mLockCheckbox);
            horzLayout->addWidget(mRegexCheckbox);

            // Add searchbar placeholder
            QWidget* horzPlaceholder = new QWidget(this);
            horzPlaceholder->setLayout(horzLayout);

            barSplitter->addWidget(horzPlaceholder);
        }

        // Minimum size for the search box
        barSplitter->setStretchFactor(0, 1000);
        barSplitter->setStretchFactor(0, 1);

        // Disable main splitter
        for(int i = 0; i < barSplitter->count(); i++)
            barSplitter->handle(i)->setEnabled(false);
    }

    // Set the main layout which holds the splitter
    QVBoxLayout* mainLayout = new QVBoxLayout();
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(barSplitter);
    setLayout(mainLayout);

    // Set global variables
    mCurList = abstractSearchList->list();
    mSearchStartCol = 0;

    // Install input event filter
    mSearchBox->installEventFilter(this);
    if(parent)
        mSearchBox->setWindowTitle(parent->metaObject()->className());

    // Setup search menu action
    mSearchAction = new QAction(DIcon("find"), tr("Search..."), this);
    connect(mSearchAction, SIGNAL(triggered()), this, SLOT(searchSlot()));

    // https://wiki.qt.io/Delay_action_to_wait_for_user_interaction
    mTypingTimer = new QTimer(this);
    mTypingTimer->setSingleShot(true);
    connect(mTypingTimer, SIGNAL(timeout()), this, SLOT(filterEntries()));

    // Slots
    connect(abstractSearchList->list(), SIGNAL(contextMenuSignal(QPoint)), this, SLOT(listContextMenu(QPoint)));
    connect(abstractSearchList->list(), SIGNAL(doubleClickedSignal()), this, SLOT(doubleClickedSlot()));
    connect(abstractSearchList->searchList(), SIGNAL(contextMenuSignal(QPoint)), this, SLOT(listContextMenu(QPoint)));
    connect(abstractSearchList->searchList(), SIGNAL(doubleClickedSignal()), this, SLOT(doubleClickedSlot()));
    connect(mSearchBox, SIGNAL(textEdited(QString)), this, SLOT(searchTextEdited(QString)));
    connect(mRegexCheckbox, SIGNAL(stateChanged(int)), this, SLOT(on_checkBoxRegex_stateChanged(int)));
    connect(mLockCheckbox, SIGNAL(toggled(bool)), mSearchBox, SLOT(setDisabled(bool)));

    // List input should always be forwarded to the filter edit
    abstractSearchList->searchList()->setFocusProxy(mSearchBox);
    abstractSearchList->list()->setFocusProxy(mSearchBox);
}

bool SearchListView::findTextInList(AbstractStdTable* list, QString text, int row, int startcol, bool startswith)
{
    int count = list->getColumnCount();
    if(startcol + 1 > count)
        return false;
    if(startswith)
    {
        for(int i = startcol; i < count; i++)
            if(list->getCellContent(row, i).startsWith(text, Qt::CaseInsensitive))
                return true;
    }
    else
    {
        for(int i = startcol; i < count; i++)
        {
            auto state = mRegexCheckbox->checkState();
            if(state != Qt::Unchecked)
            {
                if(list->getCellContent(row, i).contains(QRegExp(text, state == Qt::PartiallyChecked ? Qt::CaseInsensitive : Qt::CaseSensitive)))
                    return true;
            }
            else
            {
                if(list->getCellContent(row, i).contains(text, Qt::CaseInsensitive))
                    return true;
            }
        }
    }
    return false;
}

void SearchListView::filterEntries()
{
    mAbstractSearchList->lock();

    // store the first selection value
    QString mLastFirstColValue;
    auto selList = mCurList->getSelection();
    if(!selList.empty() && mCurList->isValidIndex(selList[0], 0))
        mLastFirstColValue = mCurList->getCellContent(selList[0], 0);

    // get the correct previous list instance
    auto mPrevList = mAbstractSearchList->list()->isVisible() ? mAbstractSearchList->list() : mAbstractSearchList->searchList();

    if(mFilterText.length())
    {
        MethodInvoker::invokeMethod([this]()
        {
            mAbstractSearchList->list()->hide();
            mAbstractSearchList->searchList()->show();
        });

        mCurList = mAbstractSearchList->searchList();

        // filter the list
        auto filterType = AbstractSearchList::FilterContainsTextCaseInsensitive;
        switch(mRegexCheckbox->checkState())
        {
        case Qt::PartiallyChecked:
            filterType = AbstractSearchList::FilterRegexCaseInsensitive;
            break;
        case Qt::Checked:
            filterType = AbstractSearchList::FilterRegexCaseSensitive;
            break;
        }
        mAbstractSearchList->filter(mFilterText, filterType, mSearchStartCol);
    }
    else
    {
        MethodInvoker::invokeMethod([this]()
        {
            mAbstractSearchList->searchList()->hide();
            mAbstractSearchList->list()->show();
        });

        mCurList = mAbstractSearchList->list();
    }

    // attempt to restore previous selection
    bool hasSetSingleSelection = false;
    if(!mLastFirstColValue.isEmpty())
    {
        int rows = mCurList->getRowCount();
        mCurList->setTableOffset(0);
        for(int i = 0; i < rows; i++)
        {
            if(mCurList->getCellContent(i, 0) == mLastFirstColValue)
            {
                if(rows > mCurList->getViewableRowsCount())
                {
                    int cur = i - mCurList->getViewableRowsCount() / 2;
                    if(!mCurList->isValidIndex(cur, 0))
                        cur = i;
                    mCurList->setTableOffset(cur);
                }
                mCurList->setSingleSelection(i);
                hasSetSingleSelection = true;
                break;
            }
        }
    }
    if(!hasSetSingleSelection)
        mCurList->setSingleSelection(0);

    if(!mCurList->getRowCount())
        emit emptySearchResult();

    // Do not highlight with regex
    // TODO: fully respect highlighting mode
    if(mRegexCheckbox->checkState() == Qt::Unchecked)
        mAbstractSearchList->searchList()->setHighlightText(mFilterText, mSearchStartCol);
    else
        mAbstractSearchList->searchList()->setHighlightText(QString());

    // Reload the search list data
    mAbstractSearchList->searchList()->reloadData();

    // setup the same layout of the previous list control
    if(mPrevList != mCurList)
    {
        int cols = mPrevList->getColumnCount();
        for(int i = 0; i < cols; i++)
        {
            mCurList->setColumnOrder(i, mPrevList->getColumnOrder(i));
            mCurList->setColumnHidden(i, mPrevList->getColumnHidden(i));
            mCurList->setColumnWidth(i, mPrevList->getColumnWidth(i));
        }
    }

    mAbstractSearchList->unlock();
}

void SearchListView::searchTextEdited(const QString & text)
{
    mFilterText = text;
    mAbstractSearchList->lock();
    mTypingTimer->setInterval([](dsint rowCount)
    {
        // These numbers are kind of arbitrarily chosen, but seem to work
        if(rowCount <= 10000)
            return 0;
        else if(rowCount <= 600000)
            return 100;
        else
            return 350;
    }(mAbstractSearchList->list()->getRowCount()));
    mAbstractSearchList->unlock();
    mTypingTimer->start(); // This will fire filterEntries after interval ms.
    // If the user types something before it fires, the timer restarts counting
}

void SearchListView::refreshSearchList()
{
    filterEntries();
}

void SearchListView::clearFilter()
{
    bool isFilterAlreadyEmpty = mFilterText.isEmpty();
    mFilterText.clear();

    if(!isFilterAlreadyEmpty)
    {
        MethodInvoker::invokeMethod([this]()
        {
            mSearchBox->clear();
        });

        filterEntries();
    }
}

void SearchListView::listContextMenu(const QPoint & pos)
{
    QMenu wMenu(this);
    emit listContextMenuSignal(&wMenu);
    wMenu.addSeparator();
    wMenu.addAction(mSearchAction);
    QMenu wCopyMenu(tr("&Copy"), this);
    wCopyMenu.setIcon(DIcon("copy"));
    mCurList->setupCopyMenu(&wCopyMenu);
    if(wCopyMenu.actions().length())
        wMenu.addMenu(&wCopyMenu);
    wMenu.exec(mCurList->mapToGlobal(pos));
}

void SearchListView::doubleClickedSlot()
{
    emit enterPressedSignal();
}

void SearchListView::on_checkBoxRegex_stateChanged(int state)
{
    QString tooltip;
    switch(state)
    {
    default:
    case Qt::Unchecked:
        //No tooltip
        break;
    case Qt::Checked:
        tooltip = tr("Use case sensitive regular expression");
        break;
    case Qt::PartiallyChecked:
        tooltip = tr("Use case insensitive regular expression");
        break;
    }
    mRegexCheckbox->setToolTip(tooltip);

    refreshSearchList();
}

bool SearchListView::isSearchBoxLocked()
{
    return mLockCheckbox->isChecked();
}

bool SearchListView::eventFilter(QObject* obj, QEvent* event)
{
    // Keyboard button press being sent to the QLineEdit
    if(obj == mSearchBox && event->type() == QEvent::KeyPress)
    {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        int key = keyEvent->key();
        if(key == Qt::Key_Return || key == Qt::Key_Enter)
        {
            // The user pressed enter/return
            if(mCurList->getCellContent(mCurList->getInitialSelection(), 0).length())
                emit enterPressedSignal();
            return true;
        }
        if(key == Qt::Key_Escape)
            clearFilter();
        if(!mSearchBox->text().isEmpty())
        {
            switch(key)
            {
            // Search box misc controls
            case Qt::Key_Left:
            case Qt::Key_Right:
            case Qt::Key_Backspace:
            case Qt::Key_Delete:
            case Qt::Key_Home:
            case Qt::Key_End:
            case Qt::Key_Insert:
                return QWidget::eventFilter(obj, event);
            // Search box shortcuts reliant on mSearchBox not being empty
            case Qt::Key_X: //Ctrl+X
            case Qt::Key_A: //Ctrl+A
                if(keyEvent->modifiers() == Qt::ControlModifier)
                    return QWidget::eventFilter(obj, event);
            }
        }
        switch(key)
        {
        // Search box shortcuts
        case Qt::Key_V: //Ctrl+V
        case Qt::Key_Z: //Ctrl+Z
        case Qt::Key_Y: //Ctrl+Y
            if(keyEvent->modifiers() == Qt::ControlModifier)
                return QWidget::eventFilter(obj, event);
        }
        // Printable characters go to the search box
        QString keyText = keyEvent->text();
        if(!keyText.isEmpty() && QChar(keyText.toUtf8().at(0)).isPrint())
            return QWidget::eventFilter(obj, event);
        // By default, all other keys are forwarded to the search view
        return QApplication::sendEvent(mCurList, event);
    }
    return QWidget::eventFilter(obj, event);
}

void SearchListView::searchSlot()
{
    FlickerThread* thread = new FlickerThread(mSearchBox, this);
    connect(thread, SIGNAL(setStyleSheet(QString)), mSearchBox, SLOT(setStyleSheet(QString)));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    thread->start();
}
