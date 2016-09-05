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

std::vector<ActionShortcut> actionShortcutPairs;

inline QAction* connectAction(QAction* action, const char* slot)
{
    connect(action, SIGNAL(triggered(bool)), this, slot);
    return action;
}

inline QAction* connectAction(QAction* action, QActionLambda::TriggerCallback callback)
{
    auto lambda = new QActionLambda(action->parent(), callback);
    connect(action, SIGNAL(triggered(bool)), lambda, SLOT(triggeredSlot()));
    return action;
}

inline QAction* connectShortcutAction(QAction* action, const char* shortcut)
{
    actionShortcutPairs.push_back(ActionShortcut(action, shortcut));
    action->setShortcut(ConfigShortcut(shortcut));
    action->setShortcutContext(Qt::WidgetShortcut);
    addAction(action);
    return action;
}

inline QAction* connectMenuAction(QMenu* menu, QAction* action)
{
    menu->addAction(action);
    return action;
}

inline QMenu* makeMenu(const QString & title)
{
    return new QMenu(title, this);
}

inline QMenu* makeMenu(const QIcon & icon, const QString & title)
{
    QMenu* menu = new QMenu(title, this);
    menu->setIcon(icon);
    return menu;
}

template<typename T>
inline QAction* makeAction(const QString & text, T slot)
{
    return connectAction(new QAction(text, this), slot);
}

template<typename T>
inline QAction* makeAction(const QIcon & icon, const QString & text, T slot)
{
    return connectAction(new QAction(icon, text, this), slot);
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
