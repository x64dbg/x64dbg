#pragma once

#include <QObject>
#include <QWidget>
#include <QAbstractScrollArea>
#include <QPaintEvent>
#include <QTimer>
#include <QSize>
#include <QResizeEvent>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <QMutex>
#include "Bridge.h"
#include "RichTextPainter.h"
#include "QBeaEngine.h"
#include "ActionHelpers.h"
#include "VaHistory.h"

class MenuBuilder;
class CachedFontMetrics;
class GotoDialog;
class XrefBrowseDialog;
class CommonActions;

class DisassemblerGraphView : public QAbstractScrollArea, public ActionHelper<DisassemblerGraphView>
{
    Q_OBJECT
public:
    struct DisassemblerBlock;

    struct Point
    {
        int row; //point[0]
        int col; //point[1]
        int index; //point[2]
    };

    struct DisassemblerEdge
    {
        QColor color;
        DisassemblerBlock* dest;
        std::vector<Point> points;
        int start_index = 0;

        QPolygonF polyline;
        QPolygonF arrow;

        void addPoint(int row, int col, int index = 0)
        {
            Point point = {row, col, 0};
            this->points.push_back(point);
            if(int(this->points.size()) > 1)
                this->points[this->points.size() - 2].index = index;
        }
    };

    struct Text
    {
        // text to render; some words here may be colored (with highlighting mode)
        std::vector<RichTextPainter::List> lines;
        // tokens for selection in "Highlighting mode"; one "InstructionToken" per line
        std::vector<ZydisTokenizer::InstructionToken> lineTokens;

        Text() {}

        void addLine(const RichTextPainter::List & richText, const ZydisTokenizer::InstructionToken & tokens)
        {
            lines.push_back(richText);
            lineTokens.push_back(tokens);
        }

        // Highlight the given token and restore the original colors to the rest of the text
        void updateHighlighting(const ZydisTokenizer::SingleToken & highlightToken, QColor color, QColor background)
        {
            // assumption: the rich text 'lines' includes a 1:1 copy of the original tokens 'lineTokens'
            for(size_t nLine = 0; nLine < lines.size(); nLine++)
            {
                // based on the tokens X offset, find the first token in the rich text (skip RVA prefix)
                int i = 0, nRtOffset = 0;
                while(i < lineTokens[nLine].x && nRtOffset < (int)lines[nLine].size())
                {
                    i += lines[nLine][nRtOffset].text.length();
                    nRtOffset++;
                }

                // check if the rich text covers all the Zydis tokens
                if(lines[nLine].size() - nRtOffset < lineTokens[nLine].tokens.size())
                    continue; // normally should not happen

                for(size_t nToken = 0; nToken < lineTokens[nLine].tokens.size(); nToken++)
                {
                    auto & rt = lines[nLine][nToken + nRtOffset];
                    auto & token = lineTokens[nLine].tokens[nToken];

                    bool isEqual = ZydisTokenizer::TokenEquals(&token, &highlightToken);
                    auto tokenOrigColor = ZydisTokenizer::getTokenColor(token.type);
                    rt.textColor = isEqual ? color : tokenOrigColor.color;
                    rt.textBackground = isEqual ? background : tokenOrigColor.backgroundColor;
                }
            }
        }

        static RichTextPainter::CustomRichText_t makeRich(const QString & text, QColor color, QColor background)
        {
            RichTextPainter::CustomRichText_t rt;
            rt.underline = false;
            rt.text = text;
            rt.textColor = color;
            rt.textBackground = background;
            rt.flags = rt.textBackground.alpha() ? RichTextPainter::FlagAll : RichTextPainter::FlagColor;
            return rt;
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
            return std::move(result);
        }
    };

    struct Instr
    {
        duint addr = 0;
        Text text;
        std::vector<unsigned char> opcode; //instruction bytes
    };

    struct Block
    {
        Text header_text;
        std::vector<Instr> instrs;
        std::vector<duint> exits;
        duint entry = 0;
        duint true_path = 0;
        duint false_path = 0;
        bool terminal = false;
        bool indirectcall = false;
    };

    struct DisassemblerBlock
    {
        DisassemblerBlock() {}
        explicit DisassemblerBlock(Block & block)
            : block(block) {}

