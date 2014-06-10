#ifndef RICHTEXTPAINTER_H
#define RICHTEXTPAINTER_H

#include <QList>
#include <QPainter>

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
} CustomRichText_t;

class RichTextPainter
{
public:
    //functions
    static void paintRichText(QPainter* painter, int x, int y, int w, int h, int xinc, const QList<CustomRichText_t>* richText, int charwidth);
};

#endif // RICHTEXTPAINTER_H
