#include "main.h"
#include "zydis_wrapper.h"
#include "MainWindow.h"
#include "Configuration.h"
#include <QTextCodec>
#include <QFile>
#include <QTranslator>
#include <QTextStream>
#include <QLibraryInfo>
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

int main(int argc, char* argv[])
{
    qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", "1");
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    MyApplication application(argc, argv);
    QFile f(QString("%1/style.css").arg(QCoreApplication::applicationDirPath()));
    if(f.open(QFile::ReadOnly | QFile::Text))
    {
        QTextStream in(&f);
        auto style = in.readAll();
        f.close();
        application.setStyleSheet(style);
    }
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
    QAbstractEventDispatcher::instance(application.thread())->setEventFilter(MyApplication::globalEventFilter);
#else
    auto eventFilter = new MyEventFilter();
    application.installNativeEventFilter(eventFilter);
#endif

    // Get the hidden language setting (for testers)
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

    //x64dbg and x86dbg can share the same translation
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
        msg.setWindowIcon(DIcon("compile-error.png"));
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
