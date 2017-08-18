#include "BreakpointMenu.h"
#include "MenuBuilder.h"
#include <QMessageBox>
#include "StringUtil.h"
#include "MiscUtil.h"

BreakpointMenu::BreakpointMenu(QObject* parent, ActionHelperFuncs funcs, GetSelectionFunc getSelection)
    : QObject(parent), ActionHelperProxy(funcs), mGetSelection(getSelection)
{
}

void BreakpointMenu::build(MenuBuilder* builder)
{
    builder->addAction(makeShortcutAction(DIcon("bug.png"), tr("OMG?!"), std::bind(&BreakpointMenu::testSlot, this), "ActionToggleBreakpoint"));
}

void BreakpointMenu::testSlot()
{
    QMessageBox::information((QWidget*)this->parent(), "IT WORKEDZ", QString("selection: %1").arg(ToPtrString(mGetSelection())));
}
