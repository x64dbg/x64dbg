#include "CPUSideBar.h"
#include "Configuration.h"
#include "Breakpoints.h"
#include "CPUDisassembly.h"
#include "CachedFontMetrics.h"
#include <QToolTip>

CPUSideBar::CPUSideBar(CPUDisassembly* Ptr, QWidget* parent) : QAbstractScrollArea(parent)
{
    setWindowTitle("SideBar");
    topVA = -1;
    selectedVA = -1;
    viewableRows = 0;
    mFontMetrics = nullptr;

    mDisas = Ptr;

    mInstrBuffer = mDisas->instructionsBuffer();

    memset(&regDump, 0, sizeof(REGDUMP));

    updateSlots();

    this->setMouseTracking(true);

}

CPUSideBar::~CPUSideBar()
{
    delete mFontMetrics;
}

void CPUSideBar::updateSlots()
{
    connect(Config(), SIGNAL(colorsUpdated()), this, SLOT(updateColors()));
    connect(Config(), SIGNAL(fontsUpdated()), this, SLOT(updateFonts()));
    connect(Bridge::getBridge(), SIGNAL(foldDisassembly(duint, duint)), this, SLOT(foldDisassembly(duint, duint)));

    // Init all other updates once
    updateColors();
    updateFonts();
}

void CPUSideBar::updateColors()
{
    mBackgroundColor = ConfigColor("SideBarBackgroundColor");

    mConditionalJumpLineFalseColor = ConfigColor("SideBarConditionalJumpLineFalseColor");
    mUnconditionalJumpLineFalseColor = ConfigColor("SideBarUnconditionalJumpLineFalseColor");
    mConditionalJumpLineTrueColor = ConfigColor("SideBarConditionalJumpLineTrueColor");
    mUnconditionalJumpLineTrueColor = ConfigColor("SideBarUnconditionalJumpLineTrueColor");

    mBulletBreakpointColor = ConfigColor("SideBarBulletBreakpointColor");
    mBulletBookmarkColor = ConfigColor("SideBarBulletBookmarkColor");
    mBulletColor = ConfigColor("SideBarBulletColor");
    mBulletDisabledBreakpointColor = ConfigColor("SideBarBulletDisabledBreakpointColor");

    mCipLabelColor = ConfigColor("SideBarCipLabelColor");
    mCipLabelBackgroundColor = ConfigColor("SideBarCipLabelBackgroundColor");

    mChkBoxForeColor = ConfigColor("SideBarCheckBoxForeColor");
    mChkBoxBackColor = ConfigColor("SideBarCheckBoxBackColor");

    mUnconditionalPen = QPen(mUnconditionalJumpLineFalseColor, 1, Qt::SolidLine);
    mConditionalPen = QPen(mConditionalJumpLineFalseColor, 1, Qt::DashLine);
}

void CPUSideBar::updateFonts()
{
    mDefaultFont = mDisas->font();
    this->setFont(mDefaultFont);

    delete mFontMetrics;
    mFontMetrics = new CachedFontMetrics(this, mDefaultFont);
    fontWidth  = mFontMetrics->width(' ');
    fontHeight = mFontMetrics->height();

    mBulletYOffset = 2;
    mBulletRadius = fontHeight - 2 * mBulletYOffset;
}

QSize CPUSideBar::sizeHint() const
{
    return QSize(40, this->viewport()->height());
}

void CPUSideBar::debugStateChangedSlot(DBGSTATE state)
{
    if(state == stopped)
    {
        reload(); //clear
    }
}

void CPUSideBar::reload()
{
    fontHeight = mDisas->getRowHeight();
    viewport()->update();
}

void CPUSideBar::changeTopmostAddress(dsint i)
{
    topVA = i;
    DbgGetRegDumpEx(&regDump, sizeof(REGDUMP));
    reload();
}

void CPUSideBar::setViewableRows(int rows)
{
    viewableRows = rows;
}

void CPUSideBar::setSelection(dsint selVA)
{
    if(selVA != selectedVA)
    {
        selectedVA = selVA;
        reload();
    }
}

