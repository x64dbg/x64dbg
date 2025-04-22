#pragma once

#include <type_traits>

#include "Utils/MenuBuilder.h"
#include <QCoreApplication>
#include <QDebug>

class AbstractTableView;

// TODO: support QT_TR_NOOP in linguist
#define TR(str) TranslatedText(str)

template<class Base>
class MagicMenu
{
    MenuBuilder* mMenuBuilder = nullptr;

    Base* base()
    {
        return static_cast<Base*>(this);
    }

public:
    struct TranslatedText
    {
        const char* text = nullptr;

        explicit TranslatedText(const char* text)
            : text(text)
        {
        }

        TranslatedText(const TranslatedText &) = delete;
        TranslatedText(TranslatedText &&) = delete;
    };

    // TODO: see if it's possible to support ActionDescription("Goto", "G") directly in linguist
    struct TableAction
    {
        const char* name = nullptr;
        const char* defaultShortcut = nullptr;

        TableAction(const TranslatedText & name, const char* defaultShortcut = nullptr)
            : name(name.text), defaultShortcut(defaultShortcut)
        {
        }

        TableAction(const TableAction &) = delete;
        TableAction(TableAction &&) = delete;

        QAction* buildAction(QWidget* widget) const
        {
            auto className = widget->metaObject()->className();
            qDebug() << "Register shortcut action:" << className << ":" << name;
            auto qAction = new QAction(QCoreApplication::translate(className, name));
            // TODO: set shortcut based on configuration
            qAction->setShortcut(QKeySequence(defaultShortcut));
            qAction->setShortcutContext(Qt::WidgetShortcut);
            // TODO: allow shortcut to update dynamically
            widget->addAction(qAction);
            return qAction;
        }
    };

    MagicMenu()
    {
        mMenuBuilder = new MenuBuilder(base());

        base()->setContextMenuPolicy(Qt::CustomContextMenu);
        QObject::connect(base(), &QWidget::customContextMenuRequested, [this](const QPoint & pos)
        {
            if constexpr(std::is_base_of_v<AbstractTableView, Base>)
            {
                if(pos.y() < base()->getHeaderHeight())
                    return;
            }
            auto wMenu = new QMenu(base());
            mMenuBuilder->build(wMenu);
            if(wMenu->actions().length())
            {
                wMenu->popup(base()->mapToGlobal(pos));
            }
        });
    }

    MenuBuilder* menuBuilder()
    {
        return mMenuBuilder;
    }

    // TODO: support conditional actions
    template<typename SlotFn>
    void addMenuAction(const TableAction & action, SlotFn && slot)
    {
        auto qAction = action.buildAction(base());
        QObject::connect(qAction, &QAction::triggered, base(), slot);
        mMenuBuilder->addAction(qAction);
    }

    template<typename SlotFn>
    void addMenuAction(const TableAction & action, SlotFn && slot, const std::function<bool()> & condition)
    {
        auto qAction = action.buildAction(base());
        QObject::connect(qAction, &QAction::triggered, base(), slot);
        mMenuBuilder->addAction(qAction, [condition](QMenu*)
        {
            return condition();
        });
    }

    /*void addMenuAction(const TableAction& action, const char* slot)
    {
        auto qAction = action.buildAction(base());
        QObject::connect(qAction, SIGNAL(triggered(bool)), base(), SLOT(slot));
        mMenuBuilder->addAction(qAction);
    }*/

    void addMenuBuilder(const TranslatedText & title, MenuBuilder::BuildCallback && callback)
    {
        auto trTitle = QCoreApplication::translate(base()->metaObject()->className(), title.text);
        auto qMenu = new QMenu(trTitle, base());
        mMenuBuilder->addMenu(qMenu, std::move(callback));
    }
};
