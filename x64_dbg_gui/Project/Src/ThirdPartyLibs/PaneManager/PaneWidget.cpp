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
#include "PaneWidget.h"
#include "Pane.h"
#include "PaneSerialize.h"
#include <QVBoxLayout>
#include <QDebug>
#include <QEvent>
#include <QApplication>
#include <QDrag>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QDesktopWidget>
#include <QScreen>

template<class T>
T findClosestParent(QWidget* widget) {
    while(widget) {
        if (qobject_cast<T>(widget)) {
            return static_cast<T>(widget);
        }
        widget = widget->parentWidget();
    }
    return 0;
}

PaneWidget::PaneWidget(QWidget *parent) :
    QWidget(parent)
{
    m_borderSensitivity = 12;
    QSplitter* testSplitter = new QSplitter();
    m_rubberBandLineWidth = testSplitter->handleWidth()*5;
    delete testSplitter;
    m_dragIndicator = new QLabel(0, Qt::ToolTip );
    m_dragIndicator->setAttribute(Qt::WA_ShowWithoutActivating);
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    PaneSerialize* wrapper = new PaneSerialize(this);
    wrapper->setWindowFlags(wrapper->windowFlags() & ~Qt::Tool);
    mainLayout->addWidget(wrapper);
    connect(&m_dropSuggestionSwitchTimer, SIGNAL(timeout()),
            this, SLOT(showNextDropSuggestion()));
    m_dropSuggestionSwitchTimer.setInterval(1000);
    m_dropCurrentSuggestionIndex = 0;

    m_rectRubberBand = new QRubberBand(QRubberBand::Rectangle, this);
    m_lineRubberBand = new QRubberBand(QRubberBand::Line, this);
}

PaneWidget::~PaneWidget() {
    while(!mPanes.isEmpty()) {
        delete mPanes.first();
    }
    while(!m_wrappers.isEmpty()) {
        delete m_wrappers.first();
    }
}

void PaneWidget::addWidget(QWidget *widget, const AreaPointer &area) {
    if (!widget) {
        qWarning("cannot add null widget");
        return;
    }
    if (m_widgets.contains(widget)) {
        qWarning("this widget has already been added");
        return;
    }
    widget->hide();
    widget->setParent(0);
    m_widgets.append(widget);
    moveWidget(widget, area);
}

void PaneWidget::addWidgets(QList<QWidget *> widgets, const PaneWidget::AreaPointer &area) {
    foreach(QWidget* widget, widgets) {
        addWidget(widget,area);
    }
}

Pane *PaneWidget::areaOf(QWidget *widget) const {
    return findClosestParent<Pane*>(widget);
}

