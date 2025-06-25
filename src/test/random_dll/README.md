# Random DLL Loading Test Case

## Purpose

This test case demonstrates the scenario described in the x64dbg feature request for wildcard DLL breakpoints. It simulates a process that generates and loads DLLs with random names at runtime, specifically using the `.xxl` extension as mentioned in the issue.

## Problem Statement

The original issue described:
- Process generates a DLL at runtime with random names like `tuavp.xxl` in the %TEMP% directory
- Second run generates a different random name like `hpsk.xxl`
- User wants x64dbg to break on every `.xxl` load using wildcard patterns

## Test Case Components

### Files:
- `main.cpp` - Main executable that generates and loads random DLLs
- `test_dll.cpp` - Simple DLL that gets copied with random names
- `CMakeLists.txt` - CMake build configuration for both targets

### What the test does:

1. **Builds a test DLL** that contains:
   - Exported functions for testing
   - Console output when loaded/unloaded

2. **Main executable** that:
   - Generates random 8-character names with `.xxl` extension
   - Copies the test DLL to %TEMP% with the random name
   - Loads the DLL using `LoadLibrary`
   - Calls exported functions
   - Unloads and cleans up the DLL
   - Repeats the process to simulate multiple runs

## Usage Instructions

### Building
1. Configure the project: `cmake -B build`
2. Build the project `cmake --build build --config Release`

### Testing with x64dbg

- Load the `build/Release/random_dll_test.exe` in x64dbg
- Run to the entry point
- Set a breakpoint on `LoadLibraryExW`
  - Break condition: `0`
  - Command condition: `strstr(utf16(arg(0)), ".xxl")`
  - Command text: `$bpdll {utf16(arg(0))}, a, 1`
- Run, this should break on DLL load (1)
- Run, this should break on DLL entry (1)
- Run, this should break on DLL load (2)
- Run, this should break on DLL entry (3)
- Run, this should exit with code 0
