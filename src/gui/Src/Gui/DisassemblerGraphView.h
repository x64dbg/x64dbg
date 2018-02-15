#ifndef DISASSEMBLERGRAPHVIEW_H
#define DISASSEMBLERGRAPHVIEW_H

// Based on the DisassemblerGraphView from x64dbg

#include <QWidget>
#include <QPainter>
#include <QShortcut>

#include "GraphView.h"
#include "RichTextPainter.h"
#include "CachedFontMetrics.h"

using RVA = size_t;

class DisassemblerGraphView : public GraphView
{
    Q_OBJECT


    struct Token
    {
        int start; //token[0]
        int length; //token[1]
        QString type; //token[2]
        ut64 addr; //token[3]
        QString name; //token[4]
    };

    struct HighlightToken
    {
        QString type; //highlight_token[0]
        ut64 addr; //highlight_token[1]
        QString name; //highlight_token[2]

        bool equalsToken(const Token & token)
        {
            return this->type == token.type &&
                   this->addr == token.addr &&
                   this->name == token.name;
        }

        static HighlightToken* fromToken(const Token & token)
        {
            //TODO: memory leaks
            auto result = new HighlightToken();
            result->type = token.type;
            result->addr = token.addr;
            result->name = token.name;
            return result;
        }
    };

    struct Text
    {
        std::vector<RichTextPainter::List> lines;

        Text() {}

        Text(const QString & text, QColor color, QColor background)
        {
            RichTextPainter::List richText;
            RichTextPainter::CustomRichText_t rt;
            rt.highlight = false;
            rt.text = text;
            rt.textColor = color;
            rt.textBackground = background;
            rt.flags = rt.textBackground.alpha() ? RichTextPainter::FlagAll : RichTextPainter::FlagColor;
            richText.push_back(rt);
            lines.push_back(richText);
        }

        Text(const RichTextPainter::List & richText)
        {
            lines.push_back(richText);
        }

        QString ToQString() const
        {
            QString result;
            for(auto & line : lines)
            {
                for(auto & t : line)
                {
                    result += t.text;
                }
            }
            return result;
        }
    };

    struct Instr
    {
        ut64 addr = 0;
        ut64 size = 0;
        Text text;
        Text fullText;
        std::vector<unsigned char> opcode; //instruction bytes
    };


    struct DisassemblyBlock
    {
        Text header_text;
        std::vector<Instr> instrs;
        ut64 entry = 0;
        ut64 true_path = 0;
        ut64 false_path = 0;
        bool terminal = false;
        bool indirectcall = false;
    };

    struct Function
    {
        bool ready;
        ut64 entry;
        ut64 update_id;
        std::vector<DisassemblyBlock> blocks;
    };

    struct Analysis
    {
        ut64 entry = 0;
        std::unordered_map<ut64, Function> functions;
        bool ready = false;
        ut64 update_id = 0;
        QString status = "Analyzing...";

        bool find_instr(ut64 addr, ut64 & func, ut64 & instr)
        {
            //TODO implement
            Q_UNUSED(addr);
            Q_UNUSED(func);
            Q_UNUSED(instr);
            return false;
        }

        //dummy class
    };

public:
    DisassemblerGraphView(QWidget* parent);
    ~DisassemblerGraphView();
    std::unordered_map<ut64, DisassemblyBlock> disassembly_blocks;
    virtual void drawBlock(QPainter & p, GraphView::GraphBlock & block) override;
    virtual void blockClicked(GraphView::GraphBlock & block, QMouseEvent* event, QPoint pos) override;
    virtual void blockDoubleClicked(GraphView::GraphBlock & block, QMouseEvent* event, QPoint pos) override;
    virtual bool helpEvent(QHelpEvent* event) override;
    virtual void blockHelpEvent(GraphView::GraphBlock & block, QHelpEvent* event, QPoint pos) override;
    virtual GraphView::EdgeConfiguration edgeConfiguration(GraphView::GraphBlock & from, GraphView::GraphBlock* to) override;
    virtual void blockTransitionedTo(GraphView::GraphBlock* to) override;

    void loadCurrentGraph();
    //    bool navigate(ut64 addr);

public slots:
    void refreshView();
    void colorsUpdatedSlot();
    void fontsUpdatedSlot();
    void onSeekChanged(RVA addr);

    void zoomIn();
    void zoomOut();
    void zoomReset();

    void takeTrue();
    void takeFalse();

    void nextInstr();
    void prevInstr();

private slots:
    void seekPrev();

private:
    bool first_draw = true;
    bool transition_dont_seek = false;
    bool sent_seek = false;

    HighlightToken* highlight_token;
    // Font data
    CachedFontMetrics* mFontMetrics;
    qreal charWidth;
    int charHeight;
    int charOffset;
    int baseline;

    //DisassemblyContextMenu* mMenu;

    void initFont();
    void prepareGraphNode(GraphBlock & block);
    RVA getAddrForMouseEvent(GraphBlock & block, QPoint* point);
    Instr* getInstrForMouseEvent(GraphBlock & block, QPoint* point);
    DisassemblyBlock* blockForAddress(RVA addr);
    void seek(RVA addr, bool update_viewport = true);
    void seekInstruction(bool previous_instr);

    //QList<QShortcut*> shortcuts;

    QColor disassemblyBackgroundColor;
    QColor disassemblySelectedBackgroundColor;
    QColor disassemblySelectionColor;
    QColor disassemblyTracedColor;
    QColor disassemblyTracedSelectionColor;
    QColor jmpColor;
    QColor brtrueColor;
    QColor brfalseColor;
    QColor retShadowColor;
    QColor indirectcallShadowColor;
    QColor mAutoCommentColor;
    QColor mAutoCommentBackgroundColor;
    QColor mCommentColor;
    QColor mCommentBackgroundColor;
    QColor mLabelColor;
    QColor mLabelBackgroundColor;
    QColor graphNodeColor;
    QColor mAddressColor;
    QColor mAddressBackgroundColor;
    QColor mCipColor;
    QColor mBreakpointColor;
    QColor mDisabledBreakpointColor;
};

#endif // DISASSEMBLERGRAPHVIEW_H