void PaneWidget::moveWidget(QWidget *widget, AreaPointer area) {

    if (!m_widgets.contains(widget)) {
        qWarning("unknown widget");
        return;
    }
    if (widget->parentWidget() != 0) {
        releasewidget(widget);
    }


    if (area.type() == InLastUsedArea && !m_lastUsedArea) {
        Pane* foundArea = findChild<Pane*>();
        if (foundArea) {
            area = AreaPointer(AddTo, foundArea);
        } else {
            area = InEmptySpace;
        }
    }

    if (area.type() == NoArea) {
        //do nothing
    } else if (area.type() == InNewFloatingArea) {
        Pane* area = createArea();
        area->addWidget(widget);
        PaneSerialize* wrapper = new PaneSerialize(this);
        wrapper->layout()->addWidget(area);
        wrapper->move(QCursor::pos());
        wrapper->show();
    } else if (area.type() == AddTo) {
        area.area()->addWidget(widget);
    } else if (area.type() == LeftOf || area.type() == RightOf ||
               area.type() == TopOf || area.type() == BottomOf) {
        QSplitter* parentSplitter = qobject_cast<QSplitter*>(area.widget()->parentWidget());
        PaneSerialize* wrapper = qobject_cast<PaneSerialize*>(area.widget()->parentWidget());
        if (!parentSplitter && !wrapper) {
            qWarning("unknown parent type");
            return;
        }
        bool useParentSplitter = false;
        int indexInParentSplitter = 0;
        if (parentSplitter) {
            indexInParentSplitter = parentSplitter->indexOf(area.widget());
            if (parentSplitter->orientation() == Qt::Vertical) {
                useParentSplitter = area.type() == TopOf || area.type() == BottomOf;
            } else {
                useParentSplitter = area.type() == LeftOf || area.type() == RightOf;
            }
        }
        if (useParentSplitter) {
            if (area.type() == BottomOf || area.type() == RightOf) {
                indexInParentSplitter++;
            }
            Pane* newArea = createArea();
            newArea->addWidget(widget);
            parentSplitter->insertWidget(indexInParentSplitter, newArea);
        } else {
            area.widget()->hide();
            area.widget()->setParent(0);
            QSplitter* splitter = createSplitter();
            if (area.type() == TopOf || area.type() == BottomOf) {
                splitter->setOrientation(Qt::Vertical);
            } else {
                splitter->setOrientation(Qt::Horizontal);
            }
            splitter->addWidget(area.widget());
            area.widget()->show();
            Pane* newArea = createArea();
            if (area.type() == TopOf || area.type() == LeftOf) {
                splitter->insertWidget(0, newArea);
            } else {
                splitter->addWidget(newArea);
            }
            if (parentSplitter) {
                parentSplitter->insertWidget(indexInParentSplitter, splitter);
            } else {
                wrapper->layout()->addWidget(splitter);
            }
            newArea->addWidget(widget);
        }
    } else if (area.type() == InEmptySpace) {
        Pane* newArea = createArea();
        findChild<PaneSerialize*>()->layout()->addWidget(newArea);
        newArea->addWidget(widget);
    } else if (area.type() == InLastUsedArea) {
        m_lastUsedArea->addWidget(widget);
    } else {
        qWarning("invalid type");
    }
    simplifyLayout();

    emit widgetVisibilityChanged(widget, widget->parent() != 0);

    ///////////////
}

void PaneWidget::moveWidgets(QList<QWidget *> widgets,
                                 PaneWidget::AreaPointer area) {
    foreach(QWidget* widget, widgets) {
        moveWidget(widget,area);
    }

}

void PaneWidget::removeWidget(QWidget *widget) {
    if (!m_widgets.contains(widget)) {
        qWarning("unknown widget");
        return;
    }
    moveWidget(widget, NoArea);
    m_widgets.removeOne(widget);
}

void PaneWidget::setSuggestionSwitchInterval(int msec) {
    m_dropSuggestionSwitchTimer.setInterval(msec);
}

int PaneWidget::suggestionSwitchInterval() {
    return m_dropSuggestionSwitchTimer.interval();
}

void PaneWidget::setBorderSensitivity(int pixels) {
    m_borderSensitivity = pixels;
}

void PaneWidget::setRubberBandLineWidth(int pixels) {
    m_rubberBandLineWidth = pixels;
}

QVariant PaneWidget::saveState() {
    QVariantMap result;
    result["PaneWidgetStateFormat"] = 1;
    PaneSerialize* mainWrapper = findChild<PaneSerialize*>();
    if (!mainWrapper) {
        qWarning("can't find main wrapper");
        return QVariant();
    }
    result["mainWrapper"] = mainWrapper->saveState();
    QVariantList floatingWindowsData;
    foreach(PaneSerialize* wrapper, m_wrappers) {
        if (!wrapper->isWindow()) { continue; }
        floatingWindowsData << wrapper->saveState();
    }
    result["floatingWindows"] = floatingWindowsData;
    return result;
}

void PaneWidget::restoreState(const QVariant &data) {
    if (!data.isValid()) { return; }
    QVariantMap dataMap = data.toMap();
    if (dataMap["PaneWidgetStateFormat"].toInt() != 1) {
        qWarning("state format is not recognized");
        return;
    }
    moveWidgets(m_widgets, NoArea);
    PaneSerialize* mainWrapper = findChild<PaneSerialize*>();
    if (!mainWrapper) {
        qWarning("can't find main wrapper");
        return;
    }
    mainWrapper->restoreState(dataMap["mainWrapper"].toMap());
    foreach(QVariant windowData, dataMap["floatingWindows"].toList()) {
        PaneSerialize* wrapper = new PaneSerialize(this);
        wrapper->restoreState(windowData.toMap());
        wrapper->show();
    }
    simplifyLayout();
    foreach(QWidget* widget, m_widgets) {
        emit widgetVisibilityChanged(widget, widget->parentWidget() != 0);
    }
}

