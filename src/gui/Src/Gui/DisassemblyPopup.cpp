#include "DisassemblyPopup.h"
#include "Disassembly.h"
#include "CachedFontMetrics.h"
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
    setLineWidth(2);
    mMaxInstructions = 20;
}

DisassemblyPopup::~DisassemblyPopup()
{

}

void DisassemblyPopup::updateColors()
{
    disassemblyBackgroundColor = ConfigColor("DisassemblyBackgroundColor");
    disassemblyTracedColor = ConfigColor("DisassemblyTracedBackgroundColor");
    labelColor = ConfigColor("DisassemblyLabelColor");
    labelBackgroundColor = ConfigColor("DisassemblyLabelBackgroundColor");
    commentColor = ConfigColor("DisassemblyCommentColor");
    commentBackgroundColor = ConfigColor("DisassemblyCommentBackgroundColor");
    commentAutoColor = ConfigColor("DisassemblyAutoCommentColor");
    commentAutoBackgroundColor = ConfigColor("DisassemblyAutoCommentBackgroundColor");
    QPalette palette;
    palette.setColor(QPalette::Foreground, ConfigColor("AbstractTableViewSeparatorColor"));
    setPalette(palette);
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
    p.fillRect(3, 2 + lineWidth(), addrWidth, charHeight, QBrush(labelBackgroundColor));
    p.drawText(3, 2, addrWidth, charHeight, 0, addrText);
    // Draw Comments
    if(!addrComment.isEmpty())
    {
        int commentWidth = mFontMetrics->width(addrComment);
        QBrush background = QBrush(addrCommentAuto ? commentAutoBackgroundColor : commentBackgroundColor);
        p.setPen(addrCommentAuto ? commentAutoColor : commentColor);
        p.fillRect(3 + addrWidth, 2, commentWidth, charHeight, background);
        p.drawText(3 + addrWidth, 2, commentWidth, charHeight, 0, addrComment);
    }
    // Draw Instructions
    int y = charHeight + 1;
    for(auto & instruction : mDisassemblyToken)
    {
        if(instruction.second)
            p.fillRect(QRect(3, y, mWidth - 3, charHeight), disassemblyTracedColor);
        RichTextPainter::paintRichText(&p, 3, y, mWidth - 3, charHeight, 0, instruction.first, mFontMetrics);
        y += charHeight;
    }
    QFrame::paintEvent(event);
}

void DisassemblyPopup::setAddress(duint Address)
{
    addr = Address;
    QList<Instruction_t> mInstBuffer;
    mDisassemblyToken.clear();
    if(addr != 0)
    {
        mWidth = 1;
        // Get RVA
        duint base = parent->getBase();
        dsint rva = Address - base;
        // Prepare RVA of every instruction
        unsigned int i = 0;
        mInstBuffer.clear();
        QList<dsint> rvaList;
        dsint nextRva = rva;
        bool hadBranch = false;
        do
        {
            rvaList.append(nextRva);
            Instruction_t instruction = parent->DisassembleAt(nextRva);
            if(!hadBranch && instruction.tokens.tokens[0].text.toLower() == "ret")
                break;
            if(instruction.branchDestination)
                hadBranch = true;
            auto nextRva2 = nextRva + instruction.length;
            if(nextRva2 == nextRva)
                break;
            else
                nextRva = nextRva2;
            if(DbgGetFunctionTypeAt(nextRva + base - 1) == FUNC_END)
                break;
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
            mDisassemblyToken.push_back(std::make_pair(std::move(richText), DbgFunctions()->GetTraceRecordHitCount(instruction.rva + base) != 0));
        }
        // Address
        addrText = parent->getAddrText(addr, nullptr);
        // Comments
        GetCommentFormat(addr, addrComment, &addrCommentAuto);
        if(addrComment.length())
            addrText.append(' ');
        // Calculate width of address
        mWidth = std::max(mWidth, mFontMetrics->width(addrText) + mFontMetrics->width(addrComment));
        mWidth += charWidth * 6;
        // Resize popup
        resize(mWidth + 2, charHeight * int(mDisassemblyToken.size() + 1) + 2);
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
