#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QLabel>
#include "SearchListView.h"
#include "FlickerThread.h"

SearchListView::SearchListView(bool EnableRegex, QWidget* parent, bool EnableLock) : QWidget(parent)
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
            // Create reference & search list
            mList = new SearchListViewTable();
            mSearchList = new SearchListViewTable();
            mSearchList->hide();

            // Vertical layout
            QVBoxLayout* listLayout = new QVBoxLayout();
            listLayout->setContentsMargins(0, 0, 0, 0);
            listLayout->setSpacing(0);
            listLayout->addWidget(mList);
            listLayout->addWidget(mSearchList);

            // Add list placeholder
            QWidget* listPlaceholder = new QWidget();
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

            if(!EnableRegex)
                mRegexCheckbox->hide();

            if(!EnableLock)
                mLockCheckbox->hide();

            // Horizontal layout
            QHBoxLayout* horzLayout = new QHBoxLayout();
            horzLayout->setContentsMargins(4, 0, (EnableRegex || EnableLock) ? 0 : 4, 0);
            horzLayout->setSpacing(2);
            horzLayout->addWidget(new QLabel(tr("Search: ")));
            horzLayout->addWidget(mSearchBox);
            horzLayout->addWidget(mLockCheckbox);
            horzLayout->addWidget(mRegexCheckbox);

            // Add searchbar placeholder
            QWidget* horzPlaceholder = new QWidget();
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
    mCurList = mList;
    mSearchStartCol = 0;

    // Install input event filter
    mSearchBox->installEventFilter(this);
    if(parent)
        mSearchBox->setWindowTitle(parent->metaObject()->className());

    // Setup search menu action
    mSearchAction = new QAction(DIcon("find.png"), tr("Search..."), this);
    connect(mSearchAction, SIGNAL(triggered()), this, SLOT(searchSlot()));

    // Slots
    connect(mList, SIGNAL(contextMenuSignal(QPoint)), this, SLOT(listContextMenu(QPoint)));
    connect(mList, SIGNAL(doubleClickedSignal()), this, SLOT(doubleClickedSlot()));
    connect(mSearchList, SIGNAL(contextMenuSignal(QPoint)), this, SLOT(listContextMenu(QPoint)));
    connect(mSearchList, SIGNAL(doubleClickedSignal()), this, SLOT(doubleClickedSlot()));
    connect(mSearchBox, SIGNAL(textChanged(QString)), this, SLOT(searchTextChanged(QString)));
    connect(mRegexCheckbox, SIGNAL(stateChanged(int)), this, SLOT(on_checkBoxRegex_stateChanged(int)));
    connect(mLockCheckbox, SIGNAL(toggled(bool)), mSearchBox, SLOT(setDisabled(bool)));

    // List input should always be forwarded to the filter edit
    mSearchList->setFocusProxy(mSearchBox);
    mList->setFocusProxy(mSearchBox);
}

SearchListView::~SearchListView()
{
}

bool SearchListView::findTextInList(SearchListViewTable* list, QString text, int row, int startcol, bool startswith)
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

void SearchListView::searchTextChanged(const QString & arg1)
{
    SearchListViewTable* mPrevList = NULL;

    if(mSearchList->isHidden())
    {
        auto selList = mList->getSelection();
        if(!selList.empty() && mList->isValidIndex(selList[0], 0))
            mLastFirstColValue = mList->getCellContent(selList[0], 0);
    }
    else
    {
        auto selList = mSearchList->getSelection();
        if(!selList.empty() && mSearchList->isValidIndex(selList[0], 0))
            mLastFirstColValue = mSearchList->getCellContent(selList[0], 0);
    }

    // get the correct previous list instance
    if(mList->isVisible())
        mPrevList = mList;
    else
        mPrevList = mSearchList;

    if(arg1.length())
    {
        mList->hide();
        mSearchList->show();
        mCurList = mSearchList;
    }
    else
    {
        mSearchList->hide();
        mList->show();
        mCurList = mList;
    }

    mSearchList->setRowCount(0);
    int rows = mList->getRowCount();
    int columns = mList->getColumnCount();
    for(int i = 0, j = 0; i < rows; i++)
    {
        if(findTextInList(mList, arg1, i, mSearchStartCol, false))
        {
            mSearchList->setRowCount(j + 1);
            for(int k = 0; k < columns; k++)
                mSearchList->setCellContent(j, k, mList->getCellContent(i, k));
            j++;
        }
    }

    mSearchList->reloadData();

    bool hasSetSingleSelection = false;
    if(!mLastFirstColValue.isEmpty())
    {
        rows = mCurList->getRowCount();
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

    if(rows == 0)
        emit emptySearchResult();

    // Do not highlight with regex
    if(mRegexCheckbox->checkState() == Qt::Unchecked)
        mSearchList->highlightText = arg1;
    else
        mSearchList->highlightText = "";

    // setup the same layout of the prev list control
    LoadPrevListLayout(mPrevList);
}

void SearchListView::refreshSearchList()
{
    searchTextChanged(mSearchBox->text());
}

void SearchListView::listContextMenu(const QPoint & pos)
{
    QMenu wMenu(this);
    emit listContextMenuSignal(&wMenu);
    wMenu.addSeparator();
    wMenu.addAction(mSearchAction);
    QMenu wCopyMenu(tr("&Copy"), this);
    wCopyMenu.setIcon(DIcon("copy.png"));
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

        switch(keyEvent->key())
        {
        // The user pressed enter/return
        case Qt::Key_Return:
        case Qt::Key_Enter:
            if(mCurList->getCellContent(mCurList->getInitialSelection(), 0).length())
                emit enterPressedSignal();
            return true;

        // Search box misc controls
        case Qt::Key_Escape:
            mSearchBox->clear();
        case Qt::Key_Left:
        case Qt::Key_Right:
        case Qt::Key_Backspace:
        case Qt::Key_Delete:
        case Qt::Key_Home:
        case Qt::Key_End:
        case Qt::Key_Insert:
            return QWidget::eventFilter(obj, event);

        // Search box shortcuts
        case Qt::Key_V: //Ctrl+V
        case Qt::Key_X: //Ctrl+X
        case Qt::Key_Z: //Ctrl+Z
        case Qt::Key_A: //Ctrl+A
        case Qt::Key_Y: //Ctrl+Y
            if(keyEvent->modifiers() == Qt::CTRL)
                return QWidget::eventFilter(obj, event);
        }

        // Printable characters go to the search box
        char key = keyEvent->text().toUtf8().constData()[0];

        if(isprint(key))
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
    thread->start();
}

void SearchListView::LoadPrevListLayout(SearchListViewTable* mPrevList)
{
    if(mPrevList == NULL || mPrevList == mCurList)
        return;

    int cols = mPrevList->getColumnCount();
    for(int i = 0; i < cols; i++)
    {
        mCurList->setColumnOrder(i, mPrevList->getColumnOrder(i));
        mCurList->setColumnHidden(i, mPrevList->getColumnHidden(i));
        mCurList->setColumnWidth(i, mPrevList->getColumnWidth(i));
    }
}
