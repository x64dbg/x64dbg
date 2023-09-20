#pragma once

#include <QDialog>

namespace Ui
{
    class CodepageSelectionDialog;
}

class CodepageSelectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CodepageSelectionDialog(QWidget* parent = nullptr);
    ~CodepageSelectionDialog();
    QByteArray getSelectedCodepage();

private:
    Ui::CodepageSelectionDialog* ui;
    QList<QByteArray> mCodepages;
};
