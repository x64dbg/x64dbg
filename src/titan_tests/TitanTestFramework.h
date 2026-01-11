#pragma once

/**
 * TitanEngine Test Framework
 *
 * A minimal test framework for TitanEngine API testing.
 * Designed for context-efficient output: single summary on success,
 * minimal actionable output on failure.
 *
 * Usage:
 *   TITAN_TEST(SW_01, "Software BP: Basic INT3") {
 *       // Test implementation
 *       TEST_ASSERT(condition, "Error message");
 *       return true;
 *   }
 */

#include <windows.h>
#include <functional>
#include <string>
#include <vector>
#include <chrono>
#include <map>
#include <cstdio>
#include <regex>

namespace TitanTest
{

// Forward declarations
struct TestResult;
struct TestInfo;
class TestRegistry;

//-----------------------------------------------------------------------------
// Latency tracking for performance-sensitive operations
//-----------------------------------------------------------------------------
struct LatencyStats
{
    double minMs = 0.0;
    double maxMs = 0.0;
    double totalMs = 0.0;
    size_t count = 0;

    void Record(double ms)
    {
        if (count == 0)
        {
            minMs = maxMs = ms;
        }
        else
        {
            if (ms < minMs) minMs = ms;
            if (ms > maxMs) maxMs = ms;
        }
        totalMs += ms;
        count++;
    }

    double AverageMs() const
    {
        return count > 0 ? totalMs / count : 0.0;
    }
};

// Global latency trackers for common operations
inline LatencyStats g_bpHitLatency;      // Breakpoint hit latency
inline LatencyStats g_stepLatency;       // Single step latency
inline LatencyStats g_contextLatency;    // Context get/set latency

class LatencyTimer
{
public:
    LatencyTimer(LatencyStats& stats) : m_stats(stats)
    {
        m_start = std::chrono::high_resolution_clock::now();
    }

    ~LatencyTimer()
    {
        auto end = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - m_start).count();
        m_stats.Record(ms);
    }

private:
    LatencyStats& m_stats;
    std::chrono::high_resolution_clock::time_point m_start;
};

// Macros for latency tracking
#define TITAN_TRACK_BP_HIT() TitanTest::LatencyTimer _bpTimer(TitanTest::g_bpHitLatency)
#define TITAN_TRACK_STEP() TitanTest::LatencyTimer _stepTimer(TitanTest::g_stepLatency)
#define TITAN_TRACK_CONTEXT() TitanTest::LatencyTimer _ctxTimer(TitanTest::g_contextLatency)

//-----------------------------------------------------------------------------
// Test result structure
//-----------------------------------------------------------------------------
enum class TestStatus
{
    Passed,
    Failed,
    Skipped
};

struct TestResult
{
    TestStatus status = TestStatus::Skipped;
    std::string errorMessage;
    std::string file;
    int line = 0;
    double durationMs = 0.0;
};

//-----------------------------------------------------------------------------
// Test information and registration
//-----------------------------------------------------------------------------
struct TestInfo
{
    std::string id;           // e.g., "SW-01", "HW-03"
    std::string description;  // Human-readable description
    std::function<bool()> testFunc;
    TestResult result;
};

//-----------------------------------------------------------------------------
// Test assertion context (thread-local for nested tests)
//-----------------------------------------------------------------------------
struct AssertionContext
{
    bool failed = false;
    std::string errorMessage;
    std::string file;
    int line = 0;
};

inline thread_local AssertionContext g_assertionContext;

//-----------------------------------------------------------------------------
// Test registry singleton
//-----------------------------------------------------------------------------
class TestRegistry
{
public:
    static TestRegistry& Instance()
    {
        static TestRegistry instance;
        return instance;
    }

    void Register(const std::string& id, const std::string& description, std::function<bool()> testFunc)
    {
        TestInfo info;
        info.id = id;
        info.description = description;
        info.testFunc = testFunc;
        m_tests.push_back(info);
    }

    std::vector<TestInfo>& GetTests()
    {
        return m_tests;
    }

