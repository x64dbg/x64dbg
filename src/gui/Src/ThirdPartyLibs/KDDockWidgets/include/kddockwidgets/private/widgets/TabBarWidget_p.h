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

#ifndef KD_TABBAR_WIDGET_P_H
#define KD_TABBAR_WIDGET_P_H

#include "TabWidget_p.h"

#include <QTabBar>

class QMouseEvent;

namespace KDDockWidgets
{

    class DockWidget;
    class TabWidget;

    class DOCKS_EXPORT TabBarWidget : public QTabBar, public TabBar
    {
        Q_OBJECT
    public:
        explicit TabBarWidget(TabWidget* parent = nullptr);
        int numDockWidgets() const override;
        int tabAt(QPoint localPos) const override;
    protected:
        void mousePressEvent(QMouseEvent*) override;

    };
}

#endif
