#ifndef COMMONACTIONS_H
#define COMMONACTIONS_H

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
        ActionDisasm = 1,
        ActionDisasmMore = 1 << 1,
        ActionDisasmDump = 1 << 2,
        ActionDisasmDumpMore = 1 << 3,
        ActionDisasmData = 1 << 4,
        ActionDump = 1 << 5,
        ActionDumpMore = 1 << 6,
        ActionDumpN = 1 << 7,
        ActionDumpData = 1 << 8,
        ActionStackDump = 1 << 9,
        ActionMemoryMap = 1 << 10,
        ActionGraph = 1 << 11,
        ActionBreakpoint = 1 << 12,
        ActionMemoryBreakpoint = 1 << 13,
        ActionMnemonicHelp = 1 << 14,
        ActionLabel = 1 << 15,
        ActionLabelMore = 1 << 16,
        ActionComment = 1 << 17,
        ActionCommentMore = 1 << 18,
        ActionBookmark = 1 << 19,
        ActionBookmarkMore = 1 << 20,
        ActionFindref = 1 << 21,
        ActionFindrefMore = 1 << 22,
        ActionXref = 1 << 23,
        ActionXrefMore = 1 << 24,
        ActionNewOrigin = 1 << 25,
        ActionNewThread = 1 << 26,
        ActionWatch = 1 << 27
    } CommonActionsList;

    using GetSelectionFunc = std::function<duint()>;

    explicit CommonActions(QWidget* parent, ActionHelperFuncs funcs, GetSelectionFunc getSelection);
    void build(MenuBuilder* builder, int actions);
    void build(MenuBuilder* builder, int actions, std::function<void(QList<std::pair<QString, duint>>&, CommonActionsList)> additionalAddress);

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

    void setNewOriginHereActionSlot();
    void createThreadSlot();
private:
    GetSelectionFunc mGetSelection;
    QWidget* widgetparent();
};


#endif //COMMONACTIONS_H
