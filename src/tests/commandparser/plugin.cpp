#include <Windows.h>

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

#include "_plugins.h"
#include "bridgemain.h"

namespace
{
    int gPluginHandle = 0;
    std::string gExpected;
    std::vector<std::string> gExpectedArgs;

    // Frozen copy of the parser from before issue #3921 was fixed. The
    // differential tests below use it to identify accidental compatibility
    // changes separately from the intentional backslash/quote changes.
    std::vector<std::string> legacyParse(const std::string & command)
    {
        enum ParseState
        {
            Default,
            Escaped,
            Text,
            TextEscaped,
            StringFormat,
        } state = Default;

        std::vector<std::string> tokens;
        std::string data;
        auto finish = [&]()
        {
            tokens.push_back(data);
            data.clear();
        };

        const auto len = (int)command.length();
        for(int i = 0; i < len; i++)
        {
            const auto ch = command[i];
            switch(state)
            {
            case Default:
                switch(ch)
                {
                case '\t':
                case ' ':
                    if(tokens.empty())
                        finish();
                    break;
                case ',':
                    finish();
                    break;
                case '\\':
                    state = Escaped;
                    break;
                case '\"':
                    state = Text;
                    break;
                default:
                    data += ch;
                    break;
                }
                break;
            case Escaped:
                switch(ch)
                {
                case '\t':
                case ' ':
                    data += ' ';
                    break;
                case ',':
                case '\"':
                    data += ch;
                    break;
                default:
                    data += '\\';
                    data += ch;
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
                    data += ch;
                    break;
                default:
                    data += ch;
                    break;
                }
                break;
            case TextEscaped:
                switch(ch)
                {
                case '\"':
                case '{':
                    data += ch;
                    break;
                default:
                    data += '\\';
                    data += ch;
                    break;
                }
                state = Text;
                break;
            case StringFormat:
            {
                const auto nextch = i + 1 < len ? command[i + 1] : '\0';
                switch(ch)
                {
                case '{':
                    data += ch;
                    if(nextch == '{')
                        data += command[++i];
                    break;
                case '}':
                    data += ch;
                    if(nextch == '}')
                        data += command[++i];
                    else
                        state = Text;
                    break;
                case '\\':
                    if(nextch == '\"' || nextch == '\\')
                        data += command[++i];
                    else
                        data += ch;
                    break;
                default:
                    data += ch;
                    break;
                }
                break;
            }
            }
        }
        if(state == Escaped || state == TextEscaped)
            data += '\\';
        finish();
        return tokens;
    }

    bool cbCapture(int argc, char* argv[])
    {
        if(!_plugin_testassert(argc == 2, "commandparsercapture expected 1 argument, got %d", argc - 1))
            return false;
        return _plugin_testassert(gExpected == argv[1], "command parser round trip mismatch: expected '%s', got '%s'", gExpected.c_str(), argv[1]);
    }

    bool cbCompare(int argc, char* argv[])
    {
        bool success = _plugin_testassert((size_t)argc == gExpectedArgs.size(),
                                          "differential argument count mismatch: expected %zu, got %d",
                                          gExpectedArgs.size() - 1, argc - 1);
        const auto count = std::min((size_t)argc, gExpectedArgs.size());
        for(size_t i = 0; i < count; i++)
        {
            if(!_plugin_testassert(gExpectedArgs[i] == argv[i],
                                   "differential mismatch at argv[%zu]: expected '%s', got '%s'",
                                   i, gExpectedArgs[i].c_str(), argv[i]))
                success = false;
        }
        return success;
    }

    bool dispatchAndCompare(const std::string & argumentText, const std::vector<std::string> & expected)
    {
        const auto command = std::string("commandparsercompare ") + argumentText;
        gExpectedArgs = expected;
        return _plugin_testassert(DbgCmdExecDirect(command.c_str()), "differential command failed: %s", command.c_str());
    }

    bool compareWithLegacy(const std::string & argumentText)
    {
        const auto command = std::string("commandparsercompare ") + argumentText;
        auto expected = legacyParse(command);
        expected[0] = command; // Command callbacks receive the original command in argv[0].
        return dispatchAndCompare(argumentText, expected);
    }

