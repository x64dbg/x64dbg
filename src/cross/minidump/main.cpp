#include <QApplication>

#include "MainWindow.h"
#include "Configuration.h"

#if QT_VERSION < QT_VERSION_CHECK(5, 12, 0)
#error Your Qt version is likely too old, upgrade to 5.12 or higher
#endif // QT_VERSION

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    Configuration config;
    MainWindow w;
    w.show();
    return a.exec();
}
