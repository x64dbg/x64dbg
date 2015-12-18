#ifndef _GRAPH_NODE_H
#define _GRAPH_NODE_H

#include <QWidget>
#include <QFrame>
#include <QMouseEvent>
#include <QMessageBox>
#include <QFontMetrics>
#include <vector>
#include "Imports.h"
#include "RichTextPainter.h"
#include "capstone_gui.h"
#include "QBeaEngine.h"

class GraphNode : public QFrame
{
    Q_OBJECT

public:
    GraphNode();
    GraphNode(std::vector<Instruction_t> &instructionsVector, duint address = 0);
    GraphNode(const GraphNode & other);
    GraphNode & operator=(const GraphNode & other);
    QRectF boundingRect() const;
    void paintEvent(QPaintEvent* event);
    dsint getInstructionIndexAtPos(const QPoint &pos) const;
    bool eventFilter(QObject *object, QEvent *event);
//    void mousePressEvent(QMouseEvent* event);
    void updateTokensVector();
    void setInstructionsVector(const std::vector<Instruction_t> &instructionsVector);
    void updateCache();
    void updateRichText();
    QString getLongestInstruction();
    duint address();

signals:
    void drawGraphAt(duint va);

private:
    duint mHighlightInstructionAt;
    duint mAddress;
    duint mLineHeight;
    qreal mCachedWidth;
    qreal mCachedHeight;
    const duint mSpacingX = 12;
    const duint mSpacingY = 12;
    const duint mLineSpacingY = 3;
    QFont mFont = QFont("Lucida Console", 8, QFont::Normal, false);
    std::vector<Instruction_t> mInstructionsVector;
    std::vector<CapstoneTokenizer::InstructionToken> mTokensVector;
    std::vector<QList<RichTextPainter::CustomRichText_t> > mRichTextVector;
};

#endif //_GRAPH_NODE_H
