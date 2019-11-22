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

#ifndef KD_DOCKS_EXPORT_H
#define KD_DOCKS_EXPORT_H

#include <QtCore/QtGlobal>

#if QT_VERSION < QT_VERSION_CHECK(5,7,0)
#include <QFlags>
#include <QDataStream>

// this adds const to non-const objects (like std::as_const)
template <typename T>
Q_DECL_CONSTEXPR typename std::add_const<T>::type & qAsConst(T & t) Q_DECL_NOTHROW { return t; }
// prevent rvalue arguments:
template <typename T>
void qAsConst(const T &&) Q_DECL_EQ_DELETE;

template<class Enum>
inline QDataStream & operator>>(QDataStream & ds, QFlags<Enum> & qenum)
{
    int i = 0;
    ds >> i;
    qenum = QFlags<Enum>(static_cast<Enum>(i));
    return ds;
}
#endif

#if defined(BUILDING_DOCKS_LIBRARY)
#  define DOCKS_EXPORT Q_DECL_EXPORT
#  if defined(DOCKS_DEVELOPER_MODE)
#    define DOCKS_EXPORT_FOR_UNIT_TESTS Q_DECL_EXPORT
#  else
#    define DOCKS_EXPORT_FOR_UNIT_TESTS
#  endif
#else
#  define DOCKS_EXPORT Q_DECL_IMPORT
#  if defined(DOCKS_DEVELOPER_MODE)
#    define DOCKS_EXPORT_FOR_UNIT_TESTS Q_DECL_IMPORT
# else
#    define DOCKS_EXPORT_FOR_UNIT_TESTS Q_DECL_IMPORT
# endif
#endif

#endif
