#include "GraphView.h"

#include <vector>
#include <QPainter>
#include <QMouseEvent>
#include <QPropertyAnimation>
#include <QDebug>

GraphView::GraphView(QWidget* parent)
    : QAbstractScrollArea(parent)
{
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    horizontalScrollBar()->setSingleStep(charWidth);
    verticalScrollBar()->setSingleStep(charWidth);
    QSize areaSize = viewport()->size();
    adjustSize(areaSize.width(), areaSize.height());
}

GraphView::~GraphView()
{
    // TODO: Cleanups
}

// Vector functions
template<class T>
static void removeFromVec(std::vector<T> & vec, T elem)
{
    vec.erase(std::remove(vec.begin(), vec.end(), elem), vec.end());
}

template<class T>
static void initVec(std::vector<T> & vec, size_t size, T value)
{
    vec.resize(size);
    for(size_t i = 0; i < size; i++)
        vec[i] = value;
}

// Callbacks
void GraphView::drawBlock(QPainter & p, GraphView::GraphBlock & block)
{
    Q_UNUSED(p);
    Q_UNUSED(block);
    qWarning() << "Draw block not overriden!";
}

void GraphView::blockClicked(GraphView::GraphBlock & block, QMouseEvent* event, QPoint pos)
{
    Q_UNUSED(block);
    Q_UNUSED(event);
    Q_UNUSED(pos);
    qWarning() << "Block clicked not overridden!";
}

void GraphView::blockDoubleClicked(GraphView::GraphBlock & block, QMouseEvent* event, QPoint pos)
{
    Q_UNUSED(block);
    Q_UNUSED(event);
    Q_UNUSED(pos);
    qWarning() << "Block double clicked not overridden!";
}

void GraphView::blockHelpEvent(GraphView::GraphBlock & block, QHelpEvent* event, QPoint pos)
{
    Q_UNUSED(block);
    Q_UNUSED(event);
    Q_UNUSED(pos);
}

bool GraphView::helpEvent(QHelpEvent* event)
{
    int x = ((event->pos().x() - unscrolled_render_offset_x) / current_scale) + horizontalScrollBar()->value();
    int y = ((event->pos().y() - unscrolled_render_offset_y) / current_scale) + verticalScrollBar()->value();

    for(auto & blockIt : blocks)
    {
        GraphBlock & block = blockIt.second;

        if((block.x <= x) && (block.y <= y) &&
                (x <= block.x + block.width) & (y <= block.y + block.height))
        {
            QPoint pos = QPoint(x - block.x, y - block.y);
            blockHelpEvent(block, event, pos);
            return true;
        }
    }

    return false;
}

void GraphView::blockTransitionedTo(GraphView::GraphBlock* to)
{
    Q_UNUSED(to);
    qWarning() << "blockTransitionedTo not overridden!";
}

GraphView::EdgeConfiguration GraphView::edgeConfiguration(GraphView::GraphBlock & from, GraphView::GraphBlock* to)
{
    Q_UNUSED(from);
    Q_UNUSED(to);
    qWarning() << "Edge configuration not overridden!";
    EdgeConfiguration ec;
    return ec;
}

void GraphView::adjustSize(int new_width, int new_height)
{
    double hfactor = 0.0;
    double vfactor = 0.0;
    if(horizontalScrollBar()->maximum())
    {
        hfactor = (double)horizontalScrollBar()->value() / (double)horizontalScrollBar()->maximum();
    }
    if(verticalScrollBar()->maximum())
    {
        vfactor = (double)verticalScrollBar()->value() / (double)verticalScrollBar()->maximum();
    }

    //Update scroll bar information
    horizontalScrollBar()->setPageStep(new_width);
    horizontalScrollBar()->setRange(0, width - (new_width / current_scale));
    verticalScrollBar()->setPageStep(new_height);
    verticalScrollBar()->setRange(0, height - (new_height / current_scale));
    horizontalScrollBar()->setValue((int)((double)horizontalScrollBar()->maximum() * hfactor));
    verticalScrollBar()->setValue((int)((double)verticalScrollBar()->maximum() * vfactor));
}

bool GraphView::event(QEvent* event)
{
    if(event->type() == QEvent::ToolTip)
    {
        if(helpEvent(static_cast<QHelpEvent*>(event)))
        {
            return true;
        }
    }

    return QAbstractScrollArea::event(event);
}

