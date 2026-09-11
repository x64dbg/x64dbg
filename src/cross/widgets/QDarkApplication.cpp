#include "QDarkApplication.h"

#include <QStyleFactory>
#include <QPalette>

#ifdef Q_OS_WIN
#    ifndef NOMINMAX
#        define NOMINMAX
#    endif
#    ifndef WIN32_LEAN_AND_MEAN
#        define WIN32_LEAN_AND_MEAN
#    endif
#    include <windows.h>
#    include <QTimer>
#    include <QPointer>
#    include <QWidget>
#endif

QDarkApplication::QDarkApplication(int & argc, char** argv)
    : QApplication(argc, argv)
{
    // https://www.qt.io/blog/dark-mode-on-windows-11-with-qt-6.5
    setStyle(QStyleFactory::create("Fusion"));

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

    setPalette(darkPalette);
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

    setPalette(dark);
#endif

    const QColor background("#212121");
    const QColor base("#313131");
    const QColor hover("#414141");
    const QColor text("#e0e0e0");
    const QColor accent("#89a2f6");
    const QColor disabled("#646464");
    const QColor border("#515151");
    const QColor mnemonic("#c678dd");
    const QColor call("#61afef");
    const QColor jump("#98c379");
    const QColor ret("#e06c75");
    const QColor number("#d19a66");
    const QColor reg("#e06c75");
    const QColor constant("#56b6c2");

    QPalette palette;
    palette.setColor(QPalette::Window, background);
    palette.setColor(QPalette::WindowText, text);
    palette.setColor(QPalette::Base, base);
    palette.setColor(QPalette::AlternateBase, background);
    palette.setColor(QPalette::ToolTipBase, hover);
    palette.setColor(QPalette::ToolTipText, Qt::white);
    palette.setColor(QPalette::Text, text);
    palette.setColor(QPalette::Button, background);
    palette.setColor(QPalette::ButtonText, text);
    palette.setColor(QPalette::BrightText, Qt::white);
    palette.setColor(QPalette::Link, accent);
    palette.setColor(QPalette::LinkVisited, mnemonic);
    palette.setColor(QPalette::Highlight, accent);
    palette.setColor(QPalette::HighlightedText, Qt::black);

    // Fusion draws bevels/frames/grooves from these five shading roles.
    palette.setColor(QPalette::Light, border);
    palette.setColor(QPalette::Midlight, QColor("#3f3f3f"));
    palette.setColor(QPalette::Mid, base);
    palette.setColor(QPalette::Dark, QColor("#161616"));
    palette.setColor(QPalette::Shadow, QColor("#0e0e0e"));
    palette.setColor(QPalette::PlaceholderText, disabled);

    palette.setColor(QPalette::Disabled, QPalette::WindowText, disabled);
    palette.setColor(QPalette::Disabled, QPalette::Text, disabled);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, disabled);
    palette.setColor(QPalette::Disabled, QPalette::Highlight, hover);
    palette.setColor(QPalette::Disabled, QPalette::HighlightedText, disabled);
    palette.setColor(QPalette::Disabled, QPalette::Base, background);

    setPalette(palette);

    ConfigurationPalette p;
    p.background = background;
    p.darkGrey = border;
    p.lightGrey = base;
    p.black = text;

    // Create the global configuration instance.
    mConfiguration = std::make_unique<Configuration>(p);

    Config()->Colors["DisassemblyBreakpointColor"] = Qt::black;
    Config()->Colors["DisassemblyBreakpointBackgroundColor"] = Qt::red;

    Config()->Colors["HexDumpByte00Color"] = accent;
    Config()->Colors["HexDumpByte7FColor"] = ret;
    Config()->Colors["HexDumpByteFFColor"] = ret;
    Config()->Colors["HexDumpByteIsPrintColor"] = jump;

    Config()->Colors["RegistersBackgroundColor"] = background;
    Config()->Colors["RegistersLabelColor"] = disabled;
    Config()->Colors["RegistersArgumentLabelColor"] = accent;
    Config()->Colors["RegistersColor"] = text;
    Config()->Colors["RegistersModifiedColor"] = Qt::red;
    Config()->Colors["RegistersSelectionColor"] = hover;
    Config()->Colors["RegistersExtraInfoColor"] = disabled;

    Config()->Colors["StackCspBackgroundColor"] = Qt::transparent;
    Config()->Colors["StackCspColor"] = QColor("#a6f93e");
    Config()->Colors["StackAddressColor"] = QColor("#a0a0a0");
    Config()->Colors["StackAddressBackgroundColor"] = Qt::transparent;
    Config()->Colors["StackSelectedAddressColor"] = text;
    Config()->Colors["StackSelectedAddressBackgroundColor"] = Qt::transparent;
    Config()->Colors["StackInactiveTextColor"] = QColor("#a0a0a0");
    Config()->Colors["StackSelectionColor"] = hover;
    Config()->Colors["StackReturnToColor"] = QColor("#f55f86");

    Config()->Colors["ThreadCurrentBackgroundColor"] = QColor("#C24000");
    Config()->Colors["ThreadCurrentColor"] = Qt::white;

    // Instruction colors shared by all cross-platform data views.
    const QColor& comment = disabled;

    auto setColorPair = [this](const QString& name, const QColor& fg)
    {
        Config()->Colors[name + "Color"] = fg;
        Config()->Colors[name + "BackgroundColor"] = Qt::transparent;
    };

    setColorPair("InstructionComma", text);
    setColorPair("InstructionPrefix", mnemonic);
    setColorPair("InstructionUncategorized", text);
    setColorPair("InstructionAddress", accent);
    setColorPair("InstructionValue", number);
    setColorPair("TraceNewValue", QColor(Qt::red));
    setColorPair("InstructionMnemonic", mnemonic);
    setColorPair("InstructionPushPop", mnemonic);
    setColorPair("InstructionCall", call);
    setColorPair("InstructionRet", ret);
    setColorPair("InstructionConditionalJump", jump);
    setColorPair("InstructionUnconditionalJump", jump);
    setColorPair("InstructionNop", comment);
    setColorPair("InstructionFar", ret);
    setColorPair("InstructionInt3", ret);
    setColorPair("InstructionUnusual", ret);
    setColorPair("InstructionMemorySize", comment);
    setColorPair("InstructionMemorySegment", constant);
    setColorPair("InstructionMemoryBrackets", text);
    setColorPair("InstructionMemoryStackBrackets", constant);
    setColorPair("InstructionMemoryBaseRegister", reg);
    setColorPair("InstructionMemoryIndexRegister", reg);
    setColorPair("InstructionMemoryScale", number);
    setColorPair("InstructionMemoryOperator", text);
    setColorPair("InstructionGeneralRegister", reg);
    setColorPair("InstructionFpuRegister", constant);
    setColorPair("InstructionMmxRegister", constant);
    setColorPair("InstructionXmmRegister", constant);
    setColorPair("InstructionYmmRegister", constant);
    setColorPair("InstructionZmmRegister", constant);

    Config()->Colors["DisassemblyCommentColor"] = comment;
    Config()->Colors["DisassemblyAutoCommentColor"] = number;
}

