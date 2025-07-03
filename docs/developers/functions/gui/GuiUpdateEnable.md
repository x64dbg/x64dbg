# GuiUpdateEnable

Sets an internal variable to enable (or re-enable if previously disabled via GuiUpdateDisable) the refresh of all views. The updateNow parameter can be used to force the update straight away, otherwise the updates of views will continue as per normal message scheduling, now that they have been enabled with GuiUpdateEnable.

```c++
void GuiUpdateEnable(bool updateNow);
```

## Parameters

`updateNow` is a boolean value indicating if the update of all views should occur straight away.

## Return Value

This function does not return a value.

## Example

```c++
GuiUpdateEnable(bool updateNow);
```

## Related functions

- [GuiIsUpdateDisabled](./GuiIsUpdateDisabled.md)
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