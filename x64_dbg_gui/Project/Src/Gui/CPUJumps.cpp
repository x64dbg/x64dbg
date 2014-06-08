#include "CPUJumps.h"

CPUJumps::CPUJumps(CPUDisassembly *Ptr, QWidget *parent) :
    QAbstractScrollArea(parent)
{

    topVA = -1;
    viewableRows = 0;

    CodePtr = Ptr;

    m_DefaultFont = QFont("Monospace", 8);

    const QFontMetrics metrics(m_DefaultFont);
    fontWidth  = metrics.width('W');
    fontHeight = metrics.height();

    InstrBuffer = CodePtr->instructionsBuffer();

    connect(Bridge::getBridge(), SIGNAL(disassembleAt(int_t, int_t)), this, SLOT(disassembleAt(int_t, int_t)));
}

QSize CPUJumps::sizeHint() const
{
    return QSize(40,this->viewport()->height());
}


void CPUJumps::disassembleAt(int_t parVA, int_t parCIP)
{
    repaint();
}

void CPUJumps::repaint(){

    viewport()->repaint();
}

void CPUJumps::changeTopmostAddress(int i)
{
    if(i!=topVA){
        topVA = i;
    }
}

void CPUJumps::setViewableRows(int rows)
{
    viewableRows = rows;
}

bool CPUJumps::isJump(int i) const{

    int BranchType=InstrBuffer->at(i).disasm.Instruction.BranchType;
    if(BranchType && BranchType!=RetType && BranchType!=CallType){
        return true;
    }
    return false;
}

void CPUJumps::paintEvent(QPaintEvent *event)
{
    painter = new QPainter(viewport());

    int jumpoffset = 0;

    int_t last_va = InstrBuffer->last().rva + CodePtr->getBase();
    int_t first_va = InstrBuffer->first().rva + CodePtr->getBase();

    for(int line=0;line<viewableRows;line++){
        // draw bullet
        drawBullets(line,DbgGetBpxTypeAt(InstrBuffer->at(line).rva + CodePtr->getBase()) == bp_none);


        if(isJump(line)){
            jumpoffset++;

            int_t destRVA = (int_t)InstrBuffer->at(line).disasm.Instruction.AddrValue;

            if( InstrBuffer->at(line).disasm.Instruction.Opcode == 0xFF)
                continue;


            bool cond = !((InstrBuffer->at(line).disasm.Instruction.Opcode == 0xEB) || InstrBuffer->at(line).disasm.Instruction.Opcode == 0xE9);
            if(destRVA <= last_va && destRVA >= first_va){

                int destLine = line+1;

                while(InstrBuffer->at(destLine).rva + CodePtr->getBase() != destRVA
                      && destLine <viewableRows )
                    destLine++;


                drawJump(line,destLine,jumpoffset,cond,DbgIsJumpGoingToExecute(InstrBuffer->at(line).rva+CodePtr->getBase())&&CodePtr->currentEIP() == InstrBuffer->at(line).rva);


            }else if(destRVA > last_va){
                drawJump(line,viewableRows+6,jumpoffset,cond,DbgIsJumpGoingToExecute(InstrBuffer->at(line).rva+CodePtr->getBase())&&CodePtr->currentEIP() == InstrBuffer->at(line).rva);
            }

        }

        if(InstrBuffer->at(line).rva == CodePtr->currentEIP()){
            drawLabel(line,"EIP");
        }

    }


    //delete painter;
}

void CPUJumps::drawJump(int startLine,int endLine,int jumpoffset, bool conditional, bool isexecute){
    painter->save();
    if(!conditional){
        painter->setPen(QPen(QColor("#000000"),1, Qt::SolidLine));  // jmp
    }else{
        painter->setPen(QPen(QColor("#000000"),1, Qt::DashLine));
    }
    QPen tmp = painter->pen();
    if(isexecute){
            tmp.setWidth(2);
            //tmp.setColor(Qt::red);
    }
    painter->setPen(tmp);

    const int JumpPadding = 15;

    const int x = viewport()->width()-jumpoffset*JumpPadding - 12;
    const int x_right = viewport()->width()- 12;
    const int y_start =  fontHeight*(1+startLine)-0.5*fontHeight;
    const int y_end =  fontHeight*(1+endLine)-0.5*fontHeight;

    // vertical
    painter->drawLine(x,y_start,x,y_end);
    // start horizontal
    painter->drawLine(x,y_start,x_right,y_start);
    painter->drawLine(x,y_end,x_right,y_end);

    const int ArrowSizeX = 2;  // width  of arrow tip in pixel
    const int ArrowSizeY = 3;  // height of arrow tip in pixel


    tmp = painter->pen();
    tmp.setStyle(Qt::SolidLine);
    tmp.setWidth(2);
    painter->setPen(tmp);
    painter->drawLine(x_right-ArrowSizeX,y_end-ArrowSizeY,x_right,y_end);
    painter->drawLine(x_right-ArrowSizeX,y_end+ArrowSizeY,x_right,y_end);


    painter->restore();
}

void CPUJumps::drawBullets(int line, bool isbp){


    painter->save();

    if( isbp)
        painter->setBrush(QBrush("#808080"));
    else
        painter->setBrush(QBrush(Qt::red));

    const int y = fontHeight*(1+line)-0.8*fontHeight ;
    const int x = viewport()->width() - 10;
    const int radius = 8;

    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(QPen("#ffffff"));
    painter->drawEllipse(x,y,radius,radius);



    painter->restore();
}




void CPUJumps::drawLabel(int Line, QString Text){

    painter->save();
    const int LineCoordinate = fontHeight*(1+Line);
    int length = Text.length();

    painter->setBrush(QBrush(QColor("#4040ff")));
    painter->setPen(QPen(QColor("#4040ff")));
    int y = LineCoordinate-fontHeight;

    painter->drawRect(1,y,length*fontWidth,fontHeight);
    painter->setPen(QPen(QColor("#ffffff")));
    painter->drawText(2,LineCoordinate-0.2*fontHeight,Text);

    y = fontHeight*(1+Line)-0.5*fontHeight;
    painter->setPen(QPen(QColor("#4040ff"),2));
    painter->setBrush(QBrush(QColor("#4040ff")));

    drawStraightArrow(painter,length*fontWidth,y,this->viewport()->width()-2-15,y);
    painter->restore();

}

void CPUJumps::drawStraightArrow(QPainter *painter, int x1, int y1, int x2, int y2)
{


    painter->drawLine(x1,y1,x2,y2);



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


    painter->drawLine(x2,y2,x2-ArrowSizeX,y2-ArrowSizeY);
    painter->drawLine(x2,y2,x2-ArrowSizeX,y2+ArrowSizeY);

}

