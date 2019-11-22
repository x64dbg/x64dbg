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

#ifndef KD_LAYOUTSAVER_H
#define KD_LAYOUTSAVER_H

/**
 * @file
 * @brief Class to save and restore dockwidget layouts.
 *
 * @author Sérgio Martins \<sergio.martins@kdab.com\>
 */

#include "docks_export.h"

class QByteArray;

namespace KDDockWidgets
{

    class DockWidgetBase;

    class DOCKS_EXPORT LayoutSaver
    {
    public:
        ///@brief Constructor. Construction on the stack is suggested.
        LayoutSaver();

        ///@brief Destructor.
        ~LayoutSaver();

        ///@brief returns whether a restore (@ref restoreLayout) is in progress
        static bool restoreInProgress();

        /**
         * @brief saves the layout to disk using QSettings
         */
        bool saveToDisk();

        /**
         * @brief restores the layout from disk using QSettings.
         * @return true on success
         */
        bool restoreFromDisk();

        /**
         * @brief saves the layout into a byte array
         */
        QByteArray serializeLayout() const;

        /**
         * @brief restores the layout from a byte array
         * All MainWindows and DockWidgets should have been created before calling
         * this function.
         *
         * If not all DockWidgets can be created beforehand then make sure to set
         * a DockWidget factory via Config::setDockWidgetFactoryFunc()
         *
         * @sa Config::setDockWidgetFactoryFunc()
         *
         * @return true on success
         */
        bool restoreLayout(const QByteArray &);

        /**
         * @brief returns a list of dock widgets which were restored since the last
         * @ref restoreLayout() or @ref restoreFromDisk()
         *
         * Useful since some dock widgets can be new, and hence not be included in the last saved layout.
         */
        QVector<DockWidgetBase*> restoredDockWidgets() const;

        struct Layout;
        struct MainWindow;
        struct FloatingWindow;
        struct DockWidget;
        struct LastPosition;
        struct MultiSplitterLayout;
        struct Item;
        struct Anchor;
        struct Frame;
        struct Placeholder;

    private:
        friend class TestDocks;

        class Private;
        Private* const d;
    };
}

#endif