bool CPUSideBar::isJump(int i) const
{
    if(i < 0 || i >= mInstrBuffer->size())
        return false;

    const Instruction_t & instr = mInstrBuffer->at(i);
    Instruction_t::BranchType branchType = instr.branchType;
    if(branchType == Instruction_t::Unconditional || branchType == Instruction_t::Conditional)
    {
        duint start = mDisas->getBase();
        duint end = start + mDisas->getSize();
        duint addr = DbgGetBranchDestination(start + instr.rva);

        if(!addr)
            return false;

        return addr >= start && addr < end; //do not draw jumps that go out of the section
    }

    return false;
}

void CPUSideBar::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this->viewport());

    // Paints background
    painter.fillRect(painter.viewport(), mBackgroundColor);

    // Don't draw anything if there aren't any instructions to draw
    if(mInstrBuffer->size() == 0)
        return;

    if(mCodeFoldingManager.isFolded(regDump.regcontext.cip))
    {
        mCodeFoldingManager.expandFoldSegment(regDump.regcontext.cip);
        mDisas->reloadData();
    }

    int jumpoffset = 0;

    duint last_va = mInstrBuffer->last().rva + mDisas->getBase();
    duint first_va = mInstrBuffer->first().rva + mDisas->getBase();

    QVector<std::pair<QString, duint>> regLabel;
    auto appendReg = [&regLabel, last_va, first_va](const QString & name, duint value)
    {
        if(value >= first_va && value <= last_va)
            regLabel.append(std::make_pair(name, value));
    };
#ifdef _WIN64
    appendReg("RIP", regDump.regcontext.cip);
    appendReg("RAX", regDump.regcontext.cax);
    appendReg("RCX", regDump.regcontext.ccx);
    appendReg("RDX", regDump.regcontext.cdx);
    appendReg("RBX", regDump.regcontext.cbx);
    appendReg("RSP", regDump.regcontext.csp);
    appendReg("RBP", regDump.regcontext.cbp);
    appendReg("RSI", regDump.regcontext.csi);
    appendReg("RDI", regDump.regcontext.cdi);
    appendReg("R8", regDump.regcontext.r8);
    appendReg("R9", regDump.regcontext.r9);
    appendReg("R10", regDump.regcontext.r10);
    appendReg("R11", regDump.regcontext.r11);
    appendReg("R12", regDump.regcontext.r12);
    appendReg("R13", regDump.regcontext.r13);
    appendReg("R14", regDump.regcontext.r14);
    appendReg("R15", regDump.regcontext.r15);
#else //x86
    appendReg("EIP", regDump.regcontext.cip);
    appendReg("EAX", regDump.regcontext.cax);
    appendReg("ECX", regDump.regcontext.ccx);
    appendReg("EDX", regDump.regcontext.cdx);
    appendReg("EBX", regDump.regcontext.cbx);
    appendReg("ESP", regDump.regcontext.csp);
    appendReg("EBP", regDump.regcontext.cbp);
    appendReg("ESI", regDump.regcontext.csi);
    appendReg("EDI", regDump.regcontext.cdi);
