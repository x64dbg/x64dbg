#include "RichTextPainter.h"
#include "CachedFontMetrics.h"
#include <QPainter>

//TODO: fix performance (possibly use QTextLayout?)
void RichTextPainter::paintRichText(QPainter* painter, int x, int y, int w, int h, int xinc, const List & richText, CachedFontMetrics* fontMetrics)
{
    QPen pen;
    QPen highlightPen;
    highlightPen.setWidth(2);
    QBrush brush(Qt::cyan);
    for(const auto & curRichText : richText)
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
            if(backgroundWidth > 0)
            {
                brush.setColor(curRichText.textBackground);
                painter->fillRect(QRect(x + xinc, y, backgroundWidth, h), brush);
            }
            break;
        case FlagAll: //color+background
            if(backgroundWidth > 0)
            {
                brush.setColor(curRichText.textBackground);
                painter->fillRect(QRect(x + xinc, y, backgroundWidth, h), brush);
            }
            pen.setColor(curRichText.textColor);
            painter->setPen(pen);
            break;
        }
        painter->drawText(QRect(x + xinc, y, w - xinc, h), Qt::TextBypassShaping, curRichText.text);
        if(curRichText.highlight)
        {
            highlightPen.setColor(curRichText.highlightColor);
            painter->setPen(highlightPen);
            painter->drawLine(x + xinc + 1, y + h - 1, x + xinc + backgroundWidth - 1, y + h - 1);
        }
        xinc += textWidth;
    }
}
