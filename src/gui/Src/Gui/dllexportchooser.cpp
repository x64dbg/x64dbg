#include "DLLExportChooser.h"

DLLExportChooser::DLLExportChooser(QWidget* parent, SYMBOLINFO* exportList, unsigned int numberOfExports) :
    QDialog(parent),
    ui(new Ui::DLLExportChooser),
    exportList(exportList),
    numberOfExports(numberOfExports)
{
    ui->setupUi(this);
    for(auto i = 0 ; i < numberOfExports ; ++i)
    {
        ui->lstExports->addItem(exportList[i].decoratedSymbol);
    }
}

DLLExportChooser::~DLLExportChooser()
{
    BridgeFree(exportList);
}
