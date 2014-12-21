#ifndef CPUSIDEBAR_H
#define CPUSIDEBAR_H

#include <QAbstractScrollArea>
#include "CPUDisassembly.h"

class CPUSideBar : public QAbstractScrollArea
{
    Q_OBJECT
    QPair<int_t, int_t> mHighlightedJump;
public:
    explicit CPUSideBar(CPUDisassembly* Ptr, QWidget* parent = 0);
    QSize sizeHint() const;
    void drawStraightArrow(QPainter* painter, int x1, int y1, int x2, int y2);

public slots:
    void debugStateChangedSlot(DBGSTATE state);
    void repaint();
    void changeTopmostAddress(int_t i);
    void setViewableRows(int rows);
    void setSelection(int_t selVA);

protected:
    virtual void paintEvent(QPaintEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* e);

    void drawLabel(QPainter* painter, int Line, QString Text);
    void drawBullets(QPainter* painter, int line, bool ispb, bool isbpdisabled, bool isbookmark);
    bool isJump(int i) const;
    void drawJump(QPainter* painter, int startLine, int endLine, int jumpoffset, bool conditional, bool isexecute, bool isactive);

private:
    int_t topVA;
    int_t selectedVA;
    QFont m_DefaultFont;
    int fontWidth, fontHeight;
    int viewableRows;

    CPUDisassembly* CodePtr;
    QList<Instruction_t>* InstrBuffer;
    QColor backgroundColor;
    REGDUMP regDump;
};

#endif // CPUSIDEBAR_H
