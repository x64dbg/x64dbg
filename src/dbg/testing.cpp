#include "testing.h"

#include "console.h"
#include "simplescript.h"
#include "stringformat.h"
#include "value.h"

#include <atomic>
#include <mutex>

struct TEST_STATE
{
    std::atomic<bool> enabled{ false };
    bool startupScriptExecuted = false;
    bool startupScriptSucceeded = true;
    std::string firstFailureMessage;
    std::atomic<unsigned long long> assertionCount{ 0 };
    std::atomic<bool> assertionFailed{ false };
    std::atomic<bool> finalized{ false };
};

static TEST_STATE gTestState;
static std::mutex gTestStateMutex;

static String formatOptionalMessage(int argc, char* argv[], int firstArg)
{
    if(argc <= firstArg)
        return {};
    if(argc == firstArg + 1)
        return stringformatinline(argv[firstArg]);

    FormatValueVector formatArgs;
    for(int i = firstArg + 1; i < argc; i++)
        formatArgs.push_back(argv[i]);
    return stringformat(argv[firstArg], formatArgs);
}

static void setFirstFailureMessage(const String & message)
{
    if(message.empty())
        return;
    std::lock_guard<std::mutex> lock(gTestStateMutex);
    if(gTestState.firstFailureMessage.empty())
        gTestState.firstFailureMessage = message;
}

static void logAssertionFailure(const char* source, const char* expression, const char* message)
{
    if(expression && *expression && message && *message)
        dprintf_untranslated("[x64dbg-test] ASSERT FAIL source=%s expr=\"%s\" message=\"%s\"\n", source, expression, message);
    else if(expression && *expression)
        dprintf_untranslated("[x64dbg-test] ASSERT FAIL source=%s expr=\"%s\"\n", source, expression);
    else if(message && *message)
        dprintf_untranslated("[x64dbg-test] ASSERT FAIL source=%s message=\"%s\"\n", source, message);
    else
        dprintf_untranslated("[x64dbg-test] ASSERT FAIL source=%s\n", source);
}

static bool assertCommon(bool condition, const char* source, const char* expression, const char* message)
{
    gTestState.assertionCount.fetch_add(1);
    if(condition)
        return true;

    gTestState.assertionFailed = true;
    if(message && *message)
        setFirstFailureMessage(message);
    else if(expression && *expression)
        setFirstFailureMessage(StringUtils::sprintf("assertion failed: %s", expression));
    logAssertionFailure(source, expression, message);
    ScriptInterruptAwait(ScriptInterrupt::AbortAssertion);
    return false;
}

void TestInitialize(bool enabled)
{
    std::lock_guard<std::mutex> lock(gTestStateMutex);
    gTestState.enabled = enabled;
    gTestState.startupScriptExecuted = false;
    gTestState.startupScriptSucceeded = true;
    gTestState.firstFailureMessage.clear();
    gTestState.assertionCount = 0;
    gTestState.assertionFailed = false;
    gTestState.finalized = false;
}

void TestShutdown()
{
    TestInitialize(false);
}

bool TestIsEnabled()
{
    return gTestState.enabled.load();
}

bool TestRunStartupScript(const char* filename)
{
    const bool success = ScriptExecAwait(filename, false);
    if(TestIsEnabled())
    {
        std::lock_guard<std::mutex> lock(gTestStateMutex);
        gTestState.startupScriptExecuted = true;
        gTestState.startupScriptSucceeded = success;
    }
    return success;
}

bool TestAssertCommand(bool condition, const char* expression, const char* message)
{
    return assertCommon(condition, "script", expression, message);
}

bool TestAssertPlugin(bool condition, const char* message)
{
    return assertCommon(condition, "plugin", nullptr, message);
}

bool cbInstrTestAssert(int argc, char* argv[])
{
    if(!TestIsEnabled())
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "[x64dbg-test] testassert requires -testing"));
        return false;
    }
    if(IsArgumentsLessThan(argc, 2))
        return false;

    duint value = 0;
    String message;
    bool success = valfromstring(argv[1], &value, false);
    if(argc >= 3)
        message = formatOptionalMessage(argc, argv, 2);
    if(!success)
    {
        if(message.empty())
            message = StringUtils::sprintf("invalid expression: %s", argv[1]);
        return TestAssertCommand(false, argv[1], message.c_str());
    }
    return TestAssertCommand(value != 0, argv[1], message.empty() ? nullptr : message.c_str());
}

bool cbInstrTestFinalize(int argc, char* argv[])
{
    if(!TestIsEnabled())
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "[x64dbg-test] testfinalize requires -testing"));
        return false;
    }

    if(gTestState.finalized.exchange(true))
        return true;

    const auto asserts = gTestState.assertionCount.load();
    bool startupScriptExecuted = false;
    bool startupScriptSucceeded = true;
    {
        std::lock_guard<std::mutex> lock(gTestStateMutex);
        startupScriptExecuted = gTestState.startupScriptExecuted;
        startupScriptSucceeded = gTestState.startupScriptSucceeded;
    }

    const char* reason = nullptr;
    if(asserts == 0)
        reason = "no_asserts";
    else if(gTestState.assertionFailed.load())
        reason = "assert_failed";
    else if(startupScriptExecuted && !startupScriptSucceeded)
        reason = "script_failed";

    if(reason)
        dprintf_untranslated("[x64dbg-test] FINAL status=fail asserts=%llu reason=%s\n", asserts, reason);
    else
        dprintf_untranslated("[x64dbg-test] FINAL status=pass asserts=%llu\n", asserts);

    if(BridgeIsHeadless())
        GuiCloseApplication();

    return reason == nullptr;
}

bool cbInstrTestScript(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    return TestRunStartupScript(argv[1]);
}

bool cbInstrSettingSet(int argc, char* argv[])
{
    if(argc != 3 && argc != 4)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Usage: settingset section, key[, value]"));
        return false;
    }
    if(argv[1] == nullptr || argv[2] == nullptr || *argv[1] == '\0' || *argv[2] == '\0')
        return false;

    const char* value = argc == 4 ? argv[3] : nullptr;
    if(!BridgeSettingSet(argv[1], argv[2], value))
        return false;
    DbgSettingsUpdated();
    return true;
}
