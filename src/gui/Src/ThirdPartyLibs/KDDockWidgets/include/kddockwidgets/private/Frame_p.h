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
 * @brief A DockWidget wrapper that adds a QTabWidget and a TitleBar.
 *
 * @author Sérgio Martins \<sergio.martins@kdab.com\>
 */

#ifndef KD_FRAME_P_H
#define KD_FRAME_P_H

#include "docks_export.h"
#include "QWidgetAdapter.h"
#include "LayoutSaver_p.h"

#include <QWidget>
#include <QVector>
#include <QDebug>
#include <QPointer>

namespace KDDockWidgets
{

    class TitleBar;
    class TabWidget;
    class DropArea;
    class DockWidgetBase;
    class Item;
    class FloatingWindow;

    /**
     * @brief A DockWidget wrapper that adds a QTabWidget and a TitleBar
     *
     * Frame is the actual widget that goes into the MultiSplitter. It provides a TitleBar which you
     * can use to detach, and also a QTabWidget so you can tab dock widgets together.
     *
     * This class doesn't actually add window frames and it's never a top-level widget. A Frame is always
     * inside a MultiSplitter (DropArea). Be it a MultiSplitter belonging to a MainWindow or belonging
     * to a FloatingWindow.
     */
    class DOCKS_EXPORT Frame : public QWidgetAdapter
    {
        Q_OBJECT

        Q_PROPERTY(KDDockWidgets::TitleBar* titleBar READ titleBar CONSTANT)
    public:
        typedef QList<Frame*> List;

        explicit Frame(QWidgetOrQuick* parent = nullptr, FrameOptions = FrameOption_None);
        ~Frame() override;

        static Frame* deserialize(const LayoutSaver::Frame &);
        LayoutSaver::Frame serialize() const;

        ///@brief Adds a widget into the Frame's TabWidget
        void addWidget(DockWidgetBase*);
        ///@overload
        void addWidget(Frame*);
        ///@overload
        void addWidget(FloatingWindow* floatingWindow);

        ///@brief Inserts a widget into the Frame's TabWidget at @p index
        void insertWidget(DockWidgetBase*, int index);

        ///@brief removes a dockwidget from the frame
        void removeWidget(DockWidgetBase*);

        void updateTitleAndIcon();
        void updateTitleBarVisibility();
        bool containsMouse(QPoint globalPos) const;
        TitleBar* titleBar() const;
        TitleBar* actualTitleBar() const;
        TabWidget* tabWidget() const;
        QString title() const;
        QIcon icon() const;
        const QVector<DockWidgetBase*> dockWidgets() const;
        DockWidgetBase* dockWidgetAt(int index) const;
        void setDropArea(DropArea*);

        bool isTheOnlyFrame() const;

        /**
         * @brief Returns whether this frame is floating. A floating frame isn't attached to any other MainWindow,
         * and if it's attached to a FloatingWindow then it's considered floating if it's the only frame in that Window.
         * A floating frame can have multiple dock widgets (tabbed), in which case each DockWidget::isFloating() returns false,
         * in which case you can use isInFloatingWindow() which would still return true
         */
        bool isFloating() const;

        /**
         * @brief Returns whether this frame is in a FloatingWindow, as opposed to MainWindow.
         *
         * After setup it's equivalent to !isInMainWindow().
         */
        bool isInFloatingWindow() const;

        /**
         * @brief Returns whether this frame is docked inside a MainWindow.
         */
        bool isInMainWindow() const;

        /**
         * @brief returns if this widget is the central frame
         * MainWindow supports a mode where the middle frame is persistent even if no dock widget is there.
         *
         * @return whether this widget is the central frame in a main window
         */
        bool isCentralFrame() const { return m_options & FrameOption_IsCentralFrame; }

        /**
         * @brief whether the tab widget will always show tabs, even if there's only 1 dock widget
         *
         * While technically a non-floating dock widget is always tabbed, the user won't see the tabs
         * as in most cases there's only 1 widget tabbed. But for the main window central frame it's
         * often wanted to see tabs even if there's only 1 widget, where each widget represents a "document".
         *
         * @return whether the tab widget will always show tabs, even if there's only 1 dock widget
         */
        bool alwaysShowsTabs() const { return m_options & FrameOption_AlwaysShowsTabs; }


        /// @brief returns the number of dock widgets inside the frame
        int dockWidgetCount() const;

        /// @brief returns whether the dockwidget @p w is inside this frame
        bool contains(DockWidgetBase* w) const;


        ///@brief returns the FloatingWindow this frame is in, if any
        FloatingWindow* floatingWindow() const;

        /**
         * @brief Puts the Frame back in its previous main window position
         *
         * Usually DockWidget::Private::restoreToPreviousPosition() is used, but
         * when we have a floating frame with tabs we just reuse the frame instead of
         * moving the tabbed dock widgets one by one.
         */
        void restoreToPreviousPosition();

        void onCloseEvent(QCloseEvent* e) override;
        int currentTabIndex() const;
        void setCurrentTabIndex(int);

        DockWidgetBase* currentDockWidget() const;

        FrameOptions options() const { return m_options; }
        bool anyNonClosable() const;


        ///@brief returns whether there's 0 dock widgets. If not persistent then the Frame will delete itself.
        bool isEmpty() const { return dockWidgetCount() == 0; }

        ///@brief returns whether there's only 1 dock widget.
        bool hasSingleDockWidget() const { return dockWidgetCount() == 1; }

        ///@brief Called when a dock widget child @p w is shown
        void onDockWidgetShown(DockWidgetBase* w);

        ///@brief Called when a dock widget child @p w is hidden
        void onDockWidgetHidden(DockWidgetBase* w);

        ///@brief sets the layout item that either contains this Frame in the layout or is a placeholder
        void setLayoutItem(Item* item);

        ///@brief returns the layout item that either contains this Frame in the layout or is a placeholder
        Item* layoutItem() const;

        ///@brief For tests-only. Returns the number of Frame instances in the whole application.
        static int dbg_numFrames();

        ///@brief for debug only, prints some debug information
        void dumpDebug();

        /**
         * @brief Returns whether a deleteLater has already been issued
         */
        bool beingDeletedLater() const;

        /**
         * @brief returns true if tabs are visible
         *
         * @sa hasTabsVisibleChanged()
         **/
        bool hasTabsVisible() const;

    Q_SIGNALS:
        void currentDockWidgetChanged(KDDockWidgets::DockWidgetBase*);
        void numDockWidgetsChanged();
        void hasTabsVisibleChanged();
        void layoutInvalidated();

    private:
        Q_DISABLE_COPY(Frame)
        friend class TestDocks;
        friend class TabWidget;
        void onDockWidgetCountChanged();
        void onCurrentTabChanged(int index);
        void scheduleDeleteLater();
        bool event(QEvent*) override;
        TabWidget* const m_tabWidget;
        TitleBar* const m_titleBar;
        DropArea* m_dropArea = nullptr;
        const FrameOptions m_options;
        QPointer<Item> m_layoutItem;
        bool m_beingDeleted = false;
    };

}

inline QDebug operator<< (QDebug d, KDDockWidgets::Frame* frame)
{
    if(frame)
    {
        d << static_cast<QObject*>(frame);
        d << "; window=" << frame->window();
        d << "; options=" << frame->options();
        d << "; dockwidgets=" << frame->dockWidgets();
    }
    else
    {
        d << "nullptr";
    }

    return d;
}

#endif