#endif //_WIN64
    if(ConfigBool("Gui", "SidebarWatchLabels"))
    {
        BridgeList<WATCHINFO> WatchList;
        DbgGetWatchList(&WatchList);
        for(int i = 0; i < WatchList.Count(); i++)
        {
            if(WatchList[i].varType == WATCHVARTYPE::TYPE_UINT || WatchList[i].varType == WATCHVARTYPE::TYPE_ASCII || WatchList[i].varType == WATCHVARTYPE::TYPE_UNICODE)
            {
                appendReg(QString(WatchList[i].WatchName), WatchList[i].value);
            }
        }
    }

    std::vector<JumpLine> jumpLines;
    std::vector<LabelArrow> labelArrows;

    for(int line = 0; line < viewableRows; line++)
    {
        if(line >= mInstrBuffer->size()) //at the end of the page it will crash otherwise
            break;

        const Instruction_t & instr = mInstrBuffer->at(line);
        duint instrVA = instr.rva + mDisas->getBase();
        duint instrVAEnd = instrVA + instr.length;

        // draw bullet
        drawBullets(&painter, line, DbgGetBpxTypeAt(instrVA) != bp_none, DbgIsBpDisabled(instrVA), DbgGetBookmarkAt(instrVA));

        if(isJump(line)) //handle jumps
        {
            duint baseAddr = mDisas->getBase();
            duint destVA = DbgGetBranchDestination(baseAddr + instr.rva);

            JumpLine jmp;
            jmp.isJumpGoingToExecute = DbgIsJumpGoingToExecute(instrVA);
            jmp.isSelected = (selectedVA == instrVA || selectedVA == destVA);
            jmp.isConditional = instr.branchType == Instruction_t::Conditional;
            jmp.line = line;

            /*
            if(mDisas->currentEIP() != mInstrBuffer->at(line).rva) //create a setting for this
                isJumpGoingToExecute=false;
            */

            jumpoffset++;

            // Do not draw jumps that leave the memory range
            if(destVA >= mDisas->getBase() + mDisas->getSize() || destVA < mDisas->getBase())
                continue;

            if(destVA <= last_va && destVA >= first_va)
            {
                int destLine = line;
                while(destLine > -1 && destLine < mInstrBuffer->size())
                {
                    duint va = mInstrBuffer->at(destLine).rva + mDisas->getBase();
                    if(destVA > instrVA) //jump goes down
                    {
                        duint vaEnd = va + mInstrBuffer->at(destLine).length - 1;
                        if(vaEnd >= destVA)
                            break;
                        destLine++;
                    }
                    else //jump goes up
                    {
                        if(va <= destVA)
                            break;
                        destLine--;
                    }
                }
                jmp.destLine = destLine;
            }
            else if(destVA > last_va)
                jmp.destLine = viewableRows + 6;
            else if(destVA < first_va)
                jmp.destLine = -6;
            jumpLines.emplace_back(jmp);
        }

        QString regLabelText;
        for(auto i = regLabel.cbegin(); i != regLabel.cend(); i++)
        {
            if(i->second >= instrVA && i->second < instrVAEnd)
                regLabelText += i->first + " ";
        }
        if(regLabelText.size())
        {
            regLabelText.chop(1);
            labelArrows.push_back(drawLabel(&painter, line, regLabelText));
        }

        if(isFoldingGraphicsPresent(line) == 1)
        {
            if(mCodeFoldingManager.isFoldStart(instrVA))
                drawFoldingCheckbox(&painter, line * fontHeight, mCodeFoldingManager.isFolded(instrVA));
            else
                drawFoldingCheckbox(&painter, line * fontHeight, false);
        }
        else if(mCodeFoldingManager.isFoldBody(instrVA))
        {
            painter.setPen(QColor(Qt::black));
            painter.drawLine(QPointF(viewport()->width() - fontHeight / 2 - mBulletXOffset - mBulletRadius, line * fontHeight), QPointF(viewport()->width() - fontHeight / 2 - mBulletXOffset - mBulletRadius, (line + 1) * fontHeight));
            if(mCodeFoldingManager.isFoldEnd(instrVA + instr.length - 1))
                painter.drawLine(QPointF(viewport()->width() - fontHeight / 2 - mBulletXOffset - mBulletRadius, (line + 1) * fontHeight), QPointF(viewport()->width(), (line + 1) * fontHeight));
        }
    }
    if(jumpLines.size())
    {
        AllocateJumpOffsets(jumpLines, labelArrows);
        for(auto i : jumpLines)
            drawJump(&painter, i.line, i.destLine, i.jumpOffset, i.isConditional, i.isJumpGoingToExecute, i.isSelected);
    }
    else
    {
        for(auto i = labelArrows.begin(); i != labelArrows.end(); i++)
            i->endX = viewport()->width() - 1 - 11 - (isFoldingGraphicsPresent(i->line) != 0 ? mBulletRadius + fontHeight : 0);
    }
    drawLabelArrows(&painter, labelArrows);
}

