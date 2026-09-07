#include <stdio.h>
#define WIN32_NO_STATUS
#include <Windows.h>

#include <array>
#include <atomic>
#include <cstring>
#include <string>

#include "_plugins.h"
#include "bridgemain.h"
#include "database.h"

struct TestOperation
{
    DbOperation operation;
    std::string text; // keep text alive
    bool bulk;
};

namespace
{
    int gPluginHandle = 0;
    std::vector<TestOperation> operations;
    bool gOpLogEnabled = false;
    bool gReentrantCommentSetPending = false;
    duint gReentrantCommentAddress = 0;
    std::string gReentrantCommentText;
    bool gDbLoadCommentSetPending = false;
    bool gDbLoadCommentSetSeen = false;
    duint gDbLoadCommentAddress = 0;
    std::string gDbLoadCommentText;

    void resetState(bool preserveDbLoadCommentSet = false)
    {
        operations.clear();
        gReentrantCommentSetPending = false;
        gDbLoadCommentSetSeen = false;
        if(!preserveDbLoadCommentSet)
        {
            gDbLoadCommentSetPending = false;
            gDbLoadCommentAddress = 0;
            gDbLoadCommentText.clear();
        }
    }

    void printOperation(const DbOperation & op, bool dbLoad)
    {
        std::string text = op.opType == DbOperationTypeAdd ? "[+]" : "[-]";
        char temp[256] = "";
        {
            text += " address=0x";
            sprintf_s(temp, "%zx", op.address);
            text += temp;
        }

        {
            char module[MAX_MODULE_SIZE] = "";
            auto hasModule = DbgFunctions()->ModNameFromHash(op.modhash, module);
            text += ", module=";
            if(hasModule)
                text += module;
            else
                text += "<none>";
        }

        text += ", manual=";
        text += op.manual ? "true" : "false";
        text += ", dbload=";
        text += dbLoad ? "true" : "false";

        auto printRange = [&](const char* type, duint end)
        {
            text += type;
            text += ", end=0x";
            sprintf_s(temp, "%zx", end);
            text += temp;
        };

        auto printIcount = [&](uint32_t icount)
        {
            text += ", icount=";
            sprintf_s(temp, "%u", icount);
            text += temp;
        };

        text += " type=";
        switch(op.itemType)
        {
        case DbItemTypeBookmark:
            text += "bookmark";
            break;
        case DbItemTypeLabel:
            text += "label, text=";
            text += op.label.text ? op.label.text : "<nullptr>";
            break;
        case DbItemTypeComment:
            text += "comment, text=";
            text += op.comment.text ? op.comment.text : "<nullptr>";
            break;
        case DbItemTypeFunction:
            printRange("function", op.function.end);
            text += ", parent=0x";
            sprintf_s(temp, "%zx", op.function.parent);
            text += temp;
            printIcount(op.function.icount);
            break;
        case DbItemTypeLoop:
            printRange("loop", op.loop.end);
            text += ", parent=0x";
            sprintf_s(temp, "%zx", op.loop.parent);
            text += temp;
            printIcount(op.loop.icount);
            text += ", depth=";
            sprintf_s(temp, "%d", op.loop.depth);
            text += temp;
            break;
        case DbItemTypeArgument:
            printRange("argument", op.argument.end);
            printIcount(op.argument.icount);
            break;
        case DbItemTypeAddressColor:
            printRange("addresscolor", op.addressColor.end);
            text += ", color=";
            sprintf_s(temp, "%zu", op.addressColor.color);
            text += temp;
            break;
        }

        _plugin_logputs(text.c_str());
    }

    void cbPlugin(CBTYPE cbType, void* callbackInfo)
    {
        if(cbType == CB_INITDEBUG)
        {
            resetState(true);
            return;
        }

        if(cbType == CB_DBOPERATION || cbType == CB_DBLOADOPERATION)
        {
            auto info = (PLUG_CB_DBOPERATION*) callbackInfo;

            if(cbType == CB_DBOPERATION && gReentrantCommentSetPending)
            {
                gReentrantCommentSetPending = false;
                char command[MAX_SETTING_SIZE];
                sprintf_s(command, "commentset 0x%llX, \"%s\"", (unsigned long long)gReentrantCommentAddress, gReentrantCommentText.c_str());
                _plugin_testassert(DbgCmdExecDirect(command), "reentrant command failed: %s", command);
            }

            if(cbType == CB_DBLOADOPERATION && gDbLoadCommentSetPending)
            {
                gDbLoadCommentSetPending = false;
                char command[MAX_SETTING_SIZE];
                sprintf_s(command, "commentset 0x%llX, \"%s\"", (unsigned long long)gDbLoadCommentAddress, gDbLoadCommentText.c_str());
                _plugin_testassert(DbgCmdExecDirect(command), "database-load reentrant command failed: %s", command);
            }

            for(size_t i = 0; i < info->count; i ++)
            {
                const DbOperation & operation = *info->operations[i];

                auto expectedAddress = gDbLoadCommentAddress;
                if(operation.modhash != 0)
                    expectedAddress -= DbgFunctions()->ModBaseFromAddr(expectedAddress);
                if(cbType == CB_DBOPERATION && operation.opType == DbOperationTypeAdd &&
                        operation.itemType == DbItemTypeComment && operation.address == expectedAddress &&
                        operation.comment.text != nullptr && gDbLoadCommentText == operation.comment.text)
                    gDbLoadCommentSetSeen = true;

                if(gOpLogEnabled)
                    printOperation(operation, cbType == CB_DBLOADOPERATION);

                std::string saved;

                if(operation.itemType == DbItemTypeComment && operation.comment.text)
                    saved = operation.comment.text;
                else if(operation.itemType == DbItemTypeLabel && operation.label.text)
                    saved = operation.label.text;

                bool is_bulk = info->count > 1;
                operations.push_back({operation, saved, is_bulk});
            }

            return;
        }
    }

