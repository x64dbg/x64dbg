#include "MenuBuilder.h"
#include "Bridge.h"
#include "Configuration.h"

/**
 * @brief MenuBuilder::loadFromConfig Set the menu builder to be customizable
 * @param id The id of menu builder. It should be the same on every same menu.
 */
void MenuBuilder::loadFromConfig()
{
    this->id = parent()->metaObject()->className(); // Set the ID first because the following subroutine will use it
    if(Config()->registerMenuBuilder(this, _containers.size())) // Register it to the config so the customization dialog can get the text of actions here.
        connect(this, SIGNAL(destroyed()), this, SLOT(unregisterMenuBuilder())); // Remember to unregister menu builder
}

void MenuBuilder::unregisterMenuBuilder()
{
    Config()->unregisterMenuBuilder(this);
}

QMenu* MenuBuilder::addMenu(QMenu* submenu, BuildCallback callback)
{
    addBuilder(new MenuBuilder(submenu->parent(), [submenu, callback](QMenu*)
    {
        submenu->clear();
        return callback(submenu);
    }))->addMenu(submenu);
    return submenu;
}

QMenu* MenuBuilder::addMenu(QMenu* submenu, MenuBuilder* builder)
{
    addBuilder(new MenuBuilder(submenu->parent(), [submenu, builder](QMenu*)
    {
        submenu->clear();
        return builder->build(submenu);
    }))->addMenu(submenu);
    return submenu;
}

/**
 * @brief MenuBuilder::getText Get the title of id-th element. This function is called by CustomizeMenuDialog to initialize the dialog.
 * @param id The index of the element in "_containers"
 * @return The title, or empty. If it is empty, that element will not appear in the CustomizeMenuDialog.
 */
QString MenuBuilder::getText(size_t id) const
{
    const Container & container = _containers.at(id);
    switch(container.type)
    {
    case Container::Action:
        return container.action->text();
    case Container::Menu:
        return container.menu->title();
    case Container::Builder:
    {
        if(container.builder->_containers.size() == 1)
            return container.builder->getText(0); // recursively get the text inside the menu builder
        else
            return QString();
    }
    default: // separator
        return QString();
    }
}

/**
 * @brief MenuBuilder::build Build the menu with contents stored in the menu builder
 * @param menu The menu to build.
 * @return true if the callback succeeds, false if the callback returns false.
 */
bool MenuBuilder::build(QMenu* menu) const
{
    if(_callback && !_callback(menu))
        return false;
    QMenu* submenu;
    if(id != 0)
        submenu = new QMenu(tr("More commands"), menu);
    else
        submenu = nullptr;
    for(size_t i = 0; i < _containers.size(); i++)
    {
        const Container & container = _containers.at(i);
        QMenu* _menu;
        if(id != 0 && container.type != Container::Separator && Config()->getBool("Gui", QString("Menu%1Hidden%2").arg(id).arg(i)))
            _menu = submenu;
        else
            _menu = menu;
        switch(container.type)
        {
        case Container::Separator:
            _menu->addSeparator();
            break;
        case Container::Action:
            _menu->addAction(container.action);
            break;
        case Container::Menu:
            _menu->addMenu(container.menu);
            break;
        case Container::Builder:
            container.builder->build(_menu);
            break;
        default:
            break;
        }
    }
    if(id != 0 && !submenu->actions().isEmpty())
    {
        menu->addSeparator();
        menu->addMenu(submenu);
    }
    else if(submenu)
    {
        delete submenu;
    }
    return true;
}
