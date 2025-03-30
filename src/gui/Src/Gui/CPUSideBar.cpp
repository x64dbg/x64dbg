#include "CPUSideBar.h"
#include "Configuration.h"
#include "Breakpoints.h"
#include "CPUDisassembly.h"
#include "CachedFontMetrics.h"
#include <QToolTip>

CPUSideBar::CPUSideBar(CPUDisassembly* disassembly, QWidget* parent)
    : QAbstractScrollArea(parent)
{
    setWindowTitle("SideBar");

    mDisassembly = disassembly;
    mInstrBuffer = mDisassembly->instructionsBuffer();

    updateSlots();

    setMouseTracking(true);
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
    mConditionalJumpLineFalseBackwardsColor = ConfigColor("SideBarConditionalJumpLineFalseBackwardsColor");
    mUnconditionalJumpLineFalseBackwardsColor = ConfigColor("SideBarUnconditionalJumpLineFalseBackwardsColor");
    mConditionalJumpLineTrueBackwardsColor = ConfigColor("SideBarConditionalJumpLineTrueBackwardsColor");
    mUnconditionalJumpLineTrueBackwardsColor = ConfigColor("SideBarUnconditionalJumpLineTrueBackwardsColor");

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
    mUnconditionalBackwardsPen = QPen(mUnconditionalJumpLineFalseBackwardsColor, 1, Qt::SolidLine);
    mConditionalBackwardsPen = QPen(mConditionalJumpLineFalseBackwardsColor, 1, Qt::DashLine);
}

void CPUSideBar::updateFonts()
{
    mDefaultFont = mDisassembly->font();
    setFont(mDefaultFont);

    delete mFontMetrics;
    mFontMetrics = new CachedFontMetrics(this, mDefaultFont);
    mFontWidth  = mFontMetrics->width(' ');
    mFontHeight = mFontMetrics->height();

    mBulletYOffset = 2;
    mBulletRadius = mFontHeight - 2 * mBulletYOffset;
}

QSize CPUSideBar::sizeHint() const
{
    return QSize(40, viewport()->height());
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
    mFontHeight = mDisassembly->getRowHeight();
    viewport()->update();
}

void CPUSideBar::changeTopmostAddress(duint i)
{
    mTopVA = i;
    DbgGetRegDumpEx(&mRegDump, sizeof(mRegDump));
    reload();
}

void CPUSideBar::setViewableRows(duint rows)
{
    mViewableRows = rows;
}

