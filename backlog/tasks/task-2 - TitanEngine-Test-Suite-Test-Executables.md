---
id: task-2
title: 'TitanEngine Test Suite: Test Executables'
status: To Do
assignee: []
created_date: '2026-01-11 15:18'
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
- [ ] #1 TestExe_Breakpoints with SW/HW/Memory BP targets
- [ ] #2 TestExe_Threading for multi-threaded scenarios
- [ ] #3 TestExe_Exceptions for all exception types
- [ ] #4 TestExe_Stepping for step-into/over scenarios
- [ ] #5 TestExe_DllLoad + TestDll for DLL events
- [ ] #6 TestExe_Attach for attach/detach testing
- [ ] #7 TestExe_Context for register testing
- [ ] #8 Static and ASLR variants for each executable
- [ ] #9 x86 and x64 builds
<!-- AC:END -->
