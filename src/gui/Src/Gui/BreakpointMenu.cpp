#include "BreakpointMenu.h"
#include "MenuBuilder.h"
#include <QMessageBox>
#include "StringUtil.h"
#include "MiscUtil.h"

BreakpointMenu::BreakpointMenu(QObject* parent, GetSelectionFunc getSelection)
    : QObject(parent), mGetSelection(getSelection)
{
}

void BreakpointMenu::buildMenu(MenuBuilder* builder)
{
    builder->addAction(makeAction(DIcon("bug.png"), tr("OMG?!"), [this]()
    {
        QMessageBox::information((QWidget*)this->parent(), "IT WORKEDZ", QString("selection: %1").arg(ToPtrString(mGetSelection())));
    }));
}
