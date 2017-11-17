#include "MultiItemsSelectWindow.h"

#define QTC_ASSERT(cond, action) if (cond) {} else { action; } do {} while (0)

#include <QFocusEvent>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QScrollBar>
#include <QApplication>

enum class Role
{
    ItemData = Qt::UserRole
};

MultiItemsSelectWindow::MultiItemsSelectWindow(MultiItemsDataProvider* hp, QWidget* parent, bool showIcon) :
    QFrame(parent, Qt::Popup),
    mDataProvider(hp),
    mShowIcon(showIcon),
    mEditorList(new OpenViewsTreeWidget(this))
{
    setMinimumSize(300, 200);
    mEditorList->setColumnCount(1);
    mEditorList->header()->hide();
    mEditorList->setIndentation(0);
    mEditorList->setSelectionMode(QAbstractItemView::SingleSelection);
    mEditorList->setTextElideMode(Qt::ElideMiddle);
    mEditorList->installEventFilter(this);

    setFrameStyle(mEditorList->frameStyle());
    mEditorList->setFrameStyle(QFrame::NoFrame);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->addWidget(mEditorList);

    connect(mEditorList, &QTreeWidget::itemClicked,
            this, &MultiItemsSelectWindow::editorClicked);

    setVisible(false);
}

void MultiItemsSelectWindow::gotoNextItem()
{
    if(isVisible())
    {
        selectPreviousEditor();
    }
    else
    {
        addItems();
        selectPreviousEditor();
        showPopupOrSelectDocument();
    }
}

void MultiItemsSelectWindow::gotoPreviousItem()
{
    if(isVisible())
    {
        selectNextEditor();
    }
    else
    {
        addItems();
        selectNextEditor();
        showPopupOrSelectDocument();
    }
}

void MultiItemsSelectWindow::showPopupOrSelectDocument()
{
    if(QApplication::keyboardModifiers() == Qt::NoModifier)
    {
        selectAndHide();
    }
    else
    {
        QWidget* activeWindow = QApplication::activeWindow();
        if(!activeWindow)
            return;

        QWidget* referenceWidget = activeWindow;
        const QPoint p = referenceWidget->mapToGlobal(QPoint(0, 0));

        this->setMaximumSize(qMax(this->minimumWidth(), referenceWidget->width() / 2),
                             qMax(this->minimumHeight(), referenceWidget->height() / 2));
        this->adjustSize();
        this->move((referenceWidget->width() - this->width()) / 2 + p.x(),
                   (referenceWidget->height() - this->height()) / 2 + p.y());
        this->setVisible(true);
    }
}

void MultiItemsSelectWindow::selectAndHide()
{
    setVisible(false);
    selectEditor(mEditorList->currentItem());
}

void MultiItemsSelectWindow::setVisible(bool visible)
{
    QWidget::setVisible(visible);
    if(visible)
        setFocus();
}

bool MultiItemsSelectWindow::eventFilter(QObject* obj, QEvent* e)
{
    if(obj == mEditorList)
    {
        if(e->type() == QEvent::KeyPress)
        {
            QKeyEvent* ke = static_cast<QKeyEvent*>(e);
            if(ke->key() == Qt::Key_Escape)
            {
                setVisible(false);
                return true;
            }
            if(ke->key() == Qt::Key_Return
                    || ke->key() == Qt::Key_Enter)
            {
                selectEditor(mEditorList->currentItem());
                return true;
            }
        }
        else if(e->type() == QEvent::KeyRelease)
        {
            QKeyEvent* ke = static_cast<QKeyEvent*>(e);
            if(ke->modifiers() == 0
                    /*HACK this is to overcome some event inconsistencies between platforms*/
                    || (ke->modifiers() == Qt::AltModifier
                        && (ke->key() == Qt::Key_Alt || ke->key() == -1)))
            {
                selectAndHide();
            }
        }
    }
    return QWidget::eventFilter(obj, e);
}

void MultiItemsSelectWindow::focusInEvent(QFocusEvent*)
{
    mEditorList->setFocus();
}

void MultiItemsSelectWindow::selectUpDown(bool up)
{
    int itemCount = mEditorList->topLevelItemCount();
    if(itemCount < 2)
        return;
    int index = mEditorList->indexOfTopLevelItem(mEditorList->currentItem());
    if(index < 0)
        return;
    QTreeWidgetItem* editor = 0;
    int count = 0;
    while(!editor && count < itemCount)
    {
        if(up)
        {
            index--;
            if(index < 0)
                index = itemCount - 1;
        }
        else
        {
            index++;
            if(index >= itemCount)
                index = 0;
        }
        editor = mEditorList->topLevelItem(index);
        count++;
    }
    if(editor)
    {
        mEditorList->setCurrentItem(editor);
        ensureCurrentVisible();
    }
}

void MultiItemsSelectWindow::selectPreviousEditor()
{
    selectUpDown(false);
}

QSize MultiItemsSelectWindow::OpenViewsTreeWidget::sizeHint() const
{
    return QSize(sizeHintForColumn(0) + verticalScrollBar()->width() + frameWidth() * 2,
                 viewportSizeHint().height() + frameWidth() * 2);
}

QSize MultiItemsSelectWindow::sizeHint() const
{
    return mEditorList->sizeHint() + QSize(frameWidth() * 2, frameWidth() * 2);
}

void MultiItemsSelectWindow::selectNextEditor()
{
    selectUpDown(true);
}

void MultiItemsSelectWindow::addItems()
{
    mEditorList->clear();
    auto & history = mDataProvider->MIDP_getItems();
    for(auto & i : history)
    {
        addItem(i);
    }
}

void MultiItemsSelectWindow::selectEditor(QTreeWidgetItem* item)
{
    if(!item)
        return;
    auto index = item->data(0, int(Role::ItemData)).value<MIDPKey>();
    mDataProvider->MIDP_selected(index);
}

void MultiItemsSelectWindow::editorClicked(QTreeWidgetItem* item)
{
    selectEditor(item);
    setFocus();
}

void MultiItemsSelectWindow::ensureCurrentVisible()
{
    mEditorList->scrollTo(mEditorList->currentIndex(), QAbstractItemView::PositionAtCenter);
}

void MultiItemsSelectWindow::addItem(MIDPKey index)
{
    QTreeWidgetItem* item = new QTreeWidgetItem();

    if(mShowIcon)
        item->setIcon(0, mDataProvider->MIDP_getIcon(index));
    item->setText(0, mDataProvider->MIDP_getItemName(index));
    item->setData(0, int(Role::ItemData), QVariant::fromValue(index));
    item->setTextAlignment(0, Qt::AlignLeft);

    mEditorList->addTopLevelItem(item);

    if(mEditorList->topLevelItemCount() == 1)
        mEditorList->setCurrentItem(item);
}
