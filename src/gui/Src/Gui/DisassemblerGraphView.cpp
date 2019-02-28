#include "DisassemblerGraphView.h"
#include "MenuBuilder.h"
#include "CachedFontMetrics.h"
#include "QBeaEngine.h"
#include "GotoDialog.h"
#include "XrefBrowseDialog.h"
#include "LineEditDialog.h"
#include "SnowmanView.h"
#include <vector>
#include <QPainter>
#include <QScrollBar>
#include <QClipboard>
#include <QApplication>
#include <QMimeData>
#include <QFileDialog>
#include <QMessageBox>
#include "BreakpointMenu.h"
#include "StringUtil.h"
#include "MiscUtil.h"

DisassemblerGraphView::DisassemblerGraphView(QWidget* parent)
    : QAbstractScrollArea(parent),
      mFontMetrics(nullptr),
      currentGraph(duint(0)),
      disasm(ConfigUint("Disassembler", "MaxModuleSize")),
      mCip(0),
      mGoto(nullptr),
      syncOrigin(false),
      forceCenter(false),
      layoutType(LayoutType::Medium),
      mHistoryLock(false),
      mXrefDlg(nullptr)
{
    this->status = "Loading...";

    //Start disassembly view at the entry point of the binary
    this->function = 0;
    this->ready = false;
    this->viewportReady = false;
    this->desired_pos = nullptr;
    this->highlight_token = nullptr;
    this->cur_instr = 0;
    this->scroll_base_x = 0;
    this->scroll_base_y = 0;
    this->scroll_mode = false;
    this->drawOverview = false;
    this->onlySummary = false;
    this->blocks.clear();
    this->saveGraph = false;

    //Initialize zoom values
    this->graphZoomMode = ConfigBool("Gui", "GraphZoomMode");
    this->zoomLevel = 1;
    this->zoomLevelOld = 1;
    this->zoomStep = 0.1;
    this->zoomOverviewValue = 0.5; //text is hidden
    this->zoomMaximum = 1;

    //zoomScrollThreshold (relative to viewport size) adds fixed free space around the graph in order to make it free to move/scale
    //0.9 makes at least 10% of the graph allways visible regardless of scroll value
    //0.5 - at least half of the graph
    //0 - no margins ()
    //this->zoomScrollThreshold = 1; // default now
    //TODO: implement ^^^

    this->initFont();

    //Initialize scroll bars
    this->width = 0;
    this->height = 0;
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    this->horizontalScrollBar()->setSingleStep(this->charWidth);
    this->verticalScrollBar()->setSingleStep(this->charHeight);

    //QSize areaSize = this->viewport()->size();             <-\
    //this->adjustSize(areaSize.width(), areaSize.height()); <-- useless at this point (?)

    //Setup context menu
    setupContextMenu();

    //Connect to bridge
    connect(Bridge::getBridge(), SIGNAL(loadGraph(BridgeCFGraphList*, duint)), this, SLOT(loadGraphSlot(BridgeCFGraphList*, duint)));
    connect(Bridge::getBridge(), SIGNAL(graphAt(duint)), this, SLOT(graphAtSlot(duint)));
    connect(Bridge::getBridge(), SIGNAL(updateGraph()), this, SLOT(updateGraphSlot()));
    connect(Bridge::getBridge(), SIGNAL(selectionGraphGet(SELECTIONDATA*)), this, SLOT(selectionGetSlot(SELECTIONDATA*)));
    connect(Bridge::getBridge(), SIGNAL(disassembleAt(dsint, dsint)), this, SLOT(disassembleAtSlot(dsint, dsint)));
    connect(Bridge::getBridge(), SIGNAL(focusGraph()), this, SLOT(setFocus()));

    //Connect to config
    connect(Config(), SIGNAL(colorsUpdated()), this, SLOT(colorsUpdatedSlot()));
    connect(Config(), SIGNAL(fontsUpdated()), this, SLOT(fontsUpdatedSlot()));
    connect(Config(), SIGNAL(shortcutsUpdated()), this, SLOT(shortcutsUpdatedSlot()));
    connect(Config(), SIGNAL(tokenizerConfigUpdated()), this, SLOT(tokenizerConfigUpdatedSlot()));

    //colorsUpdatedSlot(); <-- already called somewhere
}

DisassemblerGraphView::~DisassemblerGraphView()
{
    delete this->highlight_token;
}

void DisassemblerGraphView::initFont()
{
    setFont(ConfigFont("Disassembly"));
    QFontMetricsF metrics(this->font());
    this->baseline = int(metrics.ascent());
    this->charWidth = metrics.width('X');
    this->charHeight = metrics.height();
    this->charOffset = 0;
    if(mFontMetrics)
        delete mFontMetrics;
    mFontMetrics = new CachedFontMetrics(this, font());
}

void DisassemblerGraphView::adjustSize(int viewportWidth, int viewportHeight, QPoint mousePosition, bool fitToWindow)
{
    //bugfix - resize event (during several initial calls) may reset correct adjustment already made
    if(viewportHeight < 30)
        return;

    //When zoom mode is enabled, graph turns to a free scallable object with ability to zoom in/out on any point (like IDA)
    if(graphZoomMode)
    {
        qreal graphWidth = this->width;
        qreal graphHeight = this->height;
        int hScrollRange;
        int vScrollRange;

        renderWidth = graphWidth * zoomLevel;
        renderHeight = graphHeight * zoomLevel;
        renderXOfs = viewportWidth;
        renderYOfs = viewportHeight;

        //Adjust scroll bar range
        hScrollRange = renderWidth + viewportWidth;
        vScrollRange = renderHeight + viewportHeight;

        QPointF deltaMouseReal;
        QPointF deltaMouseDiff;

        //Addition to scrool position depending on viewport offset
        qreal zoomStepReal; //real step which may differ from default step
        if(zoomDirection > 0)
        {
            zoomStepReal = (zoomLevel - zoomLevelOld) / zoomLevel;
            zoomStepReal /= (1 - zoomStepReal);
        }
        else
        {
            zoomStepReal = (zoomLevelOld - zoomLevel) / zoomLevelOld;
        }
        qreal deltaXOfs = renderXOfs *  zoomStepReal * zoomDirection;
        qreal deltaYOfs = renderYOfs * zoomStepReal * zoomDirection;


        QPoint scrollPositionOld = QPoint(horizontalScrollBar()->value(), verticalScrollBar()->value());

        horizontalScrollBar()->setPageStep(viewportWidth);
        horizontalScrollBar()->setRange(0, hScrollRange);
        verticalScrollBar()->setPageStep(viewportHeight);
        verticalScrollBar()->setRange(0, vScrollRange);

        if(!mousePosition.isNull()) //Adjust to cursor position
        {
            deltaMouseReal = mousePosition / zoomLevelOld + scrollPositionOld / zoomLevelOld;
            deltaMouseDiff = deltaMouseReal * zoomLevel - deltaMouseReal * zoomLevelOld;

            horizontalScrollBar()->setValue(qRound(scrollPositionOld.x() + deltaMouseDiff.x() - deltaXOfs));
            verticalScrollBar()->setValue(qRound(scrollPositionOld.y() + deltaMouseDiff.y() - deltaYOfs));
        }
        else if(fitToWindow) //Fit to window or 50/50
        {
            horizontalScrollBar()->setValue(hScrollRange / 2);
            verticalScrollBar()->setValue(vScrollRange / 2);
        }
    }
    else //zoom mode is disabled
    {
        this->renderWidth = this->width;
        this->renderHeight = this->height;
        this->renderXOfs = 0;
        this->renderYOfs = 0;
        if(this->renderWidth < viewportWidth)
        {
            this->renderXOfs = (viewportWidth - this->renderWidth) / 2;
            this->renderWidth = viewportWidth;
        }
        if(this->renderHeight < viewportHeight)
        {
            this->renderYOfs = (viewportHeight - this->renderHeight) / 2;
            this->renderHeight = viewportHeight;
        }
        //Update scroll bar information
        this->horizontalScrollBar()->setPageStep(viewportWidth);
        this->horizontalScrollBar()->setRange(0, this->renderWidth - viewportWidth);
        this->verticalScrollBar()->setPageStep(viewportHeight);
        this->verticalScrollBar()->setRange(0, this->renderHeight - viewportHeight);
    }
}

void DisassemblerGraphView::showEvent(QShowEvent* event)
{
    //before graph tab is shown for the first time the viewport has uninitialized width and height
    if(!this->viewportReady)
        this->viewportReady = true;

    event->ignore();
}

void DisassemblerGraphView::resizeEvent(QResizeEvent* event)
{
    adjustSize(event->size().width(), event->size().height());
}

duint DisassemblerGraphView::get_cursor_pos()
{
    if(this->cur_instr == 0)
        return this->function;
    return this->cur_instr;
}

void DisassemblerGraphView::set_cursor_pos(duint addr)
{
    if(!this->navigate(addr))
    {
        //TODO: show in hex editor?
    }
}

std::tuple<duint, duint> DisassemblerGraphView::get_selection_range()
{
    return std::make_tuple<duint, duint>(get_cursor_pos(), get_cursor_pos());
}

void DisassemblerGraphView::set_selection_range(std::tuple<duint, duint> range)
{
    this->set_cursor_pos(std::get<0>(range));
}

void DisassemblerGraphView::copy_address()
{
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->clear();
    QMimeData mime;
    mime.setText(QString().sprintf("0x%p", this->get_cursor_pos()));
    clipboard->setMimeData(&mime);
}

void DisassemblerGraphView::zoomIn(QPoint mousePosition)
{
    zoomLevelOld = zoomLevel;

    if(zoomLevel == zoomMaximum)
    {
        return;
    }
    //unlike addition, multiplication (by 1/x, x<1) makes zooming more smooth
    zoomLevel /= (1 - zoomStep * zoomBoost);

    if(zoomLevel > zoomMaximum)
    {
        zoomLevel = zoomMaximum;
    }
    auto areaSize = viewport()->size();
    adjustSize(areaSize.width(), areaSize.height(), mousePosition);
    this->viewport()->update();
}

void DisassemblerGraphView::zoomOut(QPoint mousePosition)
{
    zoomLevelOld = zoomLevel;

    if(zoomLevel == zoomMinimum)
    {
        return;
    }
    //unlike subtraction, multiplication makes zooming more smooth
    zoomLevel *= (1 - zoomStep * zoomBoost);

    if(zoomLevel < zoomMinimum)
    {
        zoomLevel = zoomMinimum;
    }

    auto areaSize = viewport()->size();
    adjustSize(areaSize.width(), areaSize.height(), mousePosition);
    this->viewport()->update();
}

