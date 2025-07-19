#include "PatternHighlighter.h"

PatternHighlighter::PatternHighlighter(const CodeEditor* style, QTextDocument* parent)
    : QSyntaxHighlighter(parent)
{
    refreshColors(style);
}

static const char* keywords[] =
{
    "struct",
    "union",
    "enum",
    "bitfield",
    "float",
    "double",
    "char",
    "char16",
    "bool",
    "str",
    "auto",
    "namespace",
    "using",
    "fn",
    "if",
    "else",
    "match",
    "break",
    "continue",
    "try",
    "catch",
    "return",
    "in",
    "out",
    "static"
};

static const char* instructions[] =
{
    "addressof",
    "sizeof",
    "import",
};

struct Format
{
    Format(QTextCharFormat & format)
        : format(format)
    {
    }

    Format && self()
    {
        return std::move(*this);
    }

    Format && italic()
    {
        format.setFontItalic(true);
        return self();
    }

    Format && bold()
    {
        format.setFontWeight(QFont::Bold);
        return self();
    }

    Format && color(const QColor & color)
    {
        format.setForeground(color);
        return self();
    }

    Format && color(const QColorWrapper & color)
    {
        format.setForeground(color());
        return self();
    }

private:
    QTextCharFormat & format;
};

void PatternHighlighter::refreshColors(const CodeEditor* style)
{
    highlightingRules.clear();

    auto addRule = [this](const char* regex)
    {
        highlightingRules.append({ QRegularExpression(regex), QTextCharFormat() });
        if(!highlightingRules.back().pattern.isValid())
        {
            qFatal("Invalid regular expression");
        }
        return Format(highlightingRules.back().format);
    };

    // Rules added later can override previous rules (since they are applied in order)

    // operators
    addRule(R"regex([+\-=^|&%*/<>]+)regex")
    .color(style->operatorColor);
    addRule(R"regex([\$\?:@~!])regex")
    .bold()
    .color(style->operatorColor);

    // constants: 12, 0x33
    addRule(R"regex(\b\d+\b)regex")
    .color(style->constantColor);
    addRule(R"regex(\b0x[0-9A-Fa-f]+\b)regex")
    .color(style->constantColor);
    addRule(R"regex(\b(true|false)\b)regex")
    .color(style->constantColor);

    // Keywords
    {
        QTextCharFormat keywordFormat;
        keywordFormat.setForeground(style->keywordColor());

        for(const QString & pattern : keywords)
            highlightingRules.append({ QRegularExpression(QString("\\b%1\\b").arg(pattern)), keywordFormat });
    }
    addRule(R"regex(|\[\[|\]\])regex")
    .bold()
    .color(style->keywordColor);

    // Instructions
    {
        QTextCharFormat instructionFormat;
        instructionFormat.setForeground(style->instructionColor());
        instructionFormat.setFontWeight(QFont::Bold);

        for(const QString & pattern : instructions)
        {
            auto escapedRegex = QRegularExpression::escape(pattern);
            highlightingRules.append({ QRegularExpression(QString("\\b%1\\b").arg(escapedRegex)), instructionFormat });
        }

    }

    // preprocessor definitions
    addRule(R"regex(\w*#(pragma|define|include|ifdef|ifndef|endif|error).+$)regex")
    .bold()
    .color(style->instructionColor);

    // function(
    addRule(R"regex(\b[-a-zA-Z$._:][-a-zA-Z$._:0-9]*\w*(?=\())regex")
    .bold()
    .color(style->functionColor);

    // string literal
    addRule(R"regex("(?:\\.|[^"\\])*")regex")
    .color(style->stringColor);

    // i64 and other types
    addRule(R"regex(\b[us]\d+\b)regex")
    .color(style->integerTypeColor);

    // Line comments
    addRule(R"regex(//.*$)regex")
    .color(style->commentColor);

    // Block comments
    addRule(R"regex(\/\*[\s\S]*?\*\/)regex")
    .color(style->commentColor);
}

void PatternHighlighter::highlightBlock(const QString & text)
{
    for(int i = 0; i < highlightingRules.size(); i++)
    {
        const HighlightingRule & rule = highlightingRules[i];
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while(matchIterator.hasNext())
        {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
    setCurrentBlockState(0);

    int startIndex = 0;
    if(previousBlockState() != 1)
        startIndex = text.indexOf(commentStartExpression);

    while(startIndex >= 0)
    {
        QRegularExpressionMatch match = commentEndExpression.match(text, startIndex);
        int endIndex = match.capturedStart();
        int commentLength = 0;
        if(endIndex == -1)
        {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        }
        else
        {
            commentLength = endIndex - startIndex + match.capturedLength();
        }
        setFormat(startIndex, commentLength, multiLineCommentFormat);
        startIndex = text.indexOf(commentStartExpression, startIndex + commentLength);
    }
}
