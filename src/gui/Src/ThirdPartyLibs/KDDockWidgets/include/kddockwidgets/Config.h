/*
  This file is part of KDDockWidgets.

  Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
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
 * @brief Application-wide config to tune certain behaviours of the framework.
 *
 * @author Sérgio Martins \<sergio.martins@kdab.com\>
 */

#ifndef KD_DOCKWIDGETS_CONFIG_H
#define KD_DOCKWIDGETS_CONFIG_H

#include "docks_export.h"

class QQmlEngine;

namespace KDDockWidgets
{

    class DockWidgetBase;
    class FrameworkWidgetFactory;

    typedef KDDockWidgets::DockWidgetBase* (*DockWidgetFactoryFunc)(const QString & name);

    /**
     * @brief Singleton to allow to choose certain behaviours of the framework.
     *
     * The setters should only be used before creating any DockWidget or MainWindow,
     * preferably right after creating the QApplication.
     */
    class DOCKS_EXPORT Config
    {
    public:

        ///@brief returns the singleton Config instance
        static Config & self();

        ///@brief destructor, called at shutdown
        ~Config();

        ///@brief Flag enum to tune certain behaviours, the defaults are Flag_Default
        enum Flag
        {
            Flag_None = 0, ///> No option set
            Flag_NativeTitleBar = 1, ///> Enables the Native OS title bar on OSes that support it (Windows 10, macOS), ignored otherwise. This is mutually exclusive with Flag_AeroSnap
            Flag_AeroSnapWithClientDecos = 2, ///> Enables AeroSnap even if we're not using the native title bar. Only supported on Windows 10.
            Flag_HideTitleBarWhenTabsVisible = 8, ///> Hides the title bar if there's tabs visible. The empty space in the tab bar becomes draggable.
            Flag_AlwaysShowTabs = 16, ///> Always show tabs, even if there's only one
            Flag_Default = Flag_AeroSnapWithClientDecos ///> The defaults
        };
        Q_DECLARE_FLAGS(Flags, Flag)

        ///@brief returns the chosen flags
        Flags flags() const;

        ///@brief setter for the flags
        ///@param flags the flags to set
        ///Not all flags are guaranteed to be set, as the OS might not supported them
        ///Call @ref flags() after the setter if you need to know what was really set
        void setFlags(Flags flags);

        /**
         * @brief Registers a DockWidgetFactoryFunc.
         *
         * This is optional, the default is nullptr.
         *
         * A DockWidgetFactoryFunc is a function that receives a dock widget name
         * and returns a DockWidget instance.
         *
         * While restoring, @ref LayoutSaver requires all dock widgets to exist.
         * If a DockWidget doesn't exist then a DockWidgetFactoryFunc function is
         * required, so the layout saver can ask to create the DockWidget and then
         * restore it.
         */
        void setDockWidgetFactoryFunc(DockWidgetFactoryFunc);

        ///@brief Returns the DockWidgetFactoryFunc.
        ///nullptr by default
        DockWidgetFactoryFunc dockWidgetFactoryFunc() const;

        /**
         * @brief Sets the WidgetFactory.
         *
         * By default DefaultWidgetFactory is used, which gives you FrameWidget, TitleBarWidget,
         * TabBarWidget, TabWidgetWidget etc. You can set your own factory, to supply your own variants
         * of those classes, for the purposes of changing GUI appearance and such.
         *
         * Also potentially useful to return QtQuick classes instead of the QtWidget based ones.
         * Ownership is taken.
         */
        void setFrameworkWidgetFactory(FrameworkWidgetFactory*);

        ///@brief getter for the framework widget factory
        FrameworkWidgetFactory* frameworkWidgetFactory() const;

        /**
         * @brief Returns the thickness of the separator.
         *
         * Returns the width if the separator is vertical, otherwise the height.
         * If @p staticSeparator is true, then returns for the static separators.
         * The static separators are the ones at the edges of the window (top, left, bottom, right).
         *
         * Default is 0px for static separators and 5px for normal separators.
         */
        int separatorThickness(bool staticSeparator) const;

        ///@brief setter for @ref separatorThickness
        ///Note: Only use this function at startup before creating any DockWidget or MainWindow.
        void setSeparatorThickness(int value, bool staticSeparator);

        ///@brief Sets the QQmlEngine to use. Applicable only when using QtQuick.
        void setQmlEngine(QQmlEngine*);
        QQmlEngine* qmlEngine() const;

    private:
        Q_DISABLE_COPY(Config)
        Config();
        class Private;
        Private* const d;
    };

}

Q_DECLARE_OPERATORS_FOR_FLAGS(KDDockWidgets::Config::Flags)

#endif
