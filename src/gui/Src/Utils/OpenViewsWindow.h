#ifndef OPENVIEWSWINDOW_H
#define OPENVIEWSWINDOW_H
// stolen from http://code.qt.io/cgit/qt-creator/qt-creator.git/tree/src/plugins/coreplugin/editormanager/openeditorswindow.h

#include <QFrame>
#include <QIcon>
#include <QList>
#include <QTreeWidget>

QT_BEGIN_NAMESPACE
class QTreeWidgetItem;
QT_END_NAMESPACE

class HistoryProvider
{
public:
    virtual const QList<int>& getItems() const = 0;
    virtual QString getName(int index) = 0;
    virtual void selected(int index) = 0;
};

class OpenViewsWindow : public QFrame
{
    Q_OBJECT
public:
    explicit OpenViewsWindow(HistoryProvider* hp, QWidget *parent = 0);

    void gotoNextHistory();
    void gotoPreviousHistory();

private slots:
    void selectAndHide();

private:
    void addItems();

    bool eventFilter(QObject *src, QEvent *e);
    void focusInEvent(QFocusEvent *);

    void setVisible(bool visible);
    void selectNextEditor();
    void selectPreviousEditor();
    QSize sizeHint() const;
    void showPopupOrSelectDocument();

    void editorClicked(QTreeWidgetItem *item);
    void selectEditor(QTreeWidgetItem *item);

    void addItem(const QString& title, int index);
    void ensureCurrentVisible();
    void selectUpDown(bool up);

    class OpenViewsTreeWidget : public QTreeWidget {
    public:
        explicit OpenViewsTreeWidget(QWidget *parent = 0) : QTreeWidget(parent) {}
        ~OpenViewsTreeWidget() {}
        QSize sizeHint() const;
    };

    //const QIcon m_emptyIcon;
    OpenViewsTreeWidget *m_editorList;

    HistoryProvider* hp_ = nullptr;
};

#endif
