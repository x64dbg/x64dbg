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

#ifndef KD_DRAGGABLE_P_H
#define KD_DRAGGABLE_P_H

#include "QWidgetAdapter.h"

#include <QVector>

#include <memory>

namespace KDDockWidgets
{

    class FloatingWindow;
    class WidgetResizeHandler;
    struct WindowBeingDragged;

    /**
     * @brief Represents something that can be dragged.
     *
     * Like a tab, a docked title bar, and even a FloatingWindow if it has a native OS title bar
     *
     * \internal
     */
    class DOCKS_EXPORT Draggable
    {
    public:
        typedef QVector<Draggable*> List;

        explicit Draggable(QWidgetOrQuick* thisObject, bool enabled = true);
        virtual ~Draggable();
        QWidgetOrQuick* asWidget() const;

        /**
         * If this draggable is already a window, do nothing.
         * Otherwise it should make it a window. As that's what dragging does.
         */
        virtual std::unique_ptr<WindowBeingDragged> makeWindow() = 0;

        /**
         * @brief Returns whether point @p p is draggable.
         *
         * Because simply inheriting from Draggable doesn't mean you can click anywhere to drag.
         */
        virtual bool isPositionDraggable(QPoint p) const { Q_UNUSED(p) return true; }

        WidgetResizeHandler* widgetResizeHandler() const;
        void setWidgetResizeHandler(WidgetResizeHandler* w);

    private:
        class Private;
        Private* const d;
        Q_DISABLE_COPY(Draggable)
    };
}

#endif
