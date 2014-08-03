/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Pavel Strakhov
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */
#ifndef Pane_H
#define Pane_H

#include <QTabWidget>
#include <QVariantMap>

class PaneWidget;

/*!
 * \brief The Pane class is a tab widget used to store widgets.
 * It implements dragging of its tab or the whole tab widget.
 */
class Pane : public QTabWidget {
  Q_OBJECT
public:
  //! Creates new area.
  explicit Pane(PaneWidget* manager, QWidget *parent = 0);
  //! Destroys the area.
  virtual ~Pane();

  /*!
   * Add \a widget to this area.
   */
  void addWidget(QWidget* widget);

  /*!
   * Add \a widgets to this area.
   */
  void addWidgets(const QList<QWidget*>& widgets);

  /*!
   * Returns a list of all widgets in this area.
   */
  QList<QWidget*> widgets();

protected:
  //! Reimplemented from QTabWidget::mousePressEvent.
  virtual void mousePressEvent(QMouseEvent *);
  //! Reimplemented from QTabWidget::mouseReleaseEvent.
  virtual void mouseReleaseEvent(QMouseEvent *);
  //! Reimplemented from QTabWidget::mouseMoveEvent.
  virtual void mouseMoveEvent(QMouseEvent *);
  //! Reimplemented from QTabWidget::eventFilter.
  virtual bool eventFilter(QObject *object, QEvent *event);

private:
  PaneWidget* mPaneWidget;
  bool m_dragCanStart; // indicates that user has started mouse movement on QTabWidget
                       // that can be considered as dragging it if the cursor will leave
                       // its area

  bool m_tabDragCanStart; // indicates that user has started mouse movement on QTabWidget
                          // that can be considered as dragging current tab
                          // if the cursor will leave the tab bar area

  QVariantMap saveState(); // dump contents to variable
  void restoreState(const QVariantMap& data); //restore contents from given variable

  //check if mouse left tab widget area so that dragging should start
  void check_mouse_move();

  friend class PaneWidget;
  friend class PaneSerialize;

};

#endif // Pane_H