    duint evalExpr(const char* expr)
    {
        return expr ? DbgValFromString(expr) : 0;
    }

    bool cbReset(int, char**)
    {
        resetState();
        return _plugin_testassert(true, "state reset");
    }

    bool getOpType(char s, DbOperationType & type)
    {
        if(s == 'a')
            type = DbOperationTypeAdd;
        else if(s == 'r')
            type = DbOperationTypeRemove;
        else
            return false;

        return true;
    }

    bool getItemType(char s, DbItemType & type)
    {
        if(s == 'f')
            type = DbItemTypeFunction;
        else if(s == 'l')
            type = DbItemTypeLabel;
        else if(s == 'c')
            type = DbItemTypeComment;
        else if(s == 'b')
            type = DbItemTypeBookmark;
        else if(s == 'p')
            type = DbItemTypeLoop;
        else if(s == 'g')
            type = DbItemTypeArgument;
        else if(s == 'o')
            type = DbItemTypeAddressColor;
        else
            return false;

        return true;
    }

    bool cbAssertLastOperation(int argc, char** argv)
    {
        if(argc < 6)
            return false;

        if(!_plugin_testassert(operations.size() > 0, "no operations in memory"))
            return false;

        TestOperation & last = operations.back();

        const duint modhash = evalExpr(argv[1]);
        if(!_plugin_testassert(modhash == last.operation.modhash, "modhash passed to callback doesn't match (%llu != %llu)", (unsigned long long)last.operation.modhash, (unsigned long long)modhash))
            return false;

        const duint target = evalExpr(argv[2]);
        if(!_plugin_testassert(target != 0, "failed to resolve target expression '%s'", argv[2]))
            return false;

        DbItemType itemType;
        if(!_plugin_testassert(strlen(argv[3]) && getItemType(argv[3][0], itemType), "failed to resolve item type '%s'", argv[3]))
            return false;

        if(!_plugin_testassert(last.operation.itemType == itemType, "item type passed to callback doesn't match (%d != %d)", last.operation.itemType, itemType))
            return false;

        DbOperationType opType;
        if(!_plugin_testassert(strlen(argv[4]) && getOpType(argv[4][0], opType), "failed to resolve op type '%s'", argv[4]))
            return false;

        if(!_plugin_testassert(last.operation.opType == opType, "operation type passed to callback doesn't match (%d != %d)", last.operation.opType, opType))
            return false;

        bool bulk = (bool) evalExpr(argv[5]);

        const char* text = NULL;
        if((itemType == DbItemTypeLabel || itemType == DbItemTypeComment) && argc >= 7)
        {
            text = argv[6];
            if(!_plugin_testassert(last.text == text, "text passed to callback doesn't match (%s != %s)", last.text.c_str(), text))
                return false;
        }

        if((itemType == DbItemTypeFunction || itemType == DbItemTypeArgument || itemType == DbItemTypeLoop || itemType == DbItemTypeAddressColor) && argc >= 7)
        {
            const duint expectedEnd = evalExpr(argv[6]);
            duint actualEnd = 0;
            switch(itemType)
            {
            case DbItemTypeFunction:
                actualEnd = last.operation.function.end;
                break;
            case DbItemTypeArgument:
                actualEnd = last.operation.argument.end;
                break;
            case DbItemTypeLoop:
                actualEnd = last.operation.loop.end;
                break;
            case DbItemTypeAddressColor:
                actualEnd = last.operation.addressColor.end;
                break;
            default:
                break;
            }
            if(!_plugin_testassert(actualEnd == expectedEnd, "end passed to callback doesn't match (%llu != %llu)", (unsigned long long)actualEnd, (unsigned long long)expectedEnd))
                return false;
        }

        if(itemType == DbItemTypeLoop && argc >= 8)
        {
            const int depth = (int) evalExpr(argv[7]);
            if(!_plugin_testassert(last.operation.loop.depth == depth, "depth passed to callback doesn't match (%d != %d)", last.operation.loop.depth, depth))
                return false;
        }

        if(itemType == DbItemTypeAddressColor && argc >= 8)
        {
            const duint color = evalExpr(argv[7]);
            if(!_plugin_testassert(last.operation.addressColor.color == color, "color passed to callback doesn't match (%llu != %llu)", (unsigned long long)last.operation.addressColor.color, (unsigned long long)color))
                return false;
        }

        if(!_plugin_testassert(last.operation.address == target, "address passed to callback doesn't match (%llu != %llu)", (unsigned long long)last.operation.address, (unsigned long long)target))
            return false;

        if(!_plugin_testassert(last.bulk == bulk, "bulk passed to callback doesn't match (%d != %d)", last.bulk, bulk))
            return false;
        return true;
    }