// This calculates the full graph starting at block entry.
void GraphView::computeGraph(ut64 entry)
{
    QSize areaSize = viewport()->size();

    // Populate incoming lists
    for(auto & blockIt : blocks)
    {
        GraphBlock & block = blockIt.second;
        for(auto & edge : block.exits)
        {
            blocks[edge].incoming.push_back(block.entry);
        }
    }

    std::unordered_set<ut64> visited;
    visited.insert(entry);
    std::queue<ut64> queue;
    std::vector<ut64> block_order;
    queue.push(entry);

    bool changed = true;
    while(changed)
    {
        changed = false;

        // Pick nodes with single entrypoints
        while(!queue.empty())
        {
            GraphBlock & block = blocks[queue.front()];
            queue.pop();
            block_order.push_back(block.entry);
            for(ut64 edge : block.exits)
            {
                // Skip edge if we already visited it
                if(visited.count(edge))
                {
                    continue;
                }

                // Some edges might not be available
                if(!blocks.count(edge))
                {
                    continue;
                }

                // If this node has no other incoming edges, add it to the graph layout
                if(blocks[edge].incoming.size() == 1)
                {
                    removeFromVec(blocks[edge].incoming, block.entry);
                    block.new_exits.push_back(edge);
                    queue.push(blocks[edge].entry);
                    visited.insert(edge);
                    changed = true;
                }
                else
                {
                    // Remove from incoming edges
                    removeFromVec(blocks[edge].incoming, block.entry);
                }
            }
        }

        // No more nodes satisfy constraints, pick a node to continue constructing the graph
        ut64 best = 0;
        int best_edges;
        ut64 best_parent;
        for(auto & blockIt : blocks)
        {
            GraphBlock & block = blockIt.second;
            // Skip blocks we haven't visited yet
            if(!visited.count(block.entry))
            {
                continue;
            }
            for(ut64 edge : block.exits)
            {
                // If we already visited the exit, skip it
                if(visited.count(edge))
                {
                    continue;
                }
                if(!blocks.count(edge))
                {
                    continue;
                }
                // find best edge
                if((best == 0) || ((int)blocks[edge].incoming.size() < best_edges) || (
                            ((int)blocks[edge].incoming.size() == best_edges) && (edge < best)))
                {
                    best = edge;
                    best_edges = blocks[edge].incoming.size();
                    best_parent = block.entry;
                }
            }
        }
        if(best != 0)
        {
            GraphBlock & best_parentb = blocks[best_parent];
            removeFromVec(blocks[best].incoming, best_parentb.entry);
            best_parentb.new_exits.push_back(best);
            visited.insert(best);
            queue.push(best);
            changed = true;
        }
    }

    computeGraphLayout(blocks[entry]);

    // Prepare edge routing
    GraphBlock & entryb = blocks[entry];
    EdgesVector horiz_edges, vert_edges;
    horiz_edges.resize(entryb.row_count + 1);
    vert_edges.resize(entryb.row_count + 1);
    Matrix<bool> edge_valid;
    edge_valid.resize(entryb.row_count + 1);
    for(int row = 0; row < entryb.row_count + 1; row++)
    {
        horiz_edges[row].resize(entryb.col_count + 1);
        vert_edges[row].resize(entryb.col_count + 1);
        initVec(edge_valid[row], entryb.col_count + 1, true);
        for(int col = 0; col < entryb.col_count + 1; col++)
        {
            horiz_edges[row][col].clear();
            vert_edges[row][col].clear();
        }
    }

    for(auto & blockIt : blocks)
    {
        GraphBlock & block = blockIt.second;
        edge_valid[block.row][block.col + 1] = false;
    }

    // Perform edge routing
    for(ut64 block_id : block_order)
    {
        GraphBlock & block = blocks[block_id];
        GraphBlock & start = block;
        for(ut64 edge : block.exits)
        {
            GraphBlock & end = blocks[edge];
            start.edges.push_back(routeEdge(horiz_edges, vert_edges, edge_valid, start, end, QColor(255, 0, 0)));
        }
    }

    // Compute edge counts for each row and column
    std::vector<int> col_edge_count, row_edge_count;
    initVec(col_edge_count, entryb.col_count + 1, 0);
    initVec(row_edge_count, entryb.row_count + 1, 0);
    for(int row = 0; row < entryb.row_count + 1; row++)
    {
        for(int col = 0; col < entryb.col_count + 1; col++)
        {
            if(int(horiz_edges[row][col].size()) > row_edge_count[row])
                row_edge_count[row] = int(horiz_edges[row][col].size());
            if(int(vert_edges[row][col].size()) > col_edge_count[col])
                col_edge_count[col] = int(vert_edges[row][col].size());
        }
    }


    //Compute row and column sizes
    std::vector<int> col_width, row_height;
    initVec(col_width, entryb.col_count + 1, 0);
    initVec(row_height, entryb.row_count + 1, 0);
    for(auto & blockIt : blocks)
    {
        GraphBlock & block = blockIt.second;
        if((int(block.width / 2)) > col_width[block.col])
            col_width[block.col] = int(block.width / 2);
        if((int(block.width / 2)) > col_width[block.col + 1])
            col_width[block.col + 1] = int(block.width / 2);
        if(int(block.height) > row_height[block.row])
            row_height[block.row] = int(block.height);
    }

    // Compute row and column positions
    std::vector<int> col_x, row_y;
    initVec(col_x, entryb.col_count, 0);
    initVec(row_y, entryb.row_count, 0);
    initVec(col_edge_x, entryb.col_count + 1, 0);
    initVec(row_edge_y, entryb.row_count + 1, 0);
    int x = block_horizontal_margin * 2;
    for(int i = 0; i < entryb.col_count; i++)
    {
        col_edge_x[i] = x;
        x += block_horizontal_margin * col_edge_count[i];
        col_x[i] = x;
        x += col_width[i];
    }
    int y = block_vertical_margin * 2;
    for(int i = 0; i < entryb.row_count; i++)
    {
        row_edge_y[i] = y;
        // TODO: The 1 when row_edge_count is 0 is not needed on the original.. not sure why it's required for us
        if(!row_edge_count[i])
        {
            row_edge_count[i] = 1;
        }
        y += block_vertical_margin * row_edge_count[i];
        row_y[i] = y;
        y += row_height[i];
    }
    col_edge_x[entryb.col_count] = x;
    row_edge_y[entryb.row_count] = y;
    width = x + (block_horizontal_margin * 2) + (block_horizontal_margin * col_edge_count[entryb.col_count]);
    height = y + (block_vertical_margin * 2) + (block_vertical_margin * row_edge_count[entryb.row_count]);

    //Compute node positions
    for(auto & blockIt : blocks)
    {
        GraphBlock & block = blockIt.second;
        block.x = int(
                      (col_x[block.col] + col_width[block.col] + ((block_horizontal_margin / 2) * col_edge_count[block.col + 1])) - (block.width / 2));
        if((block.x + block.width) > (
                    col_x[block.col] + col_width[block.col] + col_width[block.col + 1] + block_horizontal_margin * col_edge_count[
                        block.col + 1]))
        {
            block.x = int((col_x[block.col] + col_width[block.col] + col_width[block.col + 1] + block_horizontal_margin * col_edge_count[
                               block.col + 1]) - block.width);
        }
        block.y = row_y[block.row];
    }

    // Precompute coordinates for edges
    for(auto & blockIt : blocks)
    {
        GraphBlock & block = blockIt.second;

        for(GraphEdge & edge : block.edges)
        {
            auto start = edge.points[0];
            auto start_col = start.col;
            auto last_index = edge.start_index;
            // This is the start point of the edge.
            auto first_pt = QPoint(col_edge_x[start_col] + (block_horizontal_margin * last_index) + (block_horizontal_margin / 2),
                                   block.y + block.height);
            auto last_pt = first_pt;
            QPolygonF pts;
            pts.append(last_pt);

            for(int i = 0; i < int(edge.points.size()); i++)
            {
                auto end = edge.points[i];
                auto end_row = end.row;
                auto end_col = end.col;
                auto last_index = end.index;
                QPoint new_pt;
                // block_vertical_margin/2 gives the margin from block to the horizontal lines
                if(start_col == end_col)
                    new_pt = QPoint(last_pt.x(), row_edge_y[end_row] + (block_vertical_margin * last_index) + (block_vertical_margin / 2));
                else
                    new_pt = QPoint(col_edge_x[end_col] + (block_horizontal_margin * last_index) + (block_horizontal_margin / 2), last_pt.y());
                pts.push_back(new_pt);
                last_pt = new_pt;
                start_col = end_col;
            }

            EdgeConfiguration ec = edgeConfiguration(block, edge.dest);

            auto new_pt = QPoint(last_pt.x(), edge.dest->y - 1);
            pts.push_back(new_pt);
            edge.polyline = pts;
            edge.color = ec.color;
            if(ec.start_arrow)
            {
                pts.clear();
                pts.append(QPoint(first_pt.x() - 3, first_pt.y() + 6));
                pts.append(QPoint(first_pt.x() + 3, first_pt.y() + 6));
                pts.append(first_pt);
                edge.arrow_start = pts;
            }
            if(ec.end_arrow)
            {
                pts.clear();
                pts.append(QPoint(new_pt.x() - 3, new_pt.y() - 6));
                pts.append(QPoint(new_pt.x() + 3, new_pt.y() - 6));
                pts.append(new_pt);
                edge.arrow_end = pts;
            }
        }
    }

    ready = true;

    viewport()->update();
    areaSize = viewport()->size();
    adjustSize(areaSize.width(), areaSize.height());
}

