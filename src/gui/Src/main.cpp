#include "main.h"
#include "zydis_wrapper.h"
#include "MainWindow.h"
#include "Configuration.h"
#include <QTextCodec>
#include <QFile>
#include <QTranslator>
#include <QTextStream>
#include <QLibraryInfo>
#include <QDebug>
#include "MiscUtil.h"

MyApplication::MyApplication(int & argc, char** argv)
    : QApplication(argc, argv)
{
}

#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
bool MyApplication::winEventFilter(MSG* message, long* result)
{
    return DbgWinEvent(message, result);
}

bool MyApplication::globalEventFilter(void* message)
{
    return DbgWinEventGlobal((MSG*)message);
}
#endif

bool MyApplication::notify(QObject* receiver, QEvent* event)
{
    bool done = true;
    try
    {
        if(event->type() == QEvent::WindowActivate && receiver->isWidgetType())
        {
            auto widget = (QWidget*)receiver;
            if((widget->windowFlags() & Qt::Window) == Qt::Window)
            {
                MainWindow::updateDarkTitleBar(widget);
            }
        }
        done = QApplication::notify(receiver, event);
    }
    catch(const std::exception & ex)
    {
        QString message = QString().sprintf("Fatal GUI Exception: %s!\n", ex.what());
        GuiAddLogMessage(message.toUtf8().constData());
        puts(message.toUtf8().constData());
        OutputDebugStringA(message.toUtf8().constData());
    }
    catch(...)
    {
        const char* message = "Fatal GUI Exception: (...)!\n";
        GuiAddLogMessage(message);
        puts(message);
        OutputDebugStringA(message);
    }
    return done;
}

static Configuration* mConfiguration;
char currentLocale[MAX_SETTING_SIZE] = "";
// Boom... VS does not support "thread_local"... and cannot use "__declspec(thread)" in a DLL... https://blogs.msdn.microsoft.com/oldnewthing/20101122-00/?p=12233
// Simulating Thread Local Storage with a map...
std::map<DWORD, TranslatedStringStorage>* TLS_TranslatedStringMap; //key = Thread Id, value = Translate Buffer

static bool isValidLocale(const QString & locale)
{
    auto allLocales = QLocale::matchingLocales(QLocale::AnyLanguage, QLocale::AnyScript, QLocale::AnyCountry);
    for(auto & l : allLocales)
        if(l.name() == locale || l.name().replace(QRegExp("_.+"), "") == locale)
            return true;
    return false;
}

// This function doesn't appear to have any effect when Qt DPI scaling is enabled.
// When scaling is disabled it drastically improves the results though.
static void setDpiUnaware()
{
    // https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setprocessdpiawarenesscontext
    typedef unsigned int(WINAPI * pfnSetProcessDpiAwarenessContext)(size_t value);
    static pfnSetProcessDpiAwarenessContext pSetProcessDpiAwarenessContext =
        (pfnSetProcessDpiAwarenessContext)GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetProcessDpiAwarenessContext");
    if(pSetProcessDpiAwarenessContext)
    {
        // It's unclear if there is any benefit to the GDI scaling, but it should work the best in theory.
        pSetProcessDpiAwarenessContext(/* DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED */ -5);
        if(GetLastError() == ERROR_INVALID_PARAMETER)
        {
            // Fall back to unaware if the option doesn't exist.
            pSetProcessDpiAwarenessContext(/* DPI_AWARENESS_CONTEXT_UNAWARE */ -1);
        }
    }
}

