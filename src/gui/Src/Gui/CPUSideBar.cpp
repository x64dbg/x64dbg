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

    InstrBuffer = mDisas->instructionsBuffer();

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
    if(i != topVA)
    {
        topVA = i;
        memset(&regDump, 0, sizeof(REGDUMP));
        DbgGetRegDump(&regDump);
        repaint();
    }
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
    const auto & instr = InstrBuffer->at(i);
    auto branchType = instr.branchType;
    if(branchType != Instruction_t::None)
    {
        duint start = mDisas->getBase();
        duint end = start + mDisas->getSize();
        duint addr = instr.branchDestination;
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
    if(InstrBuffer->size() == 0)
        return;

    // Line numbers to draw each register label
    int registerLines[9] = { -1, -1, -1, -1, -1, -1, -1, -1, -1 };

    int jumpoffset = 0;

    dsint last_va = InstrBuffer->last().rva + mDisas->getBase();
    dsint first_va = InstrBuffer->first().rva + mDisas->getBase();

    for(int line = 0; line < viewableRows; line++)
    {
        if(line >= InstrBuffer->size()) //at the end of the page it will crash otherwise
            break;
        Instruction_t instr = InstrBuffer->at(line);
        dsint instrVA = instr.rva + mDisas->getBase();

        // draw bullet
        drawBullets(&painter, line, DbgGetBpxTypeAt(instrVA) != bp_none, DbgIsBpDisabled(instrVA), DbgGetBookmarkAt(instrVA));

        if(isJump(line)) //handle jumps
        {
            bool isJumpGoingToExecute = DbgIsJumpGoingToExecute(instrVA);
            bool isSelected = (selectedVA == instrVA);
            bool isConditional = instr.branchType == Instruction_t::Conditional;

            /*
            if(mDisas->currentEIP() != InstrBuffer->at(line).rva) //create a setting for this
                isJumpGoingToExecute=false;
            */

            jumpoffset++;

            dsint destVA = instr.branchDestination;

            // Do not try to draw EBFE (Jump to the same line)
            if(destVA == instrVA)
                continue;

            // Do not draw jumps that leave the memory range
            if(destVA >= mDisas->getBase() + mDisas->getSize() || destVA < mDisas->getBase())
                continue;

            if(destVA <= last_va && destVA >= first_va)
            {
                int destLine = line;
                while(destLine > -1 && destLine < InstrBuffer->size() && InstrBuffer->at(destLine).rva + mDisas->getBase() != destVA)
                {
                    if(destVA > instrVA) //jump goes up
                        destLine++;
                    else //jump goes down
                        destLine--;
                }
                drawJump(&painter, line, destLine, jumpoffset, isConditional, isJumpGoingToExecute, isSelected);
            }
            else if(destVA > last_va)
                drawJump(&painter, line, viewableRows + 6, jumpoffset, isConditional, isJumpGoingToExecute, isSelected);
            else if(destVA < first_va)
                drawJump(&painter, line, -6, jumpoffset, isConditional, isJumpGoingToExecute, isSelected);
        }

        // Register label line positions
        const dsint cur_VA = mDisas->getBase() + InstrBuffer->at(line).rva;

        if(InstrBuffer->at(line).rva == mDisas->currentEIP())
            registerLines[0] = line;

        if(cur_VA == regDump.regcontext.cax) registerLines[1] = line;
        if(cur_VA == regDump.regcontext.cbx) registerLines[2] = line;
        if(cur_VA == regDump.regcontext.ccx) registerLines[3] = line;
        if(cur_VA == regDump.regcontext.cdx) registerLines[4] = line;
        if(cur_VA == regDump.regcontext.csi) registerLines[5] = line;
        if(cur_VA == regDump.regcontext.cdi) registerLines[6] = line;
    }

#ifdef _WIN64
    if(registerLines[0] != -1) drawLabel(&painter, registerLines[0], "RIP");
    if(registerLines[1] != -1) drawLabel(&painter, registerLines[1], "RAX");
    if(registerLines[2] != -1) drawLabel(&painter, registerLines[2], "RBX");
    if(registerLines[3] != -1) drawLabel(&painter, registerLines[3], "RCX");
    if(registerLines[4] != -1) drawLabel(&painter, registerLines[4], "RDX");
    if(registerLines[5] != -1) drawLabel(&painter, registerLines[5], "RSI");
    if(registerLines[6] != -1) drawLabel(&painter, registerLines[6], "RDI");
#else //x86
    if(registerLines[0] != -1) drawLabel(&painter, registerLines[0], "EIP");
    if(registerLines[1] != -1) drawLabel(&painter, registerLines[1], "EAX");
    if(registerLines[2] != -1) drawLabel(&painter, registerLines[2], "EBX");
    if(registerLines[3] != -1) drawLabel(&painter, registerLines[3], "ECX");
    if(registerLines[4] != -1) drawLabel(&painter, registerLines[4], "EDX");
    if(registerLines[5] != -1) drawLabel(&painter, registerLines[5], "ESI");
    if(registerLines[6] != -1) drawLabel(&painter, registerLines[6], "EDI");
#endif
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
    duint wVA = InstrBuffer->at(line).rva + mDisas->getBase();

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

    mDisas->repaint();
}

void CPUSideBar::mouseMoveEvent(QMouseEvent *event)
{
    if(!DbgIsDebugging())
    {
        QAbstractScrollArea::mouseMoveEvent(event);
        return;
    }

    QPoint mousePos = event->pos();
    QPoint globalMousePos = event->globalPos();

    const int mLine = mousePos.y() / fontHeight;
    const int mBulletX = viewport()->width() - mBulletXOffset;
    const int mBulletY = mLine * fontHeight + mBulletYOffset;

    const int mouseBulletXOffset = abs(mBulletX - mousePos.x());
    const int mouseBulletYOffset = abs(mBulletY - mousePos.y());

    // mouseCursor not on a bullet
    if(mouseBulletXOffset > mBulletRadius ||  mouseBulletYOffset > mBulletRadius)
    {
        QToolTip::hideText();
        return;
    }

    // calculate virtual address of clicked line
    duint wVA = InstrBuffer->at(mLine).rva + mDisas->getBase();

    switch(Breakpoints::BPState(bp_normal, wVA))
    {
    case bp_enabled:
        QToolTip::showText(globalMousePos, "BP enabled");
        break;
    case bp_disabled:
        QToolTip::showText(globalMousePos, "BP disabled");
        break;
    case bp_non_existent:
        QToolTip::showText(globalMousePos, "No BP set");
        break;
    }
}

void CPUSideBar::drawJump(QPainter* painter, int startLine, int endLine, int jumpoffset, bool conditional, bool isexecute, bool isactive)
{
    painter->save();
    if(!conditional)
        painter->setPen(QPen(mConditionalJumpLineFalseColor, 1, Qt::SolidLine));  // jmp
    else
        painter->setPen(QPen(mUnconditionalJumpLineFalseColor, 1, Qt::DashLine));

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

void CPUSideBar::drawLabel(QPainter* painter, int Line, QString Text)
{
    painter->save();
    const int LineCoordinate = fontHeight * (1 + Line);
    int length = Text.length();

    const QColor& IPLabel = mCipLabelColor;
    const QColor& IPLabelBG = mCipLabelBackgroundColor;

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
