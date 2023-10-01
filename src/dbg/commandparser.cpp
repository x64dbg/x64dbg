#include "commandparser.h"

Command::Command(const String & command)
{
    ParseState state = Default;
    int len = (int)command.length();
    for(int i = 0; i < len; i++)
    {
        char ch = command[i];
        switch(state)
        {
        case Default:
            switch(ch)
            {
            case '\t':
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
            switch(ch)
            {
            case '\t':
            case ' ':
                dataAppend(' ');
                break;
            case ',':
                dataAppend(ch);
                break;
            case '\"':
                dataAppend(ch);
                break;
            default:
                dataAppend('\\');
                dataAppend(ch);
                break;
            }
            state = Default;
            break;
        case Text:
            switch(ch)
            {
            case '\\':
                state = TextEscaped;
                break;
            case '\"':
                state = Default;
                break;
            case '{':
                state = StringFormat;
                dataAppend(ch);
                break;
            default:
                dataAppend(ch);
                break;
            }
            break;
        case TextEscaped:
            switch(ch)
            {
            case '\"':
                dataAppend(ch);
                break;
            case '{':
                dataAppend(ch);
                break;
            default:
                dataAppend('\\');
                dataAppend(ch);
                break;
            }
            state = Text;
            break;
        case StringFormat:
        {
            auto nextch = i + 1 < len ? command[i + 1] : '\0';
            switch(ch)
            {
            case '{':
                if(nextch == '{')
                {
                    dataAppend(ch);
                    dataAppend(nextch);
                    i++;
                }
                else
                {
                    dataAppend(ch);
                }
                break;
            case '}':
                if(nextch == '}')
                {
                    dataAppend(ch);
                    dataAppend(nextch);
                    i++;
                }
                else
                {
                    dataAppend(ch);
                    state = Text;
                }
                break;
            case '\\':
                switch(nextch)
                {
                case '\"':
                case '\\':
                    dataAppend(nextch);
                    i++;
                    break;
                default:
                    dataAppend(ch);
                    break;
                }
                break;
            default:
                dataAppend(ch);
                break;
            }
        }
        break;
        }
    }
    if(state == Escaped || state == TextEscaped)
        dataAppend('\\');
    dataFinish();
}

const String Command::GetText()
{
    return _tokens.size() ? _tokens[0] : String();
}

const int Command::GetArgCount()
{
    return _tokens.size() ? (int)_tokens.size() - 1 : 0;
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
    _tokens.push_back(_data);
    _data.clear();
}
