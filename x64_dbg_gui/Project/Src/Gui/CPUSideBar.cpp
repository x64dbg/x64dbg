#include "CPUSideBar.h"
#include "Configuration.h"
#include "Breakpoints.h"

CPUSideBar::CPUSideBar(CPUDisassembly* Ptr, QWidget* parent) : QAbstractScrollArea(parent)
{
    topVA = -1;
    selectedVA = -1;
    viewableRows = 0;

    CodePtr = Ptr;

    m_DefaultFont = CodePtr->font();
    this->setFont(m_DefaultFont);

    const QFontMetrics metrics(m_DefaultFont);
    fontWidth  = metrics.width(' ');
    fontHeight = metrics.height();

    InstrBuffer = CodePtr->instructionsBuffer();

    backgroundColor = ConfigColor("SideBarBackgroundColor");

    memset(&regDump, 0, sizeof(REGDUMP));
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
    fontHeight = CodePtr->getRowHeight();
    viewport()->repaint();
}

void CPUSideBar::changeTopmostAddress(int_t i)
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

void CPUSideBar::setSelection(int_t selVA)
{
    if(selVA != selectedVA)
    {
        selectedVA = selVA;
        repaint();
    }
}

bool CPUSideBar::isJump(int i) const
{
    int BranchType = InstrBuffer->at(i).disasm.Instruction.BranchType;
    if(BranchType && BranchType != RetType && BranchType != CallType)
    {
        uint_t start = CodePtr->getBase();
        uint_t end = start + CodePtr->getSize();
        uint_t addr = DbgGetBranchDestination(CodePtr->rvaToVa(InstrBuffer->at(i).rva));
        return addr >= start && addr < end; //do not draw jumps that go out of the section
    }
    return false;
}