Pane *PaneWidget::createArea() {
    Pane* area = new Pane(this, 0);
    connect(area, SIGNAL(tabCloseRequested(int)),
            this, SLOT(tabCloseRequested(int)));
    return area;
}


void PaneWidget::handleNoSuggestions() {
    m_rectRubberBand->hide();
    m_lineRubberBand->hide();
    m_lineRubberBand->setParent(this);
    m_rectRubberBand->setParent(this);
    m_suggestions.clear();
    m_dropCurrentSuggestionIndex = 0;
    if (m_dropSuggestionSwitchTimer.isActive()) {
        m_dropSuggestionSwitchTimer.stop();
    }
}

void PaneWidget::releasewidget(QWidget *widget) {
    Pane* previousTabWidget = findClosestParent<Pane*>(widget);
    if (!previousTabWidget) {
        qWarning("cannot find tab widget for widget");
        return;
    }
    previousTabWidget->removeTab(previousTabWidget->indexOf(widget));
    widget->hide();
    widget->setParent(0);

}

void PaneWidget::simplifyLayout() {
    foreach(Pane* area, mPanes) {
        if (area->parentWidget() == 0) {
            if (area->count() == 0) {
                if (area == m_lastUsedArea) { m_lastUsedArea = 0; }
                //QTimer::singleShot(1000, area, SLOT(deleteLater()));
                area->deleteLater();
            }
            continue;
        }
        QSplitter* splitter = qobject_cast<QSplitter*>(area->parentWidget());
        QSplitter* validSplitter = 0; // least top level splitter that should remain
        QSplitter* invalidSplitter = 0; //most top level splitter that should be deleted
        while(splitter) {
            if (splitter->count() > 1) {
                validSplitter = splitter;
                break;
            } else {
                invalidSplitter = splitter;
                splitter = qobject_cast<QSplitter*>(splitter->parentWidget());
            }
        }
        if (!validSplitter) {
            PaneSerialize* wrapper = findClosestParent<PaneSerialize*>(area);
            if (!wrapper) {
                qWarning("can't find wrapper");
                return;
            }
            if (area->count() == 0 && wrapper->isWindow()) {
                wrapper->hide();
                // can't deleteLater immediately (strange MacOS bug)
                //QTimer::singleShot(1000, wrapper, SLOT(deleteLater()));
                wrapper->deleteLater();
            } else if (area->parent() != wrapper) {
                wrapper->layout()->addWidget(area);
            }
        } else {
            if (area->count() > 0) {
                if (validSplitter && area->parent() != validSplitter) {
                    int index = validSplitter->indexOf(invalidSplitter);
                    validSplitter->insertWidget(index, area);
                }
            }
        }
        if (invalidSplitter) {
            invalidSplitter->hide();
            invalidSplitter->setParent(0);
            //QTimer::singleShot(1000, invalidSplitter, SLOT(deleteLater()));
            invalidSplitter->deleteLater();
        }
        if (area->count() == 0) {
            area->hide();
            area->setParent(0);
            if (area == m_lastUsedArea) { m_lastUsedArea = 0; }
            //QTimer::singleShot(1000, area, SLOT(deleteLater()));
            area->deleteLater();
        }
    }
}

void PaneWidget::startDrag(const QList<QWidget *> &widgets) {
    if (dragInProgress()) {
        qWarning("PaneWidget::execDrag: drag is already in progress");
        return;
    }
    if (widgets.isEmpty()) { return; }
    m_draggedwidgets = widgets;
    m_dragIndicator->setPixmap(generateDragPixmap(widgets));
    updateDragPosition();
    m_dragIndicator->show();
}

QVariantMap PaneWidget::saveSplitterState(QSplitter *splitter) {
    QVariantMap result;
    result["state"] = splitter->saveState();
    result["type"] = "splitter";
    QVariantList items;
    for(int i = 0; i < splitter->count(); i++) {
        QWidget* item = splitter->widget(i);
        QVariantMap itemValue;
        Pane* area = qobject_cast<Pane*>(item);
        if (area) {
            itemValue = area->saveState();
        } else {
            QSplitter* childSplitter = qobject_cast<QSplitter*>(item);
            if (childSplitter) {
                itemValue = saveSplitterState(childSplitter);
            } else {
                qWarning("unknown splitter item");
            }
        }
        items << itemValue;
    }
    result["items"] = items;
    return result;
}

