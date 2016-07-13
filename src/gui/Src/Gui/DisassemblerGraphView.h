#ifndef DISASSEMBLERGRAPHVIEW_H
#define DISASSEMBLERGRAPHVIEW_H

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
#include <algorithm>
#include <QMutex>
#include "Bridge.h"

class DisassemblerGraphView : public QAbstractScrollArea
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
            Point point;
            point.row = row;
            point.col = col;
            point.index = 0;
            this->points.push_back(point);
            if(int(this->points.size()) > 1)
                this->points[this->points.size() - 2].index = index;
        }
    };

    struct Token
    {
        int start; //token[0]
        int length; //token[1]
        QString type; //token[2]
        duint addr; //token[3]
        QString name; //token[4]
    };

    struct HighlightToken
    {
        QString type; //highlight_token[0]
        duint addr; //highlight_token[1]
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

    struct Line
    {
        QString text; //line[0]
        QColor color; //line[1]
    };

    struct Text
    {
        std::vector<std::vector<Line>> lines;
        std::vector<std::vector<Token>> tokens;

        Text() {}

        Text(const QString & text, QColor color, duint addr)
        {
            Token tok;
            tok.start = 0;
            tok.length = text.length();
            tok.type = "text";
            tok.addr = addr;
            tok.name = text;
            std::vector<Token> tv;
            tv.push_back(tok);
            tokens.push_back(tv);
            Line line;
            line.text = text;
            line.color = color;
            std::vector<Line> lv;
            lv.push_back(line);
            lines.push_back(lv);
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

        void print() const
        {
            puts("----BLOCK---");
            printf("header_text: %s\n", header_text.ToQString().toUtf8().constData());
            puts("exits:");
            for(auto exit : exits)
                printf("%X ", exit);
            puts("\n--ENDBLOCK--");
        }
    };

    struct DisassemblerBlock
    {
        DisassemblerBlock() {}
        explicit DisassemblerBlock(Block & block)
            : block(block) {}

        void print() const
        {
            block.print();
        }

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
        duint update_id;
        std::vector<Block> blocks;
    };

    struct Analysis
    {
        duint entry = 0;
        std::unordered_map<duint, Function> functions;
        bool ready = false;
        duint update_id = 0;
        QString status = "Analyzing...";

        bool find_instr(duint addr, duint & func, duint & instr)
        {
            Q_UNUSED(addr);
            Q_UNUSED(func);
            Q_UNUSED(instr);
            return false;
        }

        //dummy class
    };

    DisassemblerGraphView(const Analysis & analysis, QWidget* parent = nullptr);
    void initFont();
    void adjustSize(int width, int height);
    void resizeEvent(QResizeEvent* event);
    duint get_cursor_pos();
    void set_cursor_pos(duint addr);
    std::tuple<duint, duint> get_selection_range();
    void set_selection_range(std::tuple<duint, duint> range);
    void copy_address();
    //void analysis_thread_proc();
    //void closeRequest();
    void paintEvent(QPaintEvent* event);
    bool isMouseEventInBlock(QMouseEvent* event);
    duint getInstrForMouseEvent(QMouseEvent* event);
    bool getTokenForMouseEvent(QMouseEvent* event, Token & token);
    bool find_instr(duint addr, Instr & instr);
    void mousePressEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);
    void mouseDoubleClickEvent(QMouseEvent* event);
    void prepareGraphNode(DisassemblerBlock & block);
    void adjustGraphLayout(DisassemblerBlock & block, int col, int row);
    void computeGraphLayout(DisassemblerBlock & block);

    template<typename T>
    using Matrix = std::vector<std::vector<T>>;
    using EdgesVector = Matrix<std::vector<bool>>;
    bool isEdgeMarked(EdgesVector & edges, int row, int col, int index);
    void markEdge(EdgesVector & edges, int row, int col, int index);
    int findHorizEdgeIndex(EdgesVector & edges, int row, int min_col, int max_col);
    int findVertEdgeIndex(EdgesVector & edges, int col, int min_row, int max_row);
    DisassemblerEdge routeEdge(EdgesVector & horiz_edges, EdgesVector & vert_edges, Matrix<bool> & edge_valid, DisassemblerBlock & start, DisassemblerBlock & end, QColor color);
    void renderFunction(Function & func);
    void show_cur_instr();
    bool navigate(duint addr);
    void fontChanged();

public slots:
    void updateTimerEvent();

private:
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
    duint update_id;
    bool scroll_mode;
    bool ready;
    int* desired_pos;
    std::unordered_map<duint, DisassemblerBlock> blocks;
    HighlightToken* highlight_token;
    std::vector<int> col_edge_x;
    std::vector<int> row_edge_y;
};

#endif // DISASSEMBLERGRAPHVIEW_H
