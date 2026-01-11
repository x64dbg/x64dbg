---
id: task-10
title: 'TitanEngine Test Suite: Context Operation Tests'
status: Done
assignee: []
created_date: '2026-01-11 15:18'
updated_date: '2026-01-11 20:24'
labels:
  - testing
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Implement tests CX-01 through CX-10 covering register reading/writing for GPR, FPU, XMM, YMM, DR, and flags.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 CX-01: Read all GPRs
- [ ] #2 CX-02: Write GPR, continue
- [ ] #3 CX-03: Read/Write IP
- [ ] #4 CX-04: Read/Write FLAGS
- [ ] #5 CX-05: Read/Write DR0-DR7
- [ ] #6 CX-06: FPU registers
- [ ] #7 CX-07: XMM registers
- [ ] #8 CX-08: YMM registers (AVX)
- [ ] #9 CX-09: Segment registers
- [ ] #10 CX-10: Full context round-trip
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
Implemented 10 context tests (CX-01 to CX-10) covering get/set CIP, CSP, general purpose registers, flags, and debug registers.
<!-- SECTION:NOTES:END -->