QSplitter *PaneWidget::restoreSplitterState(const QVariantMap &data) {
    if (data["items"].toList().count() < 2) {
        qWarning("invalid splitter encountered");
    }
    QSplitter* splitter = createSplitter();

    foreach(QVariant itemData, data["items"].toList()) {
        QVariantMap itemValue = itemData.toMap();
        QString itemType = itemValue["type"].toString();
        if (itemType == "splitter") {
            splitter->addWidget(restoreSplitterState(itemValue));
        } else if (itemType == "area") {
            Pane* area = createArea();
            area->restoreState(itemValue);
            splitter->addWidget(area);
        } else {
            qWarning("unknown item type");
        }
    }
    splitter->restoreState(data["state"].toByteArray());
    return splitter;
}

QPixmap PaneWidget::generateDragPixmap(const QList<QWidget *> &widgets) {
    QTabBar widgett;
    widgett.setDocumentMode(true);
    foreach(QWidget* widget, widgets) {
        widgett.addTab(widget->windowIcon(), widget->windowTitle());
    }
#if QT_VERSION >= 0x050000 // Qt5
    return widgett.grab();
#else //Qt4
    return QPixmap::grabWidget(&widgett);
#endif
}

void PaneWidget::showNextDropSuggestion() {
    if (m_suggestions.isEmpty()) {
        qWarning("showNextDropSuggestion called but no suggestions");
        return;
    }
    m_dropCurrentSuggestionIndex++;
    if (m_dropCurrentSuggestionIndex >= m_suggestions.count()) {
        m_dropCurrentSuggestionIndex = 0;
    }
    const AreaPointer& suggestion = m_suggestions[m_dropCurrentSuggestionIndex];
    if (suggestion.type() == AddTo || suggestion.type() == InEmptySpace) {
        QWidget* widget;
        if (suggestion.type() == InEmptySpace) {
            widget = findChild<PaneSerialize*>();
        } else {
            widget = suggestion.widget();
        }
        QWidget* placeHolderParent;
        if (widget->topLevelWidget() == topLevelWidget()) {
            placeHolderParent = this;
        } else {
            placeHolderParent = widget->topLevelWidget();
        }
        QRect placeHolderGeometry = widget->rect();
        placeHolderGeometry.moveTopLeft(widget->mapTo(placeHolderParent,
                                                      placeHolderGeometry.topLeft()));
        m_rectRubberBand->setGeometry(placeHolderGeometry);
        m_rectRubberBand->setParent(placeHolderParent);
        m_rectRubberBand->show();
        m_lineRubberBand->hide();
    } else if (suggestion.type() == LeftOf || suggestion.type() == RightOf ||
               suggestion.type() == TopOf || suggestion.type() == BottomOf) {
        QWidget* placeHolderParent;
        if (suggestion.widget()->topLevelWidget() == topLevelWidget()) {
            placeHolderParent = this;
        } else {
            placeHolderParent = suggestion.widget()->topLevelWidget();
        }
        QRect placeHolderGeometry = sidePlaceHolderRect(suggestion.widget(), suggestion.type());
        placeHolderGeometry.moveTopLeft(suggestion.widget()->mapTo(placeHolderParent,
                                                                   placeHolderGeometry.topLeft()));

        m_lineRubberBand->setGeometry(placeHolderGeometry);
        m_lineRubberBand->setParent(placeHolderParent);
        m_lineRubberBand->show();
        m_rectRubberBand->hide();
    } else {
        qWarning("unsupported suggestion type");
    }
}

