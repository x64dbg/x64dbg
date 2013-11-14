#include <QApplication>
#include <QtGui>
#include "MainWindow.h"
#include "NewTypes.h"
#include "Bridge.h"


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Register custom data types
    //qRegisterMetaType<int32>("int32");
    //qRegisterMetaType<uint_t>("uint_t");

    qRegisterMetaType<int_t>("int_t");
    qRegisterMetaType<uint_t>("uint_t");

    qRegisterMetaType<byte_t>("byte_t");

    qRegisterMetaType<DBGSTATE>("DBGSTATE");

    // Init communication with debugger
    Bridge::initBridge();

    // Start GUI
    MainWindow w;
    w.show();

    return a.exec();
}


