#include "GotoDialog.h"
#include "Bridge.h"

GotoDialog::GotoDialog(QWidget* parent, bool checkAddress)
    : QInputDialog(parent)
    , mCheckAddress(checkAddress)
{
    setInputMode(InputMode::TextInput);
    setLabelText(tr("Address:"));
    setWindowTitle(tr("Goto"));
}

duint GotoDialog::address() const
{
    return mAddress;
}

void GotoDialog::done(int r)
{
    if(r == Accepted)
    {
        auto ok = false;
        mAddress = textValue().toLongLong(&ok, 16);
        if(!ok)
            return;
        if(mCheckAddress && !DbgMemIsValidReadPtr(mAddress))
            return;
    }
    QInputDialog::done(r);
}
