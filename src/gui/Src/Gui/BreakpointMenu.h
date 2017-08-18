#pragma once

#include <QObject>
#include <functional>
#include "ActionHelpers.h"
#include "Imports.h"

class BreakpointMenu : public QObject, public ActionHelperProxy
{
    Q_OBJECT
public:
    using GetSelectionFunc = std::function<duint()>;

    explicit BreakpointMenu(QObject* parent, ActionHelperFuncs funcs, GetSelectionFunc getSelection);
    void build(MenuBuilder* builder);

private:
    GetSelectionFunc mGetSelection;

    void testSlot();
};
