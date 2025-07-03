PLUG_CB_RESUMEDEBUG
===================
Called after the debugger has been unlocked to resume (outside of the debug loop:

::

    struct PLUG_CB_RESUMEDEBUG
    {
        void* reserved;
    };
