PLUG_CB_LOADSAVEDB
==================
Load or save data to database. Data is retrieved or stored or retrieved in a JSON format:

Two constants are defined in the _plugins.h file for the loadSaveType:

PLUG_DB_LOADSAVE_DATA
PLUG_DB_LOADSAVE_ALL

::

    struct PLUG_CB_LOADSAVEDB
    {
        json_t* root;
        int loadSaveType;
    };
