#include <QApplication>
#include <QStyleFactory>
#include <QPalette>

#include "MainWindow.h"
#include "Configuration.h"

#if QT_VERSION < QT_VERSION_CHECK(5, 12, 0)
#error Your Qt version is likely too old, upgrade to 5.12 or higher
#endif // QT_VERSION

// https://www.qt.io/blog/dark-mode-on-windows-11-with-qt-6.5

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setStyle(QStyleFactory::create("Fusion"));

#if 0
    QColor lightGray(190, 190, 190);
    QColor gray(128, 128, 128);
    QColor midDarkGray(100, 100, 100);
    QColor darkGray(53, 53, 53);
    QColor black(25, 25, 25);
    QColor blue(42, 130, 218);
    QColor white(255, 255, 255);

    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, darkGray);
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, black);
    darkPalette.setColor(QPalette::AlternateBase, darkGray);
    darkPalette.setColor(QPalette::ToolTipBase, darkGray);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, lightGray);
    darkPalette.setColor(QPalette::Button, darkGray);
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::Link, blue);
    darkPalette.setColor(QPalette::Highlight, blue);
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    darkPalette.setColor(QPalette::Light, blue);
    darkPalette.setColor(QPalette::Dark, midDarkGray);

    darkPalette.setColor(QPalette::Active, QPalette::Button, gray.darker());
    darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, gray);
    darkPalette.setColor(QPalette::Disabled, QPalette::WindowText, gray);
    darkPalette.setColor(QPalette::Disabled, QPalette::Text, gray);
    darkPalette.setColor(QPalette::Disabled, QPalette::Light, darkGray);

    app.setPalette(darkPalette);
#endif

#if 0
    QColor black(40, 42, 54);
    QColor white(248, 248, 242);
    QColor lightBlue(139, 233, 253);
    QColor green(80, 250, 123);
    QColor yellow(229, 238, 138);
    QColor red(255, 85, 85);
    QColor purple(189, 147, 249);
    QColor darkBlue(98, 114, 164);
    QColor grey(68, 71, 90);
    QColor orange(255, 184, 108);
    QColor pink(255, 121, 198);

    QPalette dark;
    dark.setColor(QPalette::Window, black);
    dark.setColor(QPalette::WindowText, white);
    dark.setColor(QPalette::Base, black);
    dark.setColor(QPalette::AlternateBase, grey);
    dark.setColor(QPalette::ToolTipBase, black);
    dark.setColor(QPalette::ToolTipText, lightBlue);
    dark.setColor(QPalette::Text, white);
    dark.setColor(QPalette::Button, black);
    dark.setColor(QPalette::ButtonText, white);
    dark.setColor(QPalette::BrightText, grey);
    dark.setColor(QPalette::Link, green);
    dark.setColor(QPalette::LinkVisited, purple);
    dark.setColor(QPalette::Highlight, grey);
    dark.setColor(QPalette::HighlightedText, white);
    dark.setColor(QPalette::Light, grey);

    dark.setColor(QPalette::Disabled, QPalette::Button, darkBlue);
    dark.setColor(QPalette::Disabled, QPalette::ButtonText, darkBlue);
    dark.setColor(QPalette::Disabled, QPalette::Text, darkBlue);
    dark.setColor(QPalette::Disabled, QPalette::WindowText, darkBlue);

    app.setPalette(dark);
#endif

    QPalette palette;

    QColor oneDarkBackground("#282c34");
    QColor oneDarkForeground("#abb2bf");
    QColor oneDarkComment("#5c6370");
    QColor oneDarkKeyword("#c678dd");
    QColor oneDarkFunction("#61afef");
    QColor oneDarkString("#98c379");
    QColor oneDarkNumber("#d19a66");
    QColor oneDarkVariable("#e06c75");
    QColor oneDarkConstant("#56b6c2");

    palette.setColor(QPalette::Window, oneDarkBackground);
    palette.setColor(QPalette::WindowText, oneDarkForeground);
    palette.setColor(QPalette::Base, QColor("#21252b"));
    palette.setColor(QPalette::AlternateBase, QColor("#2c313c")); // used for alternate row color in tree
    palette.setColor(QPalette::ToolTipBase, QColor("#3a3f4b"));
    palette.setColor(QPalette::ToolTipText, oneDarkForeground);
    palette.setColor(QPalette::Text, oneDarkForeground);
    palette.setColor(QPalette::Button, QColor("#3a3f4b"));
    palette.setColor(QPalette::ButtonText, oneDarkForeground);
    palette.setColor(QPalette::BrightText, oneDarkVariable);
    palette.setColor(QPalette::Link, oneDarkFunction);
    palette.setColor(QPalette::LinkVisited, oneDarkFunction);
    palette.setColor(QPalette::Highlight, oneDarkFunction);
    palette.setColor(QPalette::Highlight, oneDarkFunction);
    palette.setColor(QPalette::HighlightedText, oneDarkBackground);

    palette.setColor(QPalette::Light, oneDarkBackground.lighter(120));
    palette.setColor(QPalette::Midlight, oneDarkBackground.lighter(110));
    palette.setColor(QPalette::Mid, oneDarkBackground.darker(110));
    palette.setColor(QPalette::Dark, oneDarkBackground.darker(130));
    palette.setColor(QPalette::Shadow, oneDarkBackground.darker(160));

    palette.setColor(QPalette::Disabled, QPalette::WindowText, oneDarkComment);
    palette.setColor(QPalette::Disabled, QPalette::Text, oneDarkComment);
    palette.setColor(QPalette::Disabled, QPalette::Text, oneDarkComment);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, oneDarkComment);
    palette.setColor(QPalette::Disabled, QPalette::Highlight, oneDarkBackground.darker(150));
    palette.setColor(QPalette::Disabled, QPalette::HighlightedText, oneDarkComment);

    app.setPalette(palette);

    QColor separator(99, 99, 99);
    QColor header(75, 75, 75);

    // TODO: how is this derived by qt?
    separator = QColor("#616671");
    header = QColor("#484d59");

    ConfigurationPalette p;
    p.background = oneDarkBackground;
    p.darkGrey = separator;
    p.lightGrey = header;
    p.black = oneDarkForeground;

    Configuration config(p);

    auto hexText = config.Colors["HexDumpTextColor"];
    config.Colors["HexDumpByte00Color"] = oneDarkConstant;
    config.Colors["HexDumpByte7FColor"] = oneDarkVariable;
    config.Colors["HexDumpByteFFColor"] = oneDarkVariable;
    config.Colors["HexDumpByteIsPrintColor"] = oneDarkString;

    MainWindow w;
    w.show();
    return app.exec();
}
