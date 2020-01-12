#ifndef DLLEXPORTCHOOSER_H
#define DLLEXPORTCHOOSER_H

#include <QDialog>
#include <bridge/bridgemain.h>
#include <ui_DLLExportChooser.h>

class DLLExportChooser : public QDialog
{
    Q_OBJECT

public:
    explicit DLLExportChooser(QWidget* parent, SYMBOLINFO* exportList, unsigned int numberOfExports);
    ~DLLExportChooser();
private:
    Ui::DLLExportChooser* ui;
    SYMBOLINFO* exportList;
    unsigned int numberOfExports;
};

#endif // DLLEXPORTCHOOSER_H
