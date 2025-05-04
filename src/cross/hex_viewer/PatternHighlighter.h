#pragma once

#include <QObject>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>
#include "CodeEditor.h"

QT_BEGIN_NAMESPACE
class QTextDocument;
QT_END_NAMESPACE

class PatternHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    PatternHighlighter(const CodeEditor* style, QTextDocument* parent = 0);
    void refreshColors(const CodeEditor* style);

protected:
    void highlightBlock(const QString & text) override;

private:
    struct HighlightingRule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRules;

    QTextCharFormat multiLineCommentFormat;
    QRegularExpression commentStartExpression = QRegularExpression(QStringLiteral("/\\*"));
    QRegularExpression commentEndExpression = QRegularExpression(QStringLiteral("\\*/"));
};
