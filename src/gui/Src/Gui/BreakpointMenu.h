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
    explicit BreakpointMenu(QObject* parent, GetSelectionFunc getSelection);

protected:
    void buildMenu(MenuBuilder* builder) override;

private:
    GetSelectionFunc mGetSelection;
};
