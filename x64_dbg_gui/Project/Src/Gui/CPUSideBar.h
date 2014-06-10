#ifndef CPUSIDEBAR_H
#define CPUSIDEBAR_H

#include "NewTypes.h"
#include "Bridge.h"
#include "CPUDisassembly.h"
#include <QAbstractScrollArea>

class CPUSideBar : public QAbstractScrollArea
{
    Q_OBJECT
public:
    explicit CPUSideBar(CPUDisassembly *Ptr, QWidget *parent = 0);
    QSize sizeHint() const;
    void drawStraightArrow(QPainter *painter, int x1, int y1, int x2, int y2);

public slots:
    void disassembleAt(int_t parVA, int_t parCIP);
    void debugStateChangedSlot(DBGSTATE state);
    void repaint();
    void changeTopmostAddress(int i);
    void setViewableRows(int rows);
    void setSelection(int_t selVA);

protected:
    void paintEvent(QPaintEvent *event);
    void drawLabel(QPainter* painter, int Line, QString Text);
    void drawBullets(QPainter* painter, int line, bool ispb);
    bool isJump(int i) const;
    void drawJump(QPainter* painter, int startLine, int endLine, int jumpoffset, bool conditional, bool isexecute, bool isactive);

private:
    int_t topVA;
    int_t selectedVA;
    QFont m_DefaultFont;
    int fontWidth, fontHeight;
    int viewableRows;

    CPUDisassembly *CodePtr;
    QList<Instruction_t> *InstrBuffer;
    QColor backgroundColor;
};

#endif // CPUSIDEBAR_H
