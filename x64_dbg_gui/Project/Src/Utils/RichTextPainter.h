#ifndef RICHTEXTPAINTER_H
#define RICHTEXTPAINTER_H

#include <QString>
#include <QColor>
#include <QPainter>

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

    typedef struct _CustomRichText_t
    {
        QString text;
        QColor textColor;
        QColor textBackground;
        CustomRichTextFlags flags;
        bool highlight;
        QColor highlightColor;
    } CustomRichText_t;

    //functions
    static void paintRichText(QPainter* painter, int x, int y, int w, int h, int xinc, const QList<RichTextPainter::CustomRichText_t>* richText, int charwidth);
};

#endif // RICHTEXTPAINTER_H
