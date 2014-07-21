#include <QtGui>
#include "MainWindow.h"
#include "NewTypes.h"
#include "Bridge.h"
#include "main.h"

MyApplication::MyApplication(int& argc, char** argv) : QApplication(argc, argv)
{
}

bool MyApplication::winEventFilter(MSG* message, long* result)
{
    return DbgWinEvent(message, result);
}

bool MyApplication::notify(QObject* receiver, QEvent* event)
{
    bool done = true;
    try
    {
        done = QApplication::notify(receiver, event);
    }
    catch (const std::exception& ex)
    {
        const char* message=QString().sprintf("Fatal GUI Exception: %s!\n", ex.what()).toUtf8().constData();
        GuiAddLogMessage(message);
        puts(message);
        OutputDebugStringA(message);
    }
    catch (...)
    {
        const char* message="Fatal GUI Exception: (...)!\n";
        GuiAddLogMessage(message);
        puts(message);
        OutputDebugStringA(message);
    }
    return done;
}


int main(int argc, char *argv[])
{
    MyApplication a(argc, argv);

    // Register custom data types
    //qRegisterMetaType<int32>("int32");
    //qRegisterMetaType<uint_t>("uint_t");

    qRegisterMetaType<int_t>("int_t");
    qRegisterMetaType<uint_t>("uint_t");

    qRegisterMetaType<byte_t>("byte_t");
    qRegisterMetaType<DBGSTATE>("DBGSTATE");

    qRegisterMetaType<DBGSTATE>("DBGSTATE");

    // Init communication with debugger
    Bridge::initBridge();

    // Start GUI
    MainWindow w;
    w.show();

    // Set some data
    Bridge::getBridge()->winId=(void*)w.winId();

    // Init debugger
    const char* errormsg=DbgInit();
    if(errormsg)
    {
        QMessageBox msg(QMessageBox::Critical, "DbgInit Error!", QString(errormsg));
        msg.setWindowIcon(QIcon(":/icons/images/compile-error.png"));
        msg.setWindowFlags(msg.windowFlags()&(~Qt::WindowContextHelpButtonHint));
        msg.exec();
        ExitProcess(1);
    }

    return a.exec();
}


