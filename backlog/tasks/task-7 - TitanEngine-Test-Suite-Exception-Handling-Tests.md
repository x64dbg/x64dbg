---
id: task-7
title: 'TitanEngine Test Suite: Exception Handling Tests'
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
Implement tests EX-01 through EX-13 covering all exception types, first/second chance, and SEH interaction.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 EX-01: ACCESS_VIOLATION read
- [ ] #2 EX-02: ACCESS_VIOLATION write
- [ ] #3 EX-03: ACCESS_VIOLATION execute
- [ ] #4 EX-04: INT3 exception
- [ ] #5 EX-05: SINGLE_STEP exception
- [ ] #6 EX-06: DIV_BY_ZERO
- [ ] #7 EX-07: ILLEGAL_INSTRUCTION
- [ ] #8 EX-08: PRIVILEGED_INSTRUCTION
- [ ] #9 EX-09: STACK_OVERFLOW
- [ ] #10 EX-10: GUARD_PAGE
- [ ] #11 EX-11: SetNextDbgContinueStatus
- [ ] #12 EX-12: First-chance vs second-chance
- [ ] #13 EX-13: Exception in SEH handler
<!-- AC:END -->
