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

#ifndef KDDOCKWIDGETS_FRAMEWORKWIDGETFACTORY_H
#define KDDOCKWIDGETS_FRAMEWORKWIDGETFACTORY_H

#include "docks_export.h"
#include "KDDockWidgets.h"
#include "QWidgetAdapter.h"

/**
 * @file
 * @brief A factory class for allowing the user to customize some internal widgets.
 *
 * @author Sérgio Martins \<sergio.martins@kdab.com\>
 */

namespace KDDockWidgets
{

    class DropIndicatorOverlayInterface;
    class Separator;
    class FloatingWindow;
    class TabWidget;
    class TitleBar;
    class Frame;
    class DropArea;
    class Anchor;
    class TabBar;

    /**
     * @brief A factory class for allowing the user to customize some internal widgets.
     * This is optional, and if not provided, a default one will be used, @ref DefaultWidgetFactory.
     *
     * Deriving from @ref DefaultWidgetFactory is recommended, unless you need to override
     * all methods.
     *
     * Sub-classing FrameworkWidgetFactory allows for fine-grained customization and
     * styling of some non-public widgets, such as titlebars, dock widget frame and
     * tab widgets.
     *
     * To set your own factory see Config::setFrameworkWidgetFactory()
     *
     * Will also be useful to provide a QtQuickWidget factory in the future.
     *
     * @sa Config::setFrameworkWidgetFactory()
     */
    class DOCKS_EXPORT FrameworkWidgetFactory
    {
    public:
        ///@brief Destructor.Don't delete FrameworkWidgetFactory directly, it's owned
        /// by the framework.
        virtual ~FrameworkWidgetFactory();

        ///@brief Called internally by the framework to create a Frame class
        ////      Override to provide your own Frame sub-class. A frame is the
        ///       widget that holds the titlebar and tab-widget which holds the
        ///       DockWidgets.
        ///@param parent just forward to Frame's constructor
        ///@param options just forward to Frame's constructor
        virtual Frame* createFrame(QWidgetOrQuick* parent = nullptr, FrameOptions = FrameOption_None) const = 0;

        ///@brief Called internally by the framework to create a TitleBar
        ///       Override to provide your own TitleBar sub-class. If overridden then
        ///       you also need to override the overload below.
        ///@param frame Just forward to TitleBar's constructor.
        virtual TitleBar* createTitleBar(Frame* frame) const = 0;

        ///@brief Called internally by the framework to create a TitleBar
        ///       Override to provide your own TitleBar sub-class. If overridden then
        ///       you also need to override the overload above.
        ///@param floatingWindow Just forward to TitleBar's constructor.
        virtual TitleBar* createTitleBar(FloatingWindow* floatingWindow) const = 0;

        ///@brief Called internally by the framework to create a TabBar
        ///       Override to provide your own TabBar sub-class.
        ///@param parent Just forward to TabBar's's constructor.
        virtual TabBar* createTabBar(TabWidget* parent = nullptr) const = 0;

        ///@brief Called internally by the framework to create a TabWidget
        ///       Override to provide your own TabWidget sub-class.
        ///@param parent Just forward to TabWidget's constructor.
        virtual TabWidget* createTabWidget(Frame* parent) const = 0;

        ///@brief Called internally by the framework to create a Separator
        ///       Override to provide your own Separator sub-class. The Separator allows
        ///       the user to resize nested dock widgets.
        ///@param anchor Just forward to Sepataror's constructor.
        ///@param parent Just forward to Separator's constructor.
        virtual Separator* createSeparator(Anchor* anchor, QWidgetAdapter* parent = nullptr) const = 0;

        ///@brief Called internally by the framework to create a FloatingWindow
        ///       Override to provide your own FloatingWindow sub-class. If overridden then
        ///       you also need to override the overloads below.
        ///@param parent Just forward to FloatingWindow's constructor.
        virtual FloatingWindow* createFloatingWindow(QWidgetOrQuick* parent = nullptr) const = 0;

        ///@brief Called internally by the framework to create a FloatingWindow
        ///       Override to provide your own FloatingWindow sub-class. If overridden then
        ///       you also need to override the overloads above.
        ///@param frame Just forward to FloatingWindow's constructor.
        ///@param parent Just forward to FloatingWindow's constructor.
        virtual FloatingWindow* createFloatingWindow(Frame* frame, QWidgetOrQuick* parent = nullptr) const = 0;

        ///@brief Called internally by the framework to create a DropIndicatorOverlayInterface
        ///       Override to provide your own DropIndicatorOverlayInterface sub-class.
        ///@param dropArea Just forward to DropIndicatorOverlayInterface's constructor.
        virtual DropIndicatorOverlayInterface* createDropIndicatorOverlay(DropArea* dropArea) const = 0;
    };

    /**
     * @brief The FrameworkWidgetFactory that's used if none is specified.
     */
    class DOCKS_EXPORT DefaultWidgetFactory : public FrameworkWidgetFactory
    {
    public:
        Frame* createFrame(QWidgetOrQuick* parent, FrameOptions) const override;
        TitleBar* createTitleBar(Frame*) const override;
        TitleBar* createTitleBar(FloatingWindow*) const override;
        TabBar* createTabBar(TabWidget* parent) const override;
        TabWidget* createTabWidget(Frame* parent) const override;
        Separator* createSeparator(Anchor* anchor, QWidgetAdapter* parent = nullptr) const override;
        FloatingWindow* createFloatingWindow(QWidgetOrQuick* parent = nullptr) const override;
        FloatingWindow* createFloatingWindow(Frame* frame, QWidgetOrQuick* parent = nullptr) const override;
        DropIndicatorOverlayInterface* createDropIndicatorOverlay(DropArea*) const override;
    };

}

#endif
