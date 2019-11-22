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

#ifndef KD_TITLEBAR_P_H
#define KD_TITLEBAR_P_H

#include "docks_export.h"

#include "QWidgetAdapter.h"
#include "Draggable_p.h"
#include "Frame_p.h"
#include "DockWidgetBase.h"

#include <QVector>
#include <QIcon>

class QHBoxLayout;
class QLabel;

namespace KDDockWidgets
{

    class DockWidgetBase;
    class Frame;
    class Button;

    class DOCKS_EXPORT TitleBar : public QWidgetAdapter
        , public Draggable
    {
        Q_OBJECT
        Q_PROPERTY(QString title READ title NOTIFY titleChanged)
        Q_PROPERTY(bool hasIcon READ hasIcon NOTIFY iconChanged)
    public:
        typedef QVector<TitleBar*> List;
        explicit TitleBar(Frame* parent);
        explicit TitleBar(FloatingWindow* parent);
        ~TitleBar() override;

        void setTitle(const QString & title);
        QString title() const { return m_title; }

        void setIcon(const QIcon & icon);
        std::unique_ptr<WindowBeingDragged> makeWindow() override;

        ///@brief Returns true if the dock widget which has this title bar is floating
        bool isFloating() const;

        ///@brief the list of dockwidgets under this TitleBar.
        /// There should always be at least 1. If more than 1 then they are tabbed.
        DockWidgetBase::List dockWidgets() const;

        ///@brief returns whether this title bar supports a floating/unfloating button
        bool supportsFloatingButton() const;

        ///@brief returns whether this title bar has an icon
        bool hasIcon() const;

        ///@brief the icon
        QIcon icon() const;

        ///@brief toggle floating
        bool onDoubleClicked();

        ///@brief getter for m_frame
        const Frame* frame() const { return m_frame; }

        ///@brief getter for m_floatingWindow
        const FloatingWindow* floatingWindow() const { return m_floatingWindow; }

    Q_SIGNALS:
        void titleChanged();
        void iconChanged();

    protected:
        void onCloseClicked();
        void onFloatClicked();
        virtual void updateFloatButton() {}
        virtual void updateCloseButton() {}

        // The following are needed for the unit-tests
        virtual bool isCloseButtonVisible() const { return true; }
        virtual bool isCloseButtonEnabled() const { return true; }
        virtual bool isFloatButtonVisible() const { return true; }
        virtual bool isFloatButtonEnabled() const { return true; }

    private:
        friend class TestDocks;

        void init();

        QPoint m_pressPos;
        QString m_title;
        QIcon m_icon;

        Frame* const m_frame;
        FloatingWindow* const m_floatingWindow;
    };


}

#endif
