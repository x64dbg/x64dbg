#include "DisassemblyPopup.h"
#include "CachedFontMetrics.h"
#include "Configuration.h"
#include "StringUtil.h"
#include "MiscUtil.h"
#include <QPainter>
#include <QStyleOptionFrame>
#include "Bridge.h"

DisassemblyPopup::DisassemblyPopup(AbstractTableView* parent, Architecture* architecture) :
    QFrame(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus),
    mFontMetrics(nullptr),
    mDisasm(ConfigUint("Disassembler", "MaxModuleSize"), architecture),
    mParent(parent)
{
    parent->installEventFilter(this);
    parent->setMouseTracking(true);
    connect(parent->verticalScrollBar(), SIGNAL(actionTriggered(int)), this, SLOT(hide()));
    connect(parent->horizontalScrollBar(), SIGNAL(actionTriggered(int)), this, SLOT(hide()));

    connect(Config(), SIGNAL(fontsUpdated()), this, SLOT(updateFont()));
    connect(Config(), SIGNAL(colorsUpdated()), this, SLOT(updateColors()));
    connect(Config(), SIGNAL(tokenizerConfigUpdated()), this, SLOT(tokenizerConfigUpdated()));
    updateFont();
    updateColors();
    setFrameStyle(QFrame::Panel);
    setLineWidth(2);
}

void DisassemblyPopup::updateColors()
{
    mDisassemblyBackgroundColor = ConfigColor("DisassemblyBackgroundColor");
    mDisassemblyTracedColor = ConfigColor("DisassemblyTracedBackgroundColor");
    mLabelColor = ConfigColor("DisassemblyLabelColor");
    mLabelBackgroundColor = ConfigColor("DisassemblyLabelBackgroundColor");
    mCommentColor = ConfigColor("DisassemblyCommentColor");
    mCommentBackgroundColor = ConfigColor("DisassemblyCommentBackgroundColor");
    mCommentAutoColor = ConfigColor("DisassemblyAutoCommentColor");
    mCommentAutoBackgroundColor = ConfigColor("DisassemblyAutoCommentBackgroundColor");
}

void DisassemblyPopup::tokenizerConfigUpdated()
{
    mDisasm.UpdateConfig();
}

void DisassemblyPopup::updateFont()
{
    delete mFontMetrics;
    setFont(ConfigFont("Disassembly"));
    QFontMetricsF metrics(font());
    mFontMetrics = new CachedFontMetrics(this, font());
    // Update font size, used in layout calculations.
    mCharWidth = mFontMetrics->width('W');
    mCharHeight = metrics.height();
}

void DisassemblyPopup::paintEvent(QPaintEvent* event)
{
    QRect viewportRect(0, 0, width(), height());
    QPainter p(this);
    p.setFont(font());

    // Render background
    p.fillRect(viewportRect, mDisassemblyBackgroundColor);
    // Draw Address
    p.setPen(QPen(mLabelColor));
    int addrWidth = mFontMetrics->width(mAddrText);
    p.fillRect(3, 2 + lineWidth(), addrWidth, mCharHeight, QBrush(mLabelBackgroundColor));
    p.drawText(3, 2, addrWidth, mCharHeight, 0, mAddrText);
    // Draw Comments
    if(!mAddrComment.isEmpty())
    {
        int commentWidth = mFontMetrics->width(mAddrComment);
        QBrush background = QBrush(mAddrCommentAuto ? mCommentAutoBackgroundColor : mCommentBackgroundColor);
        p.setPen(mAddrCommentAuto ? mCommentAutoColor : mCommentColor);
        p.fillRect(3 + addrWidth, 2, commentWidth, mCharHeight, background);
        p.drawText(3 + addrWidth, 2, commentWidth, mCharHeight, 0, mAddrComment);
    }
    // Draw Instructions
    int y = mCharHeight + 1;
    for(auto & instruction : mDisassemblyToken)
    {
        if(instruction.second)
            p.fillRect(QRect(3, y, mWidth - 3, mCharHeight), mDisassemblyTracedColor);
        RichTextPainter::paintRichText(&p, 3, y, mWidth - 3, mCharHeight, 0, instruction.first, mFontMetrics);
        y += mCharHeight;
    }

    // The code above will destroy the stylesheet adjustments, making it impossible to change the border color.
    // To remedy this, we redraw a thin 'stylizable' frame here
    QStyleOptionFrame opt;
    initStyleOption(&opt);
    style()->drawPrimitive(QStyle::PE_Frame, &opt, &p, this);

    QFrame::paintEvent(event);
}

