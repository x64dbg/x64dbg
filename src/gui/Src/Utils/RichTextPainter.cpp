#include "RichTextPainter.h"

//TODO: sometimes this function takes 15/16ms, it is not clear to me why this is (no noticable performance impact)
//TODO: change QList to std::vector (QList.append has bad performance)
void RichTextPainter::paintRichText(QPainter* painter, int x, int y, int w, int h, int xinc, const QList<RichTextPainter::CustomRichText_t>* richText, int charwidth)
{
    int len = richText->size();
    QPen pen(Qt::black);
    for(int i = 0; i < len; i++)
    {
        const CustomRichText_t & curRichText = richText->at(i);
        int curRichTextLength = curRichText.text.length();
        int backgroundWidth = charwidth * curRichTextLength;
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
                painter->fillRect(QRect(x + xinc, y, backgroundWidth, h), QBrush(curRichText.textBackground));
            }
            break;
        case FlagAll: //color+background
            if(backgroundWidth > 0)
            {
                painter->fillRect(QRect(x + xinc, y, backgroundWidth, h), QBrush(curRichText.textBackground));
            }
            pen.setColor(curRichText.textColor);
            painter->setPen(pen);
            break;
        }
        painter->drawText(QRect(x + xinc, y, w - xinc, h), 0, curRichText.text); //TODO: this bottlenecks
        if(curRichText.highlight)
        {
            QPen pen(curRichText.highlightColor);
            pen.setWidth(2);
            painter->setPen(pen);
            painter->drawLine(x + xinc + 1, y + h - 1, x + xinc + backgroundWidth - 1, y + h - 1);
        }
        xinc += charwidth * curRichTextLength;
    }
}