void PaneWidget::findSuggestions(PaneSerialize* wrapper) {
    m_suggestions.clear();
    m_dropCurrentSuggestionIndex = -1;
    QPoint globalPos = QCursor::pos();
    QList<QWidget*> candidates;
    foreach(QSplitter* splitter, wrapper->findChildren<QSplitter*>()) {
        candidates << splitter;
    }
    foreach(Pane* area, mPanes) {
        if (area->topLevelWidget() == wrapper->topLevelWidget()) {
            candidates << area;
        }
    }
    foreach(QWidget* widget, candidates) {
        QSplitter* splitter = qobject_cast<QSplitter*>(widget);
        Pane* area = qobject_cast<Pane*>(widget);
        if (!splitter && !area) {
            qWarning("unexpected widget type");
            continue;
        }
        QSplitter* parentSplitter = qobject_cast<QSplitter*>(widget->parentWidget());
        bool lastInSplitter = parentSplitter &&
                parentSplitter->indexOf(widget) == parentSplitter->count() - 1;

        QList<AreaPointerType> allowedSides;
        if (!splitter || splitter->orientation() == Qt::Vertical) {
            allowedSides << LeftOf;
        }
        if (!splitter || splitter->orientation() == Qt::Horizontal) {
            allowedSides << TopOf;
        }
        if (!parentSplitter || parentSplitter->orientation() == Qt::Vertical || lastInSplitter) {
            if (!splitter || splitter->orientation() == Qt::Vertical) {
                allowedSides << RightOf;
            }
        }
        if (!parentSplitter || parentSplitter->orientation() == Qt::Horizontal || lastInSplitter) {
            if (!splitter || splitter->orientation() == Qt::Horizontal) {
                allowedSides << BottomOf;
            }
        }
        foreach(AreaPointerType side, allowedSides) {
            if (sideSensitiveArea(widget, side).contains(widget->mapFromGlobal(globalPos))) {
                m_suggestions << AreaPointer(side, widget);
            }
        }
        if (area && area->rect().contains(area->mapFromGlobal(globalPos))) {
            m_suggestions << AreaPointer(AddTo, area);
        }
    }
    if (candidates.isEmpty()) {
        m_suggestions << InEmptySpace;
    }

    if (m_suggestions.isEmpty()) {
        handleNoSuggestions();
    } else {
        showNextDropSuggestion();
    }
}

QRect PaneWidget::sideSensitiveArea(QWidget *widget, PaneWidget::AreaPointerType side) {
    QRect widgetRect = widget->rect();
    if (side == TopOf) {
        return QRect(QPoint(widgetRect.left(), widgetRect.top() - m_borderSensitivity),
                     QSize(widgetRect.width(), m_borderSensitivity * 2));
    } else if (side == LeftOf) {
        return QRect(QPoint(widgetRect.left() - m_borderSensitivity, widgetRect.top()),
                     QSize(m_borderSensitivity * 2, widgetRect.height()));

    } else if (side == BottomOf) {
        return QRect(QPoint(widgetRect.left(), widgetRect.top() + widgetRect.height() - m_borderSensitivity),
                     QSize(widgetRect.width(), m_borderSensitivity * 2));
    } else if (side == RightOf) {
        return QRect(QPoint(widgetRect.left() + widgetRect.width() - m_borderSensitivity, widgetRect.top()),
                     QSize(m_borderSensitivity * 2, widgetRect.height()));
    } else {
        qWarning("invalid side");
        return QRect();
    }
}

QRect PaneWidget::sidePlaceHolderRect(QWidget *widget, PaneWidget::AreaPointerType side) {
    QRect widgetRect = widget->rect();
    QSplitter* parentSplitter = qobject_cast<QSplitter*>(widget->parentWidget());
    if (parentSplitter && parentSplitter->indexOf(widget) > 0) {
        int delta = parentSplitter->handleWidth() / 2 + m_rubberBandLineWidth / 2;
        if (side == TopOf && parentSplitter->orientation() == Qt::Vertical) {
            return QRect(QPoint(widgetRect.left(), widgetRect.top() - delta),
                         QSize(widgetRect.width(), m_rubberBandLineWidth));
        } else if (side == LeftOf && parentSplitter->orientation() == Qt::Horizontal) {
            return QRect(QPoint(widgetRect.left() - delta, widgetRect.top()),
                         QSize(m_rubberBandLineWidth, widgetRect.height()));
        }
    }
    if (side == TopOf) {
        return QRect(QPoint(widgetRect.left(), widgetRect.top()),
                     QSize(widgetRect.width(), m_rubberBandLineWidth));
    } else if (side == LeftOf) {
        return QRect(QPoint(widgetRect.left(), widgetRect.top()),
                     QSize(m_rubberBandLineWidth, widgetRect.height()));
    } else if (side == BottomOf) {
        return QRect(QPoint(widgetRect.left(), widgetRect.top() + widgetRect.height() - m_rubberBandLineWidth),
                     QSize(widgetRect.width(), m_rubberBandLineWidth));
    } else if (side == RightOf) {
        return QRect(QPoint(widgetRect.left() + widgetRect.width() - m_rubberBandLineWidth, widgetRect.top()),
                     QSize(m_rubberBandLineWidth, widgetRect.height()));
    } else {
        qWarning("invalid side");
        return QRect();
    }
}

