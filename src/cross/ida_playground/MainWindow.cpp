#include "MainWindow.h"
#include "./ui_MainWindow.h"
#include "Disassembler/Architecture.h"
#include <QDebug>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QSplitter>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QCheckBox>
#include <QSpacerItem>
#include <QTimer>
#include <QThread>

// HACK: IDA defines these Qt functions in their headers, so we rename them to avoid conflicts
#define qalloc ida_qalloc
#define qrealloc ida_qrealloc
#define qcalloc ida_qcalloc
#define qfree ida_qfree
#define qstrdup ida_qstrdup
#define qstrlen ida_qstrlen
#define qstrncmp ida_qstrncmp
#define qstrncpy ida_qstrncpy
#define qsnprintf ida_qsnprintf
#define qvsnprintf ida_qvsnprintf

#define USE_DANGEROUS_FUNCTIONS // necessary to avoid renaming certain functions

//#include <importme.hpp>
#include <pro.h>
#include <prodir.h>
#include <ida.hpp>
#include <auto.hpp>
#include <expr.hpp>
#include <name.hpp>
#include <undo.hpp>
#include <name.hpp>
#include <diskio.hpp>
#include <loader.hpp>
#include <dirtree.hpp>
#include <kernwin.hpp>
#include <segment.hpp>
#include <parsejson.hpp>
#include <idalib.hpp>

#ifndef QT_NO_EMIT
#undef emit
#endif // QT_NO_EMIT
#include <hexrays.hpp>
#ifndef QT_NO_EMIT
#define emit
#endif // QT_NO_EMIT

extern "C" bool idalib_resolve();

struct my_event_listener : event_listener_t
{
    ssize_t idaapi on_event(ssize_t code, va_list va)
    {
        auto ui_code = (ui_notification_t)code;
        switch(ui_code)
        {
    case ui_msg:
            {
                const char* format = va_arg(va, const char*);
                va_list args = va_arg(va, va_list);
                qDebug() << "ui_msg:" << format;
                // TODO: format message
            }
            break;

    case ui_screenea:
            {
                // TODO: this is also implemented by idalib
            }
            break;

    case ui_broadcast:
            {
                // TODO: magic number 0x1DA11B00000000 (IDALIB_API_MAGIC) says we are running under idalib
            }
            break;

    default:
                break;
        }

        //qDebug() << "on_event" << code;
        return 0;
    }
} g_my_event_listener;

MainWindow::MainWindow(QWidget* parent)
: QMainWindow(parent)
, ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setupNavigation();
    setupWidgets();

    mIdaThread = new SimpleFunctionThread(this);
    mIdaThread->addFunction([this]
    {
        // TODO: allow interactively selecting IDA installation folder
        idalib_resolve();

        // Initialize idalib
        qDebug() << "before init_library()";
        auto status = init_library();
        qDebug() << "after init_library()";

        if(status != 0)
        {
            QMessageBox::critical(this, tr("Error"), tr("Failed to initialize idalib: %1").arg(status));
            QApplication::quit();
            return;
        }

        enable_console_messages(true);
        int major = 0, minor = 0, build = 0;
        get_library_version(major, minor, build);
        qDebug() << "idalib:" << major << minor << build;

        bool event_hooked = false;
        //event_hooked = hook_event_listener(HT_UI, &g_my_event_listener, this, HKCB_GLOBAL);
        qDebug() << "event_hooked" << event_hooked;
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::loadFile(const QString & path)
{
    /*
    auto status = open_database(path.toUtf8().constData(), true);
    if(status != 0)
    {
        QMessageBox::critical(this, tr("Error"), tr("Failed to open database: 0x%1").arg(status, 0, 16));
        return;
    }
    */
}

void MainWindow::setupNavigation()
{
}

struct DefaultArchitecture : Architecture
{
    bool disasm64() const override
    {
        return true;
    }

    bool addr64() const override
    {
        return true;
    }
} gArchitecture;

Architecture* GlobalArchitecture()
{
    return &gArchitecture;
}

void MainWindow::setupWidgets()
{

}

void MainWindow::on_action_Load_file_triggered()
{
    // TODO: remember the previous browse directory
    auto fileName = QFileDialog::getOpenFileName(this, "Load file", QString(), "All files (*)");
    if(!fileName.isEmpty())
    {
        loadFile(fileName);
    }
}
