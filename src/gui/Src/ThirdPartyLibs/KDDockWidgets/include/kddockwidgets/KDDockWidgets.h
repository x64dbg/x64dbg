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
 * @brief File with KDDockWidgets namespace-level enums and methods.
 *
 * @author Sérgio Martins \<sergio.martins@kdab.com\>
 */

#ifndef KD_KDDOCKWIDGETS_H
#define KD_KDDOCKWIDGETS_H

#include <QObject>

namespace KDDockWidgets
{
    enum Location
    {
        Location_None,
        Location_OnLeft, ///> Left docking location
        Location_OnTop,  ///> Top docking location
        Location_OnRight, ///> Right docking location
        Location_OnBottom ///> Bottom docking location
    };

    enum MainWindowOption
    {
        MainWindowOption_None = 0, ///> No option set
        MainWindowOption_HasCentralFrame = 1 ///> Makes the MainWindow always have a central frame, for tabbing documents
    };
    Q_DECLARE_FLAGS(MainWindowOptions, MainWindowOption)

    enum AddingOption
    {
        AddingOption_None = 0, ///> No option set
        AddingOption_StartHidden ///< Don't show the dock widget when adding it
    };

    ///@internal
    enum FrameOption
    {
        FrameOption_None = 0,
        FrameOption_AlwaysShowsTabs = 1,
        FrameOption_IsCentralFrame = 2
    };
    Q_DECLARE_FLAGS(FrameOptions, FrameOption)

    ///@internal
    inline Location oppositeLocation(Location loc)
    {
        switch(loc)
        {
        case Location_OnLeft:
            return Location_OnRight;
        case Location_OnTop:
            return Location_OnBottom;
        case Location_OnRight:
            return Location_OnLeft;
        case Location_OnBottom:
            return Location_OnTop;
        default:
            Q_ASSERT(false);
            return Location_None;
        }
    }

    ///@internal
    inline Location adjacentLocation(Location loc)
    {
        switch(loc)
        {
        case Location_OnLeft:
            return Location_OnTop;
        case Location_OnTop:
            return Location_OnRight;
        case Location_OnRight:
            return Location_OnBottom;
        case Location_OnBottom:
            return Location_OnLeft;
        default:
            Q_ASSERT(false);
            return Location_None;
        }
    }

    ///@internal
    inline QString locationStr(Location loc)
    {
        switch(loc)
        {
        case KDDockWidgets::Location_None:
            return QStringLiteral("none");
        case KDDockWidgets::Location_OnLeft:
            return QStringLiteral("left");
        case KDDockWidgets::Location_OnTop:
            return QStringLiteral("top");
        case KDDockWidgets::Location_OnRight:
            return QStringLiteral("right");
        case KDDockWidgets::Location_OnBottom:
            return QStringLiteral("bottom");
        }

        return QString();
    }
}

Q_DECLARE_METATYPE(KDDockWidgets::Location)
Q_DECLARE_OPERATORS_FOR_FLAGS(KDDockWidgets::FrameOptions)

#endif
