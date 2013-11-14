#ifndef BEAHIGHLIGHT_H
#define BEAHIGHLIGHT_H

#include "QBeaEngine.h"

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

class BeaHighlight
{
public:
    BeaHighlight();
    static void PrintRtfInstruction(QList<CustomRichText_t>* richText, const DISASM* MyDisasm);
private:
    static SEGMENTREG ConvertBeaSeg(int beaSeg);
    static void PrintBaseInstruction(QList<CustomRichText_t>* richText, const DISASM* MyDisasm);
    static bool PrintArgument(QList<CustomRichText_t>* richText, const ARGTYPE* Argument, const INSTRTYPE* Instruction, bool* had_arg);

};

#endif // BEAHIGHLIGHT_H
