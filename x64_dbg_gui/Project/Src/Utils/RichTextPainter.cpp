#include "RichTextPainter.h"

void RichTextPainter::paintRichText(QPainter* painter, int x, int y, int w, int h, int xinc, const QList<RichTextPainter::CustomRichText_t>* richText, int charwidth)
{
    int len=richText->size();
    for(int i=0; i<len; i++)
    {
        CustomRichText_t curRichText=richText->at(i);
        int curRichTextLength=curRichText.text.length();
        int backgroundWidth=charwidth*curRichTextLength;
        if(backgroundWidth+xinc>w)
            backgroundWidth=w-xinc;
        if(backgroundWidth<=0) //stop drawing when going outside the specified width
            break;
        switch(curRichText.flags)
        {
        case FlagNone: //defaults
            painter->drawText(QRect(x+xinc, y, w-xinc, h), 0, curRichText.text);
            break;
        case FlagColor: //color only
            painter->save();
            painter->setPen(QPen(curRichText.textColor));
            painter->drawText(QRect(x+xinc, y, w-xinc, h), 0, curRichText.text);
            painter->restore();
            break;
        case FlagBackground: //background only
            painter->save();
            if(backgroundWidth>0)
                painter->fillRect(QRect(x+xinc, y, backgroundWidth, h), QBrush(curRichText.textBackground));
            painter->drawText(QRect(x+xinc, y, w-xinc, h), 0, curRichText.text);
            painter->restore();
            break;
        case FlagAll: //color+background
            painter->save();
            if(backgroundWidth>0)
                painter->fillRect(QRect(x+xinc, y, backgroundWidth, h), QBrush(curRichText.textBackground));
            painter->setPen(QPen(curRichText.textColor));
            painter->drawText(QRect(x+xinc, y, w-xinc, h), 0, curRichText.text);
            painter->restore();
            break;
        }
        if(curRichText.highlight)
        {
            painter->save();
            painter->setPen(curRichText.highlightColor);
            painter->drawLine(x+xinc, y+h-1, x+xinc+backgroundWidth-2, y+h-1);
            painter->restore();
        }
        xinc+=charwidth*curRichTextLength;
    }
}