/*
Some resources:
- https://www.programmersought.com/article/89186999411/
- https://github.com/COVESA/dlt-viewer/issues/205
- https://vicrucann.github.io/tutorials/osg-qt-high-dpi/
- https://forum.freecadweb.org/viewtopic.php?t=52307
- https://wiki.freecadweb.org/HiDPI_support
- https://doc.qt.io/qt-5/highdpi.html#high-dpi-support-in-qt
*/
static void handleHighDpiScaling()
{
    // If the user messes with the Qt environment variables, do not set anything
    if(qEnvironmentVariableIsSet("QT_DEVICE_PIXEL_RATIO") // legacy in 5.6, but still functional
            || qEnvironmentVariableIsSet("QT_AUTO_SCREEN_SCALE_FACTOR")
            || qEnvironmentVariableIsSet("QT_SCALE_FACTOR")
            || qEnvironmentVariableIsSet("QT_SCREEN_SCALE_FACTORS")
            || qEnvironmentVariableIsSet("QT_ENABLE_HIGHDPI_SCALING")
            || qEnvironmentVariableIsSet("QT_SCALE_FACTOR_ROUNDING_POLICY"))
    {
        qDebug() << "Detected environment variables related to Qt scaling, skipping High DPI handling";
        return;
    }

    duint enableQtHighDpiScaling = true;
    BridgeSettingGetUint("Gui", "EnableQtHighDpiScaling", &enableQtHighDpiScaling);

    if(enableQtHighDpiScaling)
    {
        QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    }
    else
    {
        // This enables Windows scaling the application automatically
        setDpiUnaware();

        // These options don't seem to do anything, but the Qt documentation recommends it
        putenv("QT_AUTO_SCREEN_SCALE_FACTOR=1");
        QCoreApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
    }
}

int main(int argc, char* argv[])
{
    handleHighDpiScaling();
    MyApplication application(argc, argv);

#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
    QAbstractEventDispatcher::instance(application.thread())->setEventFilter(MyApplication::globalEventFilter);
#else
    auto eventFilter = new MyEventFilter();
    application.installNativeEventFilter(eventFilter);
#endif

    // Get the language setting
    if(!BridgeSettingGet("Engine", "Language", currentLocale) || !isValidLocale(currentLocale))
    {
        QStringList uiLanguages = QLocale::system().uiLanguages();
        QString sysLocale = uiLanguages.size() ? QLocale(uiLanguages[0]).name() : QLocale::system().name();
        strcpy_s(currentLocale, sysLocale.toUtf8().constData());
        BridgeSettingSet("Engine", "Language", currentLocale);
    }

    // Load translations for Qt
    QTranslator qtTranslator;
    if(qtTranslator.load(QString("qt_%1").arg(currentLocale), QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
        application.installTranslator(&qtTranslator);

    //x64dbg and x32dbg can share the same translation
    QTranslator x64dbgTranslator;
    auto path = QString("%1/../translations").arg(QCoreApplication::applicationDirPath());
    if(x64dbgTranslator.load(QString("x64dbg_%1").arg(currentLocale), path))
        application.installTranslator(&x64dbgTranslator);

    TLS_TranslatedStringMap = new std::map<DWORD, TranslatedStringStorage>();

    // initialize Zydis
    Zydis::GlobalInitialize();

    // load config file + set config font
    mConfiguration = new Configuration;
    application.setFont(ConfigFont("Application"));

    // Set configured link color
    QPalette appPalette = application.palette();
    appPalette.setColor(QPalette::Link, ConfigColor("LinkColor"));
    application.setPalette(appPalette);

    // Load the selected style
    MainWindow::loadSelectedTheme();

    // Register custom data types
    qRegisterMetaType<dsint>("dsint");
    qRegisterMetaType<duint>("duint");
    qRegisterMetaType<byte_t>("byte_t");
    qRegisterMetaType<DBGSTATE>("DBGSTATE");

    // Set QString codec to UTF-8
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
    QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));
#endif

    // Init communication with debugger
    Bridge::initBridge();

    // Start GUI
    MainWindow* mainWindow;
    mainWindow = new MainWindow();
    mainWindow->show();

    // Set some data
    Bridge::getBridge()->winId = (void*)mainWindow->winId();

    // Init debugger
    const char* errormsg = DbgInit();
    if(errormsg)
    {
        QMessageBox msg(QMessageBox::Critical, QObject::tr("DbgInit Error!"), QString(errormsg));
        msg.setWindowIcon(DIcon("compile-error"));
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        msg.exec();
        exit(1);
    }

    //execute the application
    int result = application.exec();
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
    application.removeNativeEventFilter(eventFilter);
#else
    QAbstractEventDispatcher::instance(application.thread())->setEventFilter(nullptr);
#endif
    delete mainWindow;
    mConfiguration->save(); //save config on exit
    {
        //delete tls
        auto temp = TLS_TranslatedStringMap;
        TLS_TranslatedStringMap = nullptr;
        delete temp;
    }

    //TODO free Zydis/config/bridge and prevent use after free.

    return result;
}
