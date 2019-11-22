/*
  This file is part of KDDockWidgets.

  Copyright (C) 2018-2019 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Sérgio Martins <sergio.martins@kdab.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * @file
 * @brief Implements a QTabWidget derived class with support for docking and undocking
 * KDockWidget::DockWidget as tabs .
 *
 * @author Sérgio Martins \<sergio.martins@kdab.com\>
 */

#ifndef KD_TAB_WIDGET_P_H
#define KD_TAB_WIDGET_P_H

#include "docks_export.h"
#include "Draggable_p.h"
#include "Frame_p.h"
#include "DockWidgetBase.h"

#include <QTabBar>
#include <QVector>

#include <memory>

namespace KDDockWidgets
{

    class DockWidgetBase;
    class TabWidget;

    ///@brief a QTabBar derived class to be used by KDDockWidgets::TabWidget
    class DOCKS_EXPORT TabBar : public Draggable
    {
    public:
        /**
         * @brief Constructs a new TabBar
         * @param parent The parent TabWidget
         */
        explicit TabBar(QWidgetOrQuick* thisWidget, TabWidget* parent = nullptr);

        /**
         * @brief returns the dock widgets at tab number @p index
         * @param index the tab number from which we want the dock widget
         * @return the dock widget at tab number @p index
         */
        DockWidgetBase* dockWidgetAt(int index) const;

        ///@overload
        DockWidgetBase* dockWidgetAt(QPoint localPos) const;

        // Draggable
        std::unique_ptr<WindowBeingDragged> makeWindow() override;

        /**
         * @brief detaches a dock widget and shows it as a floating dock widget
         * The dock widget is morphed into a FloatingWindow for convenience.
         * @param dockWidget the dock widget to detach
         * @returns the created FloatingWindow
         */
        FloatingWindow* detachTab(DockWidgetBase* dockWidget);

        void onMousePress(QPoint localPos);

        ///@brief returns whether there's only 1 tab
        bool hasSingleDockWidget() const;

        virtual int numDockWidgets() const = 0;
        virtual int tabAt(QPoint localPos) const = 0;


        /**
         * @brief Returns this class as a QWidget (if using QtWidgets) or QQuickItem
         */
        QWidgetOrQuick* asWidget() const;

    private:
        TabWidget* const m_tabWidget;
        QPointer<DockWidgetBase> m_lastPressedDockWidget = nullptr;
        QWidgetOrQuick* const m_thisWidget;
    };

    class DOCKS_EXPORT TabWidget : public Draggable
    {
    public:
        /**
         * @brief Constructs a new TabWidget, with @p frame as a parent
         */
        explicit TabWidget(QWidgetOrQuick* thisWidget, Frame* frame);

        /**
         * @brief returns the number of dock widgets in this TabWidget
         */
        virtual int numDockWidgets() const = 0;

        /**
         * @brief Removes a dock widget from the TabWidget
         */
        virtual void removeDockWidget(DockWidgetBase*) = 0;

        /**
         * @brief Returns the index of the dock widget, or -1 if it doesn't exist
         */
        virtual int indexOfDockWidget(DockWidgetBase*) const = 0;

        /**
         * @brief Sets the current dock widget index
         */
        virtual void setCurrentDockWidget(int index) = 0;
        void setCurrentDockWidget(DockWidgetBase*);

        virtual void insertDockWidget(int index, DockWidgetBase*, const QIcon &, const QString & title) = 0;

        virtual void setTabBarAutoHide(bool) = 0;

        /**
         * @brief Returns the current index
         */
        virtual int currentIndex() const = 0;

        ///@brief appends a dock widget into this TabWidget
        void addDockWidget(DockWidgetBase*);

        /**
         * @brief Returns the dock widget tabbed at index @p index
         */
        virtual DockWidgetBase* dockwidgetAt(int index) const = 0;

        /**
         * @brief detaches a dock widget and shows it as a floating dock widget
         * @param dockWidget the dock widget to detach
         */
        virtual void detachTab(DockWidgetBase* dockWidget) = 0;

        /**
         * @brief inserts @p dockwidget into the TabWidget, at @p index
         * @param dockwidget the dockwidget to insert
         * @param index The index to where to put it
         */
        void insertDockWidget(DockWidgetBase* dockwidget, int index);

        /**
         * @brief Returns whether dockwidget @p dw is contained in this tab widget
         * Equivalent to indexOf(dw) != -1
         */
        bool contains(DockWidgetBase* dw) const;

        /**
         * @brief Returns the tab bar
         */
        virtual TabBar* tabBar() const = 0;

        /**
         * @brief Returns this class as a QWidget (if using QtWidgets) or QQuickItem
         */
        QWidgetOrQuick* asWidget() const;

        ///@brief getter for the frame
        Frame* frame() const;

        // Draggable interface
        std::unique_ptr<WindowBeingDragged> makeWindow() override;

    protected:
        void onTabInserted();
        void onTabRemoved();

    private:
        Frame* const m_frame;
        QWidgetOrQuick* const m_thisWidget;
        Q_DISABLE_COPY(TabWidget)
    };
}

#endif
