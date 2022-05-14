#pragma once

#include <QAction>
#include <functional>
#include "ActionHelpers.h"

class MenuBuilder;

class CommonActions : public QObject, public ActionHelperProxy
{
    Q_OBJECT

public:
    typedef enum
    {
        ActionDisasm = 1, // Follow in Disassembly
        ActionDisasmMore = 1 << 1, // Follow in Disassembly (submenu)
        ActionDisasmDump = 1 << 2, // Follow in Dump in Disassembly Mode
        ActionDisasmDumpMore = 1 << 3, // Follow in Dump in Disassembly Mode (submenu)
        ActionDisasmData = 1 << 4, // Follow DWORD in Disassembly
        ActionDump = 1 << 5, // Follow in Dump
        ActionDumpMore = 1 << 6, // Follow in Dump (submenu)
        ActionDumpN = 1 << 7, // Follow in Dump N (submenu)
        ActionDumpData = 1 << 8, // Follow DWORD in Dump
        ActionStackDump = 1 << 9, // Follow in Stack
        ActionMemoryMap = 1 << 10, // Follow in Memory Map
        ActionGraph = 1 << 11, // Graph
        ActionBreakpoint = 1 << 12, // Breakpoint
        ActionMemoryBreakpoint = 1 << 13, // Memory Breakpoint
        ActionMnemonicHelp = 1 << 14, // Mnemonic Help
        ActionLabel = 1 << 15, // Label
        ActionLabelMore = 1 << 16, // Label (submenu)
        ActionComment = 1 << 17, // Comment
        ActionCommentMore = 1 << 18, // Comment (submenu)
        ActionBookmark = 1 << 19, // Bookmark
        ActionBookmarkMore = 1 << 20, // Bookmark (submenu)
        ActionFindref = 1 << 21, // Find references
        ActionFindrefMore = 1 << 22, // Find references (submenu)
        ActionXref = 1 << 23, // Xref
        ActionXrefMore = 1 << 24, // Xref (submenu)
        ActionNewOrigin = 1 << 25, // Set EIP/RIP Here
        ActionNewThread = 1 << 26, // Create New Thread Here
        ActionWatch = 1 << 27 // Watch DWORD
    } CommonActionsList;

    using GetSelectionFunc = std::function<duint()>;

    explicit CommonActions(QWidget* parent, ActionHelperFuncs funcs, GetSelectionFunc getSelection);
    void build(MenuBuilder* builder, int actions);
    //Reserved for future use (submenu for Dump and Search with more addresses)
    //void build(MenuBuilder* builder, int actions, std::function<void(QList<std::pair<QString, duint>>&, CommonActionsList)> additionalAddress);

    QAction* makeCommandAction(const QIcon & icon, const QString & text, const char* cmd, const char* shortcut);
    QAction* makeCommandAction(const QIcon & icon, const QString & text, const char* cmd);
public slots:
    void followDisassemblySlot();
    void setLabelSlot();
    void setCommentSlot();
    void setBookmarkSlot();

    void toggleInt3BPActionSlot();
    void editSoftBpActionSlot();
    void toggleHwBpActionSlot();
    void setHwBpOnSlot0ActionSlot();
    void setHwBpOnSlot1ActionSlot();
    void setHwBpOnSlot2ActionSlot();
    void setHwBpOnSlot3ActionSlot();
    void setHwBpAt(duint va, int slot);

    void graphSlot();
    void setNewOriginHereActionSlot();
    void createThreadSlot();
private:
    GetSelectionFunc mGetSelection;
    bool WarningBoxNotExecutable(const QString & text, duint wVA);
    QWidget* widgetparent();
};
