/**
\file console.cpp
\brief Implements the console class.
*/

#include "console.h"
#include "taskthread.h"

static void GuiAddLogMessageAsync(_In_z_ const char* msg)
{
    static StringConcatTaskThread_<void(*)(const std::string &)> task([](const std::string & msg)
    {
        GuiAddLogMessage(msg.c_str());
    });
    task.WakeUp(msg);
}

static void GuiAddLogMessageHtmlAsync(_In_z_ const char* msg)
{
    static StringConcatTaskThread_<void(*)(const std::string &)> task([](const std::string & msg)
    {
        GuiAddLogMessageHtml(msg.c_str());
    });
    task.WakeUp(msg);
}

/**
\brief Print a line with text, terminated with a newline to the console.
\param text The text to print.
*/
void dputs(_In_z_ const char* Text)
{
    // Only append the newline if the caller didn't
    const char* TranslatedText = GuiTranslateText(Text);
    size_t textlen = strlen(TranslatedText);
    if(TranslatedText[textlen - 1] != '\n')
    {
        Memory<char*> buffer(textlen + 2, "dputs");
        memcpy(buffer(), TranslatedText, textlen);
        buffer()[textlen] = '\n';
        buffer()[textlen + 1] = '\0';
        GuiAddLogMessageAsync(buffer());
    }
    else
        GuiAddLogMessageAsync(TranslatedText);
}

/**
\brief Print a formatted string to the console.
\param format The printf format to use (see documentation of printf for more information).
*/
void dprintf(_In_z_ _Printf_format_string_ const char* Format, ...)
{
    va_list args;

    va_start(args, Format);
    dprintf_args(Format, args);
    va_end(args);
}

void dprintf_untranslated(_In_z_ _Printf_format_string_ const char* Format, ...)
{
    va_list args;

    va_start(args, Format);
    dprintf_args_untranslated(Format, args);
    va_end(args);
}

/**
\brief Print a formatted string to the console.
\param format The printf format to use (see documentation of printf for more information).
\param Args The argument buffer passed to the string parser.
*/
void dprintf_args(_In_z_ _Printf_format_string_ const char* Format, va_list Args)
{
    char buffer[16384];
    vsnprintf_s(buffer, _TRUNCATE, GuiTranslateText(Format), Args);

    GuiAddLogMessageAsync(buffer);
}

/**
\brief Print a line with text, terminated with a newline to the console.
\param text The text to print.
*/
void dputs_untranslated(_In_z_ const char* Text)
{
    // Only append the newline if the caller didn't
    size_t textlen = strlen(Text);
    if(Text[textlen - 1] != '\n')
    {
        Memory<char*> buffer(textlen + 2, "dputs");
        memcpy(buffer(), Text, textlen);
        buffer()[textlen] = '\n';
        buffer()[textlen + 1] = '\0';
        GuiAddLogMessageAsync(buffer());
    }
    else
        GuiAddLogMessageAsync(Text);
}
/**
\brief Print a formatted string to the console.
\param format The printf format to use (see documentation of printf for more information).
\param Args The argument buffer passed to the string parser.
*/
void dprintf_args_untranslated(_In_z_ _Printf_format_string_ const char* Format, va_list Args)
{
    char buffer[16384];
    vsnprintf_s(buffer, _TRUNCATE, Format, Args);

    GuiAddLogMessageAsync(buffer);
}
/**
\brief Print a html string to the console.
\param Text The message to use.
*/
void dputs_untranslated_html(_In_z_ _Printf_format_string_ const char* Text)
{
    GuiAddLogMessageHtmlAsync(Text);
}