void GraphView::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter p(viewport());
    int render_offset_x = -horizontalScrollBar()->value() * current_scale;
    int render_offset_y = -verticalScrollBar()->value() * current_scale;
    int render_width = viewport()->size().width() / current_scale;
    int render_height = viewport()->size().height() / current_scale;

    // Do we have scrollbars?
    bool hscrollbar = horizontalScrollBar()->pageStep() < width;
    bool vscrollbar = verticalScrollBar()->pageStep() < height;

    // Draw background
    QRect viewportRect(viewport()->rect().topLeft(), viewport()->rect().bottomRight() - QPoint(1, 1));
    p.setBrush(backgroundColor);
    p.drawRect(viewportRect);
    p.setBrush(Qt::black);

    unscrolled_render_offset_x = 0;
    unscrolled_render_offset_y = 0;

    // We do not have a scrollbar on this axis, so we center the view
    if(!hscrollbar)
    {
        unscrolled_render_offset_x = (viewport()->size().width() - (width * current_scale)) / 2;
        render_offset_x += unscrolled_render_offset_x;
    }
    if(!vscrollbar)
    {
        unscrolled_render_offset_y = (viewport()->size().height() - (height * current_scale)) / 2;
        render_offset_y += unscrolled_render_offset_y;
    }

    p.translate(render_offset_x, render_offset_y);
    p.scale(current_scale, current_scale);


    // Draw blocks
    for(auto & blockIt : blocks)
    {
        GraphBlock & block = blockIt.second;

        // Check if block is visible
        if((block.x + block.width > -render_offset_x) ||
                (block.y + block.height > -render_offset_y) ||
                (-render_offset_x + render_width > block.x) ||
                (-render_offset_y + render_height > block.y))
        {
            // Only draw block if it is visible
            drawBlock(p, block);
        }

        p.setBrush(Qt::gray);

        // Always draw edges
        // TODO: Only draw edges if they are actually visible ...
        // Draw edges
        for(GraphEdge & edge : block.edges)
        {
            EdgeConfiguration ec = edgeConfiguration(block, edge.dest);
            QPen pen(edge.color);
            //            if(blockSelected)
            //                pen.setStyle(Qt::DashLine);
            p.setPen(pen);
            p.setBrush(edge.color);
            p.drawPolyline(edge.polyline);
            pen.setStyle(Qt::SolidLine);
            p.setPen(pen);
            if(ec.start_arrow)
            {
                p.drawConvexPolygon(edge.arrow_start);
            }
            if(ec.end_arrow)
            {
                p.drawConvexPolygon(edge.arrow_end);
            }
        }
    }
}

