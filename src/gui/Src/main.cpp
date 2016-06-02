#include "main.h"
#include "capstone_wrapper.h"
#include <QTextCodec>
#include <QFile>

MyApplication::MyApplication(int & argc, char** argv)
    : QApplication(argc, argv)
{
}

#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
bool MyApplication::globalEventFilter(void* message)
{
    return DbgWinEventGlobal((MSG*)message);
}
#endif

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

static bool isValidLocale(const QString & locale)
{
    auto allLocales = QLocale::matchingLocales(QLocale::AnyLanguage, QLocale::AnyScript, QLocale::AnyCountry);
    for(auto & l : allLocales)
        if(l.name() == locale)
            return true;
    return false;
}

int main(int argc, char* argv[])
{
    qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", "1");
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
    x64GlobalFilter* filter = new x64GlobalFilter();
    QAbstractEventDispatcher::instance(application.thread())->installNativeEventFilter(filter);
#endif

    // Get the hidden language setting (for testers)
    char locale[MAX_SETTING_SIZE] = "";
    if(!BridgeSettingGet("Engine", "Language", locale) || !isValidLocale(locale))
    {
        strcpy_s(locale, QLocale::system().name().toUtf8().constData());
        BridgeSettingSet("Engine", "Language", locale);
    }

    // Load translations for Qt
    QTranslator qtTranslator;
    if(qtTranslator.load(QString("qt_%1").arg(locale), QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
        application.installTranslator(&qtTranslator);

    //x64dbg and x32dbg can share the same translation
    QTranslator x64dbgTranslator;
    auto path = QString("%1/../translations").arg(QCoreApplication::applicationDirPath());
    if(x64dbgTranslator.load(QString("x64dbg_%1").arg(locale), path))
        application.installTranslator(&x64dbgTranslator);

    // initialize capstone
    Capstone::GlobalInitialize();

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
    MainWindow mainWindow;
    mainWindow.show();

    // Set some data
    Bridge::getBridge()->winId = (void*)mainWindow.winId();

    // Init debugger
    const char* errormsg = DbgInit();
    if(errormsg)
    {
        QMessageBox msg(QMessageBox::Critical, "DbgInit Error!", QString(errormsg));
        msg.setWindowIcon(QIcon(":/icons/images/compile-error.png"));
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        msg.exec();
        exit(1);
    }

    //execute the application
    int result = application.exec();
    mConfiguration->save(); //save config on exit
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
    QAbstractEventDispatcher::instance(application.thread())->removeNativeEventFilter(filter);
#else
    QAbstractEventDispatcher::instance(application.thread())->setEventFilter(nullptr);
#endif

    //TODO free capstone/config/bridge and prevent use after free.

    return result;
}
