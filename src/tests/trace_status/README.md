# Trace status

The plugin exposes `DbgFunctions()->IsTracing` as the `istracing()` expression function.

The script starts a conditional trace that stops as soon as `istracing()` returns 1. The test fails if `IsTracing` does not report the trace as active while tracing, or if it still reports it after the trace stopped.
