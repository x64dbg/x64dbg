#include "QHexEditPrivate.h"
#include <QApplication>
#include <QClipboard>
#include <QPainter>
#include "ArrayCommand.h"

const int HEXCHARS_IN_LINE = 47;
const int BYTES_PER_LINE = 16;

QHexEditPrivate::QHexEditPrivate(QScrollArea* parent) : QWidget(parent)
{
    _undoDataStack = new QUndoStack(this);
    _undoMaskStack = new QUndoStack(this);

    _scrollArea = parent;
    setOverwriteMode(true);

    QFont font("Monospace", 8, QFont::Normal, false);
    font.setFixedPitch(true);
    font.setStyleHint(QFont::Monospace);
    this->setFont(font);

    _size = 0;
    _horizonalSpacing = 3;
    resetSelection(0);
    setCursorPos(0);

    setFocusPolicy(Qt::StrongFocus);

    connect(&_cursorTimer, SIGNAL(timeout()), this, SLOT(updateCursor()));
    _cursorTimer.setInterval(500);
    _cursorTimer.start();

    _textColor = QColor("#000000");
    _wildcardColor = QColor("#FF0000");
    _backgroundColor = QColor("#FFF8F0");
    _selectionColor = QColor("#C0C0C0");
    _overwriteMode = false;
    _wildcardEnabled = true;
    _keepSize = false;
}

void QHexEditPrivate::setData(const QByteArray & data, const QByteArray & mask)
{
    _xData.setData(data);
    _xMask.setData(mask);
    _undoDataStack->clear();
    _undoMaskStack->clear();
    _initSize = _xData.size();
    adjust();
    setCursorPos(0);
    emit dataChanged();
}

QByteArray QHexEditPrivate::data()
{
    return _xData.data();
}

QByteArray QHexEditPrivate::mask()
{
    return _xMask.data();
}

void QHexEditPrivate::insert(int index, const QByteArray & ba, const QByteArray & mask)
{
    if(ba.length() > 0)
    {
        if(_overwriteMode)
        {
            _undoDataStack->push(new ArrayCommand(&_xData, ArrayCommand::replace, index, ba, ba.length()));
            _undoMaskStack->push(new ArrayCommand(&_xMask, ArrayCommand::replace, index, mask, mask.length()));
        }
        else
        {
            _undoDataStack->push(new ArrayCommand(&_xData, ArrayCommand::insert, index, ba, ba.length()));
            _undoMaskStack->push(new ArrayCommand(&_xMask, ArrayCommand::insert, index, mask, mask.length()));
        }

        emit dataChanged();
        emit dataEdited();
    }
}

void QHexEditPrivate::insert(int index, char ch, char mask)
{
    _undoDataStack->push(new CharCommand(&_xData, CharCommand::insert, index, ch));
    _undoMaskStack->push(new CharCommand(&_xMask, CharCommand::insert, index, mask));
    emit dataChanged();
    emit dataEdited();
}

void QHexEditPrivate::remove(int index, int len)
{
    if(len > 0)
    {
        if(len == 1)
        {
            if(_overwriteMode)
            {
                _undoDataStack->push(new CharCommand(&_xData, CharCommand::replace, index, char(0)));
                _undoMaskStack->push(new CharCommand(&_xMask, CharCommand::replace, index, char(0)));
            }
            else
            {
                _undoDataStack->push(new CharCommand(&_xData, CharCommand::remove, index, char(0)));
                _undoMaskStack->push(new CharCommand(&_xMask, CharCommand::remove, index, char(0)));
            }
        }
        else
        {
            QByteArray ba = QByteArray(len, char(0));
            if(_overwriteMode)
            {
                _undoDataStack->push(new ArrayCommand(&_xData, ArrayCommand::replace, index, ba, ba.length()));
                _undoMaskStack->push(new ArrayCommand(&_xMask, ArrayCommand::replace, index, ba, ba.length()));
            }
            else
            {
                _undoDataStack->push(new ArrayCommand(&_xData, ArrayCommand::remove, index, ba, len));
                _undoMaskStack->push(new ArrayCommand(&_xMask, ArrayCommand::remove, index, ba, len));
            }
        }

        emit dataChanged();
        emit dataEdited();
    }
}

