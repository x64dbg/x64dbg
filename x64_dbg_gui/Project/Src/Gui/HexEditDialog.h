#ifndef HEXEDITDIALOG_H
#define HEXEDITDIALOG_H

#include <QDialog>
#include "QHexEdit/QHexEdit.h"

namespace Ui
{
class HexEditDialog;
}

class HexEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HexEditDialog(QWidget* parent = 0);
    ~HexEditDialog();

    void showEntireBlock(bool show);
    bool entireBlock();

    QHexEdit* mHexEdit;

private slots:
    void updateStyle();
    void on_btnAscii2Hex_clicked();
    void on_btnUnicode2Hex_clicked();
    void on_chkKeepSize_toggled(bool checked);
    void dataChangedSlot();

private:
    Ui::HexEditDialog* ui;
};

#endif // HEXEDITDIALOG_H
