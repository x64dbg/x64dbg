/**
 * TitanEngine Test Runner
 *
 * A test runner for TitanEngine API tests.
 *
 * Usage:
 *   TitanTestRunner.exe SW-01 HW-03    # Run specific tests
 *   TitanTestRunner.exe SW-*           # Run tests matching pattern
 *   TitanTestRunner.exe --list         # List available tests
 *   TitanTestRunner.exe                # Run all tests (CI mode)
 *
 * Output:
 *   - Success: Single summary line (e.g., "87/87 tests passed")
 *   - Failure: Test name, assertion, file:line for each failure
 *
 * Exit codes:
 *   - 0: All tests passed
 *   - N: Number of failed tests
 */

#include "TitanTestFramework.h"
#include <cstring>
#include <algorithm>

// Include TitanEngine header for API access
#include "TitanEngine/TitanEngine.h"

//-----------------------------------------------------------------------------
// Main entry point
// All tests are defined in separate files under tests/ directory:
//   - SoftwareBreakpointTests.cpp (SW-*)
//   - HardwareBreakpointTests.cpp (HW-*)
//   - MemoryBreakpointTests.cpp (MB-*)
//   - SteppingTests.cpp (ST-*)
//   - ExceptionTests.cpp (EX-*)
//   - DebugEventTests.cpp (DE-*)
//   - MultiThreadingTests.cpp (MT-*)
//   - ContextTests.cpp (CX-*)
//   - AttachDetachTests.cpp (AD-*)
//   - CombinedTests.cpp (CB-*)
//   - ASLRTests.cpp (ASLR-*)
//-----------------------------------------------------------------------------

static void PrintUsage(const char* programName)
{
    fprintf(stderr, "Usage: %s [OPTIONS] [TEST_PATTERNS...]\n", programName);
    fprintf(stderr, "\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  --list      List all available tests\n");
    fprintf(stderr, "  --json      Output results in JSON format\n");
    fprintf(stderr, "  --isolate   Run each test in a fresh process (slower but isolated)\n");
    fprintf(stderr, "  --help      Show this help message\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Examples:\n");
    fprintf(stderr, "  %s SW-01 HW-03    Run specific tests\n", programName);
    fprintf(stderr, "  %s SW-*           Run tests matching pattern\n", programName);
    fprintf(stderr, "  %s --json         Run all tests with JSON output\n", programName);
    fprintf(stderr, "  %s                Run all tests (CI mode)\n", programName);
}

int main(int argc, char* argv[])
{
    // Parse command line arguments
    std::vector<std::string> patterns;
    bool listMode = false;
    TitanTest::RunConfig config;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--list") == 0)
        {
            listMode = true;
        }
        else if (strcmp(argv[i], "--json") == 0)
        {
            config.jsonOutput = true;
        }
        else if (strcmp(argv[i], "--isolate") == 0)
        {
            config.isolation = TitanTest::ProcessIsolation::FreshProcess;
        }
        else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
        {
            PrintUsage(argv[0]);
            return 0;
        }
        else if (argv[i][0] == '-')
        {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            PrintUsage(argv[0]);
            return 1;
        }
        else
        {
            patterns.push_back(argv[i]);
        }
    }

    TitanTest::TestRunner runner;
    runner.config = config;

    if (listMode)
    {
        runner.ListTests();
        return 0;
    }

    // Run tests
    int failCount = runner.Run(patterns);

    // Print latency stats if any were collected (not in JSON mode)
    if (!config.jsonOutput)
    {
        runner.PrintLatencyStats();
    }

    return failCount;
}