void QHexEditPrivate::replace(int index, char ch, char mask)
{
    _undoDataStack->push(new CharCommand(&_xData, CharCommand::replace, index, ch));
    _undoMaskStack->push(new CharCommand(&_xMask, CharCommand::replace, index, mask));
    resetSelection();
    emit dataChanged();
    emit dataEdited();
}

void QHexEditPrivate::replace(int index, const QByteArray & ba, const QByteArray & mask)
{
    _undoDataStack->push(new ArrayCommand(&_xData, ArrayCommand::replace, index, ba, ba.length()));
    _undoMaskStack->push(new ArrayCommand(&_xMask, ArrayCommand::replace, index, mask, mask.length()));
    resetSelection();
    emit dataChanged();
    emit dataEdited();
}

void QHexEditPrivate::replace(int pos, int len, const QByteArray & after, const QByteArray & mask)
{
    _undoDataStack->push(new ArrayCommand(&_xData, ArrayCommand::replace, pos, after, len));
    _undoMaskStack->push(new ArrayCommand(&_xMask, ArrayCommand::replace, pos, mask, len));
    resetSelection();
    emit dataChanged();
    emit dataEdited();
}

void QHexEditPrivate::fill(int index, const QByteArray & ba, const QByteArray & mask)
{
    int dataSize = _xData.size();
    if(index >= dataSize)
        return;
    int repeat = dataSize / ba.size() + 1;
    QByteArray fillData = ba.repeated(repeat);
    fillData.resize(dataSize);
    fillData = fillData.toHex();
    QByteArray fillMask = mask.repeated(repeat);
    fillMask.resize(dataSize);
    fillMask = fillMask.toHex();
    QByteArray origData = _xData.data().mid(index).toHex();
    for(int i = 0; i < dataSize * 2; i++)
        if(fillMask[i] == '1')
            fillData[i] = origData[i];
    this->replace(index, QByteArray().fromHex(fillData), QByteArray().fromHex(fillMask));
}

void QHexEditPrivate::setOverwriteMode(bool overwriteMode)
{
    _overwriteMode = overwriteMode;
}

bool QHexEditPrivate::overwriteMode()
{
    return _overwriteMode;
}

void QHexEditPrivate::setWildcardEnabled(bool enabled)
{
    _wildcardEnabled = enabled;
}

bool QHexEditPrivate::wildcardEnabled()
{
    return _wildcardEnabled;
}

void QHexEditPrivate::setKeepSize(bool enabled)
{
    _keepSize = enabled;
}

bool QHexEditPrivate::keepSize()
{
    return _keepSize;
}

void QHexEditPrivate::setHorizontalSpacing(int x)
{
    _horizonalSpacing = x;
    adjust();
    setCursorPos(cursorPos());
    this->update();
}

int QHexEditPrivate::horizontalSpacing()
{
    return _horizonalSpacing;
}

void QHexEditPrivate::setTextColor(QColor color)
{
    _textColor = color;
}

QColor QHexEditPrivate::textColor()
{
    return _textColor;
}

void QHexEditPrivate::setWildcardColor(QColor color)
{
    _wildcardColor = color;
}

QColor QHexEditPrivate::wildcardColor()
{
    return _wildcardColor;
}

void QHexEditPrivate::setBackgroundColor(QColor color)
{
    _backgroundColor = color;
}

QColor QHexEditPrivate::backgroundColor()
{
    return _backgroundColor;
}

void QHexEditPrivate::setSelectionColor(QColor color)
{
    _selectionColor = color;
}

QColor QHexEditPrivate::selectionColor()
{
    return _selectionColor;
}

void QHexEditPrivate::redo()
{
    if(!_undoDataStack->canRedo() || !_undoMaskStack->canRedo())
        return;
    _undoDataStack->redo();
    _undoMaskStack->redo();
    emit dataChanged();
    emit dataEdited();
    setCursorPos(_cursorPosition);
    update();
}