void DisassemblerGraphView::paintNormal(QPainter & p, QRect & viewportRect, int xofs, int yofs)
{
    //Translate the painter
    auto dbgfunctions = DbgFunctions();
    QPoint translation(this->renderXOfs - xofs, this->renderYOfs - yofs);
    p.translate(translation);
    viewportRect.translate(-translation.x(), -translation.y());

    //Render each node
    for(auto & blockIt : this->blocks)
    {
        DisassemblerBlock & block = blockIt.second;
        bool blockSelected = false;
        for(const Instr & instr : block.block.instrs)
        {
            if(instr.addr == this->cur_instr)
            {
                blockSelected = true;
            }
        }

        //Ignore blocks that are not in view
        if(viewportRect.intersects(QRect(block.x + this->charWidth, block.y + this->charWidth,
                                         block.width - (2 * this->charWidth), block.height - (2 * this->charWidth))))
        {
            //Render shadow
            p.setPen(QColor(0, 0, 0, 0));
            if(block.block.terminal)
                p.setBrush(retShadowColor);
            else if(block.block.indirectcall)
                p.setBrush(indirectcallShadowColor);
            else
                p.setBrush(QColor(0, 0, 0, 128));
            p.drawRect(block.x + this->charWidth + 4, block.y + this->charWidth + 4,
                       block.width - (4 + 2 * this->charWidth), block.height - (4 + 2 * this->charWidth));

            //Render node background
            p.setPen(graphNodeColor);
            p.setBrush(disassemblyBackgroundColor);
            p.drawRect(block.x + this->charWidth, block.y + this->charWidth,
                       block.width - (4 + 2 * this->charWidth), block.height - (4 + 2 * this->charWidth));

            //Print current instruction background
            if(this->cur_instr != 0)
            {
                int y = block.y + (2 * this->charWidth) + (int(block.block.header_text.lines.size()) * this->charHeight);
                for(Instr & instr : block.block.instrs)
                {
                    if(y > viewportRect.y() - int(instr.text.lines.size()) * this->charHeight && y < viewportRect.bottom())
                    {
                        auto selected = instr.addr == this->cur_instr;
                        auto traceCount = dbgfunctions->GetTraceRecordHitCount(instr.addr);

                        if(selected && traceCount)
                        {
                            p.fillRect(QRect(block.x + this->charWidth + 3, y, block.width - (10 + 2 * this->charWidth),
                                             int(instr.text.lines.size()) * this->charHeight), disassemblyTracedSelectionColor);
                        }
                        else if(selected)
                        {
                            p.fillRect(QRect(block.x + this->charWidth + 3, y, block.width - (10 + 2 * this->charWidth),
                                             int(instr.text.lines.size()) * this->charHeight), disassemblySelectionColor);
                        }
                        else if(traceCount)
                        {
                            // Color depending on how often a sequence of code is executed
                            int exponent = 1;
                            while(traceCount >>= 1) //log2(traceCount)
                                exponent++;
                            int colorDiff = (exponent * exponent) / 2;

                            // If the user has a light trace background color, substract
                            if(disassemblyTracedColor.blue() > 160)
                                colorDiff *= -1;

                            p.fillRect(QRect(block.x + this->charWidth + 3, y, block.width - (10 + 2 * this->charWidth), int(instr.text.lines.size()) * this->charHeight),
                                       QColor(disassemblyTracedColor.red(),
                                              disassemblyTracedColor.green(),
                                              std::max(0, std::min(256, disassemblyTracedColor.blue() + colorDiff))));
                        }
                    }
                    y += int(instr.text.lines.size()) * this->charHeight;
                }
            }

            //Render node text
            auto x = block.x + (2 * this->charWidth);
            auto y = block.y + (2 * this->charWidth);
            for(auto & line : block.block.header_text.lines)
            {
                if(y > viewportRect.y() - this->charHeight && y < viewportRect.bottom())
                {
                    RichTextPainter::paintRichText(&p, x, y, block.width, this->charHeight, 0, line, mFontMetrics);
                }
                y += this->charHeight;
            }

            for(Instr & instr : block.block.instrs)
            {
                for(auto & line : instr.text.lines)
                {
                    if(y > viewportRect.y() - this->charHeight && y < viewportRect.bottom())
                    {
                        int rectSize = qRound(this->charWidth);
                        if(rectSize % 2)
                            rectSize++;

                        // Assume charWidth <= charHeight
                        QRectF bpRect(x - rectSize / 3.0, y + (this->charHeight - rectSize) / 2.0, rectSize, rectSize);

                        bool isbp = DbgGetBpxTypeAt(instr.addr) != bp_none;
                        bool isbpdisabled = DbgIsBpDisabled(instr.addr);
                        bool iscip = instr.addr == mCip;

                        if(isbp || isbpdisabled)
                        {
                            if(iscip)
                            {
                                // Left half is cip
                                bpRect.setWidth(bpRect.width() / 2);
                                p.fillRect(bpRect, mCipColor);

                                // Right half is breakpoint
                                bpRect.translate(bpRect.width(), 0);
                            }

                            p.fillRect(bpRect, isbp ? mBreakpointColor : mDisabledBreakpointColor);
                        }
                        else if(iscip)
                            p.fillRect(bpRect, mCipColor);

                        RichTextPainter::paintRichText(&p, x + this->charWidth, y, block.width - this->charWidth, this->charHeight, 0, line, mFontMetrics);
                    }
                    y += this->charHeight;
                }
            }
        }

        // Render edges
        for(DisassemblerEdge & edge : block.edges)
        {
            QPen pen(edge.color);
            if(blockSelected)
                pen.setStyle(Qt::DashLine);
            p.setPen(pen);
            p.setBrush(edge.color);
            p.drawPolyline(edge.polyline);
            pen.setStyle(Qt::SolidLine);
            p.setPen(pen);
            p.drawConvexPolygon(edge.arrow);
        }
    }
}

void DisassemblerGraphView::paintOverview(QPainter & p, QRect & viewportRect, int xofs, int yofs)
{
    // Scale and translate painter
    auto dbgfunctions = DbgFunctions();
    qreal sx = qreal(viewportRect.width()) / qreal(this->renderWidth);
    qreal sy = qreal(viewportRect.height()) / qreal(this->renderHeight);
    qreal s = qMin(sx, sy);
    this->overviewScale = s;
    if(sx < sy)
    {
        this->overviewXOfs = this->renderXOfs * s;
        this->overviewYOfs = this->renderYOfs * s + (qreal(this->renderHeight) * sy - qreal(this->renderHeight) * s) / 2;
    }
    else if(sy < sx)
    {
        this->overviewXOfs = this->renderXOfs * s + (qreal(this->renderWidth) * sx - qreal(this->renderWidth) * s) / 2;
        this->overviewYOfs = this->renderYOfs * s;
    }
    else
    {
        this->overviewXOfs = this->renderXOfs;
        this->overviewYOfs = this->renderYOfs;
    }
    p.translate(this->overviewXOfs, this->overviewYOfs);
    p.scale(s, s);

    // Scaled pen
    QPen pen;
    qreal penWidth = 1.0 / s;
    pen.setWidthF(penWidth);

    //Render each node
    duint cipBlock = 0;
    auto found = currentBlockMap.find(mCip);
    if(found != currentBlockMap.end())
        cipBlock = found->second;
    for(auto & blockIt : this->blocks)
    {
        DisassemblerBlock & block = blockIt.second;

        // Render edges
        for(DisassemblerEdge & edge : block.edges)
        {
            pen.setColor(edge.color);
            p.setPen(pen);
            p.setBrush(edge.color);
            p.drawPolyline(edge.polyline);
            p.drawConvexPolygon(edge.arrow);
        }

        //Get block metadata
        auto traceCount = dbgfunctions->GetTraceRecordHitCount(block.block.entry);
        auto isCip = block.block.entry == cipBlock;

        //Render shadow
        p.setPen(QColor(0, 0, 0, 0));
        if((isCip || traceCount) && block.block.terminal)
            p.setBrush(retShadowColor);
        else if((isCip || traceCount) && block.block.indirectcall)
            p.setBrush(indirectcallShadowColor);
        else if(isCip)
            p.setBrush(QColor(0, 0, 0, 0));
        else
            p.setBrush(QColor(0, 0, 0, 128));
        p.drawRect(block.x + this->charWidth + 4, block.y + this->charWidth + 4,
                   block.width - (4 + 2 * this->charWidth), block.height - (4 + 2 * this->charWidth));

        //Render node background
        pen.setColor(graphNodeColor);
        p.setPen(pen);
        if(isCip)
            p.setBrush(mCipColor);
        else if(traceCount)
        {
            // Color depending on how often a sequence of code is executed
            int exponent = 1;
            while(traceCount >>= 1) //log2(traceCount)
                exponent++;
            int colorDiff = (exponent * exponent) / 2;

            // If the user has a light trace background color, substract
            if(disassemblyTracedColor.blue() > 160)
                colorDiff *= -1;

            p.setBrush(QColor(disassemblyTracedColor.red(),
                              disassemblyTracedColor.green(),
                              std::max(0, std::min(256, disassemblyTracedColor.blue() + colorDiff))));
        }
        else if(block.block.terminal)
            p.setBrush(retShadowColor);
        else if(block.block.indirectcall)
            p.setBrush(indirectcallShadowColor);
        else
            p.setBrush(disassemblyBackgroundColor);
        p.drawRect(block.x + this->charWidth, block.y + this->charWidth,
                   block.width - (4 + 2 * this->charWidth), block.height - (4 + 2 * this->charWidth));
    }

    // Draw viewport selection
    if(s < 1.0)
    {
        QPoint translation(this->renderXOfs - xofs, this->renderYOfs - yofs);
        viewportRect.translate(-translation.x(), -translation.y());
        p.setPen(QPen(graphNodeColor, penWidth, Qt::DotLine));
        p.setBrush(Qt::transparent);
        p.drawRect(viewportRect);
    }
}

