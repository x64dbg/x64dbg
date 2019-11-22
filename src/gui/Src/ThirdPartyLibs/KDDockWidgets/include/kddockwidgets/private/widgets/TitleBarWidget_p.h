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

#ifndef KD_TITLEBARWIDGET_P_H
#define KD_TITLEBARWIDGET_P_H

#include "../../docks_export.h"
#include "../TitleBar_p.h"

#include <QPainter>
#include <QToolButton>
#include <QStyle>
#include <QWidget>
#include <QVector>
#include <QStyleOptionToolButton>

class QHBoxLayout;
class QLabel;

namespace KDDockWidgets
{

    class DockWidget;
    class Frame;

    class DOCKS_EXPORT TitleBarWidget : public TitleBar
    {
        Q_OBJECT
    public:
        explicit TitleBarWidget(Frame* parent);
        explicit TitleBarWidget(FloatingWindow* parent);
        ~TitleBarWidget() override;

        ///@brief getter for the close button
        QWidget* closeButton() const;

        static QAbstractButton* createButton(QWidget* parent, const QIcon & icon);

    protected:
        void paintEvent(QPaintEvent*) override;
        void mouseDoubleClickEvent(QMouseEvent*) override;
        void updateFloatButton() override;
        void updateCloseButton() override;

        // The following are needed for the unit-tests
        bool isCloseButtonVisible() const override;
        bool isCloseButtonEnabled() const override;
        bool isFloatButtonVisible() const override;
        bool isFloatButtonEnabled() const override;

    private:
        void init();
        int buttonAreaWidth() const;

        QRect iconRect() const;

        QHBoxLayout* const m_layout;
        QAbstractButton* m_closeButton = nullptr;
        QAbstractButton* m_floatButton = nullptr;
        QLabel* m_dockWidgetIcon = nullptr;
    };

    class Button : public QToolButton
    {
        Q_OBJECT
    public:
        explicit Button(QWidget* parent)
            : QToolButton(parent)
        {
            //const int margin = style()->pixelMetric(QStyle::PM_DockWidgetTitleBarButtonMargin, nullptr, this) * 2;
            QSize sz = /*QSize(margin, margin) + */ QSize(16, 16);
            setFixedSize(sz);
        }

        ~Button() override;

        void paintEvent(QPaintEvent*) override
        {
            QPainter p(this);
            QStyleOptionToolButton opt;
            opt.init(this);

            if(isEnabled() && underMouse())
            {
                if(isDown())
                {
                    opt.state |= QStyle::State_Sunken;
                }
                else
                {
                    opt.state |= QStyle::State_Raised;
                }
                style()->drawPrimitive(QStyle::PE_PanelButtonTool, &opt, &p, this);
            }

            opt.subControls = QStyle::SC_None;
            opt.features = QStyleOptionToolButton::None;
            opt.icon = icon();
            opt.iconSize = size();
            style()->drawComplexControl(QStyle::CC_ToolButton, &opt, &p, this);
        }
    };

}

#endif