// Prepare graph
// This computes the position and (row/col based) size of the block
// Recursively calls itself for each child of the GraphBlock
void GraphView::computeGraphLayout(GraphBlock & block)
{
    int col = 0;
    int row_count = 1;
    int childColumn = 0;
    bool singleChild = block.new_exits.size() == 1;
    // Compute all children nodes
    for(size_t i = 0; i < block.new_exits.size(); i++)
    {
        ut64 edge = block.new_exits[i];
        GraphBlock & edgeb = blocks[edge];
        computeGraphLayout(edgeb);
        row_count = std::max(edgeb.row_count + 1, row_count);
        childColumn = edgeb.col;
    }

    if(layoutType != LayoutType::Wide && block.new_exits.size() == 2)
    {
        GraphBlock & left = blocks[block.new_exits[0]];
        GraphBlock & right = blocks[block.new_exits[1]];
        if(left.new_exits.size() == 0)
        {
            left.col = right.col - 2;
            int add = left.col < 0 ? - left.col : 0;
            adjustGraphLayout(right, add, 1);
            adjustGraphLayout(left, add, 1);
            col = right.col_count + add;
        }
        else if(right.new_exits.size() == 0)
        {
            adjustGraphLayout(left, 0, 1);
            adjustGraphLayout(right, left.col + 2, 1);
            col = std::max(left.col_count, right.col + 2);
        }
        else
        {
            adjustGraphLayout(left, 0, 1);
            adjustGraphLayout(right, left.col_count, 1);
            col = left.col_count + right.col_count;
        }
        block.col_count = std::max(2, col);
        if(layoutType == LayoutType::Medium)
        {
            block.col = (left.col + right.col) / 2;
        }
        else
        {
            block.col = singleChild ? childColumn : (col - 2) / 2;
        }
    }
    else
    {
        for(ut64 edge : block.new_exits)
        {
            adjustGraphLayout(blocks[edge], col, 1);
            col += blocks[edge].col_count;
        }
        if(col >= 2)
        {
            // Place this node centered over the child nodes
            block.col = singleChild ? childColumn : (col - 2) / 2;
            block.col_count = col;
        }
        else
        {
            //No child nodes, set single node's width (nodes are 2 columns wide to allow
            //centering over a branch)
            block.col = 0;
            block.col_count = 2;
        }
    }
    block.row = 0;
    block.row_count = row_count;
}