    bool cbDifferential(int, char**)
    {
        const std::vector<std::string> compatibilityCorpus =
        {
            "plain",
            "one,two,three",
            R"(one\,two\ three)",
            R"(C:\data\file.db)",
            R"(\\server\share\file.exe)",
            R"cmd("C:\Program Files\x64dbg\x64dbg.exe")cmd",
            R"cmd("\\server\share with spaces\file.exe")cmd",
            R"cmd("commas, semicolons; and spaces")cmd",
            R"(mod.fromname(\"user32\") + 90B0)",
            R"cmd("literal \{ brace")cmd",
            R"cmd("is jmp: {streq(dis.mnemonic(dis.sel()), "jmp")}")cmd",
            R"cmd("nested {ansi(rax), \"text\"}")cmd",
        };
        for(const auto & argumentText : compatibilityCorpus)
            if(!compareWithLegacy(argumentText))
                return false;

        // Exercise legacy-compatible slash runs before ordinary characters,
        // separators, whitespace, and the end of an unquoted argument.
        for(size_t count = 0; count <= 6; count++)
        {
            const auto slashes = std::string(count, '\\');
            if(!compareWithLegacy("before" + slashes + "x"))
                return false;
            if(!compareWithLegacy("before" + slashes + ",after"))
                return false;
            if(!compareWithLegacy("before" + slashes + " after"))
                return false;
            if(!compareWithLegacy("trailing" + slashes))
                return false;
            if(!compareWithLegacy("\"before" + slashes + "x after\""))
                return false;
        }

        // These are intentional differences. First prove that the legacy
        // parser does not produce the desired result, then check the new one.
        const std::string issue3921 = R"cmd("streq(\"a\",\"C:\directory\\\")")cmd";
        std::vector<std::string> issue3921Expected =
        {
            "commandparsercompare",
            R"cmd(streq("a","C:\directory\"))cmd",
        };
        const auto issue3921Command = "commandparsercompare " + issue3921;
        if(!_plugin_testassert(legacyParse(issue3921Command) != issue3921Expected,
                               "issue #3921 unexpectedly matches the legacy parser"))
            return false;
        issue3921Expected[0] = issue3921Command;
        if(!dispatchAndCompare(issue3921, issue3921Expected))
            return false;

        const std::string trailingSlash = R"cmd("C:\directory\\")cmd";
        std::vector<std::string> trailingSlashExpected =
        {
            "commandparsercompare",
            R"(C:\directory\)",
        };
        const auto trailingSlashCommand = "commandparsercompare " + trailingSlash;
        if(!_plugin_testassert(legacyParse(trailingSlashCommand) != trailingSlashExpected,
                               "quoted trailing slash unexpectedly matches the legacy parser"))
            return false;
        trailingSlashExpected[0] = trailingSlashCommand;
        return dispatchAndCompare(trailingSlash, trailingSlashExpected);
    }

    bool roundTrip(const std::string & value)
    {
        auto commandEscape = DbgFunctions()->CommandEscape;
        if(!_plugin_testassert(commandEscape != nullptr, "CommandEscape is unavailable"))
            return false;

        std::vector<char> escaped(value.size() * 2 + 1);
        if(!_plugin_testassert(commandEscape(value.c_str(), escaped.data(), escaped.size()), "CommandEscape failed for '%s'", value.c_str()))
            return false;

        gExpected = value;
        auto command = std::string("commandparsercapture \"") + escaped.data() + "\"";
        return _plugin_testassert(DbgCmdExecDirect(command.c_str()), "round-trip command failed: %s", command.c_str());
    }

    bool cbRoundTrip(int, char**)
    {
        const std::vector<std::string> values =
        {
            "",
            "plain",
            "spaces, commas; and semicolons",
            "streq(\"a\", \"b\")",
            "C:\\data\\file.db",
            "\\\\server\\share\\file.exe",
            "C:\\directory\\",
            "streq(\"a\", \"C:\\directory\\\")",
            "streq(utf16(arg(0)),\"D:\\FSViewer\\FSIV02.fslang\")",
            "mov $buf, rdx; bp [rsp], \"\", ss; bpcnd [rsp], \"streq(ansi(buf,28),\\\"require('./gamemode/index');\\\")\"",
            "C:\\{}\\file.exe",
            "quotes: \\\" and braces: \\{",
        };
        for(const auto & value : values)
            if(!roundTrip(value))
                return false;

        for(char ch = 0x20; ch <= 0x7E; ch++)
            if(!roundTrip(std::string(1, ch)))
                return false;

        for(size_t count = 0; count <= 4; count++)
        {
            auto slashes = std::string(count, '\\');
            if(!roundTrip("before quote " + slashes + "\" after"))
                return false;
            if(!roundTrip("before brace " + slashes + "{ after"))
                return false;
            if(!roundTrip("trailing " + slashes))
                return false;
        }
        return true;
    }

    bool cbIssue3921(int argc, char* argv[])
    {
        const char* expected = "streq(\"a\",\"C:\\directory\\\")";
        if(!_plugin_testassert(argc == 2, "issue3921 expected 1 argument, got %d", argc - 1))
            return false;
        return _plugin_testassert(strcmp(argv[1], expected) == 0, "issue3921 mismatch: expected '%s', got '%s'", expected, argv[1]);
    }

    bool cbLegacyFormat(int argc, char* argv[])
    {
        const char* expected = "is jmp: {streq(dis.mnemonic(dis.sel()), \"jmp\")}";
        if(!_plugin_testassert(argc == 2, "legacy format expected 1 argument, got %d", argc - 1))
            return false;
        return _plugin_testassert(strcmp(argv[1], expected) == 0, "legacy format mismatch: expected '%s', got '%s'", expected, argv[1]);
    }

    bool cbLegacyUnquoted(int argc, char* argv[])
    {
        const char* expected = "mod.fromname(\"user32\")+90B0";
        if(!_plugin_testassert(argc == 2, "legacy unquoted expected 1 argument, got %d", argc - 1))
            return false;
        return _plugin_testassert(strcmp(argv[1], expected) == 0, "legacy unquoted mismatch: expected '%s', got '%s'", expected, argv[1]);
    }

    bool cbLegacyEscaped(int argc, char* argv[])
    {
        const char* expected = "one,two three";
        if(!_plugin_testassert(argc == 2, "legacy escapes expected 1 argument, got %d", argc - 1))
            return false;
        return _plugin_testassert(strcmp(argv[1], expected) == 0, "legacy escapes mismatch: expected '%s', got '%s'", expected, argv[1]);
    }
}

extern "C" __declspec(dllexport) bool pluginit(PLUG_INITSTRUCT* initStruct)
{
    initStruct->pluginVersion = 1;
    initStruct->sdkVersion = PLUG_SDKVERSION;
    strncpy_s(initStruct->pluginName, sizeof(initStruct->pluginName), X64DBG_TEST_NAME, _TRUNCATE);
    gPluginHandle = initStruct->pluginHandle;
    _plugin_registercommand(gPluginHandle, "commandparsercapture", cbCapture, false);
    _plugin_registercommand(gPluginHandle, "commandparsercompare", cbCompare, false);
    _plugin_registercommand(gPluginHandle, "commandparserdifferential", cbDifferential, false);
    _plugin_registercommand(gPluginHandle, "commandparserroundtrip", cbRoundTrip, false);
    _plugin_registercommand(gPluginHandle, "commandparserissue3921", cbIssue3921, false);
    _plugin_registercommand(gPluginHandle, "commandparserlegacyformat", cbLegacyFormat, false);
    _plugin_registercommand(gPluginHandle, "commandparserlegacyunquoted", cbLegacyUnquoted, false);
    _plugin_registercommand(gPluginHandle, "commandparserlegacyescaped", cbLegacyEscaped, false);
    return true;
}

extern "C" __declspec(dllexport) void plugstop()
{
}

extern "C" __declspec(dllexport) void plugsetup(PLUG_SETUPSTRUCT*)
{
}
