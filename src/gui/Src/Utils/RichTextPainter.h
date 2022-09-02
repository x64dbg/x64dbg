#pragma once

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
        bool underline = false;
        QColor underlineColor;
        int underlineWidth = 2;
        bool underlineConnectPrev = false;
    };
    static_assert(std::is_move_assignable<CustomRichText_t>::value, "not movable");

    typedef std::vector<CustomRichText_t> List;

    //functions
    static void paintRichText(QPainter* painter, int x, int y, int w, int h, int xinc, const List & richText, CachedFontMetrics* fontMetrics);
    static void htmlRichText(const List & richText, QString* textHtml, QString & textPlain);
};
