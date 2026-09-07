# database callback test fixture

This directory contains the shared executable and plugin harness for validating database operation callbacks emitted.

The fixture focuses on verifying that database mutations generate the correct `CB_DBOPERATION` callback payloads, including:

* operation type
* item type
* target address
* module hash
* optional text payload
* optional function end address
* optional loop nesting depth
* single vs bulk callback behavior

## Why

The debugger supports both singular and batch-style database mutations for items such as:

* bookmarks
* comments
* labels
* functions
* loops
* arguments
* address colors

These tests ensure callback consumers can reliably distinguish:

* single-item operations from bulk operations
* add/remove events
* item categories
* associated metadata and payloads

---

# Executable Layout

`target.cpp` builds a small target module exposing both code and data symbols used by the tests.

Exports include:

* `VariableTarget`
* `FunctionTarget`

## Bookmark Tests

### `bookmark-single`

Validates that singular bookmark operations emit exactly one non-bulk callback.

### `bookmark-batch`

Validates that batch bookmark operations emit bulk callbacks.

---

## Comment Tests

### `comment-single`

Validates that singular comment operations emit exactly one non-bulk callback.

### `comment-batch`

Validates that batch comment operations emit bulk callbacks.

### `comment-reentrant`

Validates that an operation triggered reentrantly by a batch callback is delivered only
after the in-flight batch completes.

---

## Label Tests

### `label-single`

Validates that singular label operations emit exactly one non-bulk callback.

### `label-batch`

Validates that batch label operations emit bulk callbacks.

---

## Address Color Tests

### `addresscolor-single`

Validates that setting and deleting one address color emit non-bulk callbacks with the configured color preset.

### `addresscolor-batch`

Validates that setting a range emits one callback containing its inclusive start/end bounds, and clearing address colors emits a range removal.

### `addresscolor-split`

Validates that overwriting or deleting part of a color range preserves the unaffected interval fragments and emits callbacks for the normalized storage changes.

### `addresscolor-load`

Validates that loading a saved address-color range emits `CB_DBLOADOPERATION` with its inclusive bounds and configured color preset.

---

## Database Load Tests

### `db-load-reentrant`

Validates that a regular mutation triggered from `CB_DBLOADOPERATION` is emitted as
`CB_DBOPERATION`. Cross-type callback ordering is intentionally not asserted.

---

## Loop Tests

Loops are range items (start/end) that additionally carry a nesting `depth` and a
`parent`. The `depth` field is surfaced through the callback payload and asserted here.

### `loop-single`

Validates that singular loop operations emit exactly one non-bulk callback, including
the correct end address and depth (0 for a top-level loop).

### `loop-batch`

Validates that batch loop operations (e.g. `loopclear`) emit bulk callbacks.

---

## Argument Tests

Arguments are range items (start/end) with an instruction count, but no depth or parent.

### `argument-single`

Validates that singular argument operations emit exactly one non-bulk callback, including
the correct end address.

### `argument-batch`

Validates that batch argument operations (e.g. `argumentclear`) emit bulk callbacks.
