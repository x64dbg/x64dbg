#pragma once

/**
\brief Conditional tracing structures
*/
struct TraceCondition
{
    ExpressionParser condition;
    duint steps;
    duint maxSteps;

    explicit TraceCondition(const String & expression, duint maxCount)
        : condition(expression), steps(0), maxSteps(maxCount) {}

    // Return value: 0:Continue, 1:Break, -1:Error (Break)
    char BreakTrace()
    {
        steps++;
        if(steps >= maxSteps)
            return 1;
        duint value;
        if(condition.Calculate(value, valuesignedcalc(), true))
            return value != 0;
        else
            return -1;
    }
};

struct TextCondition
{
    ExpressionParser condition;
    String text;

    explicit TextCondition(const String & expression, const String & text)
        : condition(expression), text(text) {}

    char Evaluate() const
    {
        duint value;
        if(condition.Calculate(value, valuesignedcalc(), true))
            return !!value;
        return -1;
    }
};

struct TraceState
{
    bool InitTraceCondition(const String & expression, duint maxSteps)
    {
        delete traceCondition;
        traceCondition = new TraceCondition(expression, maxSteps);
        bool temp = traceCondition->condition.IsValidExpression();
        if(!temp)
        {
            delete traceCondition;
            traceCondition = nullptr;
        }
        return temp;
    }

    bool InitLogFile()
    {
        if(logFile.empty())
            return true;
        auto hFile = CreateFileW(logFile.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if(hFile == INVALID_HANDLE_VALUE)
            return false;
        logWriter = new BufferedWriter(hFile);
        duint setting;
        if(BridgeSettingGetUint("Misc", "Utf16LogRedirect", &setting))
            writeUtf16 = !!setting;
        return true;
    }

    void LogWrite(String text)
    {
        if(logWriter)
        {
            if(writeUtf16)
            {
                auto textUtf16 = StringUtils::Utf8ToUtf16(text);
                logWriter->Write(textUtf16.c_str(), textUtf16.size() * 2);
                logWriter->Write(L"\r\n", 4);
            }
            else
            {
                logWriter->Write(text.c_str(), text.size());
                logWriter->Write("\n", 1);
            }
        }
        else
            dputs_untranslated(text.c_str());
    }

    bool IsActive() const
    {
        return traceCondition != nullptr;
    }

    bool IsExtended() const
    {
        return logCondition || cmdCondition;
    }

    char BreakTrace() const
    {
        return traceCondition ? traceCondition->BreakTrace() : 1;
    }

    duint StepCount() const
    {
        return traceCondition ? traceCondition->steps : 0;
    }

    bool InitLogCondition(const String & expression, const String & text)
    {
        delete logCondition;
        logCondition = nullptr;
        if(text.empty())
            return true;
        if(expression.empty())
            logCondition = new TextCondition("1", text);
        else
            logCondition = new TextCondition(expression, text);
        return logCondition->condition.IsValidExpression();
    }

    char EvaluateLog() const
    {
        return logCondition ? logCondition->Evaluate() : 0;
    }

    const String & LogText() const
    {
        return logCondition ? logCondition->text : emptyString;
    }

    bool InitCmdCondition(const String & expression, const String & text)
    {
        delete cmdCondition;
        cmdCondition = nullptr;
        // An empty expression means the command condition defaults to the break
        // condition. The trace condition is not initialized yet at this point
        // (TraceSetCommand executes before the trace is started), so the break
        // condition evaluated during the trace is used in EvaluateCmd instead.
        cmdConditionDefault = expression.empty();
        if(text.empty())
            return true;
        cmdCondition = new TextCondition(cmdConditionDefault ? "1" : expression, text);
        return cmdCondition->condition.IsValidExpression();
    }

    char EvaluateCmd(char defaultValue) const
    {
        if(cmdCondition == nullptr || cmdConditionDefault)
            return defaultValue;
        return cmdCondition->Evaluate();
    }

    const String & CmdText() const
    {
        return cmdCondition ? cmdCondition->text : emptyString;
    }

    void SetLogFile(const char* fileName)
    {
        logFile = StringUtils::Utf8ToUtf16(fileName);
    }

    bool ForceBreakTrace()
    {
        return forceBreakTrace;
    }

    void SetForceBreakTrace()
    {
        forceBreakTrace = true;
    }

    void SetPartyFilter(int party)
    {
        partyFilter = party;
    }

    int GetPartyFilter() const
    {
        return partyFilter;
    }

    void SetRunToParty(bool enabled)
    {
        runToParty = enabled;
    }

    bool UseRunToParty() const
    {
        return runToParty;
    }

    void Clear()
    {
        delete traceCondition;
        traceCondition = nullptr;
        delete logCondition;
        logCondition = nullptr;
        delete cmdCondition;
        cmdCondition = nullptr;
        cmdConditionDefault = false;
        logFile.clear();
        delete logWriter;
        logWriter = nullptr;
        writeUtf16 = false;
        forceBreakTrace = false;
        partyFilter = -1;
        runToParty = false;
    }

private:
    TraceCondition* traceCondition = nullptr;
    TextCondition* logCondition = nullptr;
    TextCondition* cmdCondition = nullptr;
    bool cmdConditionDefault = false;
    String emptyString;
    WString logFile;
    BufferedWriter* logWriter = nullptr;
    bool writeUtf16 = false;
    bool forceBreakTrace = false;
    int partyFilter = -1;  // -1 = none, mod_user = 0, mod_system = 1
    bool runToParty = false;
};