    TestInfo* FindTest(const std::string& id)
    {
        for (auto& test : m_tests)
        {
            if (test.id == id)
                return &test;
        }
        return nullptr;
    }

    // Match tests against a pattern (supports * wildcard)
    std::vector<TestInfo*> MatchTests(const std::string& pattern)
    {
        std::vector<TestInfo*> matches;

        // Convert glob pattern to regex
        std::string regexPattern;
        for (char c : pattern)
        {
            if (c == '*')
                regexPattern += ".*";
            else if (c == '?')
                regexPattern += ".";
            else if (c == '.' || c == '+' || c == '^' || c == '$' ||
                     c == '[' || c == ']' || c == '(' || c == ')' ||
                     c == '{' || c == '}' || c == '|' || c == '\\')
                regexPattern += "\\" + std::string(1, c);
            else
                regexPattern += c;
        }

        try
        {
            std::regex re(regexPattern, std::regex::icase);
            for (auto& test : m_tests)
            {
                if (std::regex_match(test.id, re))
                    matches.push_back(&test);
            }
        }
        catch (const std::regex_error&)
        {
            // If regex fails, try exact match
            for (auto& test : m_tests)
            {
                if (test.id == pattern)
                    matches.push_back(&test);
            }
        }

        return matches;
    }

private:
    TestRegistry() = default;
    std::vector<TestInfo> m_tests;
};

//-----------------------------------------------------------------------------
// Test registration helper class
//-----------------------------------------------------------------------------
class TestRegistrar
{
public:
    TestRegistrar(const std::string& id, const std::string& description, std::function<bool()> testFunc)
    {
        TestRegistry::Instance().Register(id, description, testFunc);
    }
};

//-----------------------------------------------------------------------------
// Test assertion macros
//-----------------------------------------------------------------------------
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            TitanTest::g_assertionContext.failed = true; \
            TitanTest::g_assertionContext.errorMessage = (message); \
            TitanTest::g_assertionContext.file = __FILE__; \
            TitanTest::g_assertionContext.line = __LINE__; \
            return false; \
        } \
    } while (0)

#define TEST_ASSERT_EQ(expected, actual, message) \
    TEST_ASSERT((expected) == (actual), message)

#define TEST_ASSERT_NE(expected, actual, message) \
    TEST_ASSERT((expected) != (actual), message)

