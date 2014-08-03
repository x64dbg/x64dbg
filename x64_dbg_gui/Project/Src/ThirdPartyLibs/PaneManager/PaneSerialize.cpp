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
#include "PaneSerialize.h"
#include "PaneWidget.h"
#include "Pane.h"
#include <QVBoxLayout>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QDebug>
#include <QApplication>

PaneSerialize::PaneSerialize(PaneWidget *manager) :
  QWidget(manager)
, mPaneWidget(manager)
{
  setWindowFlags(windowFlags() | Qt::Tool);
  setWindowTitle(" ");

  QVBoxLayout* mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mPaneWidget->m_wrappers << this;
}

PaneSerialize::~PaneSerialize() {
  mPaneWidget->m_wrappers.removeOne(this);
}

void PaneSerialize::closeEvent(QCloseEvent *) {
  QList<QWidget*> widgets;
  foreach(Pane* tabWidget, findChildren<Pane*>()) {
    widgets << tabWidget->widgets();
  }
  mPaneWidget->moveWidgets(widgets, PaneWidget::NoArea);
}

QVariantMap PaneSerialize::saveState() {
  if (layout()->count() > 1) {
    qWarning("too many children for wrapper");
    return QVariantMap();
  }
  if (isWindow() && layout()->count() == 0) {
    qWarning("empty top level wrapper");
    return QVariantMap();
  }
  QVariantMap result;
  result["geometry"] = saveGeometry();
  QSplitter* splitter = findChild<QSplitter*>();
  if (splitter) {
    result["splitter"] = mPaneWidget->saveSplitterState(splitter);
  } else {
    Pane* area = findChild<Pane*>();
    if (area) {
      result["area"] = area->saveState();
    } else if (layout()->count() > 0) {
      qWarning("unknown child");
      return QVariantMap();
    }
  }
  return result;
}

void PaneSerialize::restoreState(const QVariantMap &data) {
  restoreGeometry(data["geometry"].toByteArray());
  if (layout()->count() > 0) {
    qWarning("wrapper is not empty");
    return;
  }
  if (data.contains("splitter")) {
    layout()->addWidget(mPaneWidget->restoreSplitterState(data["splitter"].toMap()));
  } else if (data.contains("area")) {
    Pane* area = mPaneWidget->createArea();
    area->restoreState(data["area"].toMap());
    layout()->addWidget(area);
  }
}
