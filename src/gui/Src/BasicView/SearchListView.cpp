#include "SearchListView.h"
#include "ui_SearchListView.h"
#include "FlickerThread.h"


SearchListView::SearchListView(QWidget* parent) :
    QWidget(parent),
    ui(new Ui::SearchListView)
{
    ui->setupUi(this);
    setContextMenuPolicy(Qt::CustomContextMenu);

    // Create the reference list
    mList = new SearchListViewTable();

    // Create the search list
    mSearchList = new SearchListViewTable();
    mSearchList->hide();

    // Set global variables
    mSearchBox = ui->searchBox;
    mCurList = mList;
    mSearchStartCol = 0;

    // Create list layout
    mListLayout = new QVBoxLayout();
    mListLayout->setContentsMargins(0, 0, 0, 0);
    mListLayout->setSpacing(0);
    mListLayout->addWidget(mList);
    mListLayout->addWidget(mSearchList);

    // Create list placeholder
    mListPlaceHolder = new QWidget();
    mListPlaceHolder->setLayout(mListLayout);

    // Insert the placeholder
    ui->mainSplitter->insertWidget(0, mListPlaceHolder);

    // Set the main layout
    mMainLayout = new QVBoxLayout();
    mMainLayout->setContentsMargins(0, 0, 0, 0);
    mMainLayout->addWidget(ui->mainSplitter);
    setLayout(mMainLayout);

    // Minimal size for the search box
    ui->mainSplitter->setStretchFactor(0, 1000);
    ui->mainSplitter->setStretchFactor(0, 1);

    // Disable main splitter
    for(int i = 0; i < ui->mainSplitter->count(); i++)
        ui->mainSplitter->handle(i)->setEnabled(false);

    // Install eventFilter
    mList->installEventFilter(this);
    mSearchList->installEventFilter(this);
    mSearchBox->installEventFilter(this);

    // Setup search menu action
    mSearchAction = new QAction("Search...", this);
    connect(mSearchAction, SIGNAL(triggered()), this, SLOT(searchSlot()));

    // Slots
    connect(mList, SIGNAL(contextMenuSignal(QPoint)), this, SLOT(listContextMenu(QPoint)));
    connect(mList, SIGNAL(doubleClickedSignal()), this, SLOT(doubleClickedSlot()));
    connect(mSearchList, SIGNAL(contextMenuSignal(QPoint)), this, SLOT(listContextMenu(QPoint)));
    connect(mSearchList, SIGNAL(doubleClickedSignal()), this, SLOT(doubleClickedSlot()));
    connect(mSearchBox, SIGNAL(textChanged(QString)), this, SLOT(searchTextChanged(QString)));

    // List input should always be forwarded to the filter edit
    mSearchList->setFocusProxy(mSearchBox);
    mList->setFocusProxy(mSearchBox);
}

SearchListView::~SearchListView()
{
    delete ui;
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
            if(ui->checkBoxRegex->checkState() == Qt::Checked)
            {
                if(list->getCellContent(row, i).contains(QRegExp(text)))
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
    rows = mSearchList->getRowCount();
    mSearchList->setTableOffset(0);
    for(int i = 0; i < rows; i++)
    {
        if(findTextInList(mSearchList, arg1, i, mSearchStartCol, true))
        {
            if(rows > mSearchList->getViewableRowsCount())
            {
                int cur = i - mSearchList->getViewableRowsCount() / 2;
                if(!mSearchList->isValidIndex(cur, 0))
                    cur = i;
                mSearchList->setTableOffset(cur);
            }
            mSearchList->setSingleSelection(i);
            break;
        }
    }

    if(rows == 0)
        emit emptySearchResult();

    // Do not highlight with regex
    if(ui->checkBoxRegex->checkState() != Qt::Checked)
        mSearchList->highlightText = arg1;
    else
        mSearchList->highlightText = "";

    mSearchList->reloadData();
}

void SearchListView::listContextMenu(const QPoint & pos)
{
    QMenu* wMenu = new QMenu(this);
    emit listContextMenuSignal(wMenu);
    wMenu->addSeparator();
    wMenu->addAction(mSearchAction);
    QMenu wCopyMenu("&Copy", this);
    mCurList->setupCopyMenu(&wCopyMenu);
    if(wCopyMenu.actions().length())
        wMenu->addMenu(&wCopyMenu);
    wMenu->exec(mCurList->mapToGlobal(pos));
}

void SearchListView::doubleClickedSlot()
{
    emit enterPressedSignal();
}

void SearchListView::on_checkBoxRegex_toggled(bool checked)
{
    Q_UNUSED(checked);
    searchTextChanged(ui->searchBox->text());
}

bool SearchListView::eventFilter(QObject* obj, QEvent* event)
{
    // Keyboard button press being sent to the table views or the QLineEdit
    if((obj == mList || obj == mSearchList || obj == mSearchBox) && event->type() == QEvent::KeyPress)
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

        // Clear the search box with the escape key
        case Qt::Key_Escape:
            mSearchBox->clear();
            return true;
        }
    }

    return QWidget::eventFilter(obj, event);
}

void SearchListView::searchSlot()
{
    FlickerThread* thread = new FlickerThread(ui->searchBox, this);
    connect(thread, SIGNAL(setStyleSheet(QString)), ui->searchBox, SLOT(setStyleSheet(QString)));
    thread->start();
}