void DisassemblerGraphView::paintZoom(QPainter & p, QRect & viewportRect, int xofs, int yofs)
{
    //Based on paintNormal and paintOverview (depending on zoom level)

    auto dbgfunctions = DbgFunctions();

    QPoint translation(this->renderXOfs - xofs, this->renderYOfs - yofs);
    p.translate(translation);
    p.scale(zoomLevel, zoomLevel);

    //Adjust imaginary viewport rect at new zoom level
    viewportRect.setWidth(viewportRect.width() / zoomLevel);
    viewportRect.setHeight(viewportRect.height() / zoomLevel);
    viewportRect.translate(-translation.x() / zoomLevel, -translation.y() / zoomLevel);

    //Current block
    duint cipBlock = 0;
    auto found = currentBlockMap.find(mCip);
    if(found != currentBlockMap.end())
        cipBlock = found->second;

    //Scaled pen
    QPen pen;
    qreal penWidth = 1 / zoomLevel;
    pen.setColor(graphNodeColor);
    pen.setWidthF(penWidth);

    //Render each node
    for(auto & blockIt : this->blocks)
    {
        DisassemblerBlock & block = blockIt.second;

        bool blockSelected = false;
        for(const Instr & instr : block.block.instrs)
        {
            if(instr.addr == this->cur_instr)
            {
                blockSelected = true;
            }
        }

        //Ignore blocks that are not in view
        if(viewportRect.intersects(QRect(block.x + this->charWidth, block.y + this->charWidth,
                                         block.width - (2 * this->charWidth), block.height - (2 * this->charWidth))))
        {
            //Get block metadata
            auto isCip = block.block.entry == cipBlock;
            auto traceCount = dbgfunctions->GetTraceRecordHitCount(block.block.entry);

            if(zoomLevel > zoomOverviewValue) //Normal mode
            {
                //Render shadow
                p.setPen(QColor(0, 0, 0, 0));
                if(isCip)
                    p.setBrush(graphCurrentShadowColor);
                else if(block.block.terminal)
                    p.setBrush(retShadowColor);
                else if(block.block.indirectcall)
                    p.setBrush(indirectcallShadowColor);
                else
                    p.setBrush(QColor(0, 0, 0, 100));
                p.drawRect(block.x + this->charWidth + 4, block.y + this->charWidth + 4,
                           block.width - (4 + 2 * this->charWidth), block.height - (4 + 2 * this->charWidth));

                //Node background
                pen.setColor(graphNodeColor);
                p.setPen(pen);
                p.setBrush(graphNodeBackgroundColor);
                p.drawRect(block.x + this->charWidth, block.y + this->charWidth,
                           block.width - (4 + 2 * this->charWidth), block.height - (4 + 2 * this->charWidth));

                //Print current instruction background
                if(this->cur_instr != 0)
                {
                    int y = block.y + (2 * this->charWidth) + (int(block.block.header_text.lines.size()) * this->charHeight);
                    for(Instr & instr : block.block.instrs)
                    {
                        if(y > viewportRect.y() - int(instr.text.lines.size()) * this->charHeight && y < viewportRect.bottom())
                        {
                            auto selected = instr.addr == this->cur_instr;
                            auto traceCount = dbgfunctions->GetTraceRecordHitCount(instr.addr);

                            if(selected && traceCount)
                            {
                                p.fillRect(QRect(block.x + this->charWidth + 3, y, block.width - (10 + 2 * this->charWidth),
                                                 int(instr.text.lines.size()) * this->charHeight), disassemblyTracedSelectionColor);
                            }
                            else if(selected)
                            {
                                p.fillRect(QRect(block.x + this->charWidth + 3, y, block.width - (10 + 2 * this->charWidth),
                                                 int(instr.text.lines.size()) * this->charHeight), disassemblySelectionColor);
                            }
                            else if(traceCount)
                            {
                                // Color depending on how often a sequence of code is executed
                                int exponent = 1;
                                while(traceCount >>= 1) //log2(traceCount)
                                    exponent++;
                                int colorDiff = (exponent * exponent) / 2;

                                // If the user has a light trace background color, substract
                                if(disassemblyTracedColor.blue() > 160)
                                    colorDiff *= -1;

                                p.fillRect(QRect(block.x + this->charWidth + 3, y, block.width - (10 + 2 * this->charWidth), int(instr.text.lines.size()) * this->charHeight),
                                           QColor(disassemblyTracedColor.red(),
                                                  disassemblyTracedColor.green(),
                                                  std::max(0, std::min(256, disassemblyTracedColor.blue() + colorDiff))));
                            }
                        }
                        y += int(instr.text.lines.size()) * this->charHeight;
                    }
                }

                //Render node text
                auto x = block.x + (2 * this->charWidth);
                auto y = block.y + (2 * this->charWidth);
                for(auto & line : block.block.header_text.lines)
                {
                    if(y > viewportRect.y() - this->charHeight && y < viewportRect.bottom())
                    {
                        RichTextPainter::paintRichText(&p, x, y, block.width, this->charHeight, 0, line, mFontMetrics);
                    }
                    y += this->charHeight;
                }

                for(Instr & instr : block.block.instrs)
                {
                    for(auto & line : instr.text.lines)
                    {
                        if(y > viewportRect.y() - this->charHeight && y < viewportRect.bottom())
                        {
                            int rectSize = qRound(this->charWidth);
                            if(rectSize % 2)
                                rectSize++;

                            // Assume charWidth <= charHeight
                            QRectF bpRect(x - rectSize / 3.0, y + (this->charHeight - rectSize) / 2.0, rectSize, rectSize);

                            bool isbp = DbgGetBpxTypeAt(instr.addr) != bp_none;
                            bool isbpdisabled = DbgIsBpDisabled(instr.addr);
                            bool iscip = instr.addr == mCip;

                            if(isbp || isbpdisabled)
                            {
                                if(iscip)
                                {
                                    // Left half is cip
                                    bpRect.setWidth(bpRect.width() / 2);
                                    p.fillRect(bpRect, mCipColor);

                                    // Right half is breakpoint
                                    bpRect.translate(bpRect.width(), 0);
                                }

                                p.fillRect(bpRect, isbp ? mBreakpointColor : mDisabledBreakpointColor);
                            }
                            else if(iscip)
                                p.fillRect(bpRect, mCipColor);

                            RichTextPainter::paintRichText(&p, x + this->charWidth, y, block.width - this->charWidth, this->charHeight, 0, line, mFontMetrics);
                        }
                        y += this->charHeight;
                    }
                }

            }
            else //Overview mode
            {
                pen.setColor(graphNodeColor);
                p.setPen(pen);
                if(isCip)
                    p.setBrush(graphCurrentShadowColor);
                else if(traceCount)
                {
                    // Color depending on how often a sequence of code is executed
                    int exponent = 1;
                    while(traceCount >>= 1) //log2(traceCount)
                        exponent++;
                    int colorDiff = (exponent * exponent) / 2;

                    // If the user has a light trace background color, substract
                    if(disassemblyTracedColor.blue() > 160)
                        colorDiff *= -1;

                    p.setBrush(QColor(disassemblyTracedColor.red(),
                                      disassemblyTracedColor.green(),
                                      std::max(0, std::min(256, disassemblyTracedColor.blue() + colorDiff))));
                }
                else if(block.block.terminal)
                    p.setBrush(retShadowColor);
                else if(block.block.indirectcall)
                    p.setBrush(indirectcallShadowColor);
                else
                    p.setBrush(graphNodeBackgroundColor);
                p.drawRect(block.x + this->charWidth, block.y + this->charWidth,
                           block.width - (4 + 2 * this->charWidth), block.height - (4 + 2 * this->charWidth));
            }
        }
        // Render edges
        for(DisassemblerEdge & edge : block.edges)
        {
            pen.setWidthF(penWidth);
            pen.setColor(edge.color);
            p.setPen(pen);

            if(blockSelected)
                pen.setStyle(Qt::DashLine);
            p.setPen(pen);
            p.setBrush(edge.color);
            p.drawPolyline(edge.polyline);
            pen.setStyle(Qt::SolidLine);
            p.setPen(pen);
            p.drawConvexPolygon(edge.arrow);
        }
    }
}

void DisassemblerGraphView::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter p(this->viewport());
    p.setFont(this->font());

    int xofs = this->horizontalScrollBar()->value();
    int yofs = this->verticalScrollBar()->value();

    //Render background
    QRect viewportRect(this->viewport()->rect().topLeft(), this->viewport()->rect().bottomRight() - QPoint(1, 1));
    p.setBrush(backgroundColor);
    p.drawRect(viewportRect);
    p.setBrush(Qt::black);

    if(!this->ready || !DbgIsDebugging())
    {
        p.setPen(graphNodeColor);
        p.drawText(viewportRect, Qt::AlignCenter | Qt::AlignVCenter, tr("Use Graph command or menu action to draw control flow graph here..."));
        return;
    }

    if(!graphZoomMode)
    {
        if(drawOverview)
            paintOverview(p, viewportRect, xofs, yofs);
        else
            paintNormal(p, viewportRect, xofs, yofs);
    }
    else
    {
        paintZoom(p, viewportRect, xofs, yofs);
    }

    if(saveGraph)
    {
        //TODO: speed up large graph saving or show gif loader so it won't look like it has crashed

        //Image corresponds to the current zoom level
        saveGraph = false;
        QString path = QFileDialog::getSaveFileName(this, tr("Save as image"), "", tr("PNG file (*.png);;BMP file (*.bmp)"));
        if(path.isEmpty())
            return;

        QSize size = this->viewport()->size();
        QPoint scrollbarPos = QPoint(this->horizontalScrollBar()->value(), this->verticalScrollBar()->value());

        // expand to full render Rectangle
        this->viewport()->resize(this->renderWidth, this->renderHeight);

        if(graphZoomMode)
        {
            adjustSize(this->renderWidth, this->renderHeight, QPoint(), true); //set scrollbars to 50%
        }

        //save viewport to image
        QRect completeRenderRect = QRect(0, 0, this->renderWidth, this->renderHeight);
        QImage img(completeRenderRect.size(), QImage::Format_ARGB32);
        QPainter painter(&img);
        this->viewport()->render(&painter);
        img.save(path);

        //restore changes made to viewport for full render saving
        this->viewport()->resize(size);

        if(graphZoomMode)
        {
            this->horizontalScrollBar()->setValue(scrollbarPos.x());
            this->verticalScrollBar()->setValue(scrollbarPos.y());
        }
    }
}

void DisassemblerGraphView::wheelEvent(QWheelEvent* event)
{
    if(!DbgIsDebugging())
        return;

    if(event->modifiers() & Qt::ControlModifier && graphZoomMode)
    {
        QPoint numDegrees = event->angleDelta() / 8;
        QPoint numSteps = numDegrees / 15;
        QPoint mousePosition = event->pos();

        zoomBoost = 1;
        //Speed up zooming on large graphs by Ctrl+Shift
        if(event->modifiers() & Qt::ShiftModifier)
            zoomBoost = 2;

        if(numSteps.y() > 0)
        {
            zoomDirection = 1;
            zoomIn(mousePosition);
        }
        else if(numSteps.y() < 0)
        {
            zoomDirection = -1;
            zoomOut(mousePosition);
        }

        event->accept();
    }
    else
    {
        QAbstractScrollArea::wheelEvent(event);
    }
}

bool DisassemblerGraphView::isMouseEventInBlock(QMouseEvent* event)
{
    //Convert coordinates to system used in blocks
    int xofs = this->horizontalScrollBar()->value();
    int yofs = this->verticalScrollBar()->value();
    int x = (event->x() + xofs - this->renderXOfs) / zoomLevel;
    int y = (event->y() + yofs - this->renderYOfs) / zoomLevel;

    // Check each block for hits
    for(auto & blockIt : this->blocks)
    {
        DisassemblerBlock & block = blockIt.second;
        //Compute coordinate relative to text area in block
        int blockx = x - (block.x + (2 * this->charWidth));
        int blocky = y - (block.y + (2 * this->charWidth));
        //Check to see if click is within bounds of block
        if((blockx < 0) || (blockx > (block.width - 4 * this->charWidth)))
            continue;
        if((blocky < 0) || (blocky > (block.height - 4 * this->charWidth)))
            continue;
        return true;
    }
    return false;
}

