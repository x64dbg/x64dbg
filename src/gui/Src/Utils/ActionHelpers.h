#ifndef ACTIONHELPERS_H
#define ACTIONHELPERS_H

#include <QAction>
#include <functional>

//TODO: add proxy declarations for the make* functions in ActionHelper, also in ActionHelperFuncs
//TODO: find the right "const &" "&", "&&" "" etc for std::function
using SlotFunc = std::function<void()>;
using MakeActionFunc = std::function<QAction*(const QIcon &, const QString &, const SlotFunc &)>;

struct ActionHelperFuncs
{
    MakeActionFunc makeAction;
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
        funcs.makeAction = [this](const QIcon & icon, const QString & text, const SlotFunc & slot)
        {
            return makeAction(icon, text, slot);
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
    void setupContextMenu(MenuBuilder* builder, ActionHelperFuncs funcs)
    {
        this->funcs = funcs;
        buildMenu(builder);
    }

protected:
    virtual void buildMenu(MenuBuilder* builder) = 0;

    inline QAction* makeAction(const QIcon & icon, const QString & text, const SlotFunc & slot)
    {
        return funcs.makeAction(icon, text, slot);
    }
};

#endif