// Edge computing stuff
bool GraphView::isEdgeMarked(EdgesVector & edges, int row, int col, int index)
{
    if(index >= int(edges[row][col].size()))
        return false;
    return edges[row][col][index];
}

void GraphView::markEdge(EdgesVector & edges, int row, int col, int index, bool used)
{
    while(int(edges[row][col].size()) <= index)
        edges[row][col].push_back(false);
    edges[row][col][index] = used;
}

GraphView::GraphEdge GraphView::routeEdge(EdgesVector & horiz_edges, EdgesVector & vert_edges, Matrix<bool> & edge_valid, GraphBlock & start, GraphBlock & end, QColor color)
{
    GraphEdge edge;
    edge.color = color;
    edge.dest = &end;

    //Find edge index for initial outgoing line
    int i = 0;
    while(true)
    {
        if(!isEdgeMarked(vert_edges, start.row + 1, start.col + 1, i))
            break;
        i += 1;
    }
    markEdge(vert_edges, start.row + 1, start.col + 1, i);
    edge.addPoint(start.row + 1, start.col + 1);
    edge.start_index = i;
    bool horiz = false;

    //Find valid column for moving vertically to the target node
    int min_row, max_row;
    if(end.row < (start.row + 1))
    {
        min_row = end.row;
        max_row = start.row + 1;
    }
    else
    {
        min_row = start.row + 1;
        max_row = end.row;
    }
    int col = start.col + 1;
    if(min_row != max_row)
    {
        auto checkColumn = [min_row, max_row, &edge_valid](int column)
        {
            if(column < 0 || column >= int(edge_valid[min_row].size()))
                return false;
            for(int row = min_row; row < max_row; row++)
            {
                if(!edge_valid[row][column])
                {
                    return false;
                }
            }
            return true;
        };

        if(!checkColumn(col))
        {
            if(checkColumn(end.col + 1))
            {
                col = end.col + 1;
            }
            else
            {
                int ofs = 0;
                while(true)
                {
                    col = start.col + 1 - ofs;
                    if(checkColumn(col))
                    {
                        break;
                    }

                    col = start.col + 1 + ofs;
                    if(checkColumn(col))
                    {
                        break;
                    }

                    ofs += 1;
                }
            }
        }
    }

    if(col != (start.col + 1))
    {
        //Not in same column, need to generate a line for moving to the correct column
        int min_col, max_col;
        if(col < (start.col + 1))
        {
            min_col = col;
            max_col = start.col + 1;
        }
        else
        {
            min_col = start.col + 1;
            max_col = col;
        }
        int index = findHorizEdgeIndex(horiz_edges, start.row + 1, min_col, max_col);
        edge.addPoint(start.row + 1, col, index);
        horiz = true;
    }

    if(end.row != (start.row + 1))
    {
        //Not in same row, need to generate a line for moving to the correct row
        if(col == (start.col + 1))
            markEdge(vert_edges, start.row + 1, start.col + 1, i, false);
        int index = findVertEdgeIndex(vert_edges, col, min_row, max_row);
        if(col == (start.col + 1))
            edge.start_index = index;
        edge.addPoint(end.row, col, index);
        horiz = false;
    }

    if(col != (end.col + 1))
    {
        //Not in ending column, need to generate a line for moving to the correct column
        int min_col, max_col;
        if(col < (end.col + 1))
        {
            min_col = col;
            max_col = end.col + 1;
        }
        else
        {
            min_col = end.col + 1;
            max_col = col;
        }
        int index = findHorizEdgeIndex(horiz_edges, end.row, min_col, max_col);
        edge.addPoint(end.row, end.col + 1, index);
        horiz = true;
    }

    //If last line was horizontal, choose the ending edge index for the incoming edge
    if(horiz)
    {
        int index = findVertEdgeIndex(vert_edges, end.col + 1, end.row, end.row);
        edge.points[int(edge.points.size()) - 1].index = index;
    }

    return edge;
}