duint DisassemblerGraphView::getInstrForMouseEvent(QMouseEvent* event)
{
    //Convert coordinates to system used in blocks
    int xofs = this->horizontalScrollBar()->value();
    int yofs = this->verticalScrollBar()->value();
    int x = (event->x() + xofs - this->renderXOfs) / zoomLevel;
    int y = (event->y() + yofs - this->renderYOfs) / zoomLevel;

    //Check each block for hits
    for(auto & blockIt : this->blocks)
    {
        DisassemblerBlock & block = blockIt.second;
        //Compute coordinate relative to text area in block
        int blockx = x - (block.x + (2 * this->charWidth));
        int blocky = y - (block.y + (2 * this->charWidth));
        //Check to see if click is within bounds of block
        if((blockx < 0) || (blockx > (block.width - 4 * this->charWidth)))
            continue;
        if((blocky < 0) || (blocky > (block.height - 4 * this->charWidth)))
            continue;
        //Compute row within text
        int row = int(blocky / this->charHeight);
        //Determine instruction for this row
        int cur_row = int(block.block.header_text.lines.size());
        if(row < cur_row)
            return block.block.entry;
        for(Instr & instr : block.block.instrs)
        {
            if(row < cur_row + int(instr.text.lines.size()))
                return instr.addr;
            cur_row += int(instr.text.lines.size());
        }
    }
    return 0;
}

bool DisassemblerGraphView::getTokenForMouseEvent(QMouseEvent* event, Token & tokenOut)
{
    Q_UNUSED(event);
    Q_UNUSED(tokenOut);
    /* TODO
    //Convert coordinates to system used in blocks
    int xofs = this->horizontalScrollBar()->value();
    int yofs = this->verticalScrollBar()->value();
    int x = event->x() + xofs - this->renderXOfs;
    int y = event->y() + yofs - this->renderYOfs;

    //Check each block for hits
    for(auto & blockIt : this->blocks)
    {
        DisassemblerBlock & block = blockIt.second;
        //Compute coordinate relative to text area in block
        int blockx = x - (block.x + (2 * this->charWidth));
        int blocky = y - (block.y + (2 * this->charWidth));
        //Check to see if click is within bounds of block
        if((blockx < 0) || (blockx > (block.width - 4 * this->charWidth)))
            continue;
        if((blocky < 0) || (blocky > (block.height - 4 * this->charWidth)))
            continue;
        //Compute row and column within text
        int col = int(blockx / this->charWidth);
        int row = int(blocky / this->charHeight);
        //Check tokens to see if one was clicked
        int cur_row = 0;
        for(auto & line : block.block.header_text.tokens)
        {
            if(cur_row == row)
            {
                for(Token & token : line)
                {
                    if((col >= token.start) && (col < (token.start + token.length)))
                    {
                        //Clicked on a token
                        tokenOut = token;
                        return true;
                    }
                }
            }
            cur_row += 1;
        }
        for(Instr & instr : block.block.instrs)
        {
            for(auto & line : instr.text.tokens)
            {
                if(cur_row == row)
                {
                    for(Token & token : line)
                    {
                        if((col >= token.start) && (col < (token.start + token.length)))
                        {
                            //Clicked on a token
                            tokenOut = token;
                            return true;
                        }
                    }
                }
                cur_row += 1;
            }
        }
    }*/
    return false;
}

bool DisassemblerGraphView::find_instr(duint addr, Instr & instrOut)
{
    for(auto & blockIt : this->blocks)
        for(Instr & instr : blockIt.second.block.instrs)
            if(instr.addr == addr)
            {
                instrOut = instr;
                return true;
            }
    return false;
}

void DisassemblerGraphView::mousePressEvent(QMouseEvent* event)
{
    if(!DbgIsDebugging() || !this->ready)
        return;

    bool inBlock = this->isMouseEventInBlock(event);

    //Save click state for actions that need to maintain cursor position and can be triggered whether via menu or by key
    //TODO: transfer this to setupContextMenu() somehow
    if(event->button() == Qt::RightButton)
    {
        this->lastRightClickPosition.pos = event->pos();
        this->lastRightClickPosition.inBlock = inBlock;
    }

    if(drawOverview)
    {
        if(event->button() == Qt::LeftButton)
        {
            //Enter scrolling mode
            this->scroll_base_x = event->x();
            this->scroll_base_y = event->y();
            this->scroll_mode = true;
            this->setCursor(Qt::ClosedHandCursor);
            this->viewport()->grabMouse();

            //Scroll to the cursor
            this->horizontalScrollBar()->setValue(((event->x() - this->overviewXOfs) / this->overviewScale) - this->viewport()->width() / 2);
            this->verticalScrollBar()->setValue(((event->y() - this->overviewYOfs) / this->overviewScale) - this->viewport()->height() / 2);
        }
        else if(event->button() == Qt::RightButton)
        {
            QMenu wMenu(this);
            mMenuBuilder->build(&wMenu);
            wMenu.exec(event->globalPos()); //execute context menu
        }
    }
    else if((event->button() == Qt::LeftButton || event->button() == Qt::RightButton) && inBlock)
    {
        //Check for click on a token and highlight it
        Token token;
        delete this->highlight_token;
        if(this->getTokenForMouseEvent(event, token))
            this->highlight_token = HighlightToken::fromToken(token);
        else
            this->highlight_token = nullptr;

        //Update current instruction
        duint instr = this->getInstrForMouseEvent(event);
        if(instr != 0)
            this->cur_instr = instr;

        this->viewport()->update();

        if(event->button() == Qt::RightButton)
        {
            showContextMenu(event);
        }
    }
    else if(event->button() == Qt::LeftButton)
    {
        //Left click outside any block, enter scrolling mode
        this->scroll_base_x = event->x();
        this->scroll_base_y = event->y();
        this->scroll_mode = true;
        this->setCursor(Qt::ClosedHandCursor);
        this->viewport()->grabMouse();
    }
    else if(event->button() == Qt::RightButton)
    {
        showContextMenu(event);
    }
}

void DisassemblerGraphView::mouseMoveEvent(QMouseEvent* event)
{
    if(this->scroll_mode)
    {
        int x_delta = this->scroll_base_x - event->x();
        int y_delta = this->scroll_base_y - event->y();
        if(drawOverview)
        {
            x_delta = -x_delta / this->overviewScale;
            y_delta = -y_delta / this->overviewScale;
        }
        this->scroll_base_x = event->x();
        this->scroll_base_y = event->y();
        this->horizontalScrollBar()->setValue(this->horizontalScrollBar()->value() + x_delta);
        this->verticalScrollBar()->setValue(this->verticalScrollBar()->value() + y_delta);
    }
}

void DisassemblerGraphView::mouseReleaseEvent(QMouseEvent* event)
{
    this->viewport()->update();

    if(event->button() == Qt::ForwardButton)
        gotoNextSlot();
    else if(event->button() == Qt::BackButton)
        gotoPreviousSlot();

    if(event->button() != Qt::LeftButton)
        return;

    if(this->scroll_mode)
    {
        this->scroll_mode = false;
        this->setCursor(Qt::ArrowCursor);
        this->viewport()->releaseMouse();
    }
}

void DisassemblerGraphView::mouseDoubleClickEvent(QMouseEvent* event)
{
    if(drawOverview)
    {
        toggleOverviewSlot();
    }
    else if(!graphZoomMode || (zoomLevel > zoomOverviewValue))
    {
        duint instr = this->getInstrForMouseEvent(event);

        //Add address to history
        if(!mHistoryLock)
            mHistory.addVaToHistory(instr);

        DbgCmdExec(QString("graph dis.branchdest(%1), silent").arg(ToPtrString(instr)).toUtf8().constData());
    }
}

void DisassemblerGraphView::prepareGraphNode(DisassemblerBlock & block)
{
    int width = 0;
    int height = 0;
    for(auto & line : block.block.header_text.lines)
    {
        int lw = 0;
        for(auto & part : line)
            lw += mFontMetrics->width(part.text);
        if(lw > width)
            width = lw;
        height += 1;
    }
    for(Instr & instr : block.block.instrs)
    {
        for(auto & line : instr.text.lines)
        {
            int lw = 0;
            for(auto & part : line)
                lw += mFontMetrics->width(part.text);
            if(lw > width)
                width = lw;
            height += 1;
        }
    }
    int extra = 4 * this->charWidth + 4;
    block.width = width + extra + this->charWidth;
    block.height = (height * this->charHeight) + extra;
}

void DisassemblerGraphView::adjustGraphLayout(DisassemblerBlock & block, int col, int row)
{
    block.col += col;
    block.row += row;
    for(duint edge : block.new_exits)
        this->adjustGraphLayout(this->blocks[edge], col, row);
}

void DisassemblerGraphView::computeGraphLayout(DisassemblerBlock & block)
{
    //Compute child node layouts and arrange them horizontally
    int col = 0;
    int row_count = 1;
    int childColumn = 0;
    bool singleChild = block.new_exits.size() == 1;
    for(size_t i = 0; i < block.new_exits.size(); i++)
    {
        duint edge = block.new_exits[i];
        this->computeGraphLayout(this->blocks[edge]);
        if((this->blocks[edge].row_count + 1) > row_count)
            row_count = this->blocks[edge].row_count + 1;
        childColumn = this->blocks[edge].col;
    }

    if(this->layoutType != LayoutType::Wide && block.new_exits.size() == 2)
    {
        DisassemblerBlock & left = this->blocks[block.new_exits[0]];
        DisassemblerBlock & right = this->blocks[block.new_exits[1]];
        if(left.new_exits.size() == 0)
        {
            left.col = right.col - 2;
            int add = left.col < 0 ? - left.col : 0;
            this->adjustGraphLayout(right, add, 1);
            this->adjustGraphLayout(left, add, 1);
            col = right.col_count + add;
        }
        else if(right.new_exits.size() == 0)
        {
            this->adjustGraphLayout(left, 0, 1);
            this->adjustGraphLayout(right, left.col + 2, 1);
            col = std::max(left.col_count, right.col + 2);
        }
        else
        {
            this->adjustGraphLayout(left, 0, 1);
            this->adjustGraphLayout(right, left.col_count, 1);
            col = left.col_count + right.col_count;
        }

        block.col_count = std::max(2, col);
        if(layoutType == LayoutType::Medium)
            block.col = (left.col + right.col) / 2;
        else
            block.col = singleChild ? childColumn : (col - 2) / 2;
    }
    else
    {
        for(duint edge : block.new_exits)
        {
            this->adjustGraphLayout(this->blocks[edge], col, 1);
            col += this->blocks[edge].col_count;
        }
        if(col >= 2)
        {
            //Place this node centered over the child nodes
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

bool DisassemblerGraphView::isEdgeMarked(EdgesVector & edges, int row, int col, int index)
{
    if(index >= int(edges[row][col].size()))
        return false;
    return edges[row][col][index];
}

void DisassemblerGraphView::markEdge(EdgesVector & edges, int row, int col, int index, bool used)
{
    while(int(edges[row][col].size()) <= index)
        edges[row][col].push_back(false);
    edges[row][col][index] = used;
}

int DisassemblerGraphView::findHorizEdgeIndex(EdgesVector & edges, int row, int min_col, int max_col)
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
        this->markEdge(edges, row, col, i);
    return i;
}

int DisassemblerGraphView::findVertEdgeIndex(EdgesVector & edges, int col, int min_row, int max_row)
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
        this->markEdge(edges, row, col, i);
    return i;
}

DisassemblerGraphView::DisassemblerEdge DisassemblerGraphView::routeEdge(EdgesVector & horiz_edges, EdgesVector & vert_edges, Matrix<bool> & edge_valid, DisassemblerBlock & start, DisassemblerBlock & end, QColor color)
{
    DisassemblerEdge edge;
    edge.color = color;
    edge.dest = &end;

    //Find edge index for initial outgoing line
    int i = 0;
    while(true)
    {
        if(!this->isEdgeMarked(vert_edges, start.row + 1, start.col + 1, i))
            break;
        i += 1;
    }
    this->markEdge(vert_edges, start.row + 1, start.col + 1, i);
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
        int index = this->findHorizEdgeIndex(horiz_edges, start.row + 1, min_col, max_col);
        edge.addPoint(start.row + 1, col, index);
        horiz = true;
    }

    if(end.row != (start.row + 1))
    {
        //Not in same row, need to generate a line for moving to the correct row
        if(col == (start.col + 1))
            this->markEdge(vert_edges, start.row + 1, start.col + 1, i, false);
        int index = this->findVertEdgeIndex(vert_edges, col, min_row, max_row);
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
        int index = this->findHorizEdgeIndex(horiz_edges, end.row, min_col, max_col);
        edge.addPoint(end.row, end.col + 1, index);
        horiz = true;
    }

    //If last line was horizontal, choose the ending edge index for the incoming edge
    if(horiz)
    {
        int index = this->findVertEdgeIndex(vert_edges, end.col + 1, end.row, end.row);
        edge.points[int(edge.points.size()) - 1].index = index;
    }

    return edge;
}

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

