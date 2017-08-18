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

    explicit BreakpointMenu(QWidget* parent, ActionHelperFuncs funcs, GetSelectionFunc getSelection);
    void build(MenuBuilder* builder);

public slots:
    void toggleInt3BPActionSlot();
    void editSoftBpActionSlot();
    void toggleHwBpActionSlot();
    void setHwBpOnSlot0ActionSlot();
    void setHwBpOnSlot1ActionSlot();
    void setHwBpOnSlot2ActionSlot();
    void setHwBpOnSlot3ActionSlot();
    void setHwBpAt(duint va, int slot);

private:
    GetSelectionFunc mGetSelection;
};
