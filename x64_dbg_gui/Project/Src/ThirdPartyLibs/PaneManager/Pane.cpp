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
#include "Pane.h"
#include "PaneWidget.h"
#include <QApplication>
#include <QMouseEvent>
#include <QDebug>

Pane::Pane(PaneWidget *manager, QWidget *parent) : QTabWidget(parent), mPaneWidget(manager), m_dragCanStart(false), m_tabDragCanStart(false)
{
  setMovable(true);
  setTabsClosable(true);
  setDocumentMode(true);
  tabBar()->installEventFilter(this);
  mPaneWidget->mPanes << this;
}

Pane::~Pane() {
  mPaneWidget->mPanes.removeOne(this);
}

void Pane::addWidget(QWidget *widget) {
  setCurrentIndex(addTab(widget, widget->windowIcon(), widget->windowTitle()));
  mPaneWidget->m_lastUsedArea = this;
}

void Pane::addWidgets(const QList<QWidget *> &widgets) {
  foreach(QWidget* widget, widgets)
      addWidget(widget);
}

QList<QWidget *> Pane::widgets() {
  QList<QWidget *> result;
  const int max_i = count();
  for(int i = 0; i < max_i; i++)
    result.append(widget(i));
  return result;
}

void Pane::mousePressEvent(QMouseEvent *e) {
  if (e->button() == Qt::LeftButton)
    m_dragCanStart = true;
}

void Pane::mouseReleaseEvent(QMouseEvent *) {
  m_dragCanStart = false;
  mPaneWidget->updateDragPosition();
}

void Pane::mouseMoveEvent(QMouseEvent *) {
  check_mouse_move();
}

bool Pane::eventFilter(QObject *object, QEvent *event) {
  if (object == tabBar()) {
    if (event->type() == QEvent::MouseButtonPress &&
        qApp->mouseButtons() == Qt::LeftButton) {
      // can start tab drag only if mouse is at some tab, not at empty tabbar space
      if (tabBar()->tabAt(static_cast<QMouseEvent*>(event)->pos()) >= 0 ) {
        m_tabDragCanStart = true;
      } else {
        m_dragCanStart = true;
      }

    } else if (event->type() == QEvent::MouseButtonRelease) {
      m_tabDragCanStart = false;
      m_dragCanStart = false;
      mPaneWidget->updateDragPosition();
    } else if (event->type() == QEvent::MouseMove) {
      mPaneWidget->updateDragPosition();
      if (m_tabDragCanStart) {
        if (tabBar()->rect().contains(static_cast<QMouseEvent*>(event)->pos())) {
          return false;
        }
        if (qApp->mouseButtons() != Qt::LeftButton) {
          return false;
        }
        QWidget* widget = currentWidget();
        if (!widget || !mPaneWidget->m_widgets.contains(widget)) {
          return false;
        }
        m_tabDragCanStart = false;
        //stop internal tab drag in QTabBar
        QMouseEvent* releaseEvent = new QMouseEvent(QEvent::MouseButtonRelease,
                                                    static_cast<QMouseEvent*>(event)->pos(),
                                                    Qt::LeftButton, Qt::LeftButton, 0);
        qApp->sendEvent(tabBar(), releaseEvent);
        mPaneWidget->startDrag(QList<QWidget*>() << widget);
      } else if (m_dragCanStart) {
        check_mouse_move();
      }
    }
  }
  return QTabWidget::eventFilter(object, event);
}

QVariantMap Pane::saveState() {
  QVariantMap result;
  result["type"] = "area";
  result["currentIndex"] = currentIndex();
  QStringList objectNames;
  for(int i = 0; i < count(); i++) {
    QString name = widget(i)->objectName();
    if (name.isEmpty()) {
      qWarning("cannot save state of widget without object name");
    } else {
      objectNames << name;
    }
  }
  result["objectNames"] = objectNames;
  return result;
}

void Pane::restoreState(const QVariantMap &data) {
  foreach(QVariant objectNameValue, data["objectNames"].toList()) {
    QString objectName = objectNameValue.toString();
    if (objectName.isEmpty()) { continue; }
    bool found = false;
    foreach(QWidget* widget, mPaneWidget->m_widgets) {
      if (widget->objectName() == objectName) {
        addWidget(widget);
        found = true;
        break;
      }
    }
    if (!found) {
      qWarning("widget with name '%s' not found", objectName.toLocal8Bit().constData());
    }
  }
  setCurrentIndex(data["currentIndex"].toInt());
}

void Pane::check_mouse_move() {
  mPaneWidget->updateDragPosition();
  if (qApp->mouseButtons() == Qt::LeftButton && !rect().contains(mapFromGlobal(QCursor::pos())) && m_dragCanStart) {
    m_dragCanStart = false;
    QList<QWidget*> widgets;
    for(int i = 0; i < count(); i++) {
      QWidget* currentWidget = widget(i);
      if (!mPaneWidget->m_widgets.contains(currentWidget)) {
        qWarning("tab widget contains unmanaged widget");
      } else {
        widgets << currentWidget;
      }
    }
    mPaneWidget->startDrag(widgets);
  }
}
