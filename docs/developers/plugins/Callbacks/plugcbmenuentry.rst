PLUG_CB_MENUENTRY
=================
Called when a menu entry created by the plugin has been clicked, the GUI will resume when this callback returns:

::

    struct PLUG_CB_MENUENTRY
    {
        int hEntry;
    };
