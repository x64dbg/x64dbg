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

#ifndef KD_LAYOUTSAVER_P_H
#define KD_LAYOUTSAVER_P_H

#include "LayoutSaver.h"
#include "KDDockWidgets.h"

#include <QRect>
#include <QDataStream>
#include <QDebug>

#include <memory>

#define ANCHOR_MAGIC_MARKER "e520c60e-cf5d-4a30-b1a7-588d2c569851"
#define MULTISPLITTER_LAYOUT_MAGIC_MARKER "bac9948e-5f1b-4271-acc5-07f1708e2611"

#define KDDOCKWIDGETS_SERIALIZATION_VERSION 1

namespace KDDockWidgets
{

    struct LayoutSaver::Placeholder
    {
        typedef QVector<LayoutSaver::Placeholder> List;
        bool isFloatingWindow;
        int indexOfFloatingWindow;
        int itemIndex;
        QString mainWindowUniqueName;
    };


    struct LayoutSaver::LastPosition
    {
        QRect lastFloatingGeometry;
        int tabIndex;
        bool wasFloating;
        LayoutSaver::Placeholder::List placeholders;
    };

    struct DOCKS_EXPORT LayoutSaver::DockWidget
    {
        // Using shared ptr, as we need to modify shared instances
        typedef std::shared_ptr<LayoutSaver::DockWidget> Ptr;
        typedef QVector<Ptr> List;
        static QHash<QString, Ptr> s_dockWidgets;

        bool isValid() const;

        static Ptr dockWidgetForName(const QString & name)
        {
            auto dw = s_dockWidgets.value(name);
            if(dw)
                return dw;

            dw = Ptr(new LayoutSaver::DockWidget);
            dw->uniqueName = name;

            return dw;
        }

        QString uniqueName;
        LayoutSaver::LastPosition lastPosition;

    private:
        DockWidget() {}
    };

    struct LayoutSaver::Frame
    {
        bool isValid() const;

        bool isNull = true;
        QString objectName;
        QRect geometry;
        int options;
        int currentTabIndex;

        LayoutSaver::DockWidget::List dockWidgets;
    };

    struct LayoutSaver::Item
    {
        typedef QVector<LayoutSaver::Item> List;

        bool isValid(const MultiSplitterLayout &) const;

        QString objectName;
        bool isPlaceholder;
        QRect geometry;
        QSize minSize;

        int indexOfLeftAnchor;
        int indexOfTopAnchor;
        int indexOfRightAnchor;
        int indexOfBottomAnchor;

        LayoutSaver::Frame frame;
    };

    struct LayoutSaver::Anchor
    {
        typedef QVector<LayoutSaver::Anchor> List;

        bool isValid(const LayoutSaver::MultiSplitterLayout & layout) const;

        QString objectName;
        QRect geometry;
        int orientation;
        int type;
        int indexOfFrom;
        int indexOfTo;
        int indexOfFollowee;
        QVector<int> side1Items;
        QVector<int> side2Items;
    };

    struct LayoutSaver::MultiSplitterLayout
    {
        bool isValid() const;

        LayoutSaver::Anchor::List anchors;
        LayoutSaver::Item::List items;
        QSize minSize;
        QSize size;
    };

    struct LayoutSaver::FloatingWindow
    {
        typedef QVector<LayoutSaver::FloatingWindow> List;

        bool isValid() const;

        LayoutSaver::MultiSplitterLayout multiSplitterLayout;
        int parentIndex = -1;
        QRect geometry;
        bool isVisible = true;
    };

    struct LayoutSaver::MainWindow
    {
    public:
        typedef QVector<LayoutSaver::MainWindow> List;

        bool isValid() const;

        KDDockWidgets::MainWindowOptions options;
        LayoutSaver::MultiSplitterLayout multiSplitterLayout;
        QString uniqueName;
        QRect geometry;
        bool isVisible;
    };

    struct LayoutSaver::Layout
    {
    public:

        bool isValid() const;
        bool fillFrom(const QByteArray & serialized);

        int serializationVersion = KDDOCKWIDGETS_SERIALIZATION_VERSION;
        LayoutSaver::MainWindow::List mainWindows;
        LayoutSaver::FloatingWindow::List floatingWindows;
        LayoutSaver::DockWidget::List closedDockWidgets;
        LayoutSaver::DockWidget::List allDockWidgets;
    };

    inline QDataStream & operator<<(QDataStream & ds, LayoutSaver::Placeholder* p)
    {
        ds << p->isFloatingWindow;
        if(p->isFloatingWindow)
            ds << p->indexOfFloatingWindow;
        else
            ds << p->mainWindowUniqueName;

        ds << p->itemIndex;

        return ds;
    }

