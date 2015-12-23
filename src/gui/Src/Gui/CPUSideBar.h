#ifndef CPUSIDEBAR_H
#define CPUSIDEBAR_H

#include <QAbstractScrollArea>
#include "CPUDisassembly.h"

class CPUSideBar : public QAbstractScrollArea
{
    Q_OBJECT
    QPair<dsint, dsint> mHighlightedJump;

public:
    // Constructors
    explicit CPUSideBar(CPUDisassembly* Ptr, QWidget* parent = 0);
    ~CPUSideBar();

    QSize sizeHint() const;
    void drawStraightArrow(QPainter* painter, int x1, int y1, int x2, int y2);

    static void* operator new(size_t size);
    static void operator delete(void* p);

public slots:
    // Configuration
    void updateSlots();
    void updateColors();
    void updateFonts();

    void debugStateChangedSlot(DBGSTATE state);
    void repaint();
    void changeTopmostAddress(dsint i);
    void setViewableRows(int rows);
    void setSelection(dsint selVA);

protected:
    void paintEvent(QPaintEvent* event);
    void mouseReleaseEvent(QMouseEvent* e);
    void mouseMoveEvent(QMouseEvent* event);

    void drawLabel(QPainter* painter, int Line, QString Text);
    void drawBullets(QPainter* painter, int line, bool ispb, bool isbpdisabled, bool isbookmark);
    bool isJump(int i) const;
    void drawJump(QPainter* painter, int startLine, int endLine, int jumpoffset, bool conditional, bool isexecute, bool isactive);

private:
    dsint topVA;
    dsint selectedVA;
    QFont m_DefaultFont;
    int fontWidth, fontHeight;
    int viewableRows;
    int mBulletRadius;
    int mBulletYOffset = 10;
    const int mBulletXOffset = 10;


    CPUDisassembly* mDisas;
    QList<Instruction_t>* InstrBuffer;
    REGDUMP regDump;

private:
    // Configuration
    QColor mBackgroundColor;

    QColor mConditionalJumpLineFalseColor;
    QColor mUnconditionalJumpLineFalseColor;
    QColor mConditionalJumpLineTrueColor;
    QColor mUnconditionalJumpLineTrueColor;

    QColor mBulletBreakpointColor;
    QColor mBulletBookmarkColor;
    QColor mBulletColor;
    QColor mBulletDisabledBreakpointColor;

    QColor mCipLabelColor;
    QColor mCipLabelBackgroundColor;
};

#endif // CPUSIDEBAR_H
