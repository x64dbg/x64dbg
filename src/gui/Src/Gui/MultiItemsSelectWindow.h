#ifndef OPENVIEWSWINDOW_H
#define OPENVIEWSWINDOW_H

// stolen from http://code.qt.io/cgit/qt-creator/qt-creator.git/tree/src/plugins/coreplugin/editormanager/openeditorswindow.h

#include <QFrame>
#include <QIcon>
#include <QList>
#include <QTreeWidget>
#include <functional>
#include <QVector>
#include <QPair>

QT_BEGIN_NAMESPACE
class QTreeWidgetItem;
class QWidget;
QT_END_NAMESPACE

typedef void* MIDPKey;

class MultiItemsDataProvider
{
public:
    virtual QList<MIDPKey> MIDP_getItems() = 0;
    virtual QString MIDP_getItemName(MIDPKey index) = 0;
    virtual QIcon MIDP_getIcon(MIDPKey index) = 0;
    virtual void MIDP_selected(MIDPKey index) = 0;
};

class MultiItemsSelectWindow : public QFrame
{
    Q_OBJECT
public:
    MultiItemsSelectWindow(MultiItemsDataProvider* hp, QWidget* parent, bool showIcon, std::function<void(MultiItemsSelectWindow*)> = nullptr);

    void gotoNextItem(bool autoNextWhenInit = true);
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
        explicit OpenViewsTreeWidget(QWidget* parent = 0) : QTreeWidget(parent) {}
        ~OpenViewsTreeWidget() {}
        QSize sizeHint() const;
    };

    OpenViewsTreeWidget* mEditorList;
    MultiItemsDataProvider* mDataProvider = nullptr;
    bool mShowIcon;
};

class FollowInDataProxy : public QObject, public MultiItemsDataProvider
{
    Q_OBJECT
public:
    typedef std::function<void(int, QVector<QPair<QString, QString>>&)> DataCallback;
    FollowInDataProxy(QWidget* parent, DataCallback cb);

protected:
    QList<MIDPKey> MIDP_getItems() override;
    QString MIDP_getItemName(MIDPKey index) override;
    void MIDP_selected(MIDPKey index) override;
    QIcon MIDP_getIcon(MIDPKey index) override;

private:
    DataCallback mDataCallback;
    MultiItemsSelectWindow* mFollowInPopupWindow = nullptr;
    int mFollowInTarget = 0; // 0: GUI_DISASSEMBLY, 1: GUI_DUMP
    QVector<QPair<QString, QString>> mFollowToData; // QPair<show name, command>
};

#endif