    inline QDataStream & operator>>(QDataStream & ds, LayoutSaver::Placeholder* p)
    {
        ds >> p->isFloatingWindow;
        if(p->isFloatingWindow)
            ds >> p->indexOfFloatingWindow;
        else
            ds >> p->mainWindowUniqueName;

        ds >> p->itemIndex;

        return ds;
    }

    inline QDataStream & operator<<(QDataStream & ds, LayoutSaver::Anchor* a)
    {
        ds << QStringLiteral(ANCHOR_MAGIC_MARKER);
        ds << a->objectName;
        ds << a->geometry;
        ds << a->orientation;
        ds << a->type;
        ds << a->indexOfFrom;
        ds << a->indexOfTo;
        ds << a->indexOfFollowee;
        ds << a->side1Items;
        ds << a->side2Items;

        return ds;
    }

    inline  QDataStream & operator>>(QDataStream & ds, LayoutSaver::Anchor* a)
    {
        QString marker;

        ds >> marker;
        if(marker != QLatin1String(ANCHOR_MAGIC_MARKER))
            qWarning() << Q_FUNC_INFO << "Corrupt stream";

        ds >> a->objectName;
        ds >> a->geometry;
        ds >> a->orientation;
        ds >> a->type;
        ds >> a->indexOfFrom;
        ds >> a->indexOfTo;
        ds >> a->indexOfFollowee;
        ds >> a->side1Items;
        ds >> a->side2Items;

        return ds;
    }

    inline QDataStream & operator<<(QDataStream & ds, LayoutSaver::Frame* frame)
    {
        ds << frame->objectName;
        ds << frame->geometry;
        ds << frame->options;
        ds << frame->currentTabIndex;
        ds << frame->dockWidgets.size();

        for(auto & dock : frame->dockWidgets)
        {
            ds << dock->uniqueName;
        }

        return ds;
    }

    inline QDataStream & operator>>(QDataStream & ds, LayoutSaver::Frame* frame)
    {
        int numDockWidgets;
        frame->dockWidgets.clear();
        frame->isNull = false;

        ds >> frame->objectName;
        ds >> frame->geometry;
        ds >> frame->options;
        ds >> frame->currentTabIndex;
        ds >> numDockWidgets;

        for(int i = 0; i < numDockWidgets; ++i)
        {
            QString name;
            ds >> name;
            auto dw = LayoutSaver::DockWidget::dockWidgetForName(name);
            frame->dockWidgets.push_back(dw);
        }

        return ds;
    }

    inline QDataStream & operator<<(QDataStream & ds, LayoutSaver::Item* item)
    {
        ds << item->objectName;
        ds << item->isPlaceholder;
        ds << item->geometry;
        ds << item->minSize;

        ds << item->indexOfLeftAnchor;
        ds << item->indexOfTopAnchor;
        ds << item->indexOfRightAnchor;
        ds << item->indexOfBottomAnchor;

        const bool hasFrame = !item->frame.isNull;
        ds << hasFrame;
        if(hasFrame)
            ds << &item->frame;

        return ds;
    }

    inline QDataStream & operator>>(QDataStream & ds, LayoutSaver::Item* item)
    {
        ds >> item->objectName;
        ds >> item->isPlaceholder;
        ds >> item->geometry;
        ds >> item->minSize;

        ds >> item->indexOfLeftAnchor;
        ds >> item->indexOfTopAnchor;
        ds >> item->indexOfRightAnchor;
        ds >> item->indexOfBottomAnchor;

        bool hasFrame;
        ds >> hasFrame;
        if(hasFrame)
        {
            ds >> &item->frame;
            item->frame.isNull = false;
        }
        else
        {
            item->frame.isNull = true;
        }

        return ds;
    }

    inline QDataStream & operator<<(QDataStream & ds, LayoutSaver::MultiSplitterLayout* l)
    {
        ds << QStringLiteral(MULTISPLITTER_LAYOUT_MAGIC_MARKER);

        ds << l->size;
        ds << l->minSize;
        ds << l->items.size();
        ds << l->anchors.size();

        for(auto & item : l->items)
            ds << &item;

        for(auto & anchor : l->anchors)
            ds << &anchor;

        return ds;
    }

    inline QDataStream & operator>>(QDataStream & ds, LayoutSaver::MultiSplitterLayout* l)
    {
        int numItems;
        int numAnchors;
        QString marker;
        ds >> marker;

        if(marker != QLatin1String(MULTISPLITTER_LAYOUT_MAGIC_MARKER))
            qWarning() << Q_FUNC_INFO << "Corrupt stream, invalid magic";

        ds >> l->size;
        ds >> l->minSize;
        ds >> numItems;
        ds >> numAnchors;

        l->items.clear();
        l->anchors.clear();

        for(int i = 0 ; i < numItems; ++i)
        {
            LayoutSaver::Item item;
            ds >> &item;
            l->items.push_back(item);
        }

        for(int i = 0 ; i < numAnchors; ++i)
        {
            LayoutSaver::Anchor a;
            ds >> &a;
            l->anchors.push_back(a);
        }

        return ds;
    }

