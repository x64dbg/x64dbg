# issue3932

Regression test for x64dbg issue #3932.

The driver starts `issue3932.exe` through a differently-cased path plus the argument
`issue3932-payload`, attaches with `-p`, and the plugin checks that
`DbgFunctions()->GetProcessList` reports `szExeArgs` as that payload instead of a
truncated command line.
