#include "RichTextPainter.h"

//TODO: sometimes this function takes 15/16ms, it is not clear to me why this is (no noticable performance impact)
void RichTextPainter::paintRichText(QPainter* painter, int x, int y, int w, int h, int xinc, const QList<RichTextPainter::CustomRichText_t>* richText, int charwidth)
{
    int len = richText->size();
    for(int i = 0; i < len; i++)
    {
        CustomRichText_t curRichText = richText->at(i);
        int curRichTextLength = curRichText.text.length();
        int backgroundWidth = charwidth * curRichTextLength;
        if(backgroundWidth + xinc > w)
            backgroundWidth = w - xinc;
        if(backgroundWidth <= 0) //stop drawing when going outside the specified width
            break;
        switch(curRichText.flags)
        {
        case FlagNone: //defaults
            painter->drawText(QRect(x + xinc, y, w - xinc, h), 0, curRichText.text);
            break;
        case FlagColor: //color only
            painter->setPen(QPen(curRichText.textColor));
            painter->drawText(QRect(x + xinc, y, w - xinc, h), 0, curRichText.text);
            break;
        case FlagBackground: //background only
            if(backgroundWidth > 0)
                painter->fillRect(QRect(x + xinc, y, backgroundWidth, h), QBrush(curRichText.textBackground));
            painter->drawText(QRect(x + xinc, y, w - xinc, h), 0, curRichText.text);
            break;
        case FlagAll: //color+background
            if(backgroundWidth > 0)
                painter->fillRect(QRect(x + xinc, y, backgroundWidth, h), QBrush(curRichText.textBackground));
            painter->setPen(QPen(curRichText.textColor));
            painter->drawText(QRect(x + xinc, y, w - xinc, h), 0, curRichText.text);
            break;
        }
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