void QHexEditPrivate::undo()
{
    if(!_undoDataStack->canUndo() || !_undoMaskStack->canUndo())
        return;
    _undoDataStack->undo();
    _undoMaskStack->undo();
    emit dataChanged();
    emit dataEdited();
    setCursorPos(_cursorPosition);
    update();
}

void QHexEditPrivate::focusInEvent(QFocusEvent* event)
{
    ensureVisible();
    QWidget::focusInEvent(event);
}

void QHexEditPrivate::resizeEvent(QResizeEvent* event)
{
    adjust();
    QWidget::resizeEvent(event);
}

void QHexEditPrivate::keyPressEvent(QKeyEvent* event)
{
    int charX = (_cursorX - _xPosHex) / _charWidth;
    int posX = (charX / 3) * 2 + (charX % 3);
    int posBa = (_cursorY / _charHeight) * BYTES_PER_LINE + posX / 2;


    /*****************************************************************************/
    /* Cursor movements */
    /*****************************************************************************/

    if(event->matches(QKeySequence::MoveToNextChar))
    {
        setCursorPos(_cursorPosition + 1);
        resetSelection(_cursorPosition);
    }
    if(event->matches(QKeySequence::MoveToPreviousChar))
    {
        setCursorPos(_cursorPosition - 1);
        resetSelection(_cursorPosition);
    }
    if(event->matches(QKeySequence::MoveToEndOfLine))
    {
        setCursorPos(_cursorPosition | (2 * BYTES_PER_LINE - 1));
        resetSelection(_cursorPosition);
    }
    if(event->matches(QKeySequence::MoveToStartOfLine))
    {
        setCursorPos(_cursorPosition - (_cursorPosition % (2 * BYTES_PER_LINE)));
        resetSelection(_cursorPosition);
    }
    if(event->matches(QKeySequence::MoveToPreviousLine))
    {
        setCursorPos(_cursorPosition - (2 * BYTES_PER_LINE));
        resetSelection(_cursorPosition);
    }
    if(event->matches(QKeySequence::MoveToNextLine))
    {
        setCursorPos(_cursorPosition + (2 * BYTES_PER_LINE));
        resetSelection(_cursorPosition);
    }

    if(event->matches(QKeySequence::MoveToNextPage))
    {
        setCursorPos(_cursorPosition + (((_scrollArea->viewport()->height() / _charHeight) - 1) * 2 * BYTES_PER_LINE));
        resetSelection(_cursorPosition);
    }
    if(event->matches(QKeySequence::MoveToPreviousPage))
    {
        setCursorPos(_cursorPosition - (((_scrollArea->viewport()->height() / _charHeight) - 1) * 2 * BYTES_PER_LINE));
        resetSelection(_cursorPosition);
    }
    if(event->matches(QKeySequence::MoveToEndOfDocument))
    {
        setCursorPos(_xData.size() * 2);
        resetSelection(_cursorPosition);
    }
    if(event->matches(QKeySequence::MoveToStartOfDocument))
    {
        setCursorPos(0);
        resetSelection(_cursorPosition);
    }

    /*****************************************************************************/
    /* Select commands */
    /*****************************************************************************/
    if(event->matches(QKeySequence::SelectAll))
    {
        resetSelection(0);
        setSelection(2 * _xData.size() + 1);
    }
    if(event->matches(QKeySequence::SelectNextChar))
    {
        int pos = _cursorPosition + 1;
        setCursorPos(pos);
        setSelection(pos);
    }
    if(event->matches(QKeySequence::SelectPreviousChar))
    {
        int pos = _cursorPosition - 1;
        setSelection(pos);
        setCursorPos(pos);
    }
    if(event->matches(QKeySequence::SelectEndOfLine))
    {
        int pos = _cursorPosition - (_cursorPosition % (2 * BYTES_PER_LINE)) + (2 * BYTES_PER_LINE);
        setCursorPos(pos);
        setSelection(pos);
    }
    if(event->matches(QKeySequence::SelectStartOfLine))
    {
        int pos = _cursorPosition - (_cursorPosition % (2 * BYTES_PER_LINE));
        setCursorPos(pos);
        setSelection(pos);
    }
    if(event->matches(QKeySequence::SelectPreviousLine))
    {
        int pos = _cursorPosition - (2 * BYTES_PER_LINE);
        setCursorPos(pos);
        setSelection(pos);
    }
    if(event->matches(QKeySequence::SelectNextLine))
    {
        int pos = _cursorPosition + (2 * BYTES_PER_LINE);
        setCursorPos(pos);
        setSelection(pos);
    }

    if(event->matches(QKeySequence::SelectNextPage))
    {
        int pos = _cursorPosition + (((_scrollArea->viewport()->height() / _charHeight) - 1) * 2 * BYTES_PER_LINE);
        setCursorPos(pos);
        setSelection(pos);
    }
    if(event->matches(QKeySequence::SelectPreviousPage))
    {
        int pos = _cursorPosition - (((_scrollArea->viewport()->height() / _charHeight) - 1) * 2 * BYTES_PER_LINE);
        setCursorPos(pos);
        setSelection(pos);
    }
    if(event->matches(QKeySequence::SelectEndOfDocument))
    {
        int pos = _xData.size() * 2;
        setCursorPos(pos);
        setSelection(pos);
    }
    if(event->matches(QKeySequence::SelectStartOfDocument))
    {
        int pos = 0;
        setCursorPos(pos);
        setSelection(pos);
    }

    /*****************************************************************************/
    /* Edit Commands */
    /*****************************************************************************/
    /* Hex input */
    int key = int(event->text().toLower()[0].toLatin1());
    if((key >= '0' && key <= '9') || (key >= 'a' && key <= 'f') || (_wildcardEnabled && key == '?'))
    {
        if(getSelectionBegin() != getSelectionEnd())
        {
            posBa = getSelectionBegin();
            remove(posBa, getSelectionEnd() - posBa);
            setCursorPos(2 * posBa);
            resetSelection(2 * posBa);
        }

        // If insert mode, then insert a byte
        if(_overwriteMode == false)
        {
            if(_keepSize && _xData.size() >= _initSize)
            {
                QWidget::keyPressEvent(event);
                return;
            }
            if((charX % 3) == 0)
                insert(posBa, char(0), _wildcardEnabled);
        }

        // Change content
        if(_xData.size() > 0)
        {
            QByteArray hexMask = _xMask.data().mid(posBa, 1).toHex();
            QByteArray hexValue = _xData.data().mid(posBa, 1).toHex();
            if(key == '?') //wildcard
            {
                if((charX % 3) == 0)
                    hexMask[0] = '1';
                else
                    hexMask[1] = '1';
            }
            else
            {
                if((charX % 3) == 0)
                {
                    hexValue[0] = key;
                    hexMask[0] = '0';
                }
                else
                {
                    hexValue[1] = key;
                    hexMask[1] = '0';
                }
            }
            replace(posBa, QByteArray().fromHex(hexValue)[0], QByteArray().fromHex(hexMask)[0]);

            setCursorPos(_cursorPosition + 1);
            resetSelection(_cursorPosition);
        }
    }

    /* Cut & Paste */
    if(event->matches(QKeySequence::Cut))
    {
        QString result;
        for(int idx = getSelectionBegin(); idx < getSelectionEnd(); idx++)
        {
            QString byte = _xData.data().mid(idx, 1).toHex();
            QString mask = _xMask.data().mid(idx, 1).toHex();
            if(mask[0] == '1')
                result += "?";
            else
                result += byte[0];
            if(mask[1] == '1')
                result += "?";
            else
                result += byte[1];
            result += " ";
        }
        remove(getSelectionBegin(), getSelectionEnd() - getSelectionBegin());
        QClipboard* clipboard = QApplication::clipboard();
        clipboard->setText(result.toUpper());
        setCursorPos(getSelectionBegin() + 2);
        resetSelection(getSelectionBegin() + 2);
    }

    if(event->matches(QKeySequence::Paste))
    {
        QClipboard* clipboard = QApplication::clipboard();
        QString convert;
        QString pattern = clipboard->text();
        for(int i = 0; i < pattern.length(); i++)
        {
            QChar ch = pattern[i].toLower();
            if((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (_wildcardEnabled && ch == '?'))
                convert += ch;
        }
        if(convert.length() % 2) //odd length
            convert += "0";
        if(_keepSize && convert.length() / 2 >= _initSize)
            convert.truncate((_initSize - _xData.size()) * 2 - 1);
        QByteArray data(convert.length(), 0);
        QByteArray mask(data.length(), 0);
        for(int i = 0; i < convert.length(); i++)
        {
            if(convert[i] == '?')
            {
                data[i] = '0';
                mask[i] = '1';
            }
            else
            {
                data[i] = convert[i].toLatin1();
                mask[i] = '0';
            }
        }
        data = QByteArray().fromHex(data);
        mask = QByteArray().fromHex(mask);
        insert(_cursorPosition / 2, data, mask);
        setCursorPos(_cursorPosition + 2 * data.length());
        resetSelection(getSelectionBegin());
    }


    /* Delete char */
    if(event->matches(QKeySequence::Delete))
    {
        if(getSelectionBegin() != getSelectionEnd())
        {
            posBa = getSelectionBegin();
            remove(posBa, getSelectionEnd() - posBa);
            setCursorPos(2 * posBa);
            resetSelection(2 * posBa);
        }
        else
        {
            if(_overwriteMode)
                replace(posBa, char(0), char(0));
            else
                remove(posBa, 1);
        }
    }

    /* Backspace */
    if((event->key() == Qt::Key_Backspace) && (event->modifiers() == Qt::NoModifier))
    {
        if(getSelectionBegin() != getSelectionEnd())
        {
            posBa = getSelectionBegin();
            remove(posBa, getSelectionEnd() - posBa);
            setCursorPos(2 * posBa);
            resetSelection(2 * posBa);
        }
        else if(_cursorPosition == 1)
        {
            if(getSelectionBegin() != getSelectionEnd())
            {
                posBa = getSelectionBegin();
                remove(posBa, getSelectionEnd() - posBa);
                setCursorPos(2 * posBa);
                resetSelection(2 * posBa);
            }
            else
            {
                if(_overwriteMode)
                    replace(posBa, char(0), char(0));
                else
                    remove(posBa, 1);
            }
            setCursorPos(0);
        }
        else if(posBa > 0)
        {
            int delta = 1;
            if(_cursorPosition % 2) //odd cursor position
                delta = 0;
            if(_overwriteMode)
                replace(posBa - delta, char(0), char(0));
            else
                remove(posBa - delta, 1);
            setCursorPos(_cursorPosition - 1 - delta);
        }
    }

    /* undo */
    if(event->matches(QKeySequence::Undo))
    {
        undo();
    }

    /* redo */
    if(event->matches(QKeySequence::Redo))
    {
        redo();
    }

    if(event->matches(QKeySequence::Copy))
    {
        QString result;
        for(int idx = getSelectionBegin(); idx < getSelectionEnd(); idx++)
        {
            QString byte = _xData.data().mid(idx, 1).toHex();
            if(!byte.length())
                break;
            QString mask = _xMask.data().mid(idx, 1).toHex();
            if(mask[0] == '1')
                result += "?";
            else
                result += byte[0];
            if(mask[1] == '1')
                result += "?";
            else
                result += byte[1];
            result += " ";
        }
        QClipboard* clipboard = QApplication::clipboard();
        if(result.length())
            clipboard->setText(result.toUpper().trimmed());
    }

    // Switch between insert/overwrite mode
    if((event->key() == Qt::Key_Insert) && (event->modifiers() == Qt::NoModifier))
    {
        _overwriteMode = !_overwriteMode;
        setCursorPos(_cursorPosition);
        overwriteModeChanged(_overwriteMode);
    }

    ensureVisible();
    update();
    QWidget::keyPressEvent(event);
}

void QHexEditPrivate::mouseMoveEvent(QMouseEvent* event)
{
    _blink = false;
    update();
    int actPos = cursorPos(event->pos());
    setCursorPos(actPos);
    setSelection(actPos);
    QWidget::mouseMoveEvent(event);
}

void QHexEditPrivate::mousePressEvent(QMouseEvent* event)
{
    _blink = false;
    update();
    int cPos = cursorPos(event->pos());
    if((event->modifiers()&Qt::ShiftModifier) == Qt::ShiftModifier)
    {
        setCursorPos(cPos);
        setSelection(cPos);
    }
    else
    {
        resetSelection(cPos);
        setCursorPos(cPos);
    }
    QWidget::mousePressEvent(event);
}

void QHexEditPrivate::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setFont(font());
    painter.setLayoutDirection(Qt::LayoutDirectionAuto);

    painter.fillRect(event->rect(), _backgroundColor);

    // calc position
    int firstLineIdx = ((event->rect().top() / _charHeight) - _charHeight) * BYTES_PER_LINE;
    if(firstLineIdx < 0)
        firstLineIdx = 0;
    int lastLineIdx = ((event->rect().bottom() / _charHeight) + _charHeight) * BYTES_PER_LINE;
    if(lastLineIdx > _xData.size())
        lastLineIdx = _xData.size();
    int yPosStart = ((firstLineIdx) / BYTES_PER_LINE) * _charHeight + _charHeight;

    // paint hex area
    QByteArray hexBa(_xData.data().mid(firstLineIdx, lastLineIdx - firstLineIdx + 1).toHex());
    if(_wildcardEnabled)
    {
        QByteArray hexMask(_xMask.data().mid(firstLineIdx, lastLineIdx - firstLineIdx + 1).toHex());
        for(int i = 0; i < hexBa.size(); i++)
            if(hexMask[i] == '1')
                hexBa[i] = '?';
    }

    for(int lineIdx = firstLineIdx, yPos = yPosStart; lineIdx < lastLineIdx; lineIdx += BYTES_PER_LINE, yPos += _charHeight)
    {
        QString hex;
        int xPos = _xPosHex;
        for(int colIdx = 0; ((lineIdx + colIdx) < _xData.size() && (colIdx < BYTES_PER_LINE)); colIdx++)
        {
            int posBa = lineIdx + colIdx;
            if((getSelectionBegin() <= posBa) && (getSelectionEnd() > posBa))
            {
                painter.setBackground(_selectionColor);
                painter.setBackgroundMode(Qt::OpaqueMode);
            }
            else
            {
                painter.setBackground(_backgroundColor);
                painter.setBackgroundMode(Qt::TransparentMode);
            }

            // render hex value
            painter.setPen(_textColor);
            if(colIdx == 0)
            {
                hex = hexBa.mid((lineIdx - firstLineIdx) * 2, 2).toUpper();
                for(int i = 0; i < hex.length(); i++)
                {
                    if(hex[i] == '?') //wildcard
                        painter.setPen(_wildcardColor);
                    else
                        painter.setPen(_textColor);
                    painter.drawText(xPos + _charWidth * i, yPos, QString(hex[i]));
                }
                xPos += 2 * _charWidth;
            }
            else
            {
                if(getSelectionBegin() == posBa)
                {
                    hex = hexBa.mid((lineIdx + colIdx - firstLineIdx) * 2, 2).toUpper();
                    xPos += _charWidth;
                }
                else
                    hex = hexBa.mid((lineIdx + colIdx - firstLineIdx) * 2, 2).prepend(" ").toUpper();
                for(int i = 0; i < hex.length(); i++)
                {
                    if(hex[i] == '?') //wildcard
                        painter.setPen(_wildcardColor);
                    else
                        painter.setPen(_textColor);
                    painter.drawText(xPos + _charWidth * i, yPos, QString(hex[i]));
                }
                xPos += hex.length() * _charWidth;
            }
        }
    }
    painter.setBackgroundMode(Qt::TransparentMode);
    painter.setPen(this->palette().color(QPalette::WindowText));

    // paint cursor
    if(_blink && hasFocus())
    {
        if(_overwriteMode)
            painter.fillRect(_cursorX, _cursorY + _charHeight - 2, _charWidth, 2, this->palette().color(QPalette::WindowText));
        else
            painter.fillRect(_cursorX, _cursorY, 2, _charHeight, this->palette().color(QPalette::WindowText));
    }

    if(_size != _xData.size())
    {
        _size = _xData.size();
        emit currentSizeChanged(_size);
    }
}

