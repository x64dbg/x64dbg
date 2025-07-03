# GuiIsUpdateDisabled

Returns the status of the internal update flag, which can be disabled via GuiUpdateDisable function or enabled vis the GuiUpdateEnable function.

```c++
bool GuiIsUpdateDisabled()
```

## Parameters

This function has no parameters.

## Return Value

Returns a boolean value indicating if the internal update flag is set to disabled. If it is set to disabled the value is TRUE otherwise updates are enabled and the value is FALSE.

## Example

```c++
bool bUpdate = GuiIsUpdateDisabled();
```

## Related functions

- [GuiUpdateAllViews](./GuiUpdateAllViews.md)
- [GuiUpdateArgumentWidget](./GuiUpdateArgumentWidget.md)
- [GuiUpdateBreakpointsView](./GuiUpdateBreakpointsView.md)
- [GuiUpdateCallStack](./GuiUpdateCallStack.md)
- [GuiUpdateDisable](./GuiUpdateDisable.md)
- [GuiUpdateDisassemblyView](./GuiUpdateDisassemblyView.md)
- [GuiUpdateDumpView](./GuiUpdateDumpView.md)
- [GuiUpdateEnable](./GuiUpdateEnable.md)
- [GuiUpdateGraphView](./GuiUpdateGraphView.md)
- [GuiUpdateMemoryView](./GuiUpdateMemoryView.md)
- [GuiUpdatePatches](./GuiUpdatePatches.md)
- [GuiUpdateRegisterView](./GuiUpdateRegisterView.md)
- [GuiUpdateSEHChain](./GuiUpdateSEHChain.md)
- [GuiUpdateSideBar](./GuiUpdateSideBar.md)
- [GuiUpdateThreadView](./GuiUpdateThreadView.md)
- [GuiUpdateTimeWastedCounter](./GuiUpdateTimeWastedCounter.md)
- [GuiUpdateWatchView](./GuiUpdateWatchView.md)
- [GuiUpdateWindowTitle](./GuiUpdateWindowTitle.md)