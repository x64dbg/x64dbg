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
    connect(mRegexCheckbox, SIGNAL(toggled(bool)), this, SLOT(on_checkBoxRegex_toggled(bool)));
    connect(mLockCheckbox, SIGNAL(toggled(bool)), this, SLOT(on_checkBoxLock_toggled(bool)));

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
            if(mRegexCheckbox->checkState() == Qt::Checked)
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
    mCurList->setSingleSelection(0);
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
    if(mRegexCheckbox->checkState() != Qt::Checked)
        mSearchList->highlightText = arg1;
    else
        mSearchList->highlightText = "";

    mSearchList->reloadData();
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

void SearchListView::on_checkBoxRegex_toggled(bool checked)
{
    Q_UNUSED(checked);
    refreshSearchList();
}

void SearchListView::on_checkBoxLock_toggled(bool checked)
{
    mSearchBox->setDisabled(checked);
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
