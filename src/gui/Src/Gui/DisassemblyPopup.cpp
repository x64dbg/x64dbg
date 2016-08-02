#include "DisassemblyPopup.h"
#include "Disassembly.h"
#include "Configuration.h"
#include "StringUtil.h"
#include <QPainter>

DisassemblyPopup::DisassemblyPopup(Disassembly* parent) :
    QFrame(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus),
    parent(parent),
    mFontMetrics(nullptr)
{
    addr = 0;
    addrText = nullptr;
    connect(Config(), SIGNAL(fontsUpdated()), this, SLOT(updateFont()));
    connect(Config(), SIGNAL(colorsUpdated()), this, SLOT(updateColors()));
    updateFont();
    updateColors();
    setFrameStyle(QFrame::Panel);
    mMaxInstructions = 20;
}

DisassemblyPopup::~DisassemblyPopup()
{

}

void DisassemblyPopup::updateColors()
{
    disassemblyBackgroundColor = ConfigColor("DisassemblyBackgroundColor");
    labelColor = ConfigColor("DisassemblyLabelColor");
    labelBackgroundColor = ConfigColor("DisassemblyLabelBackgroundColor");
    commentColor = ConfigColor("DisassemblyCommentColor");
    commentBackgroundColor = ConfigColor("DisassemblyCommentBackgroundColor");
    commentAutoColor = ConfigColor("DisassemblyAutoCommentColor");
    commentAutoBackgroundColor = ConfigColor("DisassemblyAutoCommentBackgroundColor");
}

void DisassemblyPopup::updateFont()
{
    delete mFontMetrics;
    setFont(ConfigFont("Disassembly"));
    QFontMetricsF metrics(font());
    mFontMetrics = new CachedFontMetrics(this, font());
    // Update font size, used in layout calculations.
    charWidth = mFontMetrics->width('W');
    charHeight = metrics.height();
}

void DisassemblyPopup::paintEvent(QPaintEvent* event)
{
    QRect viewportRect(0, 0, width(), height());
    QPainter p(this);
    p.setFont(font());

    // Render background
    p.fillRect(viewportRect, disassemblyBackgroundColor);
    // Draw Address
    p.setPen(QPen(labelColor));
    int addrWidth = mFontMetrics->width(addrText);
    p.fillRect(2, 1, addrWidth, charHeight, QBrush(labelBackgroundColor));
    p.drawText(2, 1, addrWidth, charHeight, 0, addrText);
    // Draw Comments
    if(!addrComment.isEmpty())
    {
        int commentWidth = mFontMetrics->width(addrComment);
        QBrush background = QBrush(addrCommentAuto ? commentAutoBackgroundColor : commentBackgroundColor);
        p.setPen(addrCommentAuto ? commentAutoColor : commentColor);
        p.fillRect(2 + addrWidth, 1, commentWidth, charHeight, background);
        p.drawText(2 + addrWidth, 1, commentWidth, charHeight, 0, addrComment);
    }
    // Draw Instructions
    int y = charHeight + 1;
    for(auto & instruction : mDisassemblyToken)
    {
        RichTextPainter::paintRichText(&p, 2, y, mWidth - 2, charHeight, 0, instruction, mFontMetrics);
        y += charHeight;
    }
    QFrame::paintEvent(event);
}

void DisassemblyPopup::setAddress(duint Address)
{
    addr = Address;
    mInstBuffer.clear();
    mDisassemblyToken.clear();
    if(addr != 0)
    {
        mWidth = 1;
        // Get RVA
        dsint rva = Address - parent->getBase();
        // Prepare RVA of every instruction
        unsigned int i = 0;
        mInstBuffer.clear();
        QList<dsint> rvaList;
        dsint nextRva = rva;
        do
        {
            dsint nextRva2;
            rvaList.append(nextRva);
            nextRva2 = parent->getNextInstructionRVA(nextRva, 1, true);
            if(nextRva2 == nextRva)
                break;
            else
                nextRva = nextRva2;
            i++;
        }
        while(i < mMaxInstructions);
        // Disassemble
        parent->prepareDataCount(rvaList, &mInstBuffer);
        for(auto & instruction : mInstBuffer)
        {
            RichTextPainter::List richText;
            CapstoneTokenizer::TokenToRichText(instruction.tokens, richText, nullptr);
            // Calculate width
            int currentInstructionWidth = 0;
            for(auto & token : richText)
                currentInstructionWidth += mFontMetrics->width(token.text);
            mWidth = std::max(mWidth, currentInstructionWidth);
            mDisassemblyToken.push_back(std::move(richText));
        }
        // Address
        addrText = parent->getAddrText(addr, nullptr);
        // Comments
        GetCommentFormat(addr, addrComment, &addrCommentAuto);
        // Calculate width of address
        mWidth = std::max(mWidth, mFontMetrics->width(addrText) + mFontMetrics->width(addrComment));
        mWidth += charWidth * 6;
        // Resize popup
        resize(mWidth + 2, charHeight * (mDisassemblyToken.size() + 1) + 2);
    }
    update();
}

duint DisassemblyPopup::getAddress()
{
    return addr;
}

void DisassemblyPopup::hide()
{
    addr = 0;
    QFrame::hide();
}