    inline QDataStream & operator<<(QDataStream & ds, LayoutSaver::LastPosition* lp)
    {
        ds << lp->placeholders.size();

        for(auto & p : lp->placeholders)
        {
            ds << &p;
        }

        ds << lp->lastFloatingGeometry;
        ds << lp->tabIndex;
        ds << lp->wasFloating;

        return ds;
    }

    inline QDataStream & operator>>(QDataStream & ds, LayoutSaver::LastPosition* lp)
    {
        int numPlaceholders;
        ds >> numPlaceholders;

        lp->placeholders.clear();
        for(int i = 0 ; i < numPlaceholders; ++i)
        {
            LayoutSaver::Placeholder p;
            ds >> &p;
            lp->placeholders.push_back(p);
        }

        ds >> lp->lastFloatingGeometry;
        ds >> lp->tabIndex;
        ds >> lp->wasFloating;

        return ds;
    }

    inline QDataStream & operator<<(QDataStream & ds, LayoutSaver::FloatingWindow* fw)
    {
        ds << fw->parentIndex;
        ds << fw->geometry;
        ds << fw->isVisible;
        ds << &fw->multiSplitterLayout;
        return ds;
    }

    inline QDataStream & operator>>(QDataStream & ds, LayoutSaver::FloatingWindow* fw)
    {
        ds >> fw->parentIndex;
        ds >> fw->geometry;
        ds >> fw->isVisible;
        ds >> &fw->multiSplitterLayout;
        return ds;
    }

    inline QDataStream & operator<<(QDataStream & ds, LayoutSaver::MainWindow* m)
    {
        ds << m->uniqueName;
        ds << m->geometry;
        ds << m->isVisible;
        ds << m->options;
        ds << &m->multiSplitterLayout;
        return ds;
    }

    inline QDataStream & operator>>(QDataStream & ds, LayoutSaver::MainWindow* m)
    {
        ds >> m->uniqueName;
        ds >> m->geometry;
        ds >> m->isVisible;
        ds >> m->options;
        ds >> &m->multiSplitterLayout;
        return ds;
    }

    inline QDataStream & operator<<(QDataStream & ds, LayoutSaver::Layout* l)
    {
        ds << l->serializationVersion;
        ds << l->mainWindows.size();
        for(auto & m : l->mainWindows)
        {
            ds << &m;
        }

        ds << l->floatingWindows.size();
        for(auto & fw : l->floatingWindows)
        {
            ds << &fw;
        }

        ds << l->closedDockWidgets.size();
        for(auto & dw : l->closedDockWidgets)
        {
            ds << dw->uniqueName;
        }

        ds << l->allDockWidgets.size();
        for(auto & dw : l->allDockWidgets)
        {
            ds << dw->uniqueName;
            ds << &dw->lastPosition;
        }

        return ds;
    }

    inline QDataStream & operator>>(QDataStream & ds, LayoutSaver::Layout* l)
    {
        LayoutSaver::DockWidget::s_dockWidgets.clear();
        int numMainWindows;
        int numFloatingWindows;
        int numClosedDockWidgets;
        int numAllDockWidgets;

        ds >> l->serializationVersion;

        ds >> numMainWindows;
        l->mainWindows.clear();
        for(int i = 0; i < numMainWindows; ++i)
        {
            LayoutSaver::MainWindow m;
            ds >> &m;
            l->mainWindows.push_back(m);
        }

        ds >> numFloatingWindows;
        l->floatingWindows.clear();
        for(int i = 0; i < numFloatingWindows; ++i)
        {
            LayoutSaver::FloatingWindow m;
            ds >> &m;
            l->floatingWindows.push_back(m);
        }

        ds >> numClosedDockWidgets;
        l->closedDockWidgets.clear();
        for(int i = 0; i < numClosedDockWidgets; ++i)
        {
            QString name;
            ds >> name;
            auto dw = LayoutSaver::DockWidget::dockWidgetForName(name);
            l->closedDockWidgets.push_back(dw);
        }

        ds >> numAllDockWidgets;
        l->allDockWidgets.clear();
        for(int i = 0; i < numAllDockWidgets; ++i)
        {
            QString name;
            ds >> name;

            auto dw = LayoutSaver::DockWidget::dockWidgetForName(name);
            ds >> &dw->lastPosition;
            l->allDockWidgets.push_back(dw);
        }

        return ds;
    }

}

#endif