void DisassemblerGraphView::renderFunction(Function & func)
{
    //puts("Starting renderFunction");

    //Create render nodes
    this->blocks.clear();
    if(func.entry == 0)
    {
        this->ready = false;
        return;
    }
    for(Block & block : func.blocks)
    {
        this->blocks[block.entry] = DisassemblerBlock(block);
        this->prepareGraphNode(this->blocks[block.entry]);
    }
    //puts("Create render nodes");

    //Populate incoming lists
    for(auto & blockIt : this->blocks)
    {
        DisassemblerBlock & block = blockIt.second;
        for(auto & edge : block.block.exits)
            this->blocks[edge].incoming.push_back(block.block.entry);
    }
    //puts("Populate incoming lists");

    //Construct acyclic graph where each node is used as an edge exactly once
    std::unordered_set<duint> visited;
    visited.insert(func.entry);
    std::queue<duint> queue;
    queue.push(this->blocks[func.entry].block.entry);
    std::vector<duint> blockOrder;
    bool changed = true;

    while(changed)
    {
        changed = false;

        //First pick nodes that have single entry points
        while(!queue.empty())
        {
            DisassemblerBlock & block = this->blocks[queue.front()];
            queue.pop();
            blockOrder.push_back(block.block.entry);

            for(duint edge : block.block.exits)
            {
                if(visited.count(edge))
                    continue;

                //If node has no more unseen incoming edges, add it to the graph layout now
                if(int(this->blocks[edge].incoming.size()) == 1)
                {
                    removeFromVec(this->blocks[edge].incoming, block.block.entry);
                    block.new_exits.push_back(edge);
                    queue.push(this->blocks[edge].block.entry);
                    visited.insert(edge);
                    changed = true;
                }
                else
                {
                    removeFromVec(this->blocks[edge].incoming, block.block.entry);
                }
            }
        }

        //No more nodes satisfy constraints, pick a node to continue constructing the graph
        duint best = 0;
        int best_edges;
        duint best_parent;
        for(auto & blockIt : this->blocks)
        {
            DisassemblerBlock & block = blockIt.second;
            if(!visited.count(block.block.entry))
                continue;
            for(duint edge : block.block.exits)
            {
                if(visited.count(edge))
                    continue;
                if((best == 0) || (int(this->blocks[edge].incoming.size()) < best_edges) || (
                            (int(this->blocks[edge].incoming.size()) == best_edges) && (edge < best)))
                {
                    best = edge;
                    best_edges = int(this->blocks[edge].incoming.size());
                    best_parent = block.block.entry;
                }
            }
        }

        if(best != 0)
        {
            DisassemblerBlock & best_parentb = this->blocks[best_parent];
            removeFromVec(this->blocks[best].incoming, best_parentb.block.entry);
            best_parentb.new_exits.push_back(best);
            visited.insert(best);
            queue.push(best);
            changed = true;
        }
    }
    //puts("Construct acyclic graph where each node is used as an edge exactly once");

    //Compute graph layout from bottom up
    this->computeGraphLayout(this->blocks[func.entry]);

    //Optimize layout to be more compact
    /*std::vector<DisassemblerBlock> rowBlocks;
    for(auto blockIt : this->blocks)
        rowBlocks.push_back(blockIt.second);
    std::sort(rowBlocks.begin(), rowBlocks.end(), [](DisassemblerBlock & a, DisassemblerBlock & b)
    {
        if(a.row < b.row)
            return true;
        if(a.row == b.row)
            return a.col < b.col;
        return false;
    });
    std::vector<std::vector<DisassemblerBlock>> rowMap;
    for(DisassemblerBlock & block : rowBlocks)
    {
        if(block.row == rowMap.size())
            rowMap.push_back(std::vector<DisassemblerBlock>());
        rowMap[block.row].push_back(block);
    }
    int median = this->blocks[func.entry].col;
    for(auto & blockVec : rowMap)
    {
        int len = int(blockVec.size());
        if(len == 1)
            continue;
        int bestidx = 0;
        int bestdist = median;
        for(int i = 0; i < len; i++)
        {
            auto & block = blockVec[i];
            int dist = std::abs(block.col - median);
            if(dist < bestdist)
            {
                bestdist = dist;
                bestidx = i;
            }
        }
        for(int j = bestidx - 1; j > -1; j--)
            blockVec[j].col = blockVec[j + 1].col - 2;
        for(int j = bestidx + 1; j < len; j++)
            blockVec[j].col = blockVec[j - 1].col + 2;
    }
    for(auto & blockVec : rowMap)
        for(DisassemblerBlock & block : blockVec)
            blocks[block.block.entry] = block;*/

    //puts("Compute graph layout from bottom up");

    //Prepare edge routing
    EdgesVector horiz_edges, vert_edges;
    horiz_edges.resize(this->blocks[func.entry].row_count + 1);
    vert_edges.resize(this->blocks[func.entry].row_count + 1);
    Matrix<bool> edge_valid;
    edge_valid.resize(this->blocks[func.entry].row_count + 1);
    for(int row = 0; row < this->blocks[func.entry].row_count + 1; row++)
    {
        horiz_edges[row].resize(this->blocks[func.entry].col_count + 1);
        vert_edges[row].resize(this->blocks[func.entry].col_count + 1);
        initVec(edge_valid[row], this->blocks[func.entry].col_count + 1, true);
        for(int col = 0; col < this->blocks[func.entry].col_count + 1; col++)
        {
            horiz_edges[row][col].clear();
            vert_edges[row][col].clear();
        }
    }
    for(auto & blockIt : this->blocks)
    {
        DisassemblerBlock & block = blockIt.second;
        edge_valid[block.row][block.col + 1] = false;
    }
    //puts("Prepare edge routing");

    //Perform edge routing
    for(duint blockId : blockOrder)
    {
        DisassemblerBlock & block = blocks[blockId];
        DisassemblerBlock & start = block;
        for(duint edge : block.block.exits)
        {
            DisassemblerBlock & end = this->blocks[edge];
            QColor color = jmpColor;
            if(edge == block.block.true_path)
                color = brtrueColor;
            else if(edge == block.block.false_path)
                color = brfalseColor;
            start.edges.push_back(this->routeEdge(horiz_edges, vert_edges, edge_valid, start, end, color));
        }
    }
    //puts("Perform edge routing");

    //Compute edge counts for each row and column
    std::vector<int> col_edge_count, row_edge_count;
    initVec(col_edge_count, this->blocks[func.entry].col_count + 1, 0);
    initVec(row_edge_count, this->blocks[func.entry].row_count + 1, 0);
    for(int row = 0; row < this->blocks[func.entry].row_count + 1; row++)
    {
        for(int col = 0; col < this->blocks[func.entry].col_count + 1; col++)
        {
            if(int(horiz_edges[row][col].size()) > row_edge_count[row])
                row_edge_count[row] = int(horiz_edges[row][col].size());
            if(int(vert_edges[row][col].size()) > col_edge_count[col])
                col_edge_count[col] = int(vert_edges[row][col].size());
        }
    }
    //puts("Compute edge counts for each row and column");

    //Compute row and column sizes
    std::vector<int> col_width, row_height;
    initVec(col_width, this->blocks[func.entry].col_count + 1, 0);
    initVec(row_height, this->blocks[func.entry].row_count + 1, 0);
    for(auto & blockIt : this->blocks)
    {
        DisassemblerBlock & block = blockIt.second;
        if((int(block.width / 2)) > col_width[block.col])
            col_width[block.col] = int(block.width / 2);
        if((int(block.width / 2)) > col_width[block.col + 1])
            col_width[block.col + 1] = int(block.width / 2);
        if(int(block.height) > row_height[block.row])
            row_height[block.row] = int(block.height);
    }
    //puts("Compute row and column sizes");

    //Compute row and column positions
    std::vector<int> col_x, row_y;
    initVec(col_x, this->blocks[func.entry].col_count, 0);
    initVec(row_y, this->blocks[func.entry].row_count, 0);
    initVec(this->col_edge_x, this->blocks[func.entry].col_count + 1, 0);
    initVec(this->row_edge_y, this->blocks[func.entry].row_count + 1, 0);
    int x = 16;
    for(int i = 0; i < this->blocks[func.entry].col_count; i++)
    {
        this->col_edge_x[i] = x;
        x += 8 * col_edge_count[i];
        col_x[i] = x;
        x += col_width[i];
    }
    int y = 16;
    for(int i = 0; i < this->blocks[func.entry].row_count; i++)
    {
        this->row_edge_y[i] = y;
        y += 8 * row_edge_count[i];
        row_y[i] = y;
        y += row_height[i];
    }
    this->col_edge_x[this->blocks[func.entry].col_count] = x;
    this->row_edge_y[this->blocks[func.entry].row_count] = y;
    this->width = x + 16 + (8 * col_edge_count[this->blocks[func.entry].col_count]);
    this->height = y + 16 + (8 * row_edge_count[this->blocks[func.entry].row_count]);
    //puts("Compute row and column positions");

    //Compute node positions
    for(auto & blockIt : this->blocks)
    {
        DisassemblerBlock & block = blockIt.second;
        block.x = int(
                      (col_x[block.col] + col_width[block.col] + 4 * col_edge_count[block.col + 1]) - (block.width / 2));
        if((block.x + block.width) > (
                    col_x[block.col] + col_width[block.col] + col_width[block.col + 1] + 8 * col_edge_count[
                        block.col + 1]))
        {
            block.x = int((col_x[block.col] + col_width[block.col] + col_width[block.col + 1] + 8 * col_edge_count[
                               block.col + 1]) - block.width);
        }
        block.y = row_y[block.row];
    }
    //puts("Compute node positions");

    //Precompute coordinates for edges
    for(auto & blockIt : this->blocks)
    {
        DisassemblerBlock & block = blockIt.second;
        for(DisassemblerEdge & edge : block.edges)
        {
            auto start = edge.points[0];
            auto start_col = start.col;
            auto last_index = edge.start_index;
            auto last_pt = QPoint(this->col_edge_x[start_col] + (8 * last_index) + 4,
                                  block.y + block.height + 4 - (2 * this->charWidth));
            QPolygonF pts;
            pts.append(last_pt);

            for(int i = 0; i < int(edge.points.size()); i++)
            {
                auto end = edge.points[i];
                auto end_row = end.row;
                auto end_col = end.col;
                auto last_index = end.index;
                QPoint new_pt;
                if(start_col == end_col)
                    new_pt = QPoint(last_pt.x(), this->row_edge_y[end_row] + (8 * last_index) + 4);
                else
                    new_pt = QPoint(this->col_edge_x[end_col] + (8 * last_index) + 4, last_pt.y());
                pts.push_back(new_pt);
                last_pt = new_pt;
                start_col = end_col;
            }

            auto new_pt = QPoint(last_pt.x(), edge.dest->y + this->charWidth - 1);
            pts.push_back(new_pt);
            edge.polyline = pts;

            pts.clear();
            pts.append(QPoint(new_pt.x() - 3, new_pt.y() - 6));
            pts.append(QPoint(new_pt.x() + 3, new_pt.y() - 6));
            pts.append(new_pt);
            edge.arrow = pts;
        }
    }
    //puts("Precompute coordinates for edges");

    //Adjust scroll bars for new size
    QSize areaSize;
    if(this->viewportReady)
    {
        areaSize = this->viewport()->size();
    }
    else
    {
        //before graph tab is shown for the first time the viewport is kind of 98x28 so setting the parent size almost fixes this problem
        areaSize = this->parentWidget()->size() - QSize(20, 20);
    }

    qreal sx = qreal(areaSize.width()) / qreal(this->width);
    qreal sy = qreal(areaSize.height()) / qreal(this->height);
    zoomMinimum = qMin(qMin(sx, sy) * (1 - zoomStep), 0.05); //if graph is very lagre

    this->adjustSize(areaSize.width(), areaSize.height());
    puts("Adjust scroll bars for new size");

    if(this->desired_pos)
    {
        //There was a position saved, navigate to it
        this->horizontalScrollBar()->setValue(this->desired_pos[0]);
        this->verticalScrollBar()->setValue(this->desired_pos[1]);
    }
    else if(this->cur_instr != 0)
    {
        this->show_cur_instr(this->forceCenter);
        this->forceCenter = false;
    }
    else
    {
        //Ensure start node is visible
        auto start_x = this->blocks[func.entry].x + this->renderXOfs + int(this->blocks[func.entry].width / 2);
        this->horizontalScrollBar()->setValue(start_x - int(areaSize.width() / 2));
        this->verticalScrollBar()->setValue(0);
    }

    this->ready = true;
    this->viewport()->update(0, 0, areaSize.width(), areaSize.height());
    //puts("Finished");
}

