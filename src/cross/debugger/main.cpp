#include <CrossAccessible.h>
#include <QDarkApplication.h>
#include <QIcon>

#include "gui/MainWindow.h"

int main(int argc, char* argv[])
{
    qRegisterMetaType<REGDUMP>("REGDUMP");
    qRegisterMetaType<QVector<DbgThreadInfo>>("QVector<DbgThreadInfo>");
    qRegisterMetaType<QVector<DbgMemoryPage>>("QVector<DbgMemoryPage>");

#ifndef QT_NO_ACCESSIBILITY
    QAccessible::installFactory(crossAccessibleInterfaceFactory);
#endif

    QDarkApplication app(argc, argv);
    QIcon::setThemeSearchPaths({":/"});
    QIcon::setThemeName("Default");

    // Keep the debugger's dense data views while using the shared dark theme.
    QFont tableFont = Config()->Fonts["AbstractTableView"];
    tableFont.setPointSize(9);
    for(const auto* fontName : {"AbstractTableView", "Disassembly", "HexDump", "Stack", "Registers", "Log"})
        Config()->Fonts[fontName] = tableFont;

    MainWindow w;
    w.show();
    QDarkApplication::applyDarkTitleBar(&w);
    return app.exec();
}
