#pragma once

#include <QInputDialog>
#include "Types.h"

class GotoDialog : public QInputDialog
{
public:
    GotoDialog(QWidget* parent = nullptr, bool checkAddress = true);
    using QInputDialog::exec;

    duint address() const;

protected:
    void done(int r) override;

private:
    bool mCheckAddress = true;
    duint mAddress = 0;
};