bool DisassemblyPopup::eventFilter(QObject* object, QEvent* event)
{
    if(object == parent())
    {
        switch(event->type())
        {
        case QEvent::Leave:
        case QEvent::Hide:
        case QEvent::KeyPress:
        {
            hide();
            stopPopupTimer();
        }
        break;

        case QEvent::MouseMove:
        {
            auto mouseEvent = (QMouseEvent*)event;
            auto x = mouseEvent->x();
            auto y = mouseEvent->y();

            // TODO: make sure the cursor isn't on the column separators
            if(y > mParent->getHeaderHeight())
            {
                // Show the popup if relevant for the current position
                auto addr = mParent->getAddressForPosition(x, y);
                if(getAddress() != addr)
                {
                    if(DbgFunctions()->MemIsCodePage(addr, false))
                    {
                        move(mParent->mapToGlobal(QPoint(x + 20, y + fontMetrics().height() * 2)));
                        setAddress(addr);
                        if(mPopupTimer == 0)
                            mPopupTimer = startTimer(QApplication::startDragTime());
                    }
                    else
                    {
                        hide();
                        stopPopupTimer();
                    }
                }
            }
        }
        break;

        default:
            break;
        }
    }

    return QFrame::eventFilter(object, event);
}

void DisassemblyPopup::timerEvent(QTimerEvent* event)
{
    if(event->timerId() == mPopupTimer)
    {
        // Show the popup
        show();

        stopPopupTimer();
    }

    QFrame::timerEvent(event);
}

void DisassemblyPopup::stopPopupTimer()
{
    if(mPopupTimer != 0)
    {
        killTimer(mPopupTimer);
        mPopupTimer = 0;
    }
}

void DisassemblyPopup::setAddress(duint addr)
{
    mAddr = addr;
    QList<Instruction_t> instBuffer;
    mDisassemblyToken.clear();
    mDisasm.UpdateArchitecture();

    if(mAddr != 0)
    {
        mWidth = 1;
        // Get RVA
        duint size;
        duint base = DbgMemFindBaseAddr(addr, &size);

        // Prepare RVA of every instruction
        unsigned int i = 0;
        instBuffer.clear();
        auto nextAddr = addr;
        bool hadBranch = false;
        duint bestBranch = 0;
        uint8_t data[64];
        do
        {
            if(nextAddr >= base + size)
                break;
            if(!DbgMemRead(nextAddr, data, sizeof(data)))
                break;
            auto instruction = mDisasm.DisassembleAt(data, sizeof(data), 0, nextAddr);
            if(!instruction.length)
                break;
            instBuffer.append(instruction);
            if(!hadBranch || bestBranch <= nextAddr)
            {
                if(instruction.instStr.contains("ret"))
                    break;
                if(instruction.instStr.contains("jmp") && instruction.instStr.contains("["))
                    break;
            }
            if(instruction.branchDestination && !instruction.instStr.contains("call") && !instruction.instStr.contains("ret"))
            {
                hadBranch = true;
                if(instruction.branchDestination > bestBranch)
                    bestBranch = instruction.branchDestination;
            }
            auto nextAddr2 = nextAddr + instruction.length;
            if(nextAddr2 == nextAddr)
                break;
            else
                nextAddr = nextAddr2;
            if(DbgGetFunctionTypeAt(nextAddr - 1) == FUNC_END)
                break;
            i++;
        }
        while(i < mMaxInstructions);

        // Disassemble
        for(auto & instruction : instBuffer)
        {
            RichTextPainter::List richText;
            ZydisTokenizer::TokenToRichText(instruction.tokens, richText, nullptr);
            // Calculate width
            int currentInstructionWidth = 0;
            for(auto & token : richText)
                currentInstructionWidth += mFontMetrics->width(token.text);
            mWidth = std::max(mWidth, currentInstructionWidth);
            mDisassemblyToken.push_back(std::make_pair(std::move(richText), DbgFunctions()->GetTraceRecordHitCount(instruction.rva) != 0));
        }

        // Address
        mAddrText = getSymbolicName(addr);

        // Comments
        GetCommentFormat(addr, mAddrComment, &mAddrCommentAuto);
        if(mAddrComment.length())
            mAddrText.append(' ');

        // Truncate first line to something reasonable
        if(mAddrText.length() + mAddrComment.length() > 100)
            mAddrComment.clear();
        if(mAddrText.length() > 100)
            mAddrText = mAddrText.left(100) + " ...";

        // Calculate width of address
        mWidth = std::max(mWidth, mFontMetrics->width(mAddrText) + mFontMetrics->width(mAddrComment));

        mWidth += 3;

        // Resize popup
        resize(mWidth + 2, mCharHeight * int(mDisassemblyToken.size() + 1) + 4);
    }
    update();
}

duint DisassemblyPopup::getAddress() const
{
    return mAddr;
}

void DisassemblyPopup::hide()
{
    mAddr = 0;
    QFrame::hide();
}