// Based on DarkTheme::applyToWindow() from REVIDE (https://github.com/x64dbg/REVIDE)
void QDarkApplication::applyDarkTitleBar(QWidget* window)
{
#ifdef Q_OS_WIN
    if(!window)
        return;

    auto apply = [](QWidget* w)
    {
        HMODULE dwmapi = LoadLibraryW(L"dwmapi.dll");
        if(!dwmapi)
            return;

        using DwmSetWindowAttributeFn = HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
        auto setWindowAttribute = reinterpret_cast<DwmSetWindowAttributeFn>(GetProcAddress(dwmapi, "DwmSetWindowAttribute"));
        if(setWindowAttribute)
        {
            const BOOL enabled = TRUE;
            HWND hwnd = reinterpret_cast<HWND>(w->winId());
            constexpr DWORD ImmersiveDarkMode = 20;
            constexpr DWORD ImmersiveDarkModeBefore20H1 = 19;
            setWindowAttribute(hwnd, ImmersiveDarkModeBefore20H1, &enabled, sizeof(enabled));
            setWindowAttribute(hwnd, ImmersiveDarkMode, &enabled, sizeof(enabled));
            SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        }

        FreeLibrary(dwmapi);
    };

    // The window frame is not always ready when show() returns, so retry on the
    // next event loop iterations (QPointer guards against early destruction)
    apply(window);
    QPointer<QWidget> guard(window);
    QTimer::singleShot(0, window, [guard, apply]()
    {
        if(guard)
            apply(guard);
    });
    QTimer::singleShot(100, window, [guard, apply]()
    {
        if(guard)
            apply(guard);
    });
#else
    Q_UNUSED(window);
#endif
}