void PaneWidget::updateDragPosition() {
    if (!dragInProgress()) { return; }
    if (!(qApp->mouseButtons() & Qt::LeftButton)) {
        finishDrag();
        return;
    }

    QPoint pos = QCursor::pos();
    m_dragIndicator->move(pos + QPoint(1, 1));
    bool foundWrapper = false;

    QWidget* window = qApp->topLevelAt(pos);
    foreach(PaneSerialize* wrapper, m_wrappers) {
        if (wrapper->window() == window) {
            if (wrapper->rect().contains(wrapper->mapFromGlobal(pos))) {
                findSuggestions(wrapper);
                if (!m_suggestions.isEmpty()) {
                    //starting or restarting timer
                    if (m_dropSuggestionSwitchTimer.isActive()) {
                        m_dropSuggestionSwitchTimer.stop();
                    }
                    m_dropSuggestionSwitchTimer.start();
                    foundWrapper = true;
                }
            }
            break;
        }
    }
    if (!foundWrapper) {
        handleNoSuggestions();
    }
}

void PaneWidget::finishDrag() {
    if (!dragInProgress()) {
        qWarning("unexpected finishDrag");
        return;
    }
    if (m_suggestions.isEmpty()) {
        moveWidgets(m_draggedwidgets, InNewFloatingArea);

    } else {
        if (m_dropCurrentSuggestionIndex >= m_suggestions.count()) {
            qWarning("invalid m_dropCurrentSuggestionIndex");
            return;
        }
        PaneWidget::AreaPointer suggestion = m_suggestions[m_dropCurrentSuggestionIndex];
        handleNoSuggestions();
        moveWidgets(m_draggedwidgets, suggestion);
    }


    m_dragIndicator->hide();
    m_draggedwidgets.clear();
}

void PaneWidget::tabCloseRequested(int index) {
    Pane* tabWidget = qobject_cast<Pane*>(sender());
    if (!tabWidget) {
        qWarning("sender is not a Pane");
        return;
    }
    QWidget* widget = tabWidget->widget(index);
    if (!m_widgets.contains(widget)) {
        qWarning("unknown tab in tab widget");
        return;
    }
    hidewidget(widget);
}

QSplitter *PaneWidget::createSplitter() {
    QSplitter* splitter = new QSplitter();
    splitter->setChildrenCollapsible(false);
    return splitter;
}

PaneWidget::AreaPointer::AreaPointer(PaneWidget::AreaPointerType type, Pane *area) {
    m_type = type;
    setWidget(area);
}

void PaneWidget::AreaPointer::setWidget(QWidget *widget) {
    if (m_type == InLastUsedArea || m_type == InNewFloatingArea || m_type == NoArea || m_type == InEmptySpace) {
        if (widget != 0) {
            qWarning("area parameter ignored for this type");
        }
        m_widget = 0;
    } else if (m_type == AddTo) {
        m_widget = qobject_cast<Pane*>(widget);
        if (!m_widget) {
            qWarning("only Pane can be used with this type");
        }
    } else {
        if (!qobject_cast<Pane*>(widget) &&
                !qobject_cast<QSplitter*>(widget)) {
            qWarning("only Pane or splitter can be used with this type");
            m_widget = 0;
        } else {
            m_widget = widget;
        }
    }
}

Pane *PaneWidget::AreaPointer::area() const {
    return qobject_cast<Pane*>(m_widget);
}

PaneWidget::AreaPointer::AreaPointer(PaneWidget::AreaPointerType type, QWidget *widget) {
    m_type = type;
    setWidget(widget);
}
