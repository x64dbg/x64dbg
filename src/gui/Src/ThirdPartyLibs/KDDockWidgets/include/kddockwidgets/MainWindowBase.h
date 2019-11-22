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
 * @brief The MainWindow base-class that's shared between QtWidgets and QtQuick stack.
 *
 * @author Sérgio Martins \<sergio.martins@kdab.com\>
 */

#ifndef KD_MAINWINDOW_BASE_H
#define KD_MAINWINDOW_BASE_H

#include "docks_export.h"
#include "KDDockWidgets.h"
#include "QWidgetAdapter.h"
#include "LayoutSaver_p.h"

#include <QVector>

namespace KDDockWidgets
{

    class DockWidgetBase;
    class Frame;
    class DropArea;
    class MultiSplitterLayout;
    class DropAreaWithCentralFrame;

    /**
     * @brief The MainWindow base-class. MainWindow and MainWindowBase are only
     * split in two so we can share some code with the QtQuick implementation,
     * which also derives from MainWindowBase.
     *
     * Do not use instantiate directly in user code. Use MainWindow instead.
     */
    class DOCKS_EXPORT MainWindowBase : public QMainWindowOrQuick
    {
        Q_OBJECT
    public:
        typedef QVector<MainWindowBase*> List;
        explicit MainWindowBase(const QString & uniqueName, MainWindowOptions options = MainWindowOption_HasCentralFrame,
                                QWidgetOrQuick* parent = nullptr, Qt::WindowFlags flags = {});

        ~MainWindowBase() override;

        /**
         * @brief Docks a DockWidget into the central frame, tabbed.
         * @warning Requires that the MainWindow was constructed with MainWindowOption_HasCentralFrame option.
         * @param The DockWidget to dock.
         *
         * @sa DockWidgetBase::addDockWidgetAsTab()
         */
        void addDockWidgetAsTab(DockWidgetBase* dockwidget);

        /**
         * @brief Docks a DockWidget into this main window.
         * @param dockWidget The dock widget to add into this MainWindow
         * @param location the location where to dock
         * @param relativeTo In case we're docking in relation to another dock widget
         * @param option AddingOptions
         */
        void addDockWidget(DockWidgetBase* dockWidget,
                           KDDockWidgets::Location location,
                           DockWidgetBase* relativeTo = nullptr, AddingOption option = {});

        /**
         * @brief Returns the unique name that was passed via constructor.
         *        Used internally by the save/restore mechanism.
         * @internal
         */
        QString uniqueName() const;


        /// @brief Returns the main window options that were passed via constructor.
        MainWindowOptions options() const;

        ///@internal
        ///@brief returns the drop area.
        virtual DropAreaWithCentralFrame* dropArea() const = 0;

        ///@internal
        ///@brief returns the MultiSplitterLayout.
        MultiSplitterLayout* multiSplitterLayout() const;

    protected:
        void setUniqueName(const QString & uniqueName);

    Q_SIGNALS:
        void uniqueNameChanged();

    private:
        class Private;
        Private* const d;

        friend class LayoutSaver;
        bool deserialize(const LayoutSaver::MainWindow &);
        LayoutSaver::MainWindow serialize() const;
    };

}

#endif