void DisassemblerGraphView::show_cur_instr(bool force)
{
    for(auto & blockIt : this->blocks)
    {
        DisassemblerBlock & block = blockIt.second;
        auto row = int(block.block.header_text.lines.size());
        for(Instr & instr : block.block.instrs)
        {
            if(this->cur_instr == instr.addr)
            {
                //Don't update the view for blocks that are already fully in view
                int xofs = this->horizontalScrollBar()->value();
                int yofs = this->verticalScrollBar()->value();
                QRect viewportRect = this->viewport()->rect();

                if(!viewportReady)
                {
                    //before being shown for the first time viewport is kind of 98x28 so setting the parent size almost fixes this problem
                    viewportRect.setSize(this->parentWidget()->size() - QSize(20, 20));
                }

                QPoint translation(this->renderXOfs - xofs, this->renderYOfs - yofs);

                //Adjust scaled viewport
                viewportRect.setWidth(viewportRect.width() / zoomLevel);
                viewportRect.setHeight(viewportRect.height() / zoomLevel);
                viewportRect.translate(-translation.x() / zoomLevel, -translation.y() / zoomLevel);

                if(force || !viewportRect.contains(QRect(block.x + this->charWidth, block.y + this->charWidth,
                                                   block.width - (2 * this->charWidth), block.height - (2 * this->charWidth))))
                {
                    auto x = (block.x + int(block.width / 2)) * zoomLevel;
                    auto y = (block.y + (2 * this->charWidth) + int((row + 0.5) * this->charHeight)) * zoomLevel;
                    this->horizontalScrollBar()->setValue(x + this->renderXOfs -
                                                          int(this->horizontalScrollBar()->pageStep() / 2));
                    this->verticalScrollBar()->setValue(y + this->renderYOfs -
                                                        int(this->verticalScrollBar()->pageStep() / 2));
                }
                return;
            }
            row += int(instr.text.lines.size());
        }
    }
}

bool DisassemblerGraphView::navigate(duint addr)
{
    //Add address to history
    if(!mHistoryLock)
        mHistory.addVaToHistory(addr);
    //Check to see if address is within current function
    for(auto & blockIt : this->blocks)
    {
        DisassemblerBlock & block = blockIt.second;
        if(block.block.entry > addr) //optimize it
            continue;
        auto row = int(block.block.header_text.lines.size());
        for(Instr & instr : block.block.instrs)
        {
            if((addr >= instr.addr) && (addr < (instr.addr + int(instr.opcode.size()))))
            {
                this->cur_instr = instr.addr;
                this->show_cur_instr();
                this->viewport()->update();
                return true;
            }
            row += int(instr.text.lines.size());
        }
    }

    //Check other functions for this address
    duint func, instr;
    if(this->analysis.find_instr(addr, func, instr))
    {
        this->function = func;
        this->cur_instr = instr;
        this->highlight_token = nullptr;
        this->ready = false;
        this->desired_pos = nullptr;
        this->viewport()->update();
        return true;
    }

    return false;
}

void DisassemblerGraphView::fontChanged()
{
    this->initFont();

    if(this->ready)
    {
        //Rerender function to update layout
        this->renderFunction(this->analysis.functions[this->function]);
    }
}

void DisassemblerGraphView::setGraphLayout(DisassemblerGraphView::LayoutType layout)
{
    this->layoutType = layout;
    if(this->ready)
    {
        this->renderFunction(this->analysis.functions[this->function]);
    }
}

void DisassemblerGraphView::tokenizerConfigUpdatedSlot()
{
    disasm.UpdateConfig();
    loadCurrentGraph();
}

void DisassemblerGraphView::loadCurrentGraph()
{
    if(ConfigBool("Gui", "GraphZoomMode"))
    {
        graphZoomMode = true;
        drawOverview = false;
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }
    else
    {
        graphZoomMode = false;
        zoomLevel = 1;
        setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    }

    bool showGraphRva = ConfigBool("Gui", "ShowGraphRva");
    Analysis anal;
    anal.entry = currentGraph.entryPoint;
    anal.ready = true;
    {
        Function func;
        func.entry = currentGraph.entryPoint;
        func.ready = true;
        {
            for(const auto & nodeIt : currentGraph.nodes)
            {
                const BridgeCFNode & node = nodeIt.second;
                Block block;
                block.entry = node.instrs.empty() ? node.start : node.instrs[0].addr;
                block.exits = node.exits;
                block.false_path = node.brfalse;
                block.true_path = node.brtrue;
                block.terminal = node.terminal;
                block.indirectcall = node.indirectcall;
                block.header_text = Text(getSymbolicName(block.entry), mLabelColor, mLabelBackgroundColor);
                {
                    Instr instr;
                    for(const BridgeCFInstruction & nodeInstr : node.instrs)
                    {
                        auto addr = nodeInstr.addr;
                        currentBlockMap[addr] = block.entry;
                        Instruction_t instrTok = disasm.DisassembleAt((byte_t*)nodeInstr.data, sizeof(nodeInstr.data), 0, addr, false);
                        RichTextPainter::List richText;
                        ZydisTokenizer::TokenToRichText(instrTok.tokens, richText, 0);

                        // add rva to node instruction text
                        if(showGraphRva)
                        {
                            RichTextPainter::CustomRichText_t rvaText;
                            rvaText.highlight = false;
                            rvaText.textColor = mAddressColor;
                            rvaText.textBackground = mAddressBackgroundColor;
                            rvaText.text = QString().number(instrTok.rva, 16).toUpper().trimmed() + "  ";
                            rvaText.flags = rvaText.textBackground.alpha() ? RichTextPainter::FlagAll : RichTextPainter::FlagColor;
                            richText.insert(richText.begin(), rvaText);
                        }

                        auto size = instrTok.length;
                        instr.addr = addr;
                        instr.opcode.resize(size);
                        for(int j = 0; j < size; j++)
                            instr.opcode[j] = nodeInstr.data[j];

                        QString comment;
                        bool autoComment = false;
                        RichTextPainter::CustomRichText_t commentText;
                        commentText.highlight = false;
                        char label[MAX_LABEL_SIZE] = "";
                        if(GetCommentFormat(addr, comment, &autoComment))
                        {
                            if(autoComment)
                            {
                                commentText.textColor = mAutoCommentColor;
                                commentText.textBackground = mAutoCommentBackgroundColor;
                            }
                            else //user comment
                            {
                                commentText.textColor = mCommentColor;
                                commentText.textBackground = mCommentBackgroundColor;
                            }
                            commentText.text = QString("; ") + comment;
                            //add to text
                        }
                        else if(DbgGetLabelAt(addr, SEG_DEFAULT, label) && addr != block.entry) // label but no comment
                        {
                            commentText.textColor = mLabelColor;
                            commentText.textBackground = mLabelBackgroundColor;
                            commentText.text = QString("; ") + label;
                        }
                        commentText.flags = commentText.textBackground.alpha() ? RichTextPainter::FlagAll : RichTextPainter::FlagColor;
                        if(commentText.text.length())
                        {
                            RichTextPainter::CustomRichText_t spaceText;
                            spaceText.highlight = false;
                            spaceText.flags = RichTextPainter::FlagNone;
                            spaceText.text = " ";
                            richText.push_back(spaceText);
                            richText.push_back(commentText);
                        }
                        instr.text = Text(richText);

                        //The summary contains calls, rets, user comments and string references
                        if(!onlySummary ||
                                instrTok.branchType == Instruction_t::Call ||
                                instrTok.instStr.startsWith("ret", Qt::CaseInsensitive) ||
                                (!commentText.text.isEmpty() && !autoComment) ||
                                commentText.text.contains('\"'))
                            block.instrs.push_back(instr);
                    }
                }
                func.blocks.push_back(block);
            }
        }
        anal.functions.insert({func.entry, func});
    }
    this->analysis = anal;
    this->function = this->analysis.entry;
    this->renderFunction(this->analysis.functions[this->function]);
}