void CPUSideBar::setSelection(duint selVA)
{
    if(selVA != mSelectedVA)
    {
        mSelectedVA = selVA;
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
        duint start = mDisassembly->getBase();
        duint end = start + mDisassembly->getSize();
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

    QPainter painter(viewport());

    // Paints background
    painter.fillRect(painter.viewport(), mBackgroundColor);

    // Don't draw anything if there aren't any instructions to draw
    if(mInstrBuffer->size() == 0)
        return;

    if(mCodeFoldingManager.isFolded(mRegDump.regcontext.cip))
    {
        mCodeFoldingManager.expandFoldSegment(mRegDump.regcontext.cip);
        mDisassembly->reloadData();
    }

    int jumpoffset = 0;

    duint last_va = mInstrBuffer->last().rva + mDisassembly->getBase();
    duint first_va = mInstrBuffer->first().rva + mDisassembly->getBase();

    QVector<std::pair<QString, duint>> regLabel;
    auto appendReg = [&regLabel, last_va, first_va](const QString & name, duint value)
    {
        if(value >= first_va && value <= last_va)
            regLabel.append(std::make_pair(name, value));
    };
#ifdef _WIN64
    appendReg("RIP", mRegDump.regcontext.cip);
    appendReg("RAX", mRegDump.regcontext.cax);
    appendReg("RCX", mRegDump.regcontext.ccx);
    appendReg("RDX", mRegDump.regcontext.cdx);
    appendReg("RBX", mRegDump.regcontext.cbx);
    appendReg("RSP", mRegDump.regcontext.csp);
    appendReg("RBP", mRegDump.regcontext.cbp);
    appendReg("RSI", mRegDump.regcontext.csi);
    appendReg("RDI", mRegDump.regcontext.cdi);
    appendReg("R8", mRegDump.regcontext.r8);
    appendReg("R9", mRegDump.regcontext.r9);
    appendReg("R10", mRegDump.regcontext.r10);
    appendReg("R11", mRegDump.regcontext.r11);
    appendReg("R12", mRegDump.regcontext.r12);
    appendReg("R13", mRegDump.regcontext.r13);
    appendReg("R14", mRegDump.regcontext.r14);
    appendReg("R15", mRegDump.regcontext.r15);
#else //x86
    appendReg("EIP", mRegDump.regcontext.cip);
    appendReg("EAX", mRegDump.regcontext.cax);
    appendReg("ECX", mRegDump.regcontext.ccx);
    appendReg("EDX", mRegDump.regcontext.cdx);
    appendReg("EBX", mRegDump.regcontext.cbx);
    appendReg("ESP", mRegDump.regcontext.csp);
    appendReg("EBP", mRegDump.regcontext.cbp);
    appendReg("ESI", mRegDump.regcontext.csi);
    appendReg("EDI", mRegDump.regcontext.cdi);
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

    for(duint line = 0; line < mViewableRows; line++)
    {
        if(line >= (duint)mInstrBuffer->size()) //at the end of the page it will crash otherwise
            break;

        const Instruction_t & instr = mInstrBuffer->at(line);
        duint instrVA = instr.rva + mDisassembly->getBase();
        duint instrVAEnd = instrVA + instr.length;

        // draw bullet
        drawBullets(&painter, line, DbgGetBpxTypeAt(instrVA) != bp_none, DbgIsBpDisabled(instrVA), DbgGetBookmarkAt(instrVA));

        if(isJump(line)) //handle jumps
        {
            duint baseAddr = mDisassembly->getBase();
            duint destVA = DbgGetBranchDestination(baseAddr + instr.rva);

            JumpLine jmp;
            jmp.isJumpGoingToExecute = DbgIsJumpGoingToExecute(instrVA);
            jmp.isSelected = (mSelectedVA == instrVA || mSelectedVA == destVA);
            jmp.isConditional = instr.branchType == Instruction_t::Conditional;
            jmp.line = line;

            /*
            if(mDisas->currentEIP() != mInstrBuffer->at(line).rva) //create a setting for this
                isJumpGoingToExecute=false;
            */

            jumpoffset++;

            // Do not draw jumps that leave the memory range
            if(destVA >= mDisassembly->getBase() + mDisassembly->getSize() || destVA < mDisassembly->getBase())
                continue;

            if(destVA <= last_va && destVA >= first_va)
            {
                int destLine = line;
                while(destLine > -1 && destLine < mInstrBuffer->size())
                {
                    duint va = mInstrBuffer->at(destLine).rva + mDisassembly->getBase();
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
                jmp.destLine = mViewableRows + 6;
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
                drawFoldingCheckbox(&painter, line * mFontHeight, mCodeFoldingManager.isFolded(instrVA));
            else
                drawFoldingCheckbox(&painter, line * mFontHeight, false);
        }
        else if(mCodeFoldingManager.isFoldBody(instrVA))
        {
            painter.setPen(QColor(Qt::black));
            painter.drawLine(QPointF(viewport()->width() - mFontHeight / 2 - mBulletXOffset - mBulletRadius, line * mFontHeight), QPointF(viewport()->width() - mFontHeight / 2 - mBulletXOffset - mBulletRadius, (line + 1) * mFontHeight));
            if(mCodeFoldingManager.isFoldEnd(instrVA + instr.length - 1))
                painter.drawLine(QPointF(viewport()->width() - mFontHeight / 2 - mBulletXOffset - mBulletRadius, (line + 1) * mFontHeight), QPointF(viewport()->width(), (line + 1) * mFontHeight));
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
            i->endX = viewport()->width() - 1 - 11 - (isFoldingGraphicsPresent(i->line) != 0 ? mBulletRadius + mFontHeight : 0);
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
    const int line = y / mFontHeight;
    if(line < 0 || line >= mInstrBuffer->size())
        return;
    const int width = viewport()->width();

    const int bulletRadius = mFontHeight / 2 + 1; //14/2=7
    const int bulletX = width - mBulletXOffset;
    //const int bulletY = line * fontHeight + mBulletYOffset;

    const bool CheckBoxPresent = isFoldingGraphicsPresent(line);

    if(x > bulletX + bulletRadius)
        return;
    // calculate virtual address of clicked line
    duint va = mInstrBuffer->at(line).rva + mDisassembly->getBase();
    if(CheckBoxPresent)
    {
        if(x > width - mFontHeight - mBulletXOffset - mBulletRadius && x < width - mBulletXOffset - mBulletRadius)
        {
            if(e->button() == Qt::LeftButton)
            {
                duint start, end;
                const Instruction_t* instr = &mInstrBuffer->at(line);
                const dsint SelectionStart = mDisassembly->getSelectionStart();
                const dsint SelectionEnd = mDisassembly->getSelectionEnd();
                if(mCodeFoldingManager.isFoldStart(va))
                {
                    mCodeFoldingManager.setFolded(va, !mCodeFoldingManager.isFolded(va));
                }
                else if(instr->rva == SelectionStart && (SelectionEnd - SelectionStart + 1) != instr->length)
                {
                    bool success = mCodeFoldingManager.addFoldSegment(va, mDisassembly->getSelectionEnd() - mDisassembly->getSelectionStart());
                    if(!success)
                        GuiAddLogMessage(tr("Cannot fold selection.\n").toUtf8().constData());
                }
                else if((DbgArgumentGet(va, &start, &end) || DbgFunctionGet(va, &start, &end)) && va == start)
                {
                    bool success = mCodeFoldingManager.addFoldSegment(va, end - start);
                    if(!success)
                        GuiAddLogMessage(tr("Cannot fold selection.\n").toUtf8().constData());
                }
                else
                    GuiAddLogMessage(tr("Cannot fold selection.\n").toUtf8().constData());
            }
            else if(e->button() == Qt::RightButton)
            {
                mCodeFoldingManager.delFoldSegment(va);
            }
            mDisassembly->reloadData();
            viewport()->update();
        }
    }
    if(x < bulletX - mBulletRadius)
        return;
    //if(y < bulletY - mBulletRadius || y > bulletY + bulletRadius)
    //    return;

    if(e->button() == Qt::LeftButton)
    {
        QString cmd;
        // create --> disable --> delete --> create --> ...
        switch(Breakpoints::BPState(bp_normal, va))
        {
        case bp_enabled:
            // breakpoint exists and is enabled --> disable breakpoint
            cmd = "bd ";
            break;
        case bp_disabled:
            // is disabled --> delete or enable
            if(Breakpoints::BPTrival(bp_normal, va))
                cmd = "bc ";
            else
                cmd = "be ";
            break;
        case bp_non_existent:
            // no breakpoint was found --> create breakpoint
            cmd = "bp ";
            break;
        }
        cmd += ToPtrString(va);
        DbgCmdExec(cmd);
    }
}

void CPUSideBar::mouseDoubleClickEvent(QMouseEvent* event)
{
    const int line = event->y() / mFontHeight;
    if(line >= mInstrBuffer->size())
        return;
    const bool CheckBoxPresent = isFoldingGraphicsPresent(line);

    if(CheckBoxPresent)
    {
        if(event->x() > width() - mFontHeight - mBulletXOffset - mBulletRadius && event->x() < width() - mBulletXOffset - mBulletRadius)
        {
            if(event->button() == Qt::LeftButton)
            {
                duint va = mInstrBuffer->at(line).rva + mDisassembly->getBase();
                duint start = mCodeFoldingManager.getFoldBegin(va);
                duint end = mCodeFoldingManager.getFoldEnd(va);
                if(mCodeFoldingManager.isFolded(va) || (start <= mRegDump.regcontext.cip && end >= mRegDump.regcontext.cip))
                {
                    mDisassembly->setSingleSelection(start - mDisassembly->getBase());
                    mDisassembly->expandSelectionUpTo(end - mDisassembly->getBase());
                    mDisassembly->setFocus();
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

    const int mLine = mousePos.y() / mFontHeight;

    const bool lineInBounds = mLine > 0 && mLine < mInstrBuffer->size();

    const int mBulletX = width - mBulletXOffset;
    const int mBulletY = mLine * mFontHeight + mBulletYOffset;

    const int mouseBulletXOffset = abs(mBulletX - mousePos.x());
    const int mouseBulletYOffset = abs(mBulletY - mousePos.y());

    // calculate virtual address of clicked line
    duint va = 0;
    if(lineInBounds)
        va = mInstrBuffer->at(mLine).rva + mDisassembly->getBase();

    // check if mouse is on a code folding box
    if(mousePos.x() > width - mFontHeight - mBulletXOffset - mBulletRadius && mousePos.x() < width - mBulletXOffset - mBulletRadius)
    {
        if(isFoldingGraphicsPresent(mLine))
        {
            if(mCodeFoldingManager.isFolded(va))
            {
                QToolTip::showText(globalMousePos, tr("Click to unfold, right click to delete."));
            }
            else
            {
                if(mCodeFoldingManager.isFoldStart(va))
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
        switch(Breakpoints::BPState(bp_normal, va))
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

    // Pixel adjustment to make drawing lines even
    int pixel_y_offs = 0;

    int y_start =  mFontHeight * (1 + startLine) - 0.5 * mFontHeight - pixel_y_offs;
    int y_end =  mFontHeight * (1 + endLine) - 0.5 * mFontHeight;
    int y_diff = y_end >= y_start ? 1 : -1;

    if(conditional)
    {
        if(y_diff > 0)
            painter->setPen(mConditionalPen);
        else
            painter->setPen(mConditionalBackwardsPen);
    }
    else //JMP
    {
        if(y_diff > 0)
            painter->setPen(mUnconditionalPen);
        else
            painter->setPen(mUnconditionalBackwardsPen);
    }

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
            {
                if(y_diff > 0)
                    activePen.setColor(mConditionalJumpLineTrueColor);
                else
                    activePen.setColor(mConditionalJumpLineTrueBackwardsColor);
            }
            else
            {
                if(y_diff > 0)
                    activePen.setColor(mUnconditionalJumpLineTrueColor);
                else
                    activePen.setColor(mUnconditionalJumpLineTrueBackwardsColor);
            }
        }

        // Update the painter itself with the new pen style
        painter->setPen(activePen);
    }

    // Cache variables
    const int viewportWidth = viewport()->width();
    const int viewportHeight = viewport()->height();

    const int JumpPadding = 11;
    int x = viewportWidth - jumpoffset * JumpPadding - 15 - mFontHeight;
    int x_right = viewportWidth - 12;

    // special handling of self-jumping
    if(startLine == endLine)
    {
        y_start -= mFontHeight / 4;
        y_end += mFontHeight / 4;
    }

    // Horizontal (<----)
    if(!isFoldingGraphicsPresent(startLine) != 0)
        painter->drawLine(x_right, y_start, x, y_start);
    else
        painter->drawLine(x_right - mBulletRadius - mFontHeight, y_start, x, y_start);

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
            painter->drawLine(x, y_end, x_right - mBulletRadius - mFontHeight, y_end);
        else
            painter->drawLine(x, y_end, x_right, y_end);

        if(endLine == mViewableRows + 6)
        {
            int y = viewportHeight - 1;
            if(y > y_start)
            {
                painter->setPen(solidLine);
                QPoint points[] =
                {
                    QPoint(x - 3, y - 3),
                    QPoint(x, y),
                    QPoint(x + 3, y - 3),
                };
                painter->drawPolyline(points, 3);
            }
        }
        else if(endLine == -6)
        {
            int y = 0;
            painter->setPen(solidLine);
            QPoint points[] =
            {
                QPoint(x - 3, y + 3),
                QPoint(x, y),
                QPoint(x + 3, y + 3),
            };
            painter->drawPolyline(points, 3);
        }
        else
        {
            if(isFoldingGraphicsPresent(endLine) != 0)
                x_right -= mBulletRadius + mFontHeight;
            painter->setPen(solidLine);
            QPoint points[] =
            {
                QPoint(x_right - 3, y_end - 3),
                QPoint(x_right, y_end),
                QPoint(x_right - 3, y_end + 3),
            };
            painter->drawPolyline(points, 3);
        }
    }
    else
    {
        if(endLine == mViewableRows + 6)
        {
            int y = viewportHeight - 1;
            x--;
            painter->setPen(solidLine);
            QPoint points[] =
            {
                QPoint(x - 3, y - 3),
                QPoint(x, y),
                QPoint(x + 3, y - 3),
            };
            painter->drawPolyline(points, 3);
        }
        else if(endLine == -6)
        {
            int y = 0;
            painter->setPen(solidLine);
            QPoint points[] =
            {
                QPoint(x - 3, y + 3),
                QPoint(x, y),
                QPoint(x + 3, y + 3),
            };
            painter->drawPolyline(points, 3);
        }
        else
        {
            if(isFoldingGraphicsPresent(endLine) != 0)
                drawStraightArrow(painter, x, y_end, x_right - mBulletRadius - mFontHeight, y_end);
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
    const int y = line * mFontHeight; //initial y

    painter->setRenderHint(QPainter::Antialiasing, true);
    if(isbpdisabled) //disabled breakpoint
        painter->setBrush(QBrush(mBulletDisabledBreakpointColor));

    painter->drawEllipse(x, y + mBulletYOffset, mBulletRadius, mBulletRadius);

    painter->restore();
}

CPUSideBar::LabelArrow CPUSideBar::drawLabel(QPainter* painter, int Line, const QString & Text)
{
    painter->save();
    const int LineCoordinate = mFontHeight * (1 + Line);

    const QColor & IPLabel = mCipLabelColor;
    const QColor & IPLabelBG = mCipLabelBackgroundColor;

    int width = mFontMetrics->width(Text);
    int x = 1;
    int y = LineCoordinate - mFontHeight;

    QRect rect(x, y, width, mFontHeight - 1);

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
    drawStraightArrow(painter, rect.right() + 2, y, viewport()->width() - x - 11 - (isFoldingGraphicsPresent(Line) != 0 ? mBulletRadius + fontHeight : 0), y);*/

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
                int y = mFontHeight * (1 + i.line) - 0.5 * mFontHeight;
                drawStraightArrow(painter, i.startX, y, i.endX, y);
            }
        }
        painter->restore();
    }
}

void CPUSideBar::drawFoldingCheckbox(QPainter* painter, int y, bool state)
{
    int x = viewport()->width() - mFontHeight - mBulletXOffset - mBulletRadius;
    painter->save();
    painter->setBrush(QBrush(mChkBoxBackColor));
    painter->setPen(QColor(mChkBoxForeColor));
    QRect rect(x, y, mFontHeight, mFontHeight);
    painter->drawRect(rect);

    painter->drawLine(QPointF(x + mFontHeight / 4, y + mFontHeight / 2), QPointF(x + 3 * mFontHeight / 4, y + mFontHeight / 2));
    if(state) // "+"
        painter->drawLine(QPointF(x + mFontHeight / 2, y + mFontHeight / 4), QPointF(x + mFontHeight / 2, y + 3 * mFontHeight / 4));
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
    unsigned int* numLines = new unsigned int[mViewableRows * 2]; // Low:jump offsets of the vertical jumping line, High:jump offsets of the horizontal jumping line.
    memset(numLines, 0, sizeof(unsigned int) * mViewableRows * 2);
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
            for(int j = jmp.line; j <= jmp.destLine && (duint)j < mViewableRows; j++)
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
            for(int j = jmp.line; j <= jmp.destLine && (duint)j < mViewableRows; j++)
                numLines[j] = jmp.jumpOffset;
        }
        else
        {
            for(int j = jmp.line; j >= jmp.destLine && j >= 0; j--)
                numLines[j] = jmp.jumpOffset;
        }
        if(jmp.line >= 0 && (duint)jmp.line < mViewableRows)
            numLines[jmp.line + mViewableRows] = jmp.jumpOffset;
        if(jmp.destLine >= 0 && (duint)jmp.destLine < mViewableRows)
            numLines[jmp.destLine + mViewableRows] = jmp.jumpOffset;
    }
    // set label arrows according to jump offsets
    auto viewportWidth = viewport()->width();
    const int JumpPadding = 11;
    for(auto i = labelArrows.begin(); i != labelArrows.end(); i++)
    {
        if(numLines[i->line + mViewableRows] != 0)
            i->endX = viewportWidth - numLines[i->line + mViewableRows] * JumpPadding - 15 - mFontHeight; // This expression should be consistent with drawJump
        else
            i->endX = viewportWidth - 1 - 11 - (isFoldingGraphicsPresent(i->line) != 0 ? mBulletRadius + mFontHeight : 0);
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
    auto va = instr->rva + mDisassembly->getBase();
    if(mCodeFoldingManager.isFoldStart(va)) //Code is already folded
        return 1;
    const dsint SelectionStart = mDisassembly->getSelectionStart();
    const dsint SelectionEnd = mDisassembly->getSelectionEnd();
    duint start, end;
    if((DbgArgumentGet(va, &start, &end) || DbgFunctionGet(va, &start, &end)) && va == start)
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