void CPUSideBar::mouseReleaseEvent(QMouseEvent* e)
{
    // Don't care if we are not debugging
    if(!DbgIsDebugging())
        return;

    // get clicked line
    const int x = e->pos().x();
    const int y = e->pos().y();
    const int line = y / fontHeight;
    if(line >= mInstrBuffer->size())
        return;
    const int width = viewport()->width();

    const int bulletRadius = fontHeight / 2 + 1; //14/2=7
    const int bulletX = width - mBulletXOffset;
    //const int bulletY = line * fontHeight + mBulletYOffset;

    const bool CheckBoxPresent = isFoldingGraphicsPresent(line);

    if(x > bulletX + bulletRadius)
        return;
    // calculate virtual address of clicked line
    duint wVA = mInstrBuffer->at(line).rva + mDisas->getBase();
    if(CheckBoxPresent)
    {
        if(x > width - fontHeight - mBulletXOffset - mBulletRadius && x < width - mBulletXOffset - mBulletRadius)
        {
            if(e->button() == Qt::LeftButton)
            {
                duint start, end;
                const Instruction_t* instr = &mInstrBuffer->at(line);
                const dsint SelectionStart = mDisas->getSelectionStart();
                const dsint SelectionEnd = mDisas->getSelectionEnd();
                if(mCodeFoldingManager.isFoldStart(wVA))
                {
                    mCodeFoldingManager.setFolded(wVA, !mCodeFoldingManager.isFolded(wVA));
                }
                else if(instr->rva == SelectionStart && (SelectionEnd - SelectionStart + 1) != instr->length)
                {
                    bool success = mCodeFoldingManager.addFoldSegment(wVA, mDisas->getSelectionEnd() - mDisas->getSelectionStart());
                    if(!success)
                        GuiAddLogMessage(tr("Cannot fold selection.\n").toUtf8().constData());
                }
                else if((DbgArgumentGet(wVA, &start, &end) || DbgFunctionGet(wVA, &start, &end)) && wVA == start)
                {
                    bool success = mCodeFoldingManager.addFoldSegment(wVA, end - start);
                    if(!success)
                        GuiAddLogMessage(tr("Cannot fold selection.\n").toUtf8().constData());
                }
                else
                    GuiAddLogMessage(tr("Cannot fold selection.\n").toUtf8().constData());
            }
            else if(e->button() == Qt::RightButton)
            {
                mCodeFoldingManager.delFoldSegment(wVA);
            }
            mDisas->reloadData();
            viewport()->update();
        }
    }
    if(x < bulletX - mBulletRadius)
        return;
    //if(y < bulletY - mBulletRadius || y > bulletY + bulletRadius)
    //    return;

    if(e->button() == Qt::LeftButton)
    {
        QString wCmd;
        // create --> disable --> delete --> create --> ...
        switch(Breakpoints::BPState(bp_normal, wVA))
        {
        case bp_enabled:
            // breakpoint exists and is enabled --> disable breakpoint
            wCmd = "bd ";
            break;
        case bp_disabled:
            // is disabled --> delete or enable
            if(Breakpoints::BPTrival(bp_normal, wVA))
                wCmd = "bc ";
            else
                wCmd = "be ";
            break;
        case bp_non_existent:
            // no breakpoint was found --> create breakpoint
            wCmd = "bp ";
            break;
        }
        wCmd += ToPtrString(wVA);
        DbgCmdExec(wCmd.toUtf8().constData());
    }
}

void CPUSideBar::mouseDoubleClickEvent(QMouseEvent* event)
{
    const int line = event->y() / fontHeight;
    if(line >= mInstrBuffer->size())
        return;
    const bool CheckBoxPresent = isFoldingGraphicsPresent(line);

    if(CheckBoxPresent)
    {
        if(event->x() > width() - fontHeight - mBulletXOffset - mBulletRadius && event->x() < width() - mBulletXOffset - mBulletRadius)
        {
            if(event->button() == Qt::LeftButton)
            {
                duint wVA = mInstrBuffer->at(line).rva + mDisas->getBase();
                duint start = mCodeFoldingManager.getFoldBegin(wVA);
                duint end = mCodeFoldingManager.getFoldEnd(wVA);
                if(mCodeFoldingManager.isFolded(wVA) || (start <= regDump.regcontext.cip && end >= regDump.regcontext.cip))
                {
                    mDisas->setSingleSelection(start - mDisas->getBase());
                    mDisas->expandSelectionUpTo(end - mDisas->getBase());
                    mDisas->setFocus();
                }
            }
        }
    }
}

