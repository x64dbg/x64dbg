#ifndef RICHTEXTPAINTER_H
#define RICHTEXTPAINTER_H

#include <QString>
#include <QColor>
#include <vector>

class CachedFontMetrics;
class QPainter;

class RichTextPainter
{
public:
    //structures
    enum CustomRichTextFlags
    {
        FlagNone,
        FlagColor,
        FlagBackground,
        FlagAll
    };

    struct CustomRichText_t
    {
        QString text;
        QColor textColor;
        QColor textBackground;
        CustomRichTextFlags flags;
        bool highlight;
        QColor highlightColor;
        int highlightWidth = 2;
        bool highlightConnectPrev = false;

        bool operator==(const CustomRichText_t & o) const
        {
            return text == o.text
                   && textColor == o.textColor
                   && textBackground == o.textBackground
                   && flags == o.flags
                   && highlight == o.highlight
                   && highlightColor == o.highlightColor
                   && highlightWidth == o.highlightWidth
                   && highlightConnectPrev == o.highlightConnectPrev;
        }
    };

    typedef std::vector<CustomRichText_t> List;

    //functions
    static void paintRichText(QPainter* painter, int x, int y, int w, int h, int xinc, const List & richText, CachedFontMetrics* fontMetrics);
    static void htmlRichText(const List & richText, QString & textHtml, QString & textPlain);
};

#endif // RICHTEXTPAINTER_H
