#ifndef __MHTABWIDGET_H__
#define __MHTABWIDGET_H__

// Qt includes
#include <QWidget>
#include <QTabWidget>
#include <QMainWindow>
#include "TabBar.h"

// Qt forward class definitions
class MHTabBar;

//////////////////////////////////////////////////////////////////////////////
// Summary:
//    MHTabWidget implements the a Tab Widget with detach and attach
//    functionality for MHTabBar.
//////////////////////////////////////////////////////////////////////////////
class MHTabWidget: public QTabWidget
{
    Q_OBJECT

public:
    MHTabWidget(QWidget* parent = nullptr, bool allowDetach = true, bool allowDelete = false);
    virtual ~MHTabWidget(void);

    QWidget* widget(int index) const;
    int count() const;
    QList<QWidget*> windows();

    int addTabEx(QWidget* widget, const QIcon & icon, const QString & label, const QString & nativeName);
    QString getNativeName(int index);
    void showPreviousTab();
    void showNextTab();
    void deleteCurrentTab();
signals:
    void tabMovedTabWidget(int from, int to);

public slots:
    void AttachTab(QWidget* parent);
    void DetachTab(int index, QPoint &);
    void MoveTab(int fromIndex, int toIndex);
    void DeleteTab(int index);
    void tabMoved(int from, int to);

public Q_SLOTS:
    void setCurrentIndex(int index);

protected:
    MHTabBar* tabBar() const;

private:
    MHTabBar* m_tabBar;

    QList<QWidget*> m_Windows;
    QList<QString> mNativeNames;
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
    MHDetachedWindow(QWidget* parent = 0, MHTabWidget* tabwidget = 0);
    ~MHDetachedWindow(void);
    QString mNativeName;

protected:
    MHTabWidget* m_TabWidget;

    void closeEvent(QCloseEvent* event);

signals:
    void OnClose(QWidget* widget);
};

#endif // __MHTABWIDGET_H__