void QHexEditPrivate::setCursorPos(int position)
{
    // delete cursor
    _blink = false;
    update();

    // cursor in range?
    if(_overwriteMode)
    {
        if(position > (_xData.size() * 2 - 1))
            position = _xData.size() * 2 - 1;
    }
    else
    {
        if(position > (_xData.size() * 2))
            position = _xData.size() * 2;
    }

    if(position < 0)
        position = 0;

    // calc position
    _cursorPosition = position;
    _cursorY = (position / (2 * BYTES_PER_LINE)) * _charHeight + 4;
    int x = (position % (2 * BYTES_PER_LINE));
    _cursorX = (((x / 2) * 3) + (x % 2)) * _charWidth + _xPosHex;

    // immiadately draw cursor
    _blink = true;
    update();
    emit currentAddressChanged(_cursorPosition / 2);
}

int QHexEditPrivate::cursorPos(QPoint pos)
{
    int result = -1;
    // find char under cursor
    if((pos.x() >= _xPosHex) && (pos.x() < (_xPosHex + HEXCHARS_IN_LINE * _charWidth)))
    {
        int x = (pos.x() - _xPosHex) / _charWidth;
        if((x % 3) == 0)
            x = (x / 3) * 2;
        else
            x = ((x / 3) * 2) + 1;
        int y = ((pos.y() - 3) / _charHeight) * 2 * BYTES_PER_LINE;
        result = x + y;
    }
    return result;
}

