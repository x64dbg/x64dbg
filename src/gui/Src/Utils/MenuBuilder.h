#ifndef MENUBUILDER
#define MENUBUILDER

#include <QAction>
#include <QMenu>
#include <functional>

class MenuBuilder : public QObject
{
    Q_OBJECT
public:
    typedef std::function<bool(QMenu*)> BuildCallback;

    inline MenuBuilder(QObject* parent, BuildCallback callback = nullptr)
        : QObject(parent),
          _callback(callback)
    {
    }

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

    inline QMenu* addMenu(QMenu* submenu, BuildCallback callback)
    {
        addBuilder(new MenuBuilder(submenu->parent(), [submenu, callback](QMenu * menu)
        {
            submenu->clear();
            if(callback(submenu))
                menu->addMenu(submenu);
            return true;
        }));
        return submenu;
    }

    inline QMenu* addMenu(QMenu* submenu, MenuBuilder* builder)
    {
        addBuilder(new MenuBuilder(submenu->parent(), [submenu, builder](QMenu * menu)
        {
            submenu->clear();
            builder->build(submenu);
            menu->addMenu(submenu);
            return true;
        }));
        return submenu;
    }

    inline MenuBuilder* addBuilder(MenuBuilder* builder)
    {
        _containers.push_back(Container(builder));
        return builder;
    }

    inline void build(QMenu* menu) const
    {
        if(_callback && !_callback(menu))
            return;
        for(const Container & container : _containers)
        {
            switch(container.type)
            {
            case Container::Separator:
                menu->addSeparator();
                break;
            case Container::Action:
                menu->addAction(container.action);
                break;
            case Container::Menu:
                menu->addMenu(container.menu);
                break;
            case Container::Builder:
                container.builder->build(menu);
                break;
            default:
                break;
            }
        }
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
            : type(Separator)
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
            QMenu* menu ;
            MenuBuilder* builder;
        };
    };

    BuildCallback _callback;
    std::vector<Container> _containers;
};

#endif // MENUBUILDER

