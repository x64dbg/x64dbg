#ifndef ACTIONHELPERS_H
#define ACTIONHELPERS_H

#include <QAction>
#include <functional>
#include "Configuration.h"

//TODO: find the right "const &" "&", "&&" "" etc for passing around std::function
using SlotFunc = std::function<void()>;
using MakeMenuFunc1 = std::function<QMenu*(const QString &)>;
using MakeMenuFunc2 = std::function<QMenu*(const QIcon &, const QString &)>;
using MakeActionFunc1 = std::function<QAction*(const QString &, const SlotFunc &)>;
using MakeActionFunc2 = std::function<QAction*(const QIcon &, const QString &, const SlotFunc &)>;
using MakeShortcutActionFunc1 = std::function<QAction*(const QString &, const SlotFunc &, const char*)>;
using MakeShortcutActionFunc2 = std::function<QAction*(const QIcon &, const QString &, const SlotFunc &, const char*)>;
using MakeMenuActionFunc1 = std::function<QAction*(QMenu*, const QString &, const SlotFunc &)>;
using MakeMenuActionFunc2 = std::function<QAction*(QMenu*, const QIcon &, const QString &, const SlotFunc &)>;
using MakeShortcutMenuActionFunc1 = std::function<QAction*(QMenu*, const QString &, const SlotFunc &, const char*)>;
using MakeShortcutMenuActionFunc2 = std::function<QAction*(QMenu*, const QIcon &, const QString &, const SlotFunc &, const char*)>;

struct ActionHelperFuncs
{
    MakeMenuFunc1 makeMenu1;
    MakeMenuFunc2 makeMenu2;
    MakeActionFunc1 makeAction1;
    MakeActionFunc2 makeAction2;
    MakeShortcutActionFunc1 makeShortcutAction1;
    MakeShortcutActionFunc2 makeShortcutAction2;
    MakeMenuActionFunc1 makeMenuAction1;
    MakeMenuActionFunc2 makeMenuAction2;
    MakeShortcutMenuActionFunc1 makeShortcutMenuAction1;
    MakeShortcutMenuActionFunc2 makeShortcutMenuAction2;
};

template<class Base>
class ActionHelper
{
private:
    Base* getBase()
    {
        return static_cast<Base*>(this);
    }

    struct ActionShortcut
    {
        QAction* action;
        QString shortcut;

        inline ActionShortcut(QAction* action, const char* shortcut)
            : action(action),
              shortcut(shortcut)
        {
        }
    };

public:
    virtual void updateShortcuts()
    {
        for(const auto & actionShortcut : actionShortcutPairs)
            actionShortcut.action->setShortcut(ConfigShortcut(actionShortcut.shortcut));
    }

private:
    inline QAction* connectAction(QAction* action, const char* slot)
    {
        QObject::connect(action, SIGNAL(triggered(bool)), getBase(), slot);
        return action;
    }

    template<class T> // lambda or base member pointer
    inline QAction* connectAction(QAction* action, T callback)
    {
        //in case of a lambda getBase() is used as the 'context' object and not the 'receiver'
        QObject::connect(action, &QAction::triggered, getBase(), callback);
        return action;
    }

    inline QAction* connectShortcutAction(QAction* action, const char* shortcut)
    {
        actionShortcutPairs.push_back(ActionShortcut(action, shortcut));
        action->setShortcut(ConfigShortcut(shortcut));
        action->setShortcutContext(Qt::WidgetShortcut);
        getBase()->addAction(action);
        return action;
    }

    inline QAction* connectMenuAction(QMenu* menu, QAction* action)
    {
        menu->addAction(action);
        return action;
    }

protected:
    inline ActionHelperFuncs getActionHelperFuncs()
    {
        ActionHelperFuncs funcs;
        funcs.makeMenu1 = [this](const QString & title)
        {
            return makeMenu(title);
        };
        funcs.makeMenu2 = [this](const QIcon & icon, const QString & title)
        {
            return makeMenu(icon, title);
        };
        funcs.makeAction1 = [this](const QString & text, const SlotFunc & slot)
        {
            return makeAction(text, slot);
        };
        funcs.makeAction2 = [this](const QIcon & icon, const QString & text, const SlotFunc & slot)
        {
            return makeAction(icon, text, slot);
        };
        funcs.makeShortcutAction1 = [this](const QString & text, const SlotFunc & slot, const char* shortcut)
        {
            return makeShortcutAction(text, slot, shortcut);
        };
        funcs.makeShortcutAction2 = [this](const QIcon & icon, const QString & text, const SlotFunc & slot, const char* shortcut)
        {
            return makeShortcutAction(icon, text, slot, shortcut);
        };
        funcs.makeMenuAction1 = [this](QMenu * menu, const QString & text, const SlotFunc & slot)
        {
            return makeMenuAction(menu, text, slot);
        };
        funcs.makeMenuAction2 = [this](QMenu * menu, const QIcon & icon, const QString & text, const SlotFunc & slot)
        {
            return makeMenuAction(menu, icon, text, slot);
        };
        funcs.makeShortcutMenuAction1 = [this](QMenu * menu, const QString & text, const SlotFunc & slot, const char* shortcut)
        {
            return makeShortcutMenuAction(menu, text, slot, shortcut);
        };
        funcs.makeShortcutMenuAction2 = [this](QMenu * menu, const QIcon & icon, const QString & text, const SlotFunc & slot, const char* shortcut)
        {
            return makeShortcutMenuAction(menu, icon, text, slot, shortcut);
        };
        return funcs;
    }

