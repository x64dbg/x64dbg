#include "OpenViewsWindow.h"

#define QTC_ASSERT(cond, action) if (cond) {} else { action; } do {} while (0)

#include <QFocusEvent>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QScrollBar>
#include <QApplication>

enum class Role
{
    Entry = Qt::UserRole,
    View = Qt::UserRole + 1
};

OpenViewsWindow::OpenViewsWindow(HistoryProvider* hp, QWidget *parent) :
    QFrame(parent, Qt::Popup),
    hp_(hp),
    //m_emptyIcon(Utils::Icons::EMPTY14.icon()),
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

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->addWidget(m_editorList);

    connect(m_editorList, &QTreeWidget::itemClicked,
            this, &OpenViewsWindow::editorClicked);

    setVisible(false);
}

void OpenViewsWindow::gotoNextHistory()
{
    if (isVisible()) {
        selectPreviousEditor();
    } else {
        addItems();
        selectPreviousEditor();
        showPopupOrSelectDocument();
    }
}

void OpenViewsWindow::gotoPreviousHistory()
{
    if (isVisible()) {
        selectNextEditor();
    } else {
        addItems();
        selectNextEditor();
        showPopupOrSelectDocument();
    }
}

void OpenViewsWindow::showPopupOrSelectDocument()
{
    if (QApplication::keyboardModifiers() == Qt::NoModifier) {
        selectAndHide();
    } else {
        QWidget *activeWindow = QApplication::activeWindow();
        if(!activeWindow)
            return;

        QWidget *referenceWidget = activeWindow;
        const QPoint p = referenceWidget->mapToGlobal(QPoint(0, 0));

        this->setMaximumSize(qMax(this->minimumWidth(), referenceWidget->width() / 2),
                              qMax(this->minimumHeight(), referenceWidget->height() / 2));
        this->adjustSize();
        this->move((referenceWidget->width() - this->width()) / 2 + p.x(),
                    (referenceWidget->height() - this->height()) / 2 + p.y());
        this->setVisible(true);
    }
}

void OpenViewsWindow::selectAndHide()
{
    setVisible(false);
    selectEditor(m_editorList->currentItem());
}

void OpenViewsWindow::setVisible(bool visible)
{
    QWidget::setVisible(visible);
    if (visible)
        setFocus();
}

bool OpenViewsWindow::eventFilter(QObject *obj, QEvent *e)
{
    if (obj == m_editorList) {
        if (e->type() == QEvent::KeyPress) {
            QKeyEvent *ke = static_cast<QKeyEvent*>(e);
            if (ke->key() == Qt::Key_Escape) {
                setVisible(false);
                return true;
            }
            if (ke->key() == Qt::Key_Return
                    || ke->key() == Qt::Key_Enter) {
                selectEditor(m_editorList->currentItem());
                return true;
            }
        } else if (e->type() == QEvent::KeyRelease) {
            QKeyEvent *ke = static_cast<QKeyEvent*>(e);
            if (ke->modifiers() == 0
                    /*HACK this is to overcome some event inconsistencies between platforms*/
                    || (ke->modifiers() == Qt::AltModifier
                    && (ke->key() == Qt::Key_Alt || ke->key() == -1))) {
                selectAndHide();
            }
        }
    }
    return QWidget::eventFilter(obj, e);
}

void OpenViewsWindow::focusInEvent(QFocusEvent *)
{
    m_editorList->setFocus();
}

void OpenViewsWindow::selectUpDown(bool up)
{
    int itemCount = m_editorList->topLevelItemCount();
    if (itemCount < 2)
        return;
    int index = m_editorList->indexOfTopLevelItem(m_editorList->currentItem());
    if (index < 0)
        return;
    QTreeWidgetItem *editor = 0;
    int count = 0;
    while (!editor && count < itemCount) {
        if (up) {
            index--;
            if (index < 0)
                index = itemCount-1;
        } else {
            index++;
            if (index >= itemCount)
                index = 0;
        }
        editor = m_editorList->topLevelItem(index);
        count++;
    }
    if (editor) {
        m_editorList->setCurrentItem(editor);
        ensureCurrentVisible();
    }
}

void OpenViewsWindow::selectPreviousEditor()
{
    selectUpDown(false);
}

QSize OpenViewsWindow::OpenViewsTreeWidget::sizeHint() const
{
    return QSize(sizeHintForColumn(0) + verticalScrollBar()->width() + frameWidth() * 2,
                 viewportSizeHint().height() + frameWidth() * 2);
}

QSize OpenViewsWindow::sizeHint() const
{
    return m_editorList->sizeHint() + QSize(frameWidth() * 2, frameWidth() * 2);
}

void OpenViewsWindow::selectNextEditor()
{
    selectUpDown(true);
}

void OpenViewsWindow::addItems()
{
    m_editorList->clear();
    auto& history = hp_->getItems();
    for(auto& i: history)
    {
        addItem(hp_->getName(i), i);
    }
}

void OpenViewsWindow::selectEditor(QTreeWidgetItem *item)
{
    if (!item)
        return;
    auto entry = item->data(0, int(Role::Entry)).value<HPKey>();
    if(hp_)
    {
        hp_->selected(entry);
    }
}

void OpenViewsWindow::editorClicked(QTreeWidgetItem *item)
{
    selectEditor(item);
    setFocus();
}

void OpenViewsWindow::ensureCurrentVisible()
{
    m_editorList->scrollTo(m_editorList->currentIndex(), QAbstractItemView::PositionAtCenter);
}

void OpenViewsWindow::addItem(const QString& title, HPKey index)
{
    QTC_ASSERT(!title.isEmpty(), return);
    QTreeWidgetItem *item = new QTreeWidgetItem();

    /*item->setIcon(0, !entry->fileName().isEmpty() && entry->document->isFileReadOnly()
                  ? DocumentModel::lockedIcon() : m_emptyIcon);*/
    item->setText(0, title);
    //item->setToolTip(0, entry->fileName().toString());
    item->setData(0, int(Role::Entry), QVariant::fromValue(index));
    //item->setData(0, int(Role::View), QVariant::fromValue(view));
    item->setTextAlignment(0, Qt::AlignLeft);

    m_editorList->addTopLevelItem(item);

    if (m_editorList->topLevelItemCount() == 1)
        m_editorList->setCurrentItem(item);
}
