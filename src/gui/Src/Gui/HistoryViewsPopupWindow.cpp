#include "HistoryViewsPopupWindow.h"

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

HistoryViewsPopupWindow::HistoryViewsPopupWindow(HistoryProvider* hp, QWidget* parent) :
    QFrame(parent, Qt::Popup),
    hp_(hp),
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
            this, &HistoryViewsPopupWindow::editorClicked);

    setVisible(false);
}

void HistoryViewsPopupWindow::gotoNextHistory()
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

void HistoryViewsPopupWindow::gotoPreviousHistory()
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

void HistoryViewsPopupWindow::showPopupOrSelectDocument()
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

void HistoryViewsPopupWindow::selectAndHide()
{
    setVisible(false);
    selectEditor(m_editorList->currentItem());
}

void HistoryViewsPopupWindow::setVisible(bool visible)
{
    QWidget::setVisible(visible);
    if(visible)
        setFocus();
}

bool HistoryViewsPopupWindow::eventFilter(QObject* obj, QEvent* e)
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

void HistoryViewsPopupWindow::focusInEvent(QFocusEvent*)
{
    m_editorList->setFocus();
}

void HistoryViewsPopupWindow::selectUpDown(bool up)
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

void HistoryViewsPopupWindow::selectPreviousEditor()
{
    selectUpDown(false);
}

QSize HistoryViewsPopupWindow::OpenViewsTreeWidget::sizeHint() const
{
    return QSize(sizeHintForColumn(0) + verticalScrollBar()->width() + frameWidth() * 2,
                 viewportSizeHint().height() + frameWidth() * 2);
}

QSize HistoryViewsPopupWindow::sizeHint() const
{
    return m_editorList->sizeHint() + QSize(frameWidth() * 2, frameWidth() * 2);
}

void HistoryViewsPopupWindow::selectNextEditor()
{
    selectUpDown(true);
}

void HistoryViewsPopupWindow::addItems()
{
    m_editorList->clear();
    auto & history = hp_->HP_getItems();
    for(auto & i : history)
    {
        addItem(hp_->HP_getName(i), i, hp_->HP_getIcon(i));
    }
}

void HistoryViewsPopupWindow::selectEditor(QTreeWidgetItem* item)
{
    if(!item)
        return;
    auto index = item->data(0, int(Role::ItemData)).value<HPKey>();
    hp_->HP_selected(index);
}

void HistoryViewsPopupWindow::editorClicked(QTreeWidgetItem* item)
{
    selectEditor(item);
    setFocus();
}

void HistoryViewsPopupWindow::ensureCurrentVisible()
{
    m_editorList->scrollTo(m_editorList->currentIndex(), QAbstractItemView::PositionAtCenter);
}

void HistoryViewsPopupWindow::addItem(const QString & title, HPKey index, const QIcon & icon)
{
    QTC_ASSERT(!title.isEmpty(), return);
    QTreeWidgetItem* item = new QTreeWidgetItem();

    item->setIcon(0, icon);
    item->setText(0, title);
    item->setData(0, int(Role::ItemData), QVariant::fromValue(index));
    item->setTextAlignment(0, Qt::AlignLeft);

    m_editorList->addTopLevelItem(item);

    if(m_editorList->topLevelItemCount() == 1)
        m_editorList->setCurrentItem(item);
}