int QHexEditPrivate::cursorPos()
{
    return _cursorPosition;
}

void QHexEditPrivate::resetSelection()
{
    _selectionBegin = _selectionInit;
    _selectionEnd = _selectionInit;
}

void QHexEditPrivate::resetSelection(int pos)
{
    if(pos < 0)
        pos = 0;
    pos++;
    pos = pos / 2;
    _selectionInit = pos;
    _selectionBegin = pos;
    _selectionEnd = pos;
}

void QHexEditPrivate::setSelection(int pos)
{
    if(pos < 0)
        pos = 0;
    pos++;
    pos = pos / 2;
    if(pos >= _selectionInit)
    {
        _selectionEnd = pos;
        _selectionBegin = _selectionInit;
    }
    else
    {
        _selectionBegin = pos;
        _selectionEnd = _selectionInit;
    }
}

int QHexEditPrivate::getSelectionBegin()
{
    return _selectionBegin;
}

int QHexEditPrivate::getSelectionEnd()
{
    return _selectionEnd;
}


void QHexEditPrivate::updateCursor()
{
    if(_blink)
        _blink = false;
    else
        _blink = true;
    update(_cursorX, _cursorY, _charWidth, _charHeight);
}

void QHexEditPrivate::adjust()
{
    QFontMetrics metrics(this->font());
    _charWidth = metrics.width(QLatin1Char('9'));
    _charHeight = metrics.height();

    _xPosHex = _horizonalSpacing;

    // tell QAbstractScollbar, how big we are
    setMinimumHeight(((_xData.size() / 16 + 1) * _charHeight) + 5);
    setMinimumWidth(_xPosHex + HEXCHARS_IN_LINE * _charWidth);

    update();
}

void QHexEditPrivate::ensureVisible()
{
    // scrolls to cursorx, cusory (which are set by setCursorPos)
    // x-margin is 3 pixels, y-margin is half of charHeight
    _scrollArea->ensureVisible(_cursorX, _cursorY + _charHeight / 2, 3, _charHeight / 2 + 2);
}