int GraphView::findHorizEdgeIndex(EdgesVector & edges, int row, int min_col, int max_col)
{
    //Find a valid index
    int i = 0;
    while(true)
    {
        bool valid = true;
        for(int col = min_col; col < max_col + 1; col++)
            if(isEdgeMarked(edges, row, col, i))
            {
                valid = false;
                break;
            }
        if(valid)
            break;
        i++;
    }

    //Mark chosen index as used
    for(int col = min_col; col < max_col + 1; col++)
        markEdge(edges, row, col, i);
    return i;
}

int GraphView::findVertEdgeIndex(EdgesVector & edges, int col, int min_row, int max_row)
{
    //Find a valid index
    int i = 0;
    while(true)
    {
        bool valid = true;
        for(int row = min_row; row < max_row + 1; row++)
            if(isEdgeMarked(edges, row, col, i))
            {
                valid = false;
                break;
            }
        if(valid)
            break;
        i++;
    }

    //Mark chosen index as used
    for(int row = min_row; row < max_row + 1; row++)
        markEdge(edges, row, col, i);
    return i;
}

void GraphView::showBlock(GraphBlock & block, bool animated)
{
    showBlock(&block, animated);
}

void GraphView::showBlock(GraphBlock* block, bool animated)
{
    int render_width = viewport()->size().width() / current_scale;


    // Show block middle of X
    int target_x = (block->x + (block->width / 2)) - (render_width / 2);
    int show_block_offset_y = 30;
    // But beginning of Y (so we show the top of the block)
    int target_y = block->y - show_block_offset_y;

    target_x = std::max(0, target_x);
    target_y = std::max(0, target_y);
    target_x = std::min(horizontalScrollBar()->maximum(), target_x);
    target_y = std::min(verticalScrollBar()->maximum(), target_y);
    if(animated)
    {
        QPropertyAnimation* animation_x = new QPropertyAnimation(horizontalScrollBar(), "value");
        animation_x->setDuration(500);
        animation_x->setStartValue(horizontalScrollBar()->value());
        animation_x->setEndValue(target_x);
        animation_x->setEasingCurve(QEasingCurve::InOutQuad);
        animation_x->start();
        QPropertyAnimation* animation_y = new QPropertyAnimation(verticalScrollBar(), "value");
        animation_y->setDuration(500);
        animation_y->setStartValue(verticalScrollBar()->value());
        animation_y->setEndValue(target_y);
        animation_y->setEasingCurve(QEasingCurve::InOutQuad);
        animation_y->start();
    }
    else
    {
        horizontalScrollBar()->setValue(target_x);
        verticalScrollBar()->setValue(target_y);
    }

    blockTransitionedTo(block);

    viewport()->update();
}

