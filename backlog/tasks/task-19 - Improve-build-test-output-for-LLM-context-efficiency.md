---
id: task-19
title: Improve build/test output for LLM context efficiency
status: To Do
assignee: []
created_date: '2026-01-11 15:48'
labels:
  - tooling
  - context-engineering
dependencies: []
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Ensure all build and test tooling produces minimal, actionable output optimized for LLM context consumption. Every token in output goes into context and accumulates.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Build output: errors only, no progress spam
- [ ] #2 Test output: single summary line on success
- [ ] #3 Test failures: only failing test name, assertion, file:line
- [ ] #4 Exit codes: 0=success, non-zero=failure count
- [ ] #5 No verbose flags enabled by default
<!-- AC:END -->
