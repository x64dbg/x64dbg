#ifndef __MHTABWIDGET_H__
#define __MHTABWIDGET_H__

// Qt includes
#include <qdialog.h>
#include <qtabwidget.h>
#include <qtextedit.h>
#include <qwidget.h>

// Qt forward class definitions
class QTabWidget;
class QHBoxLayout;
class QMainWindow;
class QVBoxLayout;
class MHWorkflowWidget;
class MHTabBar;

//////////////////////////////////////////////////////////////////////////////
// Summary:
//    MHTabWidget implements the a Tab Widget with detach and attach
//    functionality.
//////////////////////////////////////////////////////////////////////////////
class MHTabWidget: public QTabWidget
{
  Q_OBJECT

public:
  void Initialize (QMainWindow* mainWindow);
  void ShutDown   (void);
  
  // Construction.
  MHTabWidget (QWidget *parent);

  // Destruction.
  virtual ~MHTabWidget (void);

public slots:
  // Move Tab
  void MoveTab(int fromIndex, int toIndex);

  // Detach Tab
  void DetachTab (int index, QPoint&);

  // Attach Tab
  void AttachTab (QWidget *parent);

protected:
  

private:
  MHTabBar* m_tabBar;
};

//////////////////////////////////////////////////////////////////////////////
// Summary:
//    MHWorkflowWidget implements Widget with Detach functionality.
//
// Conditions:
//    Header : MHTabWidget.h
//    Library: MassHybridGui.dll
//////////////////////////////////////////////////////////////////////////////
class MHWorkflowWidget : public QWidget
{
public:
  // Default constructor
  MHWorkflowWidget(QWidget *parent = 0);
  // Default destructor
  ~MHWorkflowWidget(void);
};

//////////////////////////////////////////////////////////////////////////////
// Summary:
//    MHDetachedWindow implements the WindowContainer for the Detached Widget
//
// Conditions:
//    Header : MHTabWidget.h
//    Library: MassHybridGui.dll
//////////////////////////////////////////////////////////////////////////////
class MHDetachedWindow : public QDialog
{
  Q_OBJECT
public:
  // Default constructor
  MHDetachedWindow(QWidget *parent = 0);
  // Default destructor
  ~MHDetachedWindow(void);

protected:
  void closeEvent(QCloseEvent *event);
signals:
  void OnClose (QWidget* widget);

};


#endif // __MHTABWIDGET_H__

