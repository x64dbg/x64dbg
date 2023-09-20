#pragma once

#include <QAbstractScrollArea>
#include <QPen>
#include "QZydis.h"
#include "CodeFolding.h"
#include "Imports.h"

class CPUDisassembly;

class CPUSideBar : public QAbstractScrollArea
{
    Q_OBJECT
    QPair<duint, duint> mHighlightedJump;

public:
    // Constructors
    CPUSideBar(CPUDisassembly* disassembly, QWidget* parent = nullptr);
    ~CPUSideBar();

    QSize sizeHint() const;
    void drawStraightArrow(QPainter* painter, int x1, int y1, int x2, int y2);
    void drawFoldingCheckbox(QPainter* painter, int y, bool state);

    CodeFoldingHelper* getCodeFoldingManager();

    static void* operator new(size_t size);
    static void operator delete(void* p);

signals:
    void folded();

public slots:
    // Configuration
    void updateSlots();
    void updateColors();
    void updateFonts();

    void debugStateChangedSlot(DBGSTATE state);
    void reload();
    void changeTopmostAddress(duint i);
    void setViewableRows(duint rows);
    void setSelection(duint selVA);
    void foldDisassembly(duint startAddress, duint length);

protected:
    void paintEvent(QPaintEvent* event);
    void mouseReleaseEvent(QMouseEvent* e);
    void mouseMoveEvent(QMouseEvent* event);
    void mouseDoubleClickEvent(QMouseEvent* event);

    void drawBullets(QPainter* painter, int line, bool ispb, bool isbpdisabled, bool isbookmark);
    bool isJump(int i) const;
    void drawJump(QPainter* painter, int startLine, int endLine, int jumpoffset, bool conditional, bool isexecute, bool isactive);
    int isFoldingGraphicsPresent(int line);
    CodeFoldingHelper mCodeFoldingManager;

private:
    CachedFontMetrics* mFontMetrics = nullptr;
    duint mTopVA = -1;
    duint mSelectedVA = -1;
    QFont mDefaultFont;
    int mFontWidth = 0;
    int mFontHeight = 0;
    duint mViewableRows = 0;
    int mBulletRadius = 0;
    int mBulletYOffset = 10;
    const int mBulletXOffset = 10;
    CPUDisassembly* mDisassembly = nullptr;
    QList<Instruction_t>* mInstrBuffer;
    REGDUMP mRegDump = {};

    struct JumpLine
    {
        int line = 0;
        int destLine = 0;
        unsigned int jumpOffset = 0;
        bool isSelected = false;
        bool isConditional = false;
        bool isJumpGoingToExecute = false;
    };
    struct LabelArrow
    {
        int line = 0;
        int startX = 0;
        int endX = 0;
    };

    void AllocateJumpOffsets(std::vector<JumpLine> & jumpLines, std::vector<LabelArrow> & labelArrows);
    LabelArrow drawLabel(QPainter* painter, int Line, const QString & Text);
    void drawLabelArrows(QPainter* painter, const std::vector<LabelArrow> & labelArrows);

    // Configuration
    QColor mBackgroundColor;

    QColor mConditionalJumpLineFalseColor;
    QColor mUnconditionalJumpLineFalseColor;
    QColor mConditionalJumpLineTrueColor;
    QColor mUnconditionalJumpLineTrueColor;
    QColor mConditionalJumpLineFalseBackwardsColor;
    QColor mUnconditionalJumpLineFalseBackwardsColor;
    QColor mConditionalJumpLineTrueBackwardsColor;
    QColor mUnconditionalJumpLineTrueBackwardsColor;

    QColor mBulletBreakpointColor;
    QColor mBulletBookmarkColor;
    QColor mBulletColor;
    QColor mBulletDisabledBreakpointColor;

    QColor mChkBoxForeColor;
    QColor mChkBoxBackColor;

    QColor mCipLabelColor;
    QColor mCipLabelBackgroundColor;

    QPen mUnconditionalPen;
    QPen mConditionalPen;
    QPen mUnconditionalBackwardsPen;
    QPen mConditionalBackwardsPen;
};