        Block block;
        std::vector<DisassemblerEdge> edges;
        std::vector<duint> incoming;
        std::vector<duint> new_exits;

        qreal x = 0.0;
        qreal y = 0.0;
        int width = 0;
        int height = 0;
        int col = 0;
        int col_count = 0;
        int row = 0;
        int row_count = 0;
    };

    struct Function
    {
        bool ready;
        duint entry;
        std::vector<Block> blocks;
    };

    struct Analysis
    {
        duint entry = 0;
        std::unordered_map<duint, Function> functions;
        bool ready = false;
        QString status = "Analyzing...";

        bool find_instr(duint addr, duint & func, duint & instr)
        {
            //TODO implement
            Q_UNUSED(addr);
            Q_UNUSED(func);
            Q_UNUSED(instr);
            return false;
        }

        //dummy class
    };

    enum class LayoutType
    {
        Wide,
        Medium,
        Narrow,
    };

    struct ClickPosition
    {
        QPoint pos = QPoint(0, 0);
        bool inBlock = false;
    };

    DisassemblerGraphView(QWidget* parent = nullptr);
    ~DisassemblerGraphView();
    void resetGraph();
    void initFont();
    void adjustSize(int viewportWidth, int viewportHeight, QPoint mousePosition = QPoint(0, 0), bool fitToWindow = false);
    void resizeEvent(QResizeEvent* event);
    duint get_cursor_pos();
    void set_cursor_pos(duint addr);
    std::tuple<duint, duint> get_selection_range();
    void set_selection_range(std::tuple<duint, duint> range);
    void copy_address();
    void paintNormal(QPainter & p, QRect & viewportRect, int xofs, int yofs);
    void paintOverview(QPainter & p, QRect & viewportRect, int xofs, int yofs);
    void paintEvent(QPaintEvent* event);
    bool isMouseEventInBlock(QMouseEvent* event);
    duint getInstrForMouseEvent(QMouseEvent* event);
    bool getTokenForMouseEvent(QMouseEvent* event, ZydisTokenizer::SingleToken & token);
    bool find_instr(duint addr, Instr & instr);
    void mousePressEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);
    void mouseDoubleClickEvent(QMouseEvent* event);
    void prepareGraphNode(DisassemblerBlock & block);
    void adjustGraphLayout(DisassemblerBlock & block, int col, int row);
    void computeGraphLayout(DisassemblerBlock & block);
    void setupContextMenu();
    void keyPressEvent(QKeyEvent* event);
    template<typename T>
    using Matrix = std::vector<std::vector<T>>;
    using EdgesVector = Matrix<std::vector<bool>>;
    bool isEdgeMarked(EdgesVector & edges, int row, int col, int index);
    void markEdge(EdgesVector & edges, int row, int col, int index, bool used = true);
    int findHorizEdgeIndex(EdgesVector & edges, int row, int min_col, int max_col);
    int findVertEdgeIndex(EdgesVector & edges, int col, int min_row, int max_row);
    DisassemblerEdge routeEdge(EdgesVector & horiz_edges, EdgesVector & vert_edges, Matrix<bool> & edge_valid, DisassemblerBlock & start, DisassemblerBlock & end, QColor color);
    void renderFunction(Function & func);
    void show_cur_instr(bool force = false);
    bool navigate(duint addr);
    void fontChanged();
    void setGraphLayout(LayoutType layout);
    void paintZoom(QPainter & p, QRect & viewportRect, int xofs, int yofs);
    void wheelEvent(QWheelEvent* event);
    void showEvent(QShowEvent* event);
    void zoomIn(QPoint mousePosition);
    void zoomOut(QPoint mousePosition);
    void showContextMenu(QMouseEvent* event);
    duint zoomActionHelper();

    VaHistory mHistory;

signals:
    void selectionChanged(dsint parVA);
    void displayLogWidget();
    void detachGraph();

