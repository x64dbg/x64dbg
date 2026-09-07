#ifndef _COMMANDPARSER_H
#define _COMMANDPARSER_H

#include "_global.h"

class Command
{
public:
    Command(const String & command);
    static String Escape(const String & argument);
    const String GetText();
    const String GetArg(const int argnum);
    const int GetArgCount();

private:
    String _data;
    std::vector<String> _tokens;

    enum ParseState
    {
        Default,
        Text,
        StringFormat,
    };

    void dataFinish();
    void dataAppend(const char ch);
};

#endif // _COMMANDPARSER_H