    bool cbAssertOperationsSize(int argc, char** argv)
    {
        if(argc < 2)
            return false;

        const duint size = evalExpr(argv[1]);
        if(!_plugin_testassert(size == operations.size(), "total operations size doesn't match (%llu != %llu)", (unsigned long long)size, (unsigned long long)operations.size()))
            return false;

        return true;
    }

    bool cbAssertAddressColor(int argc, char** argv)
    {
        if(argc != 3)
            return false;

        const duint address = evalExpr(argv[1]);
        const unsigned int expected = (unsigned int)evalExpr(argv[2]);
        unsigned int actual = 0;
        return _plugin_testassert(DbgGetAddressColorAt(address, &actual), "no address color at %p", (void*)address) &&
               _plugin_testassert(actual == expected, "address color doesn't match (%u != %u)", actual, expected);
    }

    bool cbAssertNoAddressColor(int argc, char** argv)
    {
        if(argc != 2)
            return false;

        const duint address = evalExpr(argv[1]);
        unsigned int color = 0;
        return _plugin_testassert(!DbgGetAddressColorAt(address, &color), "unexpected address color %u at %p", color, (void*)address);
    }

    bool cbOpLog(int argc, char** argv)
    {
        bool newState = !gOpLogEnabled;
        if(argc > 1)
            newState = evalExpr(argv[1]);
        gOpLogEnabled = newState;
        _plugin_logprintf("oplog state: %s\n", gOpLogEnabled ? "on" : "off");
        return true;
    }

    bool cbReentrantCommentSet(int argc, char** argv)
    {
        if(argc != 3)
            return false;

        gReentrantCommentAddress = evalExpr(argv[1]);
        gReentrantCommentText = argv[2];
        gReentrantCommentSetPending = true;
        return _plugin_testassert(gReentrantCommentAddress != 0, "failed to resolve reentrant comment address '%s'", argv[1]);
    }

    bool cbDbLoadCommentSet(int argc, char** argv)
    {
        if(argc != 3)
            return false;

        gDbLoadCommentAddress = evalExpr(argv[1]);
        gDbLoadCommentText = argv[2];
        gDbLoadCommentSetPending = true;
        gDbLoadCommentSetSeen = false;
        return _plugin_testassert(gDbLoadCommentAddress != 0, "failed to resolve database-load comment address '%s'", argv[1]);
    }

    bool cbAssertDbLoadCommentSet(int, char**)
    {
        return _plugin_testassert(!gDbLoadCommentSetPending, "database-load callback was not triggered") &&
               _plugin_testassert(gDbLoadCommentSetSeen, "regular database operation was not emitted as CB_DBOPERATION");
    }
}

extern "C" __declspec(dllexport) bool pluginit(PLUG_INITSTRUCT* initStruct)
{
    initStruct->pluginVersion = 1;
    initStruct->sdkVersion = PLUG_SDKVERSION;
    strncpy_s(initStruct->pluginName, sizeof(initStruct->pluginName), X64DBG_TEST_NAME, _TRUNCATE);
    gPluginHandle = initStruct->pluginHandle;
    _plugin_registercallback(gPluginHandle, CB_DBOPERATION, cbPlugin);
    _plugin_registercallback(gPluginHandle, CB_DBLOADOPERATION, cbPlugin);
    _plugin_registercallback(gPluginHandle, CB_INITDEBUG, cbPlugin);
    _plugin_registercommand(gPluginHandle, "dbreset", cbReset, false);
    _plugin_registercommand(gPluginHandle, "dboplog", cbOpLog, false);
    _plugin_registercommand(gPluginHandle, "dbreentrantcommentset", cbReentrantCommentSet, false);
    _plugin_registercommand(gPluginHandle, "dbloadcommentset", cbDbLoadCommentSet, false);
    _plugin_registercommand(gPluginHandle, "assertdbloadcommentset", cbAssertDbLoadCommentSet, false);
    _plugin_registercommand(gPluginHandle, "assertlastop", cbAssertLastOperation, false);
    _plugin_registercommand(gPluginHandle, "assertopsize", cbAssertOperationsSize, false);
    _plugin_registercommand(gPluginHandle, "assertaddresscolor", cbAssertAddressColor, false);
    _plugin_registercommand(gPluginHandle, "assertnoaddresscolor", cbAssertNoAddressColor, false);
    return true;
}

extern "C" __declspec(dllexport) void plugstop()
{
}

extern "C" __declspec(dllexport) void plugsetup(PLUG_SETUPSTRUCT*)
{
}
