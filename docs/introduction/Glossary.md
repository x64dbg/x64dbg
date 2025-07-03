# Glossary

This section describes various terms and concepts used by x64dbg.

-  **Breakpoint** A breakpoint defines a condition when the debuggee should be paused. There are 5 types of breakpoint, namely software breakpoint, hardware breakpoint, memory breakpoint, DLL breakpoint and exception breakpoint.
-  **Conditional Breakpoint** A conditional breakpoint lets you define some simple operations that executes automatically when the breakpoint is hit, and then conditionally resumes program execution. See [documentation for conditional breakpoint](./ConditionalBreakpoint.md) for more information.
-  **Conditional Tracing** Conditional tracing lets you execute the program step-by-step, and pause when the specified condition is met. See [documentation for conditional tracing](./ConditionalTracing.md) for more information.
-  **DLL Breakpoint** A DLL breakpoint specifies the name of a DLL. When the DLL is loaded or unloaded, the debuggee will be paused.
-  **Trace recording** A trace recording is a log of all traced instructions, typically displayed in the [Trace View](../gui/views/Trace.md).
-  **Trace coverage** Trace coverage records if and how many times an instruction has been executed. An instruction that has been covered before will be displayed in a different background color (by default green). It is analogous to leaving footprints while exploring a maze.