void CPUSideBar::mouseMoveEvent(QMouseEvent* event)
{
    if(!DbgIsDebugging() || !mInstrBuffer->size())
    {
        QAbstractScrollArea::mouseMoveEvent(event);
        QToolTip::hideText();
        setCursor(QCursor(Qt::ArrowCursor));
        return;
    }

    const QPoint & mousePos = event->pos();
    const QPoint & globalMousePos = event->globalPos();
    const int width = viewport()->width();

    const int mLine = mousePos.y() / fontHeight;

    const bool lineInBounds = mLine > 0 && mLine < mInstrBuffer->size();

    const int mBulletX = width - mBulletXOffset;
    const int mBulletY = mLine * fontHeight + mBulletYOffset;

    const int mouseBulletXOffset = abs(mBulletX - mousePos.x());
    const int mouseBulletYOffset = abs(mBulletY - mousePos.y());

    // calculate virtual address of clicked line
    duint wVA = 0;
    if(lineInBounds)
        wVA = mInstrBuffer->at(mLine).rva + mDisas->getBase();

    // check if mouse is on a code folding box
    if(mousePos.x() > width - fontHeight - mBulletXOffset - mBulletRadius && mousePos.x() < width - mBulletXOffset - mBulletRadius)
    {
        if(isFoldingGraphicsPresent(mLine))
        {
            if(mCodeFoldingManager.isFolded(wVA))
            {
                QToolTip::showText(globalMousePos, tr("Click to unfold, right click to delete."));
            }
            else
            {
                if(mCodeFoldingManager.isFoldStart(wVA))
                    QToolTip::showText(globalMousePos, tr("Click to fold, right click to delete."));
                else
                    QToolTip::showText(globalMousePos, tr("Click to fold."));
            }
        }
    }
    // if (mouseCursor not on a bullet) or (mLine not in bounds)
    else if((mouseBulletXOffset > mBulletRadius ||  mouseBulletYOffset > mBulletRadius) || !lineInBounds)
    {
        QToolTip::hideText();
        setCursor(QCursor(Qt::ArrowCursor));
    }
    else
    {
        switch(Breakpoints::BPState(bp_normal, wVA))
        {
        case bp_enabled:
            QToolTip::showText(globalMousePos, tr("Breakpoint Enabled"));
            break;
        case bp_disabled:
            QToolTip::showText(globalMousePos, tr("Breakpoint Disabled"));
            break;
        case bp_non_existent:
            QToolTip::showText(globalMousePos, tr("Breakpoint Not Set"));
            break;
        }
        setCursor(QCursor(Qt::PointingHandCursor));
    }
}

