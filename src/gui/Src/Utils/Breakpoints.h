#pragma once

#include "Bridge.h"
#include <type_traits>

enum BPXSTATE
{
    bp_enabled = 0,
    bp_disabled = 1,
    bp_non_existent = -1
};

class Breakpoints
{
    Breakpoints() = delete;

public:
    struct Data
    {
        // Read-only
        bool error = false;
        BP_REF ref = {};
        BPXTYPE type = bp_none;

        Data() = default;
        explicit Data(const BP_REF & ref);

        duint addr = 0;
        QString module;
        bool active = false;
        bool enabled = false; // TODO: allow modification?
        BPHWSIZE hwSize = {};
        duint typeEx = 0;

        // Data that gets modified
        QString breakCondition;
        QString logText;
        QString logCondition;
        QString commandText;
        QString commandCondition;
        QString name;
        duint hitCount = 0;
        QString logFile;
        bool singleshoot = false;
        bool silent = false;
        bool fastResume = false;

        // Helper functions
        void getField(BP_FIELD field, QString & value);
        void getField(BP_FIELD field, duint & value);
        void getField(BP_FIELD field, bool & value);

        template<class T, typename = typename std::enable_if< std::is_enum<T>::value, T >::type>
        void getField(BP_FIELD field, T & value)
        {
            duint n = 0;
            getField(field, n);
            value = (T)n;
        }

        bool read();
    };

    static void setBP(BPXTYPE type, duint va);
    static void enableBP(const Data & bp);
    static void disableBP(const Data & bp);
    static void removeBP(const Data & bp);
    static void toggleBPByDisabling(const Data & bp);
    static BPXSTATE BPState(BPXTYPE type, duint va);
    static bool BPTrival(BPXTYPE type, duint va);
    static bool editBP(BPXTYPE type, const QString & module, duint address, QWidget* widget, const QString & createCommand = QString());
};
