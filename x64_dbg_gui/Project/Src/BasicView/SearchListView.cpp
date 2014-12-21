#include "SearchListView.h"
#include "ui_SearchListView.h"

SearchListView::SearchListView(QWidget* parent) :
    QWidget(parent),
    ui(new Ui::SearchListView)
{
    ui->setupUi(this);

    // Create the reference list
    mList = new SearchListViewTable();
    mList->setContextMenuPolicy(Qt::CustomContextMenu);

    // Create the search list
    mSearchList = new SearchListViewTable();
    mSearchList->setContextMenuPolicy(Qt::CustomContextMenu);
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

    // Setup signals
    connect(mList, SIGNAL(keyPressedSignal(QKeyEvent*)), this, SLOT(listKeyPressed(QKeyEvent*)));
    connect(mList, SIGNAL(contextMenuSignal(QPoint)), this, SLOT(listContextMenu(QPoint)));
    connect(mList, SIGNAL(doubleClickedSignal()), this, SLOT(doubleClickedSlot()));
    connect(mSearchList, SIGNAL(keyPressedSignal(QKeyEvent*)), this, SLOT(listKeyPressed(QKeyEvent*)));
    connect(mSearchList, SIGNAL(contextMenuSignal(QPoint)), this, SLOT(listContextMenu(QPoint)));
    connect(mSearchList, SIGNAL(doubleClickedSignal()), this, SLOT(doubleClickedSlot()));
    connect(mSearchBox, SIGNAL(textChanged(QString)), this, SLOT(searchTextChanged(QString)));
}

SearchListView::~SearchListView()
{
    delete ui;
}

void SearchListView::listKeyPressed(QKeyEvent* event)
{
    char ch = event->text().toUtf8().constData()[0];
    if(isprint(ch)) //add a char to the search box
        mSearchBox->setText(mSearchBox->text().insert(mSearchBox->cursorPosition(), QString(QChar(ch))));
    else if(event->key() == Qt::Key_Backspace) //remove a char from the search box
    {
        QString newText;
        if(event->modifiers() == Qt::ControlModifier) //clear the search box
            newText = "";
        else
        {
            newText = mSearchBox->text();
            newText.chop(1);
        }
        mSearchBox->setText(newText);
    }
    else if((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)) //user pressed enter
        emit enterPressedSignal();
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
            if(list->getCellContent(row, i).contains(text, Qt::CaseInsensitive))
                return true;
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
        mList->setFocus();
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
    mSearchList->highlightText = arg1;
    mSearchList->reloadData();
    mSearchList->setFocus();
}

void SearchListView::listContextMenu(const QPoint & pos)
{
    QMenu* wMenu = new QMenu(this);
    emit listContextMenuSignal(wMenu);
    if(!wMenu->actions().length())
        return;
    QMenu wCopyMenu("&Copy", this);
    mCurList->setupCopyMenu(&wCopyMenu);
    if(wCopyMenu.actions().length())
    {
        wMenu->addSeparator();
        wMenu->addMenu(&wCopyMenu);
    }
    wMenu->exec(mCurList->mapToGlobal(pos));
}

void SearchListView::doubleClickedSlot()
{
    emit enterPressedSignal();
}
