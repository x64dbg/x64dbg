#include "RichTextPainter.h"
#include "CachedFontMetrics.h"
#include <QPainter>

//TODO: fix performance (possibly use QTextLayout?)
void RichTextPainter::paintRichText(QPainter* painter, int x, int y, int w, int h, int xinc, const List & richText, CachedFontMetrics* fontMetrics)
{
    QPen pen;
    QPen highlightPen;
    QBrush brush(Qt::cyan);
    for(const CustomRichText_t & curRichText : richText)
    {
        int textWidth = fontMetrics->width(curRichText.text);
        int backgroundWidth = textWidth;
        if(backgroundWidth + xinc > w)
            backgroundWidth = w - xinc;
        if(backgroundWidth <= 0) //stop drawing when going outside the specified width
            break;
        switch(curRichText.flags)
        {
        case FlagNone: //defaults
            break;
        case FlagColor: //color only
            pen.setColor(curRichText.textColor);
            painter->setPen(pen);
            break;
        case FlagBackground: //background only
            if(backgroundWidth > 0 && curRichText.textBackground.alpha())
            {
                brush.setColor(curRichText.textBackground);
                painter->fillRect(QRect(x + xinc, y, backgroundWidth, h), brush);
            }
            break;
        case FlagAll: //color+background
            if(backgroundWidth > 0 && curRichText.textBackground.alpha())
            {
                brush.setColor(curRichText.textBackground);
                painter->fillRect(QRect(x + xinc, y, backgroundWidth, h), brush);
            }
            pen.setColor(curRichText.textColor);
            painter->setPen(pen);
            break;
        }
        painter->drawText(QRect(x + xinc, y, w - xinc, h), Qt::TextBypassShaping, curRichText.text);
        if(curRichText.underline && curRichText.underlineColor.alpha())
        {
            highlightPen.setColor(curRichText.underlineColor);
            highlightPen.setWidth(curRichText.underlineWidth);
            painter->setPen(highlightPen);
            int highlightOffsetX = curRichText.underlineConnectPrev ? -1 : 1;
            painter->drawLine(x + xinc + highlightOffsetX, y + h - 1, x + xinc + backgroundWidth - 1, y + h - 1);
        }
        xinc += textWidth;
    }
}

/**
 * @brief RichTextPainter::htmlRichText Convert rich text in x64dbg to HTML, for use by other applications
 * @param richText The rich text to be converted to HTML format
 * @param textHtml The HTML source. Any previous content will be preserved and new content will be appended at the end.
 * @param textPlain The plain text. Any previous content will be preserved and new content will be appended at the end.
 */
void RichTextPainter::htmlRichText(const List & richText, QString & textHtml, QString & textPlain)
{
    for(const CustomRichText_t & curRichText : richText)
    {
        if(curRichText.text == " ") //blank
        {
            textHtml += " ";
            textPlain += " ";
            continue;
        }
        switch(curRichText.flags)
        {
        case FlagNone: //defaults
            textHtml += "<span>";
            break;
        case FlagColor: //color only
            textHtml += QString("<span style=\"color:%1\">").arg(curRichText.textColor.name());
            break;
        case FlagBackground: //background only
            if(curRichText.textBackground != Qt::transparent) // QColor::name() returns "#000000" for transparent color. That's not desired. Leave it blank.
                textHtml += QString("<span style=\"background-color:%1\">").arg(curRichText.textBackground.name());
            else
                textHtml += QString("<span>");
            break;
        case FlagAll: //color+background
            if(curRichText.textBackground != Qt::transparent) // QColor::name() returns "#000000" for transparent color. That's not desired. Leave it blank.
                textHtml += QString("<span style=\"color:%1; background-color:%2\">").arg(curRichText.textColor.name(), curRichText.textBackground.name());
            else
                textHtml += QString("<span style=\"color:%1\">").arg(curRichText.textColor.name());
            break;
        }
        if(curRichText.underline) //Underline highlighted token
            textHtml += "<u>";
        textHtml += curRichText.text.toHtmlEscaped();
        if(curRichText.underline)
            textHtml += "</u>";
        textHtml += "</span>"; //Close the tag
        textPlain += curRichText.text;
    }
}