void DisassemblerGraphView::loadGraphSlot(BridgeCFGraphList* graphList, duint addr)
{
    auto nodeCount = graphList->nodes.count;
    if(nodeCount > 5000) //TODO: add configuration
    {
        auto title = tr("Large number of nodes");
        auto message = tr("The graph you are trying to render has a large number of nodes (%1). This can cause x64dbg to hang or crash. It is recommended to save your data before you continue.\n\nDo you want to continue rendering this graph?").arg(nodeCount);
        if(QMessageBox::question(this, title, message, QMessageBox::Yes, QMessageBox::No | QMessageBox::Default) == QMessageBox::No)
        {
            Bridge::getBridge()->setResult(0);
            return;
        }
    }

    currentGraph = BridgeCFGraph(graphList, true);
    currentBlockMap.clear();
    this->cur_instr = addr ? addr : this->function;
    this->forceCenter = true;
    loadCurrentGraph();
    Bridge::getBridge()->setResult(1);
}

void DisassemblerGraphView::graphAtSlot(duint addr)
{
    Bridge::getBridge()->setResult(this->navigate(addr) ? this->currentGraph.entryPoint : 0);
}

void DisassemblerGraphView::updateGraphSlot()
{
    if(!DbgIsDebugging())
    {
        //happens mostly when debugging process has been terminated
        this->ready = false;
        zoomLevel = 1;
        zoomLevelOld = 1;
    }

    this->viewport()->update();
}

void DisassemblerGraphView::addReferenceAction(QMenu* menu, duint addr)
{
    QAction* action = new QAction(menu);
    action->setData(ToPtrString(addr));
    action->setText(getSymbolicName(addr));
    connect(action, SIGNAL(triggered()), this, SLOT(followActionSlot()));
    menu->addAction(action);
}

duint DisassemblerGraphView::zoomActionHelper()
{
    if(!graphZoomMode || graphZoomMode && lastRightClickPosition.pos.isNull()) //old mode or zoom mode + shurtcut
        return cur_instr;
    else if(graphZoomMode && !(lastRightClickPosition.inBlock && zoomLevel > zoomOverviewValue)) //out of block or too small zoom value
        return 0;
    else
        return cur_instr;
}

void DisassemblerGraphView::setupContextMenu()
{
    /* Unlike old style menu, the new one now depends on zoom level and cursor position.
     * There are several options for how menu will look like. This makes interaction more clear and predictable.
     * E.g clicking outside of block (especially at large zoom level) will set breakpoint menu hidden
     * as well as any action that needs text to be visible will also be hidden.
     * Notice: keyboard shortcuts still work - this implies that user understands what he is doing. */

    mMenuBuilder = new MenuBuilder(this, [this](QMenu*)
    {
        return DbgIsDebugging() && this->ready;
    });

    mMenuBuilder->addAction(makeShortcutAction(DIcon(QString("processor%1.png").arg(ArchValue("32", "64"))), tr("Follow in &Disassembler"), SLOT(followDisassemblerSlot()), "ActionGraphFollowDisassembler"), [this](QMenu*)
    {
        return this->cur_instr != 0;
    });
    mMenuBuilder->addSeparator();

    auto breakpointMenu = new BreakpointMenu(this, getActionHelperFuncs(), [this]()
    {
        return zoomActionHelper() != 0;
    });
    breakpointMenu->build(mMenuBuilder);

    mMenuBuilder->addAction(makeShortcutAction(DIcon("comment.png"), tr("&Comment"), SLOT(setCommentSlot()), "ActionSetComment"), [this](QMenu*)
    {
        return zoomActionHelper() != 0;
    });
    mMenuBuilder->addAction(makeShortcutAction(DIcon("label.png"), tr("&Label"), SLOT(setLabelSlot()), "ActionSetLabel"), [this](QMenu*)
    {
        return zoomActionHelper() != 0;
    });
    mMenuBuilder->addAction(makeShortcutAction(DIcon("xrefs.png"), tr("Xrefs..."), SLOT(xrefSlot()), "ActionXrefs"), [this](QMenu*)
    {
        return zoomActionHelper() != 0;
    });

    MenuBuilder* gotoMenu = new MenuBuilder(this);
    gotoMenu->addAction(makeShortcutAction(DIcon("geolocation-goto.png"), tr("Expression"), SLOT(gotoExpressionSlot()), "ActionGotoExpression"));
    gotoMenu->addAction(makeShortcutAction(DIcon("cbp.png"), tr("Origin"), SLOT(gotoOriginSlot()), "ActionGotoOrigin"));
    gotoMenu->addAction(makeShortcutAction(DIcon("previous.png"), tr("Previous"), SLOT(gotoPreviousSlot()), "ActionGotoPrevious"), [this](QMenu*)
    {
        return mHistory.historyHasPrev();
    });
    gotoMenu->addAction(makeShortcutAction(DIcon("next.png"), tr("Next"), SLOT(gotoNextSlot()), "ActionGotoNext"), [this](QMenu*)
    {
        return mHistory.historyHasNext();
    });
    MenuBuilder* childrenAndParentMenu = new MenuBuilder(this, [this](QMenu * menu)
    {
        if(!zoomActionHelper())
            return false;
        duint cursorpos = get_cursor_pos();
        const DisassemblerBlock* currentBlock = nullptr;
        const Instr* currentInstruction = nullptr;
        for(const auto & i : blocks)
        {
            if(i.second.block.entry > cursorpos)
                continue;
            for(const Instr & inst : i.second.block.instrs)
            {
                if(inst.addr <= cursorpos && inst.addr + inst.opcode.size() > cursorpos)
                {
                    currentBlock = &i.second;
                    currentInstruction = &inst;
                    break;
                }
            }
            if(currentInstruction)
                break;
        }
        if(currentInstruction)
        {
            for(const duint & i : currentBlock->incoming) // This list is incomplete
                addReferenceAction(menu, i);
            if(!currentBlock->block.terminal)
            {
                menu->addSeparator();
                for(const duint & i : currentBlock->block.exits)
                    addReferenceAction(menu, i);
            }
            //to do: follow a constant
            return true;
        }
        return false;
    });
    gotoMenu->addSeparator();
    gotoMenu->addBuilder(childrenAndParentMenu);
    mMenuBuilder->addMenu(makeMenu(DIcon("goto.png"), tr("Go to")), gotoMenu);

    mMenuBuilder->addAction(mZoomToCursor = makeShortcutAction(DIcon("zoom.png"), tr("&Zoom 100%"), SLOT(zoomToCursorSlot()), "ActionGraphZoomToCursor"), [this](QMenu*)
    {
        if(!graphZoomMode)
            return false;
        else
            return true;
    });
    mMenuBuilder->addAction(mFitToWindow = makeShortcutAction(DIcon("fit.png"), tr("&Fit to window"), SLOT(fitToWindowSlot()), "ActionGraphFitToWindow"), [this](QMenu*)
    {
        if(!graphZoomMode)
            return false;
        else
            return true;
    });

    mMenuBuilder->addSeparator();
    mMenuBuilder->addAction(makeShortcutAction(DIcon("snowman.png"), tr("Decompile"), SLOT(decompileSlot()), "ActionGraphDecompile"));
    mMenuBuilder->addAction(mToggleOverview = makeShortcutAction(DIcon("graph.png"), tr("&Overview"), SLOT(toggleOverviewSlot()), "ActionGraphToggleOverview"), [this](QMenu*)
    {
        if(graphZoomMode)
            return false;
        else
            return true;
    });
    mToggleOverview->setCheckable(true);
    mMenuBuilder->addAction(mToggleSummary = makeShortcutAction(DIcon("summary.png"), tr("S&ummary"), SLOT(toggleSummarySlot()), "ActionGraphToggleSummary"));
    mToggleSummary->setCheckable(true);
    mMenuBuilder->addAction(mToggleSyncOrigin = makeShortcutAction(DIcon("lock.png"), tr("&Sync with origin"), SLOT(toggleSyncOriginSlot()), "ActionGraphSyncOrigin"));
    mMenuBuilder->addAction(makeShortcutAction(DIcon("sync.png"), tr("&Refresh"), SLOT(refreshSlot()), "ActionRefresh"));
    mMenuBuilder->addAction(makeShortcutAction(DIcon("image.png"), tr("&Save as image"), SLOT(saveImageSlot()), "ActionGraphSaveImage"));

    MenuBuilder* layoutMenu = new MenuBuilder(this);
    QActionGroup* layoutGroup = new QActionGroup(this);
    layoutGroup->addAction(makeAction(DIcon("narrow.png"), tr("Narrow"), [this]() { setGraphLayout(LayoutType::Narrow); }));
    QAction* mediumLayout =
    layoutGroup->addAction(makeAction(DIcon("medium.png"), tr("Medium"), [this]() { setGraphLayout(LayoutType::Medium); }));
    layoutGroup->addAction(makeAction(DIcon("wide.png"), tr("Wide"), [this]() { setGraphLayout(LayoutType::Wide); }));
    for(QAction* layoutAction : layoutGroup->actions())
    {
        layoutAction->setCheckable(true);
        layoutMenu->addAction(layoutAction);
    }
    mediumLayout->setChecked(true);
    mMenuBuilder->addMenu(makeMenu(DIcon("layout.png"), tr("Layout")), layoutMenu);

    mMenuBuilder->loadFromConfig();
}

void DisassemblerGraphView::showContextMenu(QMouseEvent* event)
{
    QMenu wMenu(this);
    mMenuBuilder->build(&wMenu);
    wMenu.exec(event->globalPos());

    lastRightClickPosition.pos = {};
}

void DisassemblerGraphView::keyPressEvent(QKeyEvent* event)
{
    if(event->modifiers() != 0)
        return;
    int key = event->key();
    if(key == Qt::Key_Up)
        DbgCmdExec(QString("graph dis.prev(%1), silent").arg(ToPtrString(cur_instr)).toUtf8().constData());
    else if(key == Qt::Key_Down)
        DbgCmdExec(QString("graph dis.next(%1), silent").arg(ToPtrString(cur_instr)).toUtf8().constData());
    else if(key == Qt::Key_Left)
        DbgCmdExec(QString("graph dis.brtrue(%1), silent").arg(ToPtrString(cur_instr)).toUtf8().constData());
    else if(key == Qt::Key_Right)
        DbgCmdExec(QString("graph dis.brfalse(%1), silent").arg(ToPtrString(cur_instr)).toUtf8().constData());
    else if(key == Qt::Key_Return || key == Qt::Key_Enter)
    {
        //Add address to history
        if(!mHistoryLock)
            mHistory.addVaToHistory(cur_instr);
        DbgCmdExec(QString("graph dis.branchdest(%1), silent").arg(ToPtrString(cur_instr)).toUtf8().constData());
    }
}

void DisassemblerGraphView::followDisassemblerSlot()
{
    DbgCmdExec(QString("disasm %1").arg(ToPtrString(this->cur_instr)).toUtf8().constData());
}

