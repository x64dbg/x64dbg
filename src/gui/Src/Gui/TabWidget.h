#ifndef __MHTABWIDGET_H__
#define __MHTABWIDGET_H__

// Qt includes
#include <QWidget>
#include <QTabWidget>
#include <QMainWindow>
#include "TabBar.h"
#include "MultiItemsSelectWindow.h"

// Qt forward class definitions
class MHTabBar;

//////////////////////////////////////////////////////////////////////////////
// Summary:
//    MHTabWidget implements the a Tab Widget with detach and attach
//    functionality for MHTabBar.
//////////////////////////////////////////////////////////////////////////////
class MHTabWidget: public QTabWidget, public MultiItemsDataProvider
{
    Q_OBJECT

public:
    MHTabWidget(QWidget* parent = nullptr, bool allowDetach = true, bool allowDelete = false);
    virtual ~MHTabWidget();

    QWidget* widget(int index) const;
    int count() const;
    QList<QWidget*> windows();

    int addTabEx(QWidget* widget, const QIcon & icon, const QString & label, const QString & nativeName);
    QString getNativeName(int index);

signals:
    void tabMovedTabWidget(int from, int to);

public slots:
    void AttachTab(QWidget* parent);
    void DetachTab(int index, const QPoint &);
    void MoveTab(int fromIndex, int toIndex);
    void DeleteTab(int index);
    void tabMoved(int from, int to);
    void OnDetachFocused(QWidget* parent);
    void currentChanged(int index);
    void setCurrentIndex(int index);
    void showPreviousTab();
    void showNextTab();
    void showPreviousView();
    void showNextView();
    void deleteCurrentTab();

protected:
    MHTabBar* tabBar() const;
    const QList<MIDPKey> & MIDP_getItems() const override;
    QString MIDP_getItemName(MIDPKey index) override;
    void MIDP_selected(MIDPKey index) override;
    QIcon MIDP_getIcon(MIDPKey index) override;
    void setLatestFocused(QWidget* w);

private:
    MHTabBar* mTabBar;
    QList<QWidget*> mWindows;
    QList<QString> mNativeNames;
    MultiItemsSelectWindow* mHistoryPopup;
    QList<MIDPKey> mHistory;
};

//////////////////////////////////////////////////////////////////////////////
// Summary:
//    MHDetachedWindow implements the WindowContainer for the Detached Widget
//
// Conditions:
//    Header : MHTabWidget.h
//////////////////////////////////////////////////////////////////////////////
class MHDetachedWindow : public QMainWindow
{
    Q_OBJECT

public:
    MHDetachedWindow(QWidget* parent = 0);
    ~MHDetachedWindow();

    QString mNativeName;

signals:
    void OnClose(QWidget* widget);
    void OnFocused(QWidget* widget);

protected:
    void closeEvent(QCloseEvent* event);
    bool event(QEvent* event);
};

#endif // __MHTABWIDGET_H__

