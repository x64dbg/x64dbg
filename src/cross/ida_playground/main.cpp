#include <QDarkApplication.h>
#include "MainWindow.h"

// https://www.qt.io/blog/dark-mode-on-windows-11-with-qt-6.5

int main(int argc, char* argv[])
{
    QDarkApplication app(argc, argv);

    MainWindow w;
    w.show();
    QDarkApplication::applyDarkTitleBar(&w);

    // Load the file provided on the command line
    if(argc > 1)
    {
        w.loadFile(QString::fromUtf8(argv[1]));
    }

    return app.exec();
}