void CPUSideBar::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this->viewport());

    // Paints background
    painter.fillRect(painter.viewport(), ConfigColor("SideBarBackgroundColor"));

    if(InstrBuffer->size() == 0)
        return;

    int jumpoffset = 0;

    int_t last_va = InstrBuffer->last().rva + CodePtr->getBase();
    int_t first_va = InstrBuffer->first().rva + CodePtr->getBase();

    for(int line = 0; line < viewableRows; line++)
    {
        if(line >= InstrBuffer->size()) //at the end of the page it will crash otherwise
            break;
        Instruction_t instr = InstrBuffer->at(line);
        int_t instrVA = instr.rva + CodePtr->getBase();

        // draw bullet
        drawBullets(&painter, line, DbgGetBpxTypeAt(instrVA) != bp_none, DbgIsBpDisabled(instrVA), DbgGetBookmarkAt(instrVA));

        if(isJump(line)) //handle jumps
        {
            bool isJumpGoingToExecute = DbgIsJumpGoingToExecute(instrVA);
            bool isSelected = (selectedVA == instrVA);

            /*
            if(CodePtr->currentEIP() != InstrBuffer->at(line).rva) //create a setting for this
                isJumpGoingToExecute=false;
            */

            jumpoffset++;

            int_t destVA = (int_t)DbgGetBranchDestination(CodePtr->rvaToVa(instr.rva));

            if(instr.disasm.Instruction.Opcode == 0xFF)
                continue;

            bool isConditional = !((instr.disasm.Instruction.Opcode == 0xEB) || instr.disasm.Instruction.Opcode == 0xE9);

            if(destVA == instrVA) //do not try to draw EBFE
                continue;
            else if(destVA >= CodePtr->getBase() + CodePtr->getSize() || destVA < CodePtr->getBase()) //do not draw jumps that leave the page
                continue;
            else if(destVA <= last_va && destVA >= first_va)
            {
                int destLine = line;
                while(destLine > -1 && destLine < InstrBuffer->size() && InstrBuffer->at(destLine).rva + CodePtr->getBase() != destVA)
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

        if(InstrBuffer->at(line).rva == CodePtr->currentEIP())
#ifdef _WIN64
            drawLabel(&painter, line, "RIP");
#else //x86
            drawLabel(&painter, line, "EIP");
#endif

        const int_t cur_VA = CodePtr->getBase() + InstrBuffer->at(line).rva;
#ifdef _WIN64
        if(cur_VA == regDump.regcontext.cax)  drawLabel(&painter, line, "RAX");
        if(cur_VA == regDump.regcontext.cbx)  drawLabel(&painter, line, "RBX");
        if(cur_VA == regDump.regcontext.ccx)  drawLabel(&painter, line, "RCX");
        if(cur_VA == regDump.regcontext.cdx)  drawLabel(&painter, line, "RDX");
        if(cur_VA == regDump.regcontext.csi)  drawLabel(&painter, line, "RSI");
        if(cur_VA == regDump.regcontext.cdi)  drawLabel(&painter, line, "RDI");
#else //x86
        if(cur_VA == regDump.regcontext.cax)  drawLabel(&painter, line, "EAX");
        if(cur_VA == regDump.regcontext.cbx)  drawLabel(&painter, line, "EBX");
        if(cur_VA == regDump.regcontext.ccx)  drawLabel(&painter, line, "ECX");
        if(cur_VA == regDump.regcontext.cdx)  drawLabel(&painter, line, "EDX");
        if(cur_VA == regDump.regcontext.csi)  drawLabel(&painter, line, "ESI");
        if(cur_VA == regDump.regcontext.cdi)  drawLabel(&painter, line, "EDI");
#endif

    }
}

void CPUSideBar::mouseReleaseEvent(QMouseEvent* e)
{
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

    if(!DbgIsDebugging())
        return;

    // calculate virtual adress of clicked line
    uint_t wVA = InstrBuffer->at(line).rva + CodePtr->getBase();

    QString wCmd;
    // create --> disable --> delete --> create --> ...
    switch(Breakpoints::BPState(bp_normal, wVA))
    {
    case bp_enabled:
        // breakpoint exists and is enabled --> disable breakpoint
        wCmd = "bd " + QString("%1").arg(wVA, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
        DbgCmdExec(wCmd.toUtf8().constData());
        break;
    case bp_disabled:
        // is disabled --> delete
        wCmd = "bc " + QString("%1").arg(wVA, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
        DbgCmdExec(wCmd.toUtf8().constData());
        break;
    case bp_non_existent:
        // no breakpoint was found --> create breakpoint
        wCmd = "bp " + QString("%1").arg(wVA, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
        DbgCmdExec(wCmd.toUtf8().constData());
        break;

    }
}

void CPUSideBar::drawJump(QPainter* painter, int startLine, int endLine, int jumpoffset, bool conditional, bool isexecute, bool isactive)
{
    painter->save();
    if(!conditional)
        painter->setPen(QPen(ConfigColor("SideBarConditionalJumpLineFalseColor"), 1, Qt::SolidLine));  // jmp
    else
        painter->setPen(QPen(ConfigColor("SideBarUnconditionalJumpLineFalseColor"), 1, Qt::DashLine));
    QPen tmp = painter->pen();

    if(isactive) //selected
    {
        tmp.setWidth(2); //bold line = selected
        if(isexecute) //only highlight selected jumps
        {
            if(!conditional)
                tmp.setColor(ConfigColor("SideBarConditionalJumpLineTrueColor"));
            else
                tmp.setColor(ConfigColor("SideBarUnconditionalJumpLineTrueColor"));
        }
    }
    painter->setPen(tmp);

    const int JumpPadding = 11;
    int x = viewport()->width() - jumpoffset * JumpPadding - 12;
    int x_right = viewport()->width() - 12;
    const int y_start =  fontHeight * (1 + startLine) - 0.5 * fontHeight;
    const int y_end =  fontHeight * (1 + endLine) - 0.5 * fontHeight;

    //horizontal
    painter->drawLine(x_right, y_start, x, y_start);
    //vertical
    painter->drawLine(x, y_start, x, y_end);
    //arrow
    if(!isactive) //selected
    {
        //horizontal
        painter->drawLine(x, y_end, x_right, y_end);

        if(endLine == viewableRows + 6)
        {
            int y = this->viewport()->height() - 1;
            if(y > y_start)
            {
                QPen temp = painter->pen();
                temp.setStyle(Qt::SolidLine);
                painter->setPen(temp);
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
            QPen temp = painter->pen();
            temp.setStyle(Qt::SolidLine);
            painter->setPen(temp);
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
            QPen temp = painter->pen();
            temp.setStyle(Qt::SolidLine);
            painter->setPen(temp);
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
            int y = this->viewport()->height() - 1;
            x--;
            QPen temp = painter->pen();
            temp.setStyle(Qt::SolidLine);
            painter->setPen(temp);
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
            QPen temp = painter->pen();
            temp.setStyle(Qt::SolidLine);
            painter->setPen(temp);
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
        painter->setBrush(QBrush(ConfigColor("SideBarBulletBreakpointColor")));
    else if(isbookmark)
        painter->setBrush(QBrush(ConfigColor("SideBarBulletBookmarkColor")));
    else
        painter->setBrush(QBrush(ConfigColor("SideBarBulletColor")));

    painter->setPen(ConfigColor("SideBarBackgroundColor"));

    const int radius = fontHeight / 2; //14/2=7
    const int y = line * fontHeight; //initial y
    const int yAdd = (fontHeight - radius) / 2 + 1;
    const int x = viewport()->width() - 10; //initial x

    //painter->drawLine(0, y, viewport()->width(), y); //draw raster

    painter->setRenderHint(QPainter::Antialiasing, true);
    if(isbpdisabled) //disabled breakpoint
        painter->setBrush(QBrush(ConfigColor("SideBarBulletDisabledBreakpointColor")));
    painter->drawEllipse(x, y + yAdd, radius, radius);

    painter->restore();
}

void CPUSideBar::drawLabel(QPainter* painter, int Line, QString Text)
{
    painter->save();
    const int LineCoordinate = fontHeight * (1 + Line);
    int length = Text.length();

    const QColor IPLabel = ConfigColor("SideBarCipLabelColor");
    const QColor IPLabelBG = ConfigColor("SideBarCipLabelBackgroundColor");

    int width = length * fontWidth + 2;
    int x = 1;
    int y = LineCoordinate - fontHeight;

    QRect rect(x, y, width, fontHeight);

    //draw rectangle
    painter->setBrush(IPLabelBG);
    painter->setPen(QPen(IPLabelBG));
    painter->drawRect(rect);

    //draw text inside the rectangle
    painter->setPen(QPen(IPLabel));
    painter->drawText(rect, Qt::AlignHCenter | Qt::AlignVCenter, Text);

    //draw arrow
    y = fontHeight * (1 + Line) - 0.5 * fontHeight;
    //y+=3;
    painter->setPen(QPen(IPLabelBG, 2));
    painter->setBrush(QBrush(IPLabelBG));
    drawStraightArrow(painter, rect.right() + 2, y, this->viewport()->width() - x - 15, y);

    painter->restore();
}

void CPUSideBar::drawStraightArrow(QPainter* painter, int x1, int y1, int x2, int y2)
{
    painter->drawLine(x1, y1, x2, y2);

    /*
    // this does not work
    QPainterPath path;

    path.moveTo( QPointF(x2, x2));
    path.lineTo (QPointF(x2-ArrowSizeX,y2-ArrowSizeY));
    path.lineTo (QPointF(x2-ArrowSizeX,y2+ArrowSizeY));
    path.closeSubpath();

    painter->setPen (Qt :: NoPen);
    painter->fillPath (path, QBrush (color));*/
    const int ArrowSizeX = 4;  // width  of arrow tip in pixel
    const int ArrowSizeY = 4;  // height of arrow tip in pixel

    painter->drawLine(x2, y2, x2 - ArrowSizeX, y2 - ArrowSizeY);
    painter->drawLine(x2, y2, x2 - ArrowSizeX, y2 + ArrowSizeY);

}