void CPUSideBar::drawJump(QPainter* painter, int startLine, int endLine, int jumpoffset, bool conditional, bool isexecute, bool isactive)
{
    painter->save();
    if(conditional)
        painter->setPen(mConditionalPen);
    else
        painter->setPen(mUnconditionalPen); //JMP

    // Pixel adjustment to make drawing lines even
    int pixel_y_offs = 0;

    if(isactive) //selected
    {
        QPen activePen = painter->pen();

        // Active/selected jumps use a bold line (2px wide)
        activePen.setWidth(2);

        // Adjust for 2px line
        pixel_y_offs = 0;

        // Use a different color to highlight jumps that will execute
        if(isexecute)
        {
            if(conditional)
                activePen.setColor(mConditionalJumpLineTrueColor);
            else
                activePen.setColor(mUnconditionalJumpLineTrueColor);
        }

        // Update the painter itself with the new pen style
        painter->setPen(activePen);
    }

    // Cache variables
    const int viewportWidth = viewport()->width();
    const int viewportHeight = viewport()->height();

    const int JumpPadding = 11;
    int x = viewportWidth - jumpoffset * JumpPadding - 15 - fontHeight;
    int x_right = viewportWidth - 12;
    int y_start =  fontHeight * (1 + startLine) - 0.5 * fontHeight - pixel_y_offs;
    int y_end =  fontHeight * (1 + endLine) - 0.5 * fontHeight;

    // special handling of self-jumping
    if(startLine == endLine)
    {
        y_start -= fontHeight / 4;
        y_end += fontHeight / 4;
    }

    // Horizontal (<----)
    if(!isFoldingGraphicsPresent(startLine) != 0)
        painter->drawLine(x_right, y_start, x, y_start);
    else
        painter->drawLine(x_right - mBulletRadius - fontHeight, y_start, x, y_start);

    // Vertical
    painter->drawLine(x, y_start, x, y_end);

    // Specialized pen for solid lines only, keeping other styles
    QPen solidLine = painter->pen();
    solidLine.setStyle(Qt::SolidLine);

    // Draw the arrow
    if(!isactive) //selected
    {
        // Horizontal (---->)
        if(isFoldingGraphicsPresent(endLine) != 0)
            painter->drawLine(x, y_end, x_right - mBulletRadius - fontHeight, y_end);
        else
            painter->drawLine(x, y_end, x_right, y_end);

        if(endLine == viewableRows + 6)
        {
            int y = viewportHeight - 1;
            if(y > y_start)
            {
                painter->setPen(solidLine);
                QPoint wPoints[] =
                {
                    QPoint(x - 3, y - 3),
                    QPoint(x, y),
                    QPoint(x + 3, y - 3),
                };
                painter->drawPolyline(wPoints, 3);
            }
        }
        else if(endLine == -6)
        {
            int y = 0;
            painter->setPen(solidLine);
            QPoint wPoints[] =
            {
                QPoint(x - 3, y + 3),
                QPoint(x, y),
                QPoint(x + 3, y + 3),
            };
            painter->drawPolyline(wPoints, 3);
        }
        else
        {
            if(isFoldingGraphicsPresent(endLine) != 0)
                x_right -= mBulletRadius + fontHeight;
            painter->setPen(solidLine);
            QPoint wPoints[] =
            {
                QPoint(x_right - 3, y_end - 3),
                QPoint(x_right, y_end),
                QPoint(x_right - 3, y_end + 3),
            };
            painter->drawPolyline(wPoints, 3);
        }
    }
    else
    {
        if(endLine == viewableRows + 6)
        {
            int y = viewportHeight - 1;
            x--;
            painter->setPen(solidLine);
            QPoint wPoints[] =
            {
                QPoint(x - 3, y - 3),
                QPoint(x, y),
                QPoint(x + 3, y - 3),
            };
            painter->drawPolyline(wPoints, 3);
        }
        else if(endLine == -6)
        {
            int y = 0;
            painter->setPen(solidLine);
            QPoint wPoints[] =
            {
                QPoint(x - 3, y + 3),
                QPoint(x, y),
                QPoint(x + 3, y + 3),
            };
            painter->drawPolyline(wPoints, 3);
        }
        else
        {
            if(isFoldingGraphicsPresent(endLine) != 0)
                drawStraightArrow(painter, x, y_end, x_right - mBulletRadius - fontHeight, y_end);
            else
                drawStraightArrow(painter, x, y_end, x_right, y_end);
        }
    }
    painter->restore();
}

void CPUSideBar::drawBullets(QPainter* painter, int line, bool isbp, bool isbpdisabled, bool isbookmark)
{
    painter->save();

    if(isbp)
        painter->setBrush(QBrush(mBulletBreakpointColor));
    else if(isbookmark)
        painter->setBrush(QBrush(mBulletBookmarkColor));
    else
        painter->setBrush(QBrush(mBulletColor));

    painter->setPen(mBackgroundColor);

    const int x = viewport()->width() - mBulletXOffset; //initial x
    const int y = line * fontHeight; //initial y

    painter->setRenderHint(QPainter::Antialiasing, true);
    if(isbpdisabled) //disabled breakpoint
        painter->setBrush(QBrush(mBulletDisabledBreakpointColor));

    painter->drawEllipse(x, y + mBulletYOffset, mBulletRadius, mBulletRadius);

    painter->restore();
}

