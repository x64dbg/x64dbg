#include "commandparser.h"

Command::Command(const String & command)
{
    ParseState state = Default;
    int len = (int)command.length();
    auto appendBackslashes = [this](int count)
    {
        while(count-- > 0)
            dataAppend('\\');
    };
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
            {
                int slashCount = 1;
                while(i + slashCount < len && command[i + slashCount] == '\\')
                    slashCount++;
                auto nextch = i + slashCount < len ? command[i + slashCount] : '\0';
                if(nextch == '\"')
                {
                    appendBackslashes(slashCount / 2);
                    i += slashCount; // Also consume nextch.
                    if(slashCount % 2)
                        dataAppend(nextch);
                    else
                        state = Text;
                }
                else
                {
                    // Preserve the legacy escape behavior for spaces, commas and ordinary characters.
                    appendBackslashes(slashCount / 2 * 2);
                    if(slashCount % 2 == 0)
                        i += slashCount - 1;
                    else if(nextch == '\0')
                    {
                        dataAppend('\\');
                        i += slashCount - 1;
                    }
                    else
                    {
                        i += slashCount; // Also consume nextch.
                        switch(nextch)
                        {
                        case '\t':
                        case ' ':
                            dataAppend(' ');
                            break;
                        case ',':
                            dataAppend(nextch);
                            break;
                        default:
                            dataAppend('\\');
                            dataAppend(nextch);
                            break;
                        }
                    }
                }
                break;
            }
            case '\"':
                state = Text;
                break;
            default:
                dataAppend(ch);
                break;
            }
            break;
        case Text:
            switch(ch)
            {
            case '\\':
            {
                int slashCount = 1;
                while(i + slashCount < len && command[i + slashCount] == '\\')
                    slashCount++;
                auto nextch = i + slashCount < len ? command[i + slashCount] : '\0';
                if(nextch == '\"' || nextch == '{')
                {
                    appendBackslashes(slashCount / 2);
                    i += slashCount; // Also consume nextch.
                    if(nextch == '{')
                    {
                        dataAppend(nextch);
                        if(slashCount % 2 == 0)
                            state = StringFormat;
                    }
                    else if(slashCount % 2)
                        dataAppend(nextch);
                    else
                        state = Default;
                }
                else
                {
                    appendBackslashes(slashCount);
                    i += slashCount - 1;
                }
                break;
            }
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
    dataFinish();
}

String Command::Escape(const String & argument)
{
    String result;
    result.reserve(argument.size() * 2 + 1);
    auto appendBackslashes = [&result](size_t count)
    {
        result.append(count, '\\');
    };

    for(size_t i = 0; i < argument.size();)
    {
        auto ch = argument[i];
        if(ch != '\\')
        {
            if(ch == '\"' || ch == '{')
                result.push_back('\\');
            result.push_back(ch);
            i++;
            continue;
        }

        auto slashStart = i;
        while(i < argument.size() && argument[i] == '\\')
            i++;
        auto slashCount = i - slashStart;
        if(i == argument.size())
        {
            // The caller adds the closing quote, so protect trailing slashes from it.
            appendBackslashes(slashCount * 2);
        }
        else if(argument[i] == '\"' || argument[i] == '{')
        {
            // 2N+1 slashes decode to N slashes followed by a literal special character.
            appendBackslashes(slashCount * 2 + 1);
            result.push_back(argument[i++]);
        }
        else
        {
            // Backslashes before ordinary characters are always preserved verbatim.
            appendBackslashes(slashCount);
        }
    }
    return result;
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