void DisassemblerGraphView::colorsUpdatedSlot()
{
    disassemblyBackgroundColor = ConfigColor("DisassemblyBackgroundColor");
    graphNodeColor = ConfigColor("GraphNodeColor");
    graphNodeBackgroundColor = ConfigColor("GraphNodeBackgroundColor");
    if(!graphNodeBackgroundColor.alpha())
        graphNodeBackgroundColor = disassemblyBackgroundColor;
    graphCurrentShadowColor = ConfigColor("GraphCurrentShadowColor");
    disassemblySelectionColor = ConfigColor("DisassemblySelectionColor");
    disassemblyTracedColor = ConfigColor("DisassemblyTracedBackgroundColor");
    auto a = disassemblySelectionColor, b = disassemblyTracedColor;
    disassemblyTracedSelectionColor = QColor((a.red() + b.red()) / 2, (a.green() + b.green()) / 2, (a.blue() + b.blue()) / 2);
    mAutoCommentColor = ConfigColor("DisassemblyAutoCommentColor");
    mAutoCommentBackgroundColor = ConfigColor("DisassemblyAutoCommentBackgroundColor");
    mCommentColor = ConfigColor("DisassemblyCommentColor");
    mCommentBackgroundColor = ConfigColor("DisassemblyCommentBackgroundColor");
    mLabelColor = ConfigColor("DisassemblyLabelColor");
    mLabelBackgroundColor = ConfigColor("DisassemblyLabelBackgroundColor");
    mAddressColor = ConfigColor("DisassemblyAddressColor");
    mAddressBackgroundColor = ConfigColor("DisassemblyAddressBackgroundColor");

    jmpColor = ConfigColor("GraphJmpColor");
    brtrueColor = ConfigColor("GraphBrtrueColor");
    brfalseColor = ConfigColor("GraphBrfalseColor");
    retShadowColor = ConfigColor("GraphRetShadowColor");
    indirectcallShadowColor = ConfigColor("GraphIndirectcallShadowColor");
    backgroundColor = ConfigColor("GraphBackgroundColor");
    if(!backgroundColor.alpha())
        backgroundColor = disassemblySelectionColor;
    mCipColor = ConfigColor("GraphCipColor");
    mBreakpointColor = ConfigColor("GraphBreakpointColor");
    mDisabledBreakpointColor = ConfigColor("GraphDisabledBreakpointColor");

    fontChanged();
    loadCurrentGraph();
}

void DisassemblerGraphView::fontsUpdatedSlot()
{
    fontChanged();
}

void DisassemblerGraphView::shortcutsUpdatedSlot()
{
    updateShortcuts();
}

void DisassemblerGraphView::toggleOverviewSlot()
{
    if(graphZoomMode)
        return;
    drawOverview = !drawOverview;
    if(onlySummary)
    {
        onlySummary = false;
        mToggleSummary->setChecked(false);
        loadCurrentGraph();
    }
    else
        this->viewport()->update();
}

void DisassemblerGraphView::toggleSummarySlot()
{
    drawOverview = false;
    onlySummary = !onlySummary;
    loadCurrentGraph();
}

void DisassemblerGraphView::selectionGetSlot(SELECTIONDATA* selection)
{
    selection->start = selection->end = cur_instr;
    Bridge::getBridge()->setResult(1);
}

void DisassemblerGraphView::disassembleAtSlot(dsint va, dsint cip)
{
    Q_UNUSED(va);
    auto cipChanged = mCip != cip;
    mCip = cip;
    if(syncOrigin && cipChanged)
        gotoOriginSlot();
    else
        this->viewport()->update();
}

void DisassemblerGraphView::gotoExpressionSlot()
{
    if(!DbgIsDebugging())
        return;
    if(!mGoto)
        mGoto = new GotoDialog(this);
    mGoto->setInitialExpression(ToPtrString(this->cur_instr));
    if(mGoto->exec() == QDialog::Accepted)
    {
        duint value = DbgValFromString(mGoto->expressionText.toUtf8().constData());
        DbgCmdExec(QString().sprintf("graph %p, silent", value).toUtf8().constData());
    }
}

void DisassemblerGraphView::gotoOriginSlot()
{
    DbgCmdExec("graph cip, silent");
}

void DisassemblerGraphView::gotoPreviousSlot()
{
    if(mHistory.historyHasPrev())
    {
        mHistoryLock = true;
        DbgCmdExecDirect(QString("graph %1, silent").arg(ToPtrString(mHistory.historyPrev())).toUtf8().constData());
        mHistoryLock = false;
    }
}

void DisassemblerGraphView::gotoNextSlot()
{
    if(mHistory.historyHasNext())
    {
        mHistoryLock = true;
        DbgCmdExecDirect(QString("graph %1, silent").arg(ToPtrString(mHistory.historyNext())).toUtf8().constData());
        mHistoryLock = false;
    }
}

void DisassemblerGraphView::toggleSyncOriginSlot()
{
    syncOrigin = !syncOrigin;
    mToggleSyncOrigin->setCheckable(true);
    mToggleSyncOrigin->setChecked(syncOrigin);
    if(syncOrigin)
        gotoOriginSlot();
}

void DisassemblerGraphView::refreshSlot()
{
    DbgCmdExec(QString("graph %1, force").arg(ToPtrString(this->cur_instr)).toUtf8().constData());
}

void DisassemblerGraphView::saveImageSlot()
{
    saveGraph = true;
    this->viewport()->update();
}

void DisassemblerGraphView::setCommentSlot()
{
    duint wVA = this->get_cursor_pos();
    LineEditDialog mLineEdit(this);
    mLineEdit.setTextMaxLength(MAX_COMMENT_SIZE - 2);
    QString addr_text = ToPtrString(wVA);
    char comment_text[MAX_COMMENT_SIZE] = "";
    if(!DbgIsDebugging())
        return;
    if(!DbgMemIsValidReadPtr(wVA))
        return;

    if(DbgGetCommentAt((duint)wVA, comment_text))
    {
        if(comment_text[0] == '\1') //automatic comment
            mLineEdit.setText(QString(comment_text + 1));
        else
            mLineEdit.setText(QString(comment_text));
    }

    mLineEdit.setWindowTitle(tr("Add comment at ") + addr_text);

    if(mLineEdit.exec() != QDialog::Accepted)
        return;

    if(!DbgSetCommentAt(wVA, mLineEdit.editText.replace('\r', "").replace('\n', "").toUtf8().constData()))
        SimpleErrorBox(this, tr("Error!"), tr("DbgSetCommentAt failed!"));

    this->refreshSlot();
}

void DisassemblerGraphView::setLabelSlot()
{
    duint wVA = this->get_cursor_pos();
    LineEditDialog mLineEdit(this);
    mLineEdit.setTextMaxLength(MAX_LABEL_SIZE - 2);
    QString addr_text = ToPtrString(wVA);
    char label_text[MAX_LABEL_SIZE] = "";
    if(!DbgIsDebugging())
        return;
    if(!DbgMemIsValidReadPtr(wVA))
        return;

    if(DbgGetLabelAt((duint)wVA, SEG_DEFAULT, label_text))
        mLineEdit.setText(QString(label_text));

    mLineEdit.setWindowTitle(tr("Add label at ") + addr_text);
restart:
    if(mLineEdit.exec() != QDialog::Accepted)
        return;

    QByteArray utf8data = mLineEdit.editText.toUtf8();
    if(!utf8data.isEmpty() && DbgIsValidExpression(utf8data.constData()) && DbgValFromString(utf8data.constData()) != wVA)
    {
        QMessageBox msg(QMessageBox::Warning, tr("The label may be in use"),
                        tr("The label \"%1\" may be an existing label or a valid expression. Using such label might have undesired effects. Do you still want to continue?").arg(mLineEdit.editText),
                        QMessageBox::Yes | QMessageBox::No, this);
        msg.setWindowIcon(DIcon("compile-warning.png"));
        msg.setParent(this, Qt::Dialog);
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        if(msg.exec() == QMessageBox::No)
            goto restart;
    }
    if(!DbgSetLabelAt(wVA, utf8data.constData()))
        SimpleErrorBox(this, tr("Error!"), tr("DbgSetLabelAt failed!"));

    this->refreshSlot();
}

void DisassemblerGraphView::xrefSlot()
{

    if(!DbgIsDebugging())
        return;
    duint wVA = this->get_cursor_pos();
    if(!DbgMemIsValidReadPtr(wVA))
        return;
    XREF_INFO mXrefInfo;
    DbgXrefGet(wVA, &mXrefInfo);
    if(!mXrefInfo.refcount)
        return;
    BridgeFree(mXrefInfo.references);
    if(!mXrefDlg)
        mXrefDlg = new XrefBrowseDialog(this);
    mXrefDlg->setup(wVA, "graph");
    mXrefDlg->showNormal();
}

void DisassemblerGraphView::decompileSlot()
{
    std::vector<SnowmanRange> ranges;
    ranges.reserve(currentGraph.nodes.size());

    if(!DbgIsDebugging())
        return;
    if(currentGraph.nodes.empty())
        return;
    SnowmanRange r;
    for(const auto & nodeIt : currentGraph.nodes)
    {
        const BridgeCFNode & node = nodeIt.second;
        r.start = node.instrs.empty() ? node.start : node.instrs[0].addr;
        r.end = node.instrs.empty() ? node.end : node.instrs[node.instrs.size() - 1].addr;
        BASIC_INSTRUCTION_INFO info;
        DbgDisasmFastAt(r.end, &info);
        r.end += info.size - 1;
        ranges.push_back(r);
    }
    std::sort(ranges.begin(), ranges.end(), [](const SnowmanRange & a, const SnowmanRange & b)
    {
        return a.start > b.start;
    });
    emit displaySnowmanWidget();
    DecompileRanges(Bridge::getBridge()->snowmanView, ranges.data(), ranges.size());
}

void DisassemblerGraphView::followActionSlot()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if(action)
    {
        QString data = action->data().toString();
        DbgCmdExecDirect(QString("graph %1, silent").arg(data).toUtf8().constData());
    }
}

void DisassemblerGraphView::fitToWindowSlot()
{
    auto areaSize = viewport()->size();
    qreal sx = qreal(areaSize.width()) / qreal(this->width);
    qreal sy = qreal(areaSize.height()) / qreal(this->height);
    zoomLevelOld = zoomLevel;
    zoomLevel = qMin(qMin(sx, sy), qreal(1));
    zoomDirection = -1;
    this->adjustSize(areaSize.width(), areaSize.height(), QPoint(), true);
    this->viewport()->update();
}

void DisassemblerGraphView::zoomToCursorSlot()
{
    QPoint pos;

    if(!lastRightClickPosition.pos.isNull())
    {
        pos = lastRightClickPosition.pos;
    }
    else
    {
        pos = this->mapFromGlobal(QCursor::pos());
    }
    zoomLevelOld = zoomLevel;
    zoomLevel = 1;
    zoomDirection = 1;
    auto areaSize = viewport()->size();
    this->adjustSize(areaSize.width(), areaSize.height(), pos);
    this->viewport()->update();
}
