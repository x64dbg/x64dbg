#pragma once

#include <BasicView/HexDump.h>
#include <memory>

#include "core/DbgAdapter.h"

class QMenu;
class QAction;
class QContextMenuEvent;
class QMouseEvent;
class QPainter;
class QZydis;

class CPUStack : public HexDump
{
    Q_OBJECT
public:
    CPUStack(Architecture* architecture, DbgAdapter* adapter, QWidget* parent = nullptr);

    QString paintContent(QPainter* painter, duint row, duint column, int x, int y, int w, int h) override;
    void getColumnRichText(duint col, duint rva, RichTextPainter::List & richText) const override;
    void updateColors() override;

signals:
    void followDisasmRequested(duint addr);

public slots:
    void stackDumpAt(duint addr, duint csp);
    void onRegistersUpdated(const REGDUMP & regs);
    void onProcessStarted();
    void onSessionEnded();
    void gotoCspSlot();
    void gotoCbpSlot();
    void followDisasmSlot();
    void freezeStackSlot();
    void realignSlot() const;
    void modifySlot();
    void copyPtrColumnSlot() const;
    void copyCommentsColumnSlot() const;

public:
    void contextMenuEvent(QContextMenuEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    void setupColumns();
    void setupContextMenu();
    void refreshActionState() const;
    bool resolveSlotComment(duint rva, QString & out, bool & isReturnTo) const;
    bool resolveReturnTo(duint returnAddress, duint & fromAddress) const;

    std::unique_ptr<QZydis> mDisasm;
    DbgAdapter* mAdapter = nullptr;
    duint mCsp = 0;
    duint mCbp = 0;
    bool mStackFrozen = false;

    QColor mStackReturnToColor;
    QColor mStackCspColor;
    QColor mStackCspBackgroundColor;
    QColor mStackAddressColor;
    QColor mStackAddressBackgroundColor;
    QColor mStackSelectedAddressColor;
    QColor mStackSelectedAddressBackgroundColor;
    QColor mStackInactiveTextColor;

    QMenu* mContextMenu = nullptr;
    QAction* mGotoCspAction = nullptr;
    QAction* mGotoCbpAction = nullptr;
    QAction* mFollowDisasmAction = nullptr;
    QAction* mFreezeAction = nullptr;
    QAction* mRealignAction = nullptr;
    QAction* mModifyAction = nullptr;
};
