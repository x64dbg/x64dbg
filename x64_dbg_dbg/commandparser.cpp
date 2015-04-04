#include "commandparser.h"

Command::Command(const String & command)
{
    ParseState state = Default;
    int len = command.length();
    for(int i = 0; i < len; i++)
    {
        char ch = command[i];
        switch(state)
        {
        case Default:
            switch(ch)
            {
            case ' ':
                if(!_tokens.size())
                    dataFinish();
                break;
            case ',':
                dataFinish();
                break;
            case '\\':
                state = Escaped;
                break;
            case '\"':
                state = Text;
                break;
            default:
                dataAppend(ch);
                break;
            }
            break;
        case Escaped:
            dataAppend(ch);
            state = Default;
            break;
        case Text:
            switch(ch)
            {
            case '\\':
                state = TextEscaped;
                break;
            case '\"':
                dataFinish();
                state = Default;
                break;
            default:
                dataAppend(ch);
                break;
            }
            break;
        case TextEscaped:
            dataAppend(ch);
            state = Text;
            break;
        }
    }
    dataFinish();
}

const String Command::GetText()
{
    return _tokens.size() ? _tokens[0] : String();
}

const int Command::GetArgCount()
{
    return _tokens.size() ? _tokens.size() - 1 : 0;
}

const String Command::GetArg(int argnum)
{
    return (int)_tokens.size() < argnum + 1 ? String() : _tokens[argnum + 1];
}

void Command::dataAppend(const char ch)
{
    _data += ch;
}

void Command::dataFinish()
{
    if(_data.length())
    {
        _tokens.push_back(_data);
        _data = "";
    }
}
