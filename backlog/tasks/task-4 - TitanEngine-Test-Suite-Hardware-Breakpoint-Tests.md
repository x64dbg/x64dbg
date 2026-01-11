---
id: task-4
title: 'TitanEngine Test Suite: Hardware Breakpoint Tests'
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
Implement tests HW-01 through HW-10 covering execute, write, read/write breakpoints across all sizes and DR registers.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 HW-01: Execute BP (DR0-DR3)
- [ ] #2 HW-02: Write BP size 1
- [ ] #3 HW-03: Write BP size 2
- [ ] #4 HW-04: Write BP size 4
- [ ] #5 HW-05: Write BP size 8 (x64)
- [ ] #6 HW-06: Read/Write BP
- [ ] #7 HW-07: All 4 DR registers used
- [ ] #8 HW-08: GetUnusedHardwareBreakPointRegister
- [ ] #9 HW-09: Delete HW BP
- [ ] #10 HW-10: HW BP + SW BP same function
<!-- AC:END -->
