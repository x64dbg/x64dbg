#include "CPUSideBar.h"
#include "Configuration.h"
#include "Breakpoints.h"
#include <QToolTip>

CPUSideBar::CPUSideBar(CPUDisassembly* Ptr, QWidget* parent) : QAbstractScrollArea(parent)
{
    topVA = -1;
    selectedVA = -1;
    viewableRows = 0;

    mDisas = Ptr;

    mInstrBuffer = mDisas->instructionsBuffer();

    memset(&regDump, 0, sizeof(REGDUMP));

    updateSlots();

    this->setMouseTracking(true);

}

CPUSideBar::~CPUSideBar()
{
}

void CPUSideBar::updateSlots()
{
    connect(Config(), SIGNAL(colorsUpdated()), this, SLOT(updateColors()));
    connect(Config(), SIGNAL(fontsUpdated()), this, SLOT(updateFonts()));

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

    mUnconditionalPen = QPen(mUnconditionalJumpLineFalseColor, 1, Qt::SolidLine);
    mConditionalPen = QPen(mConditionalJumpLineFalseColor, 1, Qt::DashLine);
}

void CPUSideBar::updateFonts()
{
    m_DefaultFont = mDisas->font();
    this->setFont(m_DefaultFont);

    QFontMetrics metrics(m_DefaultFont);
    fontWidth  = metrics.width(' ');
    fontHeight = metrics.height();

    mBulletRadius = fontHeight / 2;
    mBulletYOffset = (fontHeight - mBulletRadius) / 2;
}

QSize CPUSideBar::sizeHint() const
{
    return QSize(40, this->viewport()->height());
}

void CPUSideBar::debugStateChangedSlot(DBGSTATE state)
{
    if(state == stopped)
    {
        repaint(); //clear
    }
}

void CPUSideBar::repaint()
{
    fontHeight = mDisas->getRowHeight();
    viewport()->update();
}

void CPUSideBar::changeTopmostAddress(dsint i)
{
    topVA = i;
    memset(&regDump, 0, sizeof(REGDUMP));
    DbgGetRegDump(&regDump);
    repaint();
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
        repaint();
    }
}

bool CPUSideBar::isJump(int i) const
{
    const Instruction_t & instr = mInstrBuffer->at(i);
    Instruction_t::BranchType branchType = instr.branchType;
    if(branchType != Instruction_t::None)
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

    QHash<duint, QString> regLabelMap;
    auto appendReg = [&](duint value, const char* x32name, const char* x64name)
    {
#ifdef _WIN64
        Q_UNUSED(x32name);
        QString name = x64name;
#else
        Q_UNUSED(x64name);
        QString name = x32name;
#endif //_WIN64
        auto found = regLabelMap.find(value);
        if(found == regLabelMap.end())
            regLabelMap.insert(value, name);
        else
            regLabelMap.insert(value, QString("%1 %2").arg(found.value()).arg(name));
    };
    const auto & regs = regDump.regcontext;
    appendReg(regs.cip, "EIP", "RIP");
    appendReg(regs.cax, "EAX", "RAX");
    appendReg(regs.cbx, "EBX", "RBX");
    appendReg(regs.ccx, "ECX", "RCX");
    appendReg(regs.cdx, "EDX", "RDX");
    appendReg(regs.cbp, "EBP", "RBP");
    appendReg(regs.csp, "ESP", "RSP");
    appendReg(regs.csi, "ESI", "RSI");
    appendReg(regs.cdi, "EDI", "RDI");
#ifdef _WIN64
    appendReg(regs.r8, "", "R8");
    appendReg(regs.r9, "", "R9");
    appendReg(regs.r10, "", "R10");
    appendReg(regs.r11, "", "R11");
    appendReg(regs.r12, "", "R12");
    appendReg(regs.r13, "", "R13");
    appendReg(regs.r14, "", "R14");
    appendReg(regs.r15, "", "R15");
#endif //_WIN64

    int jumpoffset = 0;

    dsint last_va = mInstrBuffer->last().rva + mDisas->getBase();
    dsint first_va = mInstrBuffer->first().rva + mDisas->getBase();

    for(int line = 0; line < viewableRows; line++)
    {
        if(line >= mInstrBuffer->size()) //at the end of the page it will crash otherwise
            break;
        Instruction_t instr = mInstrBuffer->at(line);
        dsint instrVA = instr.rva + mDisas->getBase();

        // draw bullet
        drawBullets(&painter, line, DbgGetBpxTypeAt(instrVA) != bp_none, DbgIsBpDisabled(instrVA), DbgGetBookmarkAt(instrVA));

        if(isJump(line)) //handle jumps
        {
            bool isJumpGoingToExecute = DbgIsJumpGoingToExecute(instrVA);
            bool isSelected = (selectedVA == instrVA);
            bool isConditional = instr.branchType == Instruction_t::Conditional;

            /*
            if(mDisas->currentEIP() != mInstrBuffer->at(line).rva) //create a setting for this
                isJumpGoingToExecute=false;
            */

            jumpoffset++;

            dsint baseAddr = mDisas->getBase();

            dsint destVA = DbgGetBranchDestination(baseAddr + instr.rva);

            // Do not try to draw EBFE (Jump to the same line)
            if(destVA == instrVA)
                continue;

            // Do not try to draw EB00 (Jump to next instruction)
            if(destVA == instrVA + instr.length)
                continue;

            // Do not draw jumps that leave the memory range
            if(destVA >= mDisas->getBase() + mDisas->getSize() || destVA < mDisas->getBase())
                continue;

            if(destVA <= last_va && destVA >= first_va)
            {
                int destLine = line;
                while(destLine > -1 && destLine < mInstrBuffer->size())
                {
                    dsint va = mInstrBuffer->at(destLine).rva + mDisas->getBase();
                    if(destVA > instrVA) //jump goes up
                    {
                        if(va >= destVA)
                            break;
                        destLine++;
                    }
                    else //jump goes down
                    {
                        if(va <= destVA)
                            break;
                        destLine--;
                    }
                }
                drawJump(&painter, line, destLine, jumpoffset, isConditional, isJumpGoingToExecute, isSelected);
            }
            else if(destVA > last_va)
                drawJump(&painter, line, viewableRows + 6, jumpoffset, isConditional, isJumpGoingToExecute, isSelected);
            else if(destVA < first_va)
                drawJump(&painter, line, -6, jumpoffset, isConditional, isJumpGoingToExecute, isSelected);
        }
        auto found = regLabelMap.find(instrVA);
        if(found != regLabelMap.end())
            drawLabel(&painter, line, found.value());
    }
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

    const int bulletRadius = fontHeight / 2; //14/2=7
    const int bulletY = line * fontHeight + (fontHeight - bulletRadius) / 2 + 1; //initial y
    const int bulletX = viewport()->width() - 10; //initial x

    if(x < bulletX || x > bulletX + bulletRadius)
        return;
    if(y < bulletY || y > bulletY + bulletRadius)
        return;

    // calculate virtual address of clicked line
    duint wVA = mInstrBuffer->at(line).rva + mDisas->getBase();

    QString wCmd;
    // create --> disable --> delete --> create --> ...
    switch(Breakpoints::BPState(bp_normal, wVA))
    {
    case bp_enabled:
        // breakpoint exists and is enabled --> disable breakpoint
        wCmd = "bd " + QString("%1").arg(wVA, sizeof(dsint) * 2, 16, QChar('0')).toUpper();
        DbgCmdExec(wCmd.toUtf8().constData());
        break;
    case bp_disabled:
        // is disabled --> delete
        wCmd = "bc " + QString("%1").arg(wVA, sizeof(dsint) * 2, 16, QChar('0')).toUpper();
        DbgCmdExec(wCmd.toUtf8().constData());
        break;
    case bp_non_existent:
        // no breakpoint was found --> create breakpoint
        wCmd = "bp " + QString("%1").arg(wVA, sizeof(dsint) * 2, 16, QChar('0')).toUpper();
        DbgCmdExec(wCmd.toUtf8().constData());
        break;
    }
}

