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

#ifndef KDDOCKWIDGETS_QWIDGETADAPTER_H
#define KDDOCKWIDGETS_QWIDGETADAPTER_H

#if !defined(KDDOCKWIDGETS_QTWIDGETS) && !defined(KDDOCKWIDGETS_QTQUICK)
# define KDDOCKWIDGETS_QTWIDGETS
#endif

/**
 * @file
 * @brief Abstraction for supporting both QtWidgets and QtQuick.
 *
 * @author Sérgio Martins \<sergio.martins@kdab.com\>
 */

#ifdef KDDOCKWIDGETS_QTWIDGETS
# include "private/widgets/QWidgetAdapter_widgets_p.h"
#include <QMainWindow>
typedef QWidget QWidgetOrQuick;
typedef QMainWindow QMainWindowOrQuick;
#else
# include "quick/QWidgetAdapter_quick_p.h"
typedef KDDockWidgets::QWidgetAdapter QWidgetOrQuick;
typedef QWidgetOrQuick QMainWindowOrQuick;
#endif

#endif
