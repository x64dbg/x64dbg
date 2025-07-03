# GuiUpdateWindowTitle

Updates the x64dbg window title with a string to be appended to the title text. Typically the string is a filename.

```c++
void GuiUpdateWindowTitle(const char* filename)
```

## Parameters

`filename` a const char variable to be appended to the x64dbg title bar.

## Return Value

This function does not return a value.

## Example

```c++
GuiUpdateWindowTitle("");
GuiUpdateWindowTitle(szFileName);
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