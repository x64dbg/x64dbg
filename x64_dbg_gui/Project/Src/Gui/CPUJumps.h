#ifndef CPUJUMPS_H
#define CPUJUMPS_H

#include "NewTypes.h"
#include "Bridge.h"
#include "CPUDisassembly.h"
#include <QAbstractScrollArea>

class CPUJumps : public QAbstractScrollArea
{
    Q_OBJECT

    int_t topVA;
    int_t selectedVA;
    QPainter *painter;
    QFont m_DefaultFont;
    int    fontWidth, fontHeight;
    int viewableRows;

    CPUDisassembly *CodePtr;
    QList<Instruction_t> *InstrBuffer;
public:
    explicit CPUJumps(CPUDisassembly *Ptr, QWidget *parent = 0);
    QSize sizeHint() const;

    void drawStraightArrow(QPainter *painter, int x1, int y1, int x2, int y2);
signals:

public slots:

    void disassembleAt(int_t parVA, int_t parCIP);
    void repaint();
    void changeTopmostAddress(int i);
    void setViewableRows(int rows);
    void setSelection(int_t selVA);
protected:
    void paintEvent(QPaintEvent *event);
    void drawLabel(int Line, QString Text);
    void drawBullets(int line, bool ispb);
    bool isJump(int i) const;
    void drawJump(int startLine, int endLine, int jumpoffset, bool conditional, bool isexecute, bool isactive);
};

#endif // CPUJUMPS_H
