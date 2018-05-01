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
    void showKeepSize(bool show);
    void updateCodepage();

    bool entireBlock();

    QHexEdit* mHexEdit;

private slots:
    void updateStyle();
    void on_chkKeepSize_toggled(bool checked);
    void dataChangedSlot();
    void dataEditedSlot();
    void on_lineEditAscii_dataEdited();
    void on_lineEditUnicode_dataEdited();
    void on_lineEditCodepage_dataEdited();
    void on_btnCodepage_clicked();

private:
    Ui::HexEditDialog* ui;
    void updateCodepage(const QByteArray & name);

    bool mDataInitialized;

    QByteArray resizeData(QByteArray & data);
    bool checkDataRepresentable(int mode); //1=ASCII, 2=Unicode, 3=User-selected codepage, 4=String editor, others(0)=All modes
};

#endif // HEXEDITDIALOG_H