    inline QMenu* makeMenu(const QString & title)
    {
        return new QMenu(title, getBase());
    }

    inline QMenu* makeMenu(const QIcon & icon, const QString & title)
    {
        QMenu* menu = new QMenu(title, getBase());
        menu->setIcon(icon);
        return menu;
    }

    template<typename T>
    inline QAction* makeAction(const QString & text, T slot)
    {
        return connectAction(new QAction(text, getBase()), slot);
    }

    template<typename T>
    inline QAction* makeAction(const QIcon & icon, const QString & text, T slot)
    {
        return connectAction(new QAction(icon, text, getBase()), slot);
    }

    template<typename T>
    inline QAction* makeShortcutAction(const QString & text, T slot, const char* shortcut)
    {
        return connectShortcutAction(makeAction(text, slot), shortcut);
    }

    template<typename T>
    inline QAction* makeShortcutAction(const QIcon & icon, const QString & text, T slot, const char* shortcut)
    {
        return connectShortcutAction(makeAction(icon, text, slot), shortcut);
    }

    template<typename T>
    inline QAction* makeMenuAction(QMenu* menu, const QString & text, T slot)
    {
        return connectMenuAction(menu, makeAction(text, slot));
    }

    template<typename T>
    inline QAction* makeMenuAction(QMenu* menu, const QIcon & icon, const QString & text, T slot)
    {
        return connectMenuAction(menu, makeAction(icon, text, slot));
    }

    template<typename T>
    inline QAction* makeShortcutMenuAction(QMenu* menu, const QString & text, T slot, const char* shortcut)
    {
        return connectShortcutAction(makeMenuAction(menu, text, slot), shortcut);
    }

    template<typename T>
    inline QAction* makeShortcutMenuAction(QMenu* menu, const QIcon & icon, const QString & text, T slot, const char* shortcut)
    {
        return connectShortcutAction(makeMenuAction(menu, icon, text, slot), shortcut);
    }

private:
    std::vector<ActionShortcut> actionShortcutPairs;
};

class MenuBuilder;

class ActionHelperProxy
{
    ActionHelperFuncs funcs;

public:
    ActionHelperProxy(ActionHelperFuncs funcs)
        : funcs(funcs) { }

protected:
    inline QMenu* makeMenu(const QString & title)
    {
        return funcs.makeMenu1(title);
    }

    inline QMenu* makeMenu(const QIcon & icon, const QString & title)
    {
        return funcs.makeMenu2(icon, title);
    }

    inline QAction* makeAction(const QString & text, const SlotFunc & slot)
    {
        return funcs.makeAction1(text, slot);
    }

    inline QAction* makeAction(const QIcon & icon, const QString & text, const SlotFunc & slot)
    {
        return funcs.makeAction2(icon, text, slot);
    }

    inline QAction* makeShortcutAction(const QString & text, const SlotFunc & slot, const char* shortcut)
    {
        return funcs.makeShortcutAction1(text, slot, shortcut);
    }

    inline QAction* makeShortcutAction(const QIcon & icon, const QString & text, const SlotFunc & slot, const char* shortcut)
    {
        return funcs.makeShortcutAction2(icon, text, slot, shortcut);
    }

    inline QAction* makeMenuAction(QMenu* menu, const QString & text, const SlotFunc & slot)
    {
        return funcs.makeMenuAction1(menu, text, slot);
    }

    inline QAction* makeMenuAction(QMenu* menu, const QIcon & icon, const QString & text, const SlotFunc & slot)
    {
        return funcs.makeMenuAction2(menu, icon, text, slot);
    }

    inline QAction* makeShortcutMenuAction(QMenu* menu, const QString & text, const SlotFunc & slot, const char* shortcut)
    {
        return funcs.makeShortcutMenuAction1(menu, text, slot, shortcut);
    }

    inline QAction* makeShortcutMenuAction(QMenu* menu, const QIcon & icon, const QString & text, const SlotFunc & slot, const char* shortcut)
    {
        return funcs.makeShortcutMenuAction2(menu, icon, text, slot, shortcut);
    }
};

#endif
