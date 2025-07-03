PLUG_CB_TRACEEXECUTE
====================
Called during conditional tracing. Set the `stop` member to `true` to stop tracing.

::

    struct PLUG_CB_TRACEEXECUTE
    {
        duint cip;
        bool stop;
    };