public slots:
    void loadGraphSlot(BridgeCFGraphList* graph, duint addr);
    void graphAtSlot(duint addr);
    void updateGraphSlot();
    void colorsUpdatedSlot();
    void fontsUpdatedSlot();
    void shortcutsUpdatedSlot();
    void toggleOverviewSlot();
    void toggleSummarySlot();
    void selectionGetSlot(SELECTIONDATA* selection);
    void tokenizerConfigUpdatedSlot();
    void loadCurrentGraph();
    void disassembleAtSlot(dsint va, dsint cip);
    void gotoExpressionSlot();
    void gotoOriginSlot();
    void gotoPreviousSlot();
    void gotoNextSlot();
    void toggleSyncOriginSlot();
    void followActionSlot();
    void followDisassemblySlot();
    void refreshSlot();
    void saveImageSlot();
    void xrefSlot();
    void mnemonicHelpSlot();
    void fitToWindowSlot();
    void zoomToCursorSlot();
    void getCurrentGraphSlot(BridgeCFGraphList* graphList);
    void dbgStateChangedSlot(DBGSTATE state);
    void copyHighlightedTokenTextSlot();
    void copyHighlightedTokenValueSlot();
    void enableHighlightingModeSlot();

private:
    bool graphZoomMode;
    qreal zoomLevel;
    qreal zoomLevelOld;
    qreal zoomMinimum;
    qreal zoomMaximum;
    qreal zoomOverviewValue;
    qreal zoomStep;
    //qreal zoomScrollThreshold;
    int zoomDirection;
    int zoomBoost;
    ClickPosition lastRightClickPosition;
    QString status;
    Analysis analysis;
    duint function;
    QTimer* updateTimer;
    int baseline;
    qreal charWidth;
    int charHeight;
    int charOffset;
    int width;
    int height;
    int renderWidth;
    int renderHeight;
    int renderXOfs;
    int renderYOfs;
    duint cur_instr;
    int scroll_base_x;
    int scroll_base_y;
    bool scroll_mode;
    bool ready;
    bool viewportReady;
    int* desired_pos;
    std::unordered_map<duint, DisassemblerBlock> blocks;
    std::vector<int> col_edge_x;
    std::vector<int> row_edge_y;
    CachedFontMetrics* mFontMetrics;
    MenuBuilder* mMenuBuilder;
    CommonActions* mCommonActions;
    QMenu* mPluginMenu;
    bool drawOverview;
    bool onlySummary;
    bool syncOrigin;
    int overviewXOfs;
    int overviewYOfs;
    qreal overviewScale;
    duint mCip;
    bool forceCenter;
    bool saveGraph;
    bool mHistoryLock; //Don't add a history while going to previous/next
    LayoutType layoutType;

    MenuBuilder* mHighlightMenuBuilder;
    ZydisTokenizer::SingleToken mHighlightToken;
    bool mHighlightingModeEnabled;
    bool mPermanentHighlightingMode;

    QAction* mToggleOverview;
    QAction* mToggleSummary;
    QAction* mToggleSyncOrigin;
    QAction* mFitToWindow;
    QAction* mZoomToCursor;

    QColor disassemblyBackgroundColor;
    QColor disassemblySelectionColor;
    QColor disassemblyTracedColor;
    QColor disassemblyTracedSelectionColor;
    QColor jmpColor;
    QColor brtrueColor;
    QColor brfalseColor;
    QColor retShadowColor;
    QColor indirectcallShadowColor;
    QColor backgroundColor;
    QColor mAutoCommentColor;
    QColor mAutoCommentBackgroundColor;
    QColor mCommentColor;
    QColor mCommentBackgroundColor;
    QColor mLabelColor;
    QColor mLabelBackgroundColor;
    QColor mAddressColor;
    QColor mAddressBackgroundColor;
    QColor mCipColor;
    QColor mBreakpointColor;
    QColor mDisabledBreakpointColor;
    QColor mBookmarkBackgroundColor;
    QColor graphNodeColor;
    QColor graphNodeBackgroundColor;
    QColor graphCurrentShadowColor;
    QColor mInstructionHighlightColor;
    QColor mInstructionHighlightBackgroundColor;

    BridgeCFGraph currentGraph;
    std::unordered_map<duint, duint> currentBlockMap;
    QBeaEngine disasm;
    GotoDialog* mGoto;
    XrefBrowseDialog* mXrefDlg;

    void addReferenceAction(QMenu* menu, duint addr, const QString & description);
    bool getHighlightedTokenValueText(QString & text);
};
