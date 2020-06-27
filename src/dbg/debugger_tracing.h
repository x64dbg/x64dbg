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

    bool BreakTrace()
    {
        steps++;
        if(steps >= maxSteps)
            return true;
        duint value;
        return !condition.Calculate(value, valuesignedcalc(), true) || value;
    }
};

struct TextCondition
{
    ExpressionParser condition;
    String text;

    explicit TextCondition(const String & expression, const String & text)
        : condition(expression), text(text) {}

    bool Evaluate(bool defaultValue) const
    {
        duint value;
        if(condition.Calculate(value, valuesignedcalc(), true))
            return !!value;
        return defaultValue;
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
            dprintf_untranslated("%s\n", text.c_str());
    }

    bool IsActive() const
    {
        return traceCondition != nullptr;
    }

    bool IsExtended() const
    {
        return logCondition || cmdCondition;
    }

    bool BreakTrace() const
    {
        return !traceCondition || traceCondition->BreakTrace();
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
        logCondition = new TextCondition(expression, text);
        return logCondition->condition.IsValidExpression();
    }

    bool EvaluateLog(bool defaultValue) const
    {
        return logCondition && logCondition->Evaluate(defaultValue);
    }

    const String & LogText() const
    {
        return logCondition ? logCondition->text : emptyString;
    }

    bool InitCmdCondition(const String & expression, const String & text)
    {
        delete cmdCondition;
        cmdCondition = nullptr;
        if(text.empty())
            return true;
        cmdCondition = new TextCondition(expression, text);
        return cmdCondition->condition.IsValidExpression();
    }

    bool EvaluateCmd(bool defaultValue) const
    {
        return cmdCondition && cmdCondition->Evaluate(defaultValue);
    }

    const String & CmdText() const
    {
        return cmdCondition ? cmdCondition->text : emptyString;
    }

    bool InitSwitchCondition(const String & expression)
    {
        delete switchCondition;
        switchCondition = nullptr;
        if(expression.empty())
            return true;
        switchCondition = new TextCondition(expression, "");
        return switchCondition->condition.IsValidExpression();
    }

    bool EvaluateSwitch(bool defaultValue) const
    {
        return switchCondition && switchCondition->Evaluate(defaultValue);
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

    void Clear()
    {
        delete traceCondition;
        traceCondition = nullptr;
        delete logCondition;
        logCondition = nullptr;
        delete cmdCondition;
        cmdCondition = nullptr;
        delete switchCondition;
        switchCondition = nullptr;
        logFile.clear();
        delete logWriter;
        logWriter = nullptr;
        writeUtf16 = false;
        forceBreakTrace = false;
    }

private:
    TraceCondition* traceCondition = nullptr;
    TextCondition* logCondition = nullptr;
    TextCondition* cmdCondition = nullptr;
    TextCondition* switchCondition = nullptr;
    String emptyString;
    WString logFile;
    BufferedWriter* logWriter = nullptr;
    bool writeUtf16 = false;
    bool forceBreakTrace = false;
};
