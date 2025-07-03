PLUG_CB_FILTERSYMBOL
====================
Called before a symbol is emitted to the automatic labels. Set `retval` to `false` if you want to filter the symbol.

::

    struct PLUG_CB_FILTERSYMBOL
    {
        const char* symbol;
        bool retval;
    };
