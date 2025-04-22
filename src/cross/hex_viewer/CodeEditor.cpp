/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "CodeEditor.h"

#include <QPainter>
#include <QTextBlock>
#include <QRegularExpression>
#include <QDebug>
#include <QAction>

CodeEditor::CodeEditor(QWidget* parent)
    : QPlainTextEdit(parent)
{
    setWordWrapMode(QTextOption::NoWrap);
    lineNumberArea = new LineNumberArea(this);

    connect(this, &CodeEditor::blockCountChanged, this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &CodeEditor::updateRequest, this, &CodeEditor::updateLineNumberArea);
    connect(this, &CodeEditor::cursorPositionChanged, this, &CodeEditor::highlightCurrentLine);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

int CodeEditor::lineNumberAreaWidth()
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10)
    {
        max /= 10;
        ++digits;
    }

    return 3 + measureFontWidth("7") * digits;
}

void CodeEditor::setErrorLine(int line)
{
    mErrorLine = line;
    highlightCurrentLine();
}

void CodeEditor::setTokenHighlights(const QString& token, const QList<int>& lines)
{
    mHighlightToken = token;
    mHighlightLines = lines;
    highlightCurrentLine(); // TODO: this will now be done twice
}

void CodeEditor::setHackedReadonly(bool readonly)
{
    mHackedReadonly = readonly;
}

int CodeEditor::measureFontWidth(const QString& str)
{
    auto metrics = fontMetrics();
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    return metrics.horizontalAdvance(str);
#else
    return metrics.width(str);
#endif
}

void CodeEditor::updateLineNumberAreaWidth(int /* newBlockCount */)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect& rect, int dy)
{
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void CodeEditor::resizeEvent(QResizeEvent* e)
{
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::changeEvent(QEvent* event)
{
    if(event->type() == QEvent::StyleChange)
    {
        highlightCurrentLine();
    }
    else if(event->type() == QEvent::FontChange)
    {
        setTabStopDistance(measureFontWidth("    "));
    }
    QPlainTextEdit::changeEvent(event);
}

void CodeEditor::keyPressEvent(QKeyEvent* event)
{
    // Shift+Enter does something sus
    if((event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) && event->modifiers() == Qt::ShiftModifier)
    {
        return;
    }

    if(mHackedReadonly)
    {
        // https://doc.qt.io/qt-6/qplaintextedit.html#read-only-key-bindings
        switch(event->key())
        {
        case Qt::Key_Copy:
        case Qt::Key_Left:
        case Qt::Key_Right:
        case Qt::Key_Up:
        case Qt::Key_Down:
        case Qt::Key_PageUp:
        case Qt::Key_PageDown:
        case Qt::Key_Home:
        case Qt::Key_End:
        case Qt::Key_Shift:
        case Qt::Key_Alt:
        case Qt::Key_Control:
        case Qt::Key_Meta:
            break;
        case Qt::Key_A:
            // Select all
            if(event->modifiers() & (Qt::ControlModifier | Qt::MetaModifier))
                break;
        case Qt::Key_C:
            // Copy
            if (event->modifiers() & (Qt::ControlModifier | Qt::MetaModifier))
                break;
        default:
        {
            QKeySequence seq(event->modifiers() | event->key());
            Q_FOREACH(auto action, actions())
            {
                auto shortcut = action->shortcut();
                if(!shortcut.isEmpty() && shortcut == seq)
                {
                    action->trigger();
                    event->accept();
                    break;
                }
            }
            return;
        }
        }
    }
    QPlainTextEdit::keyPressEvent(event);
}

void CodeEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!mHackedReadonly /* isReadOnly() */)
    {
        QTextEdit::ExtraSelection selection;
        selection.format.setBackground(selectedLineHighlightColor());
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    if (mErrorLine != -1)
    {
        QTextEdit::ExtraSelection selection;
        selection.format.setBackground(errorLineHighlightColor());
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        selection.cursor.setPosition(document()->findBlockByLineNumber(mErrorLine - 1).position());
        extraSelections.append(selection);
    }

    if(!mHighlightToken.isEmpty())
    {
        QTextCursor cursor = textCursor();

        if(mHighlightLines.empty())
        {
            // Global selection
            auto plainText = toPlainText();

            QStringMatcher matcher(mHighlightToken);
            for(auto from = 0; ;)
            {
                auto index = matcher.indexIn(plainText, from);
                if(index == -1)
                    break;

                QTextEdit::ExtraSelection currentWord;
                currentWord.format.setFontUnderline(true);
                cursor.setPosition(index, QTextCursor::MoveAnchor);
                cursor.setPosition(index + mHighlightToken.length(), QTextCursor::KeepAnchor);
                currentWord.cursor = cursor;
                extraSelections.append(currentWord);

                from = index + mHighlightToken.length();
            }
        }
    }

    setExtraSelections(extraSelections);
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent* event)
{
    QPainter painter(lineNumberArea);
    painter.setFont(font());

    painter.fillRect(event->rect(), lineNumberBackgroundColor());

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom())
    {
        if (block.isVisible() && bottom >= event->rect().top())
        {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(lineNumberColor());
            painter.drawText(0, top, lineNumberArea->width(), fontMetrics().height(), Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}