void GraphView::adjustGraphLayout(GraphBlock & block, int col, int row)
{
    block.col += col;
    block.row += row;
    for(ut64 edge : block.new_exits)
    {
        adjustGraphLayout(blocks[edge], col, row);
    }
}

void GraphView::addBlock(GraphView::GraphBlock block)
{
    blocks[block.entry] = block;
}


void GraphView::setEntry(ut64 e)
{
    entry = e;
}

bool GraphView::checkPointClicked(QPointF & point, int x, int y, bool above_y)
{
    int half_target_size = 5;
    if((point.x() - half_target_size < x) &&
            (point.y() - (above_y ? (2 * half_target_size) : 0) < y) &&
            (x < point.x() + half_target_size) &&
            (y < point.y() + (above_y ? 0 : (3 * half_target_size))))
    {
        return true;
    }
    return false;
}

void GraphView::resizeEvent(QResizeEvent* event)
{
    adjustSize(event->size().width(), event->size().height());
}

// Mouse events
void GraphView::mousePressEvent(QMouseEvent* event)
{
    int x = ((event->pos().x() - unscrolled_render_offset_x) / current_scale) + horizontalScrollBar()->value();
    int y = ((event->pos().y() - unscrolled_render_offset_y) / current_scale) + verticalScrollBar()->value();

    // Check if a block was clicked
    for(auto & blockIt : blocks)
    {
        GraphBlock & block = blockIt.second;

        if((block.x <= x) && (block.y <= y) &&
                (x <= block.x + block.width) & (y <= block.y + block.height))
        {
            QPoint pos = QPoint(x - block.x, y - block.y);
            blockClicked(block, event, pos);
            // Don't do anything else here! blockClicked might seek and
            // all our data is invalid then.
            return;
        }
    }

    // Check if a line beginning/end  was clicked
    for(auto & blockIt : blocks)
    {
        GraphBlock & block = blockIt.second;
        for(GraphEdge & edge : block.edges)
        {
            if(edge.polyline.length() < 2)
            {
                continue;
            }
            QPointF start = edge.polyline.first();
            QPointF end = edge.polyline.last();
            if(checkPointClicked(start, x, y))
            {
                showBlock(edge.dest, true);
                // TODO: Callback to child
                return;
                break;
            }
            if(checkPointClicked(end, x, y, true))
            {
                showBlock(block, true);
                // TODO: Callback to child
                return;
                break;
            }
        }
    }

    // No block was clicked
    if(event->button() == Qt::LeftButton)
    {
        //Left click outside any block, enter scrolling mode
        scroll_base_x = event->x();
        scroll_base_y = event->y();
        scroll_mode = true;
        setCursor(Qt::ClosedHandCursor);
        viewport()->grabMouse();
    }

}

void GraphView::mouseMoveEvent(QMouseEvent* event)
{
    if(scroll_mode)
    {
        int x_delta = scroll_base_x - event->x();
        int y_delta = scroll_base_y - event->y();
        scroll_base_x = event->x();
        scroll_base_y = event->y();
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() + x_delta);
        verticalScrollBar()->setValue(verticalScrollBar()->value() + y_delta);
    }
}

void GraphView::mouseDoubleClickEvent(QMouseEvent* event)
{
    int x = ((event->pos().x() - unscrolled_render_offset_x) / current_scale) + horizontalScrollBar()->value();
    int y = ((event->pos().y() - unscrolled_render_offset_y) / current_scale) + verticalScrollBar()->value();

    // Check if a block was clicked
    for(auto & blockIt : blocks)
    {
        GraphBlock & block = blockIt.second;

        if((block.x <= x) && (block.y <= y) &&
                (x <= block.x + block.width) & (y <= block.y + block.height))
        {
            QPoint pos = QPoint(x - block.x, y - block.y);
            blockDoubleClicked(block, event, pos);
            return;
        }
    }
}

void GraphView::mouseReleaseEvent(QMouseEvent* event)
{
    // TODO
    //    if(event->button() == Qt::ForwardButton)
    //        gotoNextSlot();
    //    else if(event->button() == Qt::BackButton)
    //        gotoPreviousSlot();

    if(event->button() != Qt::LeftButton)
        return;

    if(scroll_mode)
    {
        scroll_mode = false;
        setCursor(Qt::ArrowCursor);
        viewport()->releaseMouse();
    }
}
