#pragma once

// stolen from http://code.qt.io/cgit/qt-creator/qt-creator.git/tree/src/plugins/coreplugin/editormanager/openeditorswindow.h

#include <QFrame>
#include <QIcon>
#include <QList>
#include <QTreeWidget>

QT_BEGIN_NAMESPACE
class QTreeWidgetItem;
class QWidget;
QT_END_NAMESPACE

typedef void* MIDPKey;

class MultiItemsDataProvider
{
public:
    virtual const QList<MIDPKey> & MIDP_getItems() const = 0;
    virtual QString MIDP_getItemName(MIDPKey index) = 0;
    virtual QIcon MIDP_getIcon(MIDPKey index) = 0;
    virtual void MIDP_selected(MIDPKey index) = 0;
};

class MultiItemsSelectWindow : public QFrame
{
    Q_OBJECT
public:
    MultiItemsSelectWindow(MultiItemsDataProvider* hp, QWidget* parent, bool showIcon);

    void gotoNextItem();
    void gotoPreviousItem();

private slots:
    void selectAndHide();

private:
    void addItems();

    bool eventFilter(QObject* src, QEvent* e);
    void focusInEvent(QFocusEvent*);

    void setVisible(bool visible);
    void selectNextEditor();
    void selectPreviousEditor();
    QSize sizeHint() const;
    void showPopupOrSelectDocument();

    void editorClicked(QTreeWidgetItem* item);
    void selectEditor(QTreeWidgetItem* item);

    void addItem(MIDPKey index);
    void ensureCurrentVisible();
    void selectUpDown(bool up);

    class OpenViewsTreeWidget : public QTreeWidget
    {
    public:
        explicit OpenViewsTreeWidget(QWidget* parent = nullptr) : QTreeWidget(parent) {}
        ~OpenViewsTreeWidget() {}
        QSize sizeHint() const;
    };

    OpenViewsTreeWidget* mEditorList;
    MultiItemsDataProvider* mDataProvider = nullptr;
    bool mShowIcon;
};
