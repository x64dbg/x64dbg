---
id: task-13
title: 'TitanEngine Test Suite: ASLR Breakpoint Restore Tests'
status: Done
assignee:
  - '@claude'
created_date: '2026-01-11 15:19'
updated_date: '2026-01-11 20:23'
labels:
  - testing
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Test breakpoint database restore functionality when executables are loaded at different addresses due to ASLR.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Save breakpoints with ASLR executable
- [ ] #2 Restart process with different base address
- [ ] #3 Verify breakpoints restored at correct RVA
- [ ] #4 Test with SW, HW, and Memory breakpoints
<!-- AC:END -->

## Implementation Plan

<!-- SECTION:PLAN:BEGIN -->
1. Create ASLRTests.cpp in src/titan_tests/tests/
2. Implement ASLR-01: Save breakpoints with ASLR executable (set BP, save RVA)
3. Implement ASLR-02: Restart process with different base address (force ASLR relocation)
4. Implement ASLR-03: Verify breakpoints restored at correct RVA
5. Implement ASLR-04: Test with SW, HW, and Memory breakpoints
6. Add tests to TitanTestRunner.cpp includes
7. Build and verify compilation
<!-- SECTION:PLAN:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
Implemented 6 ASLR breakpoint restore tests (ASLR-01 to ASLR-06) verifying breakpoints work correctly with ASLR executables using RVA-based addressing.
<!-- SECTION:NOTES:END -->