CPUSideBar::LabelArrow CPUSideBar::drawLabel(QPainter* painter, int Line, const QString & Text)
{
    painter->save();
    const int LineCoordinate = fontHeight * (1 + Line);

    const QColor & IPLabel = mCipLabelColor;
    const QColor & IPLabelBG = mCipLabelBackgroundColor;

    int width = mFontMetrics->width(Text);
    int x = 1;
    int y = LineCoordinate - fontHeight;

    QRect rect(x, y, width, fontHeight - 1);

    // Draw rectangle
    painter->setBrush(IPLabelBG);
    painter->setPen(IPLabelBG);
    painter->drawRect(rect);

    // Draw text inside the rectangle
    painter->setPen(IPLabel);
    painter->drawText(rect, Qt::AlignHCenter | Qt::AlignVCenter, Text);

    // Draw arrow
    /*y = fontHeight * (1 + Line) - 0.5 * fontHeight;

    painter->setPen(QPen(IPLabelBG, 2.0));
    painter->setBrush(QBrush(IPLabelBG));
    drawStraightArrow(painter, rect.right() + 2, y, this->viewport()->width() - x - 11 - (isFoldingGraphicsPresent(Line) != 0 ? mBulletRadius + fontHeight : 0), y);*/

    LabelArrow labelArrow;
    labelArrow.line = Line;
    labelArrow.startX = rect.right() + 2;
    labelArrow.endX = 0;

    painter->restore();

    return labelArrow;
}

void CPUSideBar::drawLabelArrows(QPainter* painter, const std::vector<LabelArrow> & labelArrows)
{
    if(!labelArrows.empty())
    {
        painter->save();
        painter->setPen(QPen(mCipLabelBackgroundColor, 2.0));
        for(auto i : labelArrows)
        {
            if(i.startX < i.endX)
            {
                int y = fontHeight * (1 + i.line) - 0.5 * fontHeight;
                drawStraightArrow(painter, i.startX, y, i.endX, y);
            }
        }
        painter->restore();
    }
}

void CPUSideBar::drawFoldingCheckbox(QPainter* painter, int y, bool state)
{
    int x = viewport()->width() - fontHeight - mBulletXOffset - mBulletRadius;
    painter->save();
    painter->setBrush(QBrush(mChkBoxBackColor));
    painter->setPen(QColor(mChkBoxForeColor));
    QRect rect(x, y, fontHeight, fontHeight);
    painter->drawRect(rect);

    painter->drawLine(QPointF(x + fontHeight / 4, y + fontHeight / 2), QPointF(x + 3 * fontHeight / 4, y + fontHeight / 2));
    if(state) // "+"
        painter->drawLine(QPointF(x + fontHeight / 2, y + fontHeight / 4), QPointF(x + fontHeight / 2, y + 3 * fontHeight / 4));
    painter->restore();
}

void CPUSideBar::drawStraightArrow(QPainter* painter, int x1, int y1, int x2, int y2)
{
    painter->drawLine(x1, y1, x2, y2);

    const int ArrowSizeX = 4;// Width  of arrow tip in pixels
    const int ArrowSizeY = 4;// Height of arrow tip in pixels

    // X and Y values adjusted so that the arrow itself is symmetrical on 2px
    painter->drawLine(x2 - 1, y2 - 1, x2 - ArrowSizeX, y2 - ArrowSizeY);// Arrow top
    painter->drawLine(x2 - 1, y2, x2 - ArrowSizeX, y2 + ArrowSizeY - 1);// Arrow bottom
}