#define TEST_ASSERT_TRUE(condition) \
    TEST_ASSERT(condition, "Expected true: " #condition)

#define TEST_ASSERT_FALSE(condition) \
    TEST_ASSERT(!(condition), "Expected false: " #condition)

#define TEST_ASSERT_NOT_NULL(ptr) \
    TEST_ASSERT((ptr) != nullptr, "Expected non-null: " #ptr)

#define TEST_ASSERT_NULL(ptr) \
    TEST_ASSERT((ptr) == nullptr, "Expected null: " #ptr)

#define TEST_SKIP(message) \
    do { \
        TitanTest::g_assertionContext.failed = false; \
        TitanTest::g_assertionContext.errorMessage = std::string("SKIP: ") + (message); \
        TitanTest::g_assertionContext.file = __FILE__; \
        TitanTest::g_assertionContext.line = __LINE__; \
        return false; \
    } while (0)

//-----------------------------------------------------------------------------
// Test definition macro
//-----------------------------------------------------------------------------
#define TITAN_TEST(testId, description) \
    static bool TestFunc_##testId(); \
    static TitanTest::TestRegistrar g_registrar_##testId(#testId, description, TestFunc_##testId); \
    static bool TestFunc_##testId()

// Alternative macro with hyphenated IDs (converts hyphen to underscore)
#define TITAN_TEST_ID(testIdStr, funcSuffix, description) \
    static bool TestFunc_##funcSuffix(); \
    static TitanTest::TestRegistrar g_registrar_##funcSuffix(testIdStr, description, TestFunc_##funcSuffix); \
    static bool TestFunc_##funcSuffix()

//-----------------------------------------------------------------------------
// Test runner configuration
//-----------------------------------------------------------------------------
enum class ProcessIsolation
{
    ReuseProcess,   // Run all tests in same process (faster, but state can leak)
    FreshProcess    // Spawn fresh process per test (slower, but isolated)
};

struct RunConfig
{
    bool jsonOutput = false;
    ProcessIsolation isolation = ProcessIsolation::ReuseProcess;
};

//-----------------------------------------------------------------------------
// Test runner
//-----------------------------------------------------------------------------
class TestRunner
{
public:
    RunConfig config;

    // Run all tests or specific tests matching patterns
    int Run(const std::vector<std::string>& patterns)
    {
        auto& registry = TestRegistry::Instance();
        auto& allTests = registry.GetTests();

        std::vector<TestInfo*> testsToRun;

        if (patterns.empty())
        {
            // Run all tests
            for (auto& test : allTests)
                testsToRun.push_back(&test);
        }
        else
        {
            // Run tests matching patterns
            for (const auto& pattern : patterns)
            {
                auto matches = registry.MatchTests(pattern);
                for (auto* test : matches)
                {
                    // Avoid duplicates
                    bool found = false;
                    for (auto* existing : testsToRun)
                    {
                        if (existing->id == test->id)
                        {
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                        testsToRun.push_back(test);
                }
            }
        }

        if (testsToRun.empty())
        {
            fprintf(stderr, "No tests matched the specified patterns\n");
            return 1;
        }

        int passed = 0;
        int failed = 0;
        int skipped = 0;

        for (auto* test : testsToRun)
        {
            // Reset assertion context
            g_assertionContext = AssertionContext{};

            auto startTime = std::chrono::high_resolution_clock::now();

            bool success = false;
            try
            {
                success = test->testFunc();
            }
            catch (const std::exception& e)
            {
                g_assertionContext.failed = true;
                g_assertionContext.errorMessage = std::string("Exception: ") + e.what();
            }
            catch (...)
            {
                g_assertionContext.failed = true;
                g_assertionContext.errorMessage = "Unknown exception";
            }

            auto endTime = std::chrono::high_resolution_clock::now();
            double durationMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();

            test->result.durationMs = durationMs;

            if (success && !g_assertionContext.failed)
            {
                test->result.status = TestStatus::Passed;
                passed++;
            }
            else if (g_assertionContext.errorMessage.substr(0, 5) == "SKIP:")
            {
                test->result.status = TestStatus::Skipped;
                test->result.errorMessage = g_assertionContext.errorMessage.substr(6);
                skipped++;
            }
            else
            {
                test->result.status = TestStatus::Failed;
                test->result.errorMessage = g_assertionContext.errorMessage;
                test->result.file = g_assertionContext.file;
                test->result.line = g_assertionContext.line;
                failed++;

                // Print failure immediately (minimal, actionable output) unless JSON mode
                if (!config.jsonOutput)
                {
                    fprintf(stderr, "FAIL: %s\n", test->id.c_str());
                    if (!test->result.errorMessage.empty())
                        fprintf(stderr, "  %s\n", test->result.errorMessage.c_str());
                    if (!test->result.file.empty())
                        fprintf(stderr, "  at %s:%d\n", test->result.file.c_str(), test->result.line);
                }
            }
        }

        // Output results
        int total = passed + failed + skipped;
        if (config.jsonOutput)
        {
            PrintJsonResults(testsToRun, passed, failed, skipped);
        }
        else
        {
            // Print summary
            if (failed == 0)
            {
                if (skipped > 0)
                    printf("%d/%d tests passed (%d skipped)\n", passed, total, skipped);
                else
                    printf("%d/%d tests passed\n", passed, total);
            }
            else
            {
                printf("%d/%d tests passed, %d failed", passed, total, failed);
                if (skipped > 0)
                    printf(" (%d skipped)", skipped);
                printf("\n");
            }
        }

        return failed;
    }

    // List all available tests
    void ListTests()
    {
        auto& registry = TestRegistry::Instance();
        auto& tests = registry.GetTests();

        for (const auto& test : tests)
        {
            printf("%s - %s\n", test.id.c_str(), test.description.c_str());
        }
    }

    // Print JSON results
    void PrintJsonResults(const std::vector<TestInfo*>& tests, int passed, int failed, int skipped)
    {
        printf("{\n");
        printf("  \"summary\": {\n");
        printf("    \"total\": %d,\n", static_cast<int>(tests.size()));
        printf("    \"passed\": %d,\n", passed);
        printf("    \"failed\": %d,\n", failed);
        printf("    \"skipped\": %d\n", skipped);
        printf("  },\n");
        printf("  \"tests\": [\n");

        for (size_t i = 0; i < tests.size(); i++)
        {
            const auto* test = tests[i];
            const char* statusStr = "unknown";
            switch (test->result.status)
            {
                case TestStatus::Passed: statusStr = "pass"; break;
                case TestStatus::Failed: statusStr = "fail"; break;
                case TestStatus::Skipped: statusStr = "skip"; break;
            }

            printf("    {\n");
            printf("      \"id\": \"%s\",\n", test->id.c_str());
            printf("      \"description\": \"%s\",\n", EscapeJson(test->description).c_str());
            printf("      \"status\": \"%s\",\n", statusStr);
            printf("      \"duration_ms\": %.2f", test->result.durationMs);

            if (test->result.status == TestStatus::Failed || test->result.status == TestStatus::Skipped)
            {
                printf(",\n");
                printf("      \"message\": \"%s\"", EscapeJson(test->result.errorMessage).c_str());
                if (test->result.status == TestStatus::Failed && !test->result.file.empty())
                {
                    printf(",\n");
                    printf("      \"file\": \"%s\",\n", EscapeJson(test->result.file).c_str());
                    printf("      \"line\": %d", test->result.line);
                }
            }

            printf("\n    }%s\n", (i < tests.size() - 1) ? "," : "");
        }

        printf("  ]\n");
        printf("}\n");
    }

    // Escape string for JSON output
    static std::string EscapeJson(const std::string& str)
    {
        std::string result;
        for (char c : str)
        {
            switch (c)
            {
                case '"': result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\b': result += "\\b"; break;
                case '\f': result += "\\f"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default:
                    if (static_cast<unsigned char>(c) < 0x20)
                    {
                        char buf[8];
                        snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                        result += buf;
                    }
                    else
                    {
                        result += c;
                    }
            }
        }
        return result;
    }

    // Print latency stats if any operations were tracked
    void PrintLatencyStats()
    {
        bool hasStats = false;

        if (g_bpHitLatency.count > 0)
        {
            if (!hasStats)
            {
                printf("\nLatency stats:\n");
                hasStats = true;
            }
            printf("  BP hit: avg=%.2fms min=%.2fms max=%.2fms (n=%zu)\n",
                   g_bpHitLatency.AverageMs(), g_bpHitLatency.minMs,
                   g_bpHitLatency.maxMs, g_bpHitLatency.count);
        }

        if (g_stepLatency.count > 0)
        {
            if (!hasStats)
            {
                printf("\nLatency stats:\n");
                hasStats = true;
            }
            printf("  Step: avg=%.2fms min=%.2fms max=%.2fms (n=%zu)\n",
                   g_stepLatency.AverageMs(), g_stepLatency.minMs,
                   g_stepLatency.maxMs, g_stepLatency.count);
        }

        if (g_contextLatency.count > 0)
        {
            if (!hasStats)
            {
                printf("\nLatency stats:\n");
                hasStats = true;
            }
            printf("  Context: avg=%.2fms min=%.2fms max=%.2fms (n=%zu)\n",
                   g_contextLatency.AverageMs(), g_contextLatency.minMs,
                   g_contextLatency.maxMs, g_contextLatency.count);
        }
    }
};

} // namespace TitanTest
