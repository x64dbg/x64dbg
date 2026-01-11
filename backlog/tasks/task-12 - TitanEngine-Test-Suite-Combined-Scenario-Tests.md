---
id: task-12
title: 'TitanEngine Test Suite: Combined Scenario Tests'
status: To Do
assignee: []
created_date: '2026-01-11 15:19'
labels:
  - testing
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Implement tests CB-01 through CB-10 covering complex scenarios with multiple breakpoint types, exceptions, and events.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 CB-01: SW BP -> step -> HW BP
- [ ] #2 CB-02: HW BP -> step -> Memory BP
- [ ] #3 CB-03: Exception -> continue -> BP
- [ ] #4 CB-04: DLL load -> set BP in DLL
- [ ] #5 CB-05: Thread create -> set BP -> thread hits
- [ ] #6 CB-06: Multiple exception types in sequence
- [ ] #7 CB-07: BP + thread exit
- [ ] #8 CB-08: Memory BP + SW BP same page
- [ ] #9 CB-09: Detach with active BPs
- [ ] #10 CB-10: Step over function that raises exception
<!-- AC:END -->
