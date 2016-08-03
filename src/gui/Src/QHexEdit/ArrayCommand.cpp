#include "ArrayCommand.h"
#include "XByteArray.h"

CharCommand::CharCommand(XByteArray* xData, Cmd cmd, int charPos, char newChar, QUndoCommand* parent) : QUndoCommand(parent)
{
    _xData = xData;
    _charPos = charPos;
    _newChar = newChar;
    _cmd = cmd;
    _oldChar = 0;
}

bool CharCommand::mergeWith(const QUndoCommand* command)
{
    const CharCommand* nextCommand = static_cast<const CharCommand*>(command);
    bool result = false;

    if(_cmd != remove)
    {
        if(nextCommand->_cmd == replace)
            if(nextCommand->_charPos == _charPos)
            {
                _newChar = nextCommand->_newChar;
                result = true;
            }
    }
    return result;
}

void CharCommand::undo()
{
    switch(_cmd)
    {
    case insert:
        _xData->remove(_charPos, 1);
        break;
    case replace:
        _xData->replace(_charPos, _oldChar);
        break;
    case remove:
        _xData->insert(_charPos, _oldChar);
        break;
    }
}

void CharCommand::redo()
{
    switch(_cmd)
    {
    case insert:
        _xData->insert(_charPos, _newChar);
        break;
    case replace:
        _oldChar = _xData->data()[_charPos];
        _xData->replace(_charPos, _newChar);
        break;
    case remove:
        _oldChar = _xData->data()[_charPos];
        _xData->remove(_charPos, 1);
        break;
    }
}



ArrayCommand::ArrayCommand(XByteArray* xData, Cmd cmd, int baPos, QByteArray newBa, int len, QUndoCommand* parent)
    : QUndoCommand(parent)
{
    _cmd = cmd;
    _xData = xData;
    _baPos = baPos;
    _newBa = newBa;
    _len = len;
}

void ArrayCommand::undo()
{
    switch(_cmd)
    {
    case insert:
        _xData->remove(_baPos, _newBa.length());
        break;
    case replace:
        _xData->replace(_baPos, _oldBa);
        break;
    case remove:
        _xData->insert(_baPos, _oldBa);
        break;
    }
}

void ArrayCommand::redo()
{
    switch(_cmd)
    {
    case insert:
        _xData->insert(_baPos, _newBa);
        break;
    case replace:
        _oldBa = _xData->data().mid(_baPos, _len);
        _xData->replace(_baPos, _newBa);
        break;
    case remove:
        _oldBa = _xData->data().mid(_baPos, _len);
        _xData->remove(_baPos, _len);
        break;
    }
}
