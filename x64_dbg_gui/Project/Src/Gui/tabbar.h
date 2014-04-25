#ifndef __MHTABBAR_H__
#define __MHTABBAR_H__

// Qt includes
#include <QTabBar>

// Qt forward class definitions
class MHTabBar;
class QMainWindow;

//////////////////////////////////////////////////////////////////////////////
// Summary:
//    MHTabBar implements the a Tab Bar with tear-off functionality.
//////////////////////////////////////////////////////////////////////////////
class MHTabBar: public QTabBar
{
Q_OBJECT
public:
  MHTabBar (QWidget* parent);
  ~MHTabBar (void);

protected:
  void mousePressEvent(QMouseEvent* event);
  void mouseMoveEvent(QMouseEvent* event);
  void dragEnterEvent(QDragEnterEvent* event);
  void dragMoveEvent(QDragMoveEvent* event);
  void dropEvent(QDropEvent* event);

signals:
  // Detach Tab
  void OnDetachTab (int index, QPoint& dropPoint);
  // Move Tab
  void OnMoveTab (int fromIndex, int toIndex);

private:
  QPoint       m_dragStartPos;
  QPoint       m_dragMovedPos;
  QPoint       m_dragDroppedPos;
  bool         m_dragInitiated;
  int          m_dragCurrentIndex;
};

#endif // __MHTABBAR_H__
