#ifndef BEAHIGHLIGHT_H
#define BEAHIGHLIGHT_H

#include "QBeaEngine.h"
#include "RichTextPainter.h"

class BeaHighlight : RichTextPainter
{
public:
    static void PrintRtfInstruction(QList<RichTextPainter::CustomRichText_t>* richText, const DISASM* MyDisasm);
private:
    static SEGMENTREG ConvertBeaSeg(int beaSeg);
    static void PrintBaseInstruction(QList<RichTextPainter::CustomRichText_t>* richText, const DISASM* MyDisasm);
    static bool PrintArgument(QList<RichTextPainter::CustomRichText_t>* richText, const ARGTYPE* Argument, const INSTRTYPE* Instruction, bool* had_arg);

};

#endif // BEAHIGHLIGHT_H
