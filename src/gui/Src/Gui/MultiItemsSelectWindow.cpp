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
    m_dataProvider(hp),
    m_showIcon(showIcon),
    m_editorList(new OpenViewsTreeWidget(this))
{
    setMinimumSize(300, 200);
    m_editorList->setColumnCount(1);
    m_editorList->header()->hide();
    m_editorList->setIndentation(0);
    m_editorList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_editorList->setTextElideMode(Qt::ElideMiddle);
    m_editorList->installEventFilter(this);

    setFrameStyle(m_editorList->frameStyle());
    m_editorList->setFrameStyle(QFrame::NoFrame);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->addWidget(m_editorList);

    connect(m_editorList, &QTreeWidget::itemClicked,
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
    selectEditor(m_editorList->currentItem());
}

void MultiItemsSelectWindow::setVisible(bool visible)
{
    QWidget::setVisible(visible);
    if(visible)
        setFocus();
}

bool MultiItemsSelectWindow::eventFilter(QObject* obj, QEvent* e)
{
    if(obj == m_editorList)
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
                selectEditor(m_editorList->currentItem());
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
    m_editorList->setFocus();
}

void MultiItemsSelectWindow::selectUpDown(bool up)
{
    int itemCount = m_editorList->topLevelItemCount();
    if(itemCount < 2)
        return;
    int index = m_editorList->indexOfTopLevelItem(m_editorList->currentItem());
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
        editor = m_editorList->topLevelItem(index);
        count++;
    }
    if(editor)
    {
        m_editorList->setCurrentItem(editor);
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
    return m_editorList->sizeHint() + QSize(frameWidth() * 2, frameWidth() * 2);
}

void MultiItemsSelectWindow::selectNextEditor()
{
    selectUpDown(true);
}

void MultiItemsSelectWindow::addItems()
{
    m_editorList->clear();
    auto & history = m_dataProvider->MIDP_getItems();
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
    m_dataProvider->MIDP_selected(index);
}

void MultiItemsSelectWindow::editorClicked(QTreeWidgetItem* item)
{
    selectEditor(item);
    setFocus();
}

void MultiItemsSelectWindow::ensureCurrentVisible()
{
    m_editorList->scrollTo(m_editorList->currentIndex(), QAbstractItemView::PositionAtCenter);
}

void MultiItemsSelectWindow::addItem(MIDPKey index)
{
    QTreeWidgetItem* item = new QTreeWidgetItem();

    if(m_showIcon)
        item->setIcon(0, m_dataProvider->MIDP_getIcon(index));
    item->setText(0, m_dataProvider->MIDP_getItemName(index));
    item->setData(0, int(Role::ItemData), QVariant::fromValue(index));
    item->setTextAlignment(0, Qt::AlignLeft);

    m_editorList->addTopLevelItem(item);

    if(m_editorList->topLevelItemCount() == 1)
        m_editorList->setCurrentItem(item);
}
