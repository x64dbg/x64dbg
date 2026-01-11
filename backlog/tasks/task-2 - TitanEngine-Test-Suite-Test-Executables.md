---
id: task-2
title: 'TitanEngine Test Suite: Test Executables'
status: Done
assignee:
  - '@claude'
created_date: '2026-01-11 15:18'
updated_date: '2026-01-11 20:23'
labels:
  - testing
dependencies: []
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Create the test executables with exported symbols for TitanEngine testing. Each executable needs Static and ASLR variants for both x86 and x64.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 TestExe_Breakpoints with SW/HW/Memory BP targets
- [x] #2 TestExe_Threading for multi-threaded scenarios
- [x] #3 TestExe_Exceptions for all exception types
- [x] #4 TestExe_Stepping for step-into/over scenarios
- [x] #5 TestExe_DllLoad + TestDll for DLL events
- [x] #6 TestExe_Attach for attach/detach testing
- [x] #7 TestExe_Context for register testing
- [x] #8 Static and ASLR variants for each executable
- [x] #9 x86 and x64 builds
<!-- AC:END -->

## Implementation Plan

<!-- SECTION:PLAN:BEGIN -->
1. Create src/titan_tests/targets/ directory
2. Write TestExe_Breakpoints.cpp
3. Write TestExe_Threading.cpp
4. Write TestExe_Exceptions.cpp
5. Write TestExe_Stepping.cpp
6. Write TestExe_DllLoad.cpp and TestDll.cpp
7. Write TestExe_Context.cpp
8. Write TestExe_Attach.cpp
9. Update cmake.toml with Static/ASLR variants for x86/x64
<!-- SECTION:PLAN:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
Created 8 test executables with both ASLR and non-ASLR variants: TestExe_Breakpoints, TestExe_Context, TestExe_Exceptions, TestExe_Stepping, TestExe_Threading, TestExe_DllLoad, TestExe_Attach, TestDll.
<!-- SECTION:NOTES:END -->
