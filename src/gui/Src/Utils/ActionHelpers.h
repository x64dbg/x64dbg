#ifndef ACTIONHELPERS_H
#define ACTIONHELPERS_H

#include <QAction>

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

#endif