void CPUSideBar::AllocateJumpOffsets(std::vector<JumpLine> & jumpLines, std::vector<LabelArrow> & labelArrows)
{
    unsigned int* numLines = new unsigned int[viewableRows * 2]; // Low:jump offsets of the vertical jumping line, High:jump offsets of the horizontal jumping line.
    memset(numLines, 0, sizeof(unsigned int) * viewableRows * 2);
    // preprocessing
    for(size_t i = 0; i < jumpLines.size(); i++)
    {
        JumpLine & jmp = jumpLines.at(i);
        jmp.jumpOffset = abs(jmp.destLine - jmp.line);
    }
    // Sort jumpLines so that longer jumps are put at the back.
    std::sort(jumpLines.begin(), jumpLines.end(), [](const JumpLine & op1, const JumpLine & op2)
    {
        return op2.jumpOffset > op1.jumpOffset;
    });
    // Allocate jump offsets
    for(size_t i = 0; i < jumpLines.size(); i++)
    {
        JumpLine & jmp = jumpLines.at(i);
        unsigned int maxJmpOffset = 0;
        if(jmp.line < jmp.destLine)
        {
            for(int j = jmp.line; j <= jmp.destLine && j < viewableRows; j++)
            {
                if(numLines[j] > maxJmpOffset)
                    maxJmpOffset = numLines[j];
            }
        }
        else
        {
            for(int j = jmp.line; j >= jmp.destLine && j >= 0; j--)
            {
                if(numLines[j] > maxJmpOffset)
                    maxJmpOffset = numLines[j];
            }
        }
        jmp.jumpOffset = maxJmpOffset + 1;
        if(jmp.line < jmp.destLine)
        {
            for(int j = jmp.line; j <= jmp.destLine && j < viewableRows; j++)
                numLines[j] = jmp.jumpOffset;
        }
        else
        {
            for(int j = jmp.line; j >= jmp.destLine && j >= 0; j--)
                numLines[j] = jmp.jumpOffset;
        }
        if(jmp.line >= 0 && jmp.line < viewableRows)
            numLines[jmp.line + viewableRows] = jmp.jumpOffset;
        if(jmp.destLine >= 0 && jmp.destLine < viewableRows)
            numLines[jmp.destLine + viewableRows] = jmp.jumpOffset;
    }
    // set label arrows according to jump offsets
    auto viewportWidth = viewport()->width();
    const int JumpPadding = 11;
    for(auto i = labelArrows.begin(); i != labelArrows.end(); i++)
    {
        if(numLines[i->line + viewableRows] != 0)
            i->endX = viewportWidth - numLines[i->line + viewableRows] * JumpPadding - 15 - fontHeight; // This expression should be consistent with drawJump
        else
            i->endX = viewportWidth - 1 - 11 - (isFoldingGraphicsPresent(i->line) != 0 ? mBulletRadius + fontHeight : 0);
    }
    delete[] numLines;
}

int CPUSideBar::isFoldingGraphicsPresent(int line)
{
    if(line < 0 || line >= mInstrBuffer->size()) //There couldn't be a folding box out of viewport
        return 0;
    const Instruction_t* instr = &mInstrBuffer->at(line);
    if(instr == nullptr) //Selection out of a memory page
        return 0;
    auto wVA = instr->rva + mDisas->getBase();
    if(mCodeFoldingManager.isFoldStart(wVA)) //Code is already folded
        return 1;
    const dsint SelectionStart = mDisas->getSelectionStart();
    const dsint SelectionEnd = mDisas->getSelectionEnd();
    duint start, end;
    if((DbgArgumentGet(wVA, &start, &end) || DbgFunctionGet(wVA, &start, &end)) && wVA == start)
    {
        return end - start + 1 != instr->length ? 1 : 0;
    }
    else if(instr->rva >= duint(SelectionStart) && instr->rva < duint(SelectionEnd))
    {
        if(instr->rva == SelectionStart)
            return (SelectionEnd - SelectionStart + 1) != instr->length ? 1 : 0;
    }
    return 0;
}

CodeFoldingHelper* CPUSideBar::getCodeFoldingManager()
{
    return &mCodeFoldingManager;
}

void CPUSideBar::foldDisassembly(duint startAddress, duint length)
{
    mCodeFoldingManager.addFoldSegment(startAddress, length);
}

void* CPUSideBar::operator new(size_t size)
{
    return _aligned_malloc(size, 16);
}

void CPUSideBar::operator delete(void* p)
{
    _aligned_free(p);
}
