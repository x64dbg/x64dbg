#ifndef MENUBUILDER
#define MENUBUILDER

#include <QAction>
#include <QMenu>
#include <functional>
#include "ClickableMenuFilter.h"

/**
 * @brief The MenuBuilder class implements the dynamic context menu system for many views.
 */
class MenuBuilder : public QObject
{
    Q_OBJECT
public:
    typedef std::function<bool(QMenu*)> BuildCallback;

    inline MenuBuilder(QObject* parent, BuildCallback callback = nullptr)
        : QObject(parent),
          _callback(callback),
          _clickableMenuFilter(new ClickableMenuFilter(this))
    {
    }

    void loadFromConfig();

    inline void addSeparator()
    {
        _containers.push_back(Container());
    }

    inline QAction* addAction(QAction* action)
    {
        _containers.push_back(Container(action));
        return action;
    }

    inline QAction* addAction(QAction* action, BuildCallback callback)
    {
        addBuilder(new MenuBuilder(action->parent(), callback))->addAction(action);
        return action;
    }

    inline QMenu* addMenu(QMenu* menu)
    {
        _containers.push_back(Container(menu));
        return menu;
    }

    QMenu* addMenu(QMenu* submenu, BuildCallback callback);

    QMenu* addMenu(QMenu* submenu, MenuBuilder* builder);

    inline MenuBuilder* addBuilder(MenuBuilder* builder)
    {
        _containers.push_back(Container(builder));
        return builder;
    }

    QString getText(size_t id) const;

    QString getId() const
    {
        return id;
    }

    bool build(QMenu* menu) const;

    inline bool empty() const
    {
        return _containers.empty();
    }

private:
    struct Container
    {
        enum Type
        {
            Separator,
            Action,
            Menu,
            Builder
        };

        inline Container()
            : type(Separator),
              action(nullptr)
        {
        }

        inline Container(QAction* action)
            : type(Action),
              action(action)
        {
        }

        inline Container(QMenu* menu)
            : type(Menu),
              menu(menu)
        {
        }

        inline Container(MenuBuilder* builder)
            : type(Builder),
              builder(builder)
        {
        }

        Type type;
        union
        {
            QAction* action;
            QMenu* menu;
            MenuBuilder* builder;
        };
    };

    BuildCallback _callback;
    QString id;
    std::vector<Container> _containers;
    ClickableMenuFilter* _clickableMenuFilter;
};

#endif // MENUBUILDER