void CPUSideBar::mouseMoveEvent(QMouseEvent* event)
{
    if(!DbgIsDebugging() || !mInstrBuffer->size())
    {
        QAbstractScrollArea::mouseMoveEvent(event);
        return;
    }

    const QPoint & mousePos = event->pos();
    const QPoint & globalMousePos = event->globalPos();

    const int mLine = mousePos.y() / fontHeight;
    const int mBulletX = viewport()->width() - mBulletXOffset;
    const int mBulletY = mLine * fontHeight + mBulletYOffset;

    const int mouseBulletXOffset = abs(mBulletX - mousePos.x());
    const int mouseBulletYOffset = abs(mBulletY - mousePos.y());

    // if (mouseCursor not on a bullet) or (mLine not in bounds)
    if((mouseBulletXOffset > mBulletRadius ||  mouseBulletYOffset > mBulletRadius) ||
            (mLine < 0 || mLine >= mInstrBuffer->size()))
    {
        QToolTip::hideText();
        return;
    }

    // calculate virtual address of clicked line
    duint wVA = mInstrBuffer->at(mLine).rva + mDisas->getBase();

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
}

void CPUSideBar::drawJump(QPainter* painter, int startLine, int endLine, int jumpoffset, bool conditional, bool isexecute, bool isactive)
{
    painter->save();
    if(conditional)
        painter->setPen(mConditionalPen);
    else
        painter->setPen(mUnconditionalPen); //JMP

    // Pixel adjustment to make drawing lines even
    int pixel_y_offs = 1;

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
            if(!conditional)
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
    int x = viewportWidth - jumpoffset * JumpPadding - 12;
    int x_right = viewportWidth - 12;
    const int y_start =  fontHeight * (1 + startLine) - 0.5 * fontHeight - pixel_y_offs;
    const int y_end =  fontHeight * (1 + endLine) - 0.5 * fontHeight;

    // Horizontal (<----)
    painter->drawLine(x_right, y_start, x, y_start);

    // Vertical
    painter->drawLine(x, y_start, x, y_end);

    // Specialized pen for solid lines only, keeping other styles
    QPen solidLine = painter->pen();
    solidLine.setStyle(Qt::SolidLine);

    // Draw the arrow
    if(!isactive) //selected
    {
        // Horizontal (---->)
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
            drawStraightArrow(painter, x, y_end, x_right, y_end);
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

void CPUSideBar::drawLabel(QPainter* painter, int Line, const QString & Text)
{
    painter->save();
    const int LineCoordinate = fontHeight * (1 + Line);
    int length = Text.length();

    const QColor & IPLabel = mCipLabelColor;
    const QColor & IPLabelBG = mCipLabelBackgroundColor;

    int width = length * fontWidth + 2;
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
    y = fontHeight * (1 + Line) - 0.5 * fontHeight;

    painter->setPen(QPen(IPLabelBG, 2.0));
    painter->setBrush(QBrush(IPLabelBG));
    drawStraightArrow(painter, rect.right() + 2, y, this->viewport()->width() - x - 11, y);

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

void* CPUSideBar::operator new(size_t size)
{
    return _aligned_malloc(size, 16);
}

void CPUSideBar::operator delete(void* p)
{
    _aligned_free(p);
}
