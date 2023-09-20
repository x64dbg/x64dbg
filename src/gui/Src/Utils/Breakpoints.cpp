#include "Breakpoints.h"
#include "EditBreakpointDialog.h"
#include "StringUtil.h"

Breakpoints::Breakpoints(QObject* parent) : QObject(parent)
{
}

/**
 * @brief       Set a new breakpoint according to the given type at the given address.
 *
 * @param[in]   type    Type of the breakpoint
 * @param[in]   va      Virtual Address
 *
 * @return      Nothing.
 */
void Breakpoints::setBP(BPXTYPE type, duint va)
{
    QString cmd = "";

    switch(type)
    {
    case bp_normal:
    {
        cmd = "bp " + ToPtrString(va);
    }
    break;

    case bp_hardware:
    {
        cmd = "bph " + ToPtrString(va);
    }
    break;

    case bp_memory:
    {
        cmd = "bpm " + ToPtrString(va);
    }
    break;

    default:
    {

    }
    break;
    }

    DbgCmdExecDirect(cmd);
}

/**
 * @brief       Enable breakpoint according to the given breakpoint descriptor.
 *
 * @param[in]   bp  Breakpoint descriptor
 *
 * @return      Nothing.
 */
void Breakpoints::enableBP(const BRIDGEBP & bp)
{
    QString cmd = "";

    if(bp.type == bp_hardware)
    {
        cmd = QString("bphwe \"%1\"").arg(ToPtrString(bp.addr));
    }
    else if(bp.type == bp_normal)
    {
        cmd = QString("be \"%1\"").arg(ToPtrString(bp.addr));
    }
    else if(bp.type == bp_memory)
    {
        cmd = QString("bpme \"%1\"").arg(ToPtrString(bp.addr));
    }
    else if(bp.type == bp_dll)
    {
        cmd = QString("LibrarianEnableBreakPoint \"%1\"").arg(QString(bp.mod));
    }
    else if(bp.type == bp_exception)
    {
        cmd = QString("EnableExceptionBPX \"%1\"").arg(ToPtrString(bp.addr));
    }

    DbgCmdExecDirect(cmd);
}

/**
 * @brief       Enable breakpoint that has been previously disabled according to its type and virtual address.
 *              If breakpoint was removed, this method has no effect.@n
 *              Breakpoint type is useful when there are several types of breakpoints on the same address.
 *              bp_none enables all breakpoints at the given address.
 *
 * @param[in]   type    Type of the breakpoint.
 * @param[in]   va      Virtual Address
 *
 * @return      Nothing.
 */
void Breakpoints::enableBP(BPXTYPE type, duint va)
{
    BPMAP bpList;

    // Get breakpoints list
    DbgGetBpList(type, &bpList);

    // Find breakpoint at address VA
    for(int i = 0; i < bpList.count; i++)
    {
        if(bpList.bp[i].addr == va)
        {
            enableBP(bpList.bp[i]);
        }
    }
    if(bpList.count)
        BridgeFree(bpList.bp);
}

/**
 * @brief       Disable breakpoint according to the given breakpoint descriptor.
 *
 * @param[in]   bp  Breakpoint descriptor
 *
 * @return      Nothing.
 */
void Breakpoints::disableBP(const BRIDGEBP & bp)
{
    QString cmd = "";

    if(bp.type == bp_hardware)
    {
        cmd = QString("bphwd \"%1\"").arg(ToPtrString(bp.addr));
    }
    else if(bp.type == bp_normal)
    {
        cmd = QString("bd \"%1\"").arg(ToPtrString(bp.addr));
    }
    else if(bp.type == bp_memory)
    {
        cmd = QString("bpmd \"%1\"").arg(ToPtrString(bp.addr));
    }
    else if(bp.type == bp_dll)
    {
        cmd = QString("LibrarianDisableBreakPoint \"%1\"").arg(QString(bp.mod));
    }
    else if(bp.type == bp_exception)
    {
        cmd = QString("DisableExceptionBPX \"%1\"").arg(ToPtrString(bp.addr));
    }

    DbgCmdExecDirect(cmd);
}

/**
 * @brief       Disable breakpoint that has been previously enabled according to its type and virtual address.
 *              If breakpoint was removed, this method has no effect.@n
 *              Breakpoint type is useful when there are several types of breakpoints on the same address.
 *              bp_none disbales all breakpoints at the given address.
 *
 * @param[in]   type    Type of the breakpoint.
 * @param[in]   va      Virtual Address
 *
 * @return      Nothing.
 */
void Breakpoints::disableBP(BPXTYPE type, duint va)
{
    BPMAP bpList;

    // Get breakpoints list
    DbgGetBpList(type, &bpList);

    // Find breakpoint at address VA
    for(int i = 0; i < bpList.count; i++)
    {
        if(bpList.bp[i].addr == va)
        {
            disableBP(bpList.bp[i]);
        }
    }
    if(bpList.count)
        BridgeFree(bpList.bp);
}

static QString getBpIdentifier(const BRIDGEBP & bp)
{
    if(*bp.mod)
    {
        auto modbase = DbgModBaseFromName(bp.mod);
        if(!modbase)
            return QString("\"%1\":$%2").arg(bp.mod).arg(ToHexString(bp.addr));
    }
    return ToPtrString(bp.addr);
}

/**
 * @brief       Remove breakpoint according to the given breakpoint descriptor.
 *
 * @param[in]   bp  Breakpoint descriptor
 *
 * @return      Nothing.
 */
void Breakpoints::removeBP(const BRIDGEBP & bp)
{
    QString cmd = "";

    switch(bp.type)
    {
    case bp_normal:
        cmd = QString("bc \"%1\"").arg(getBpIdentifier(bp));
        break;

    case bp_hardware:
        cmd = QString("bphc \"%1\"").arg(getBpIdentifier(bp));
        break;

    case bp_memory:
        cmd = QString("bpmc \"%1\"").arg(getBpIdentifier(bp));
        break;

    case bp_dll:
        cmd = QString("bcdll \"%1\"").arg(QString(bp.mod));
        break;

    case bp_exception:
        cmd = QString("DeleteExceptionBPX \"%1\"").arg(ToPtrString(bp.addr));
        break;

    default:
        break;
    }

    DbgCmdExecDirect(cmd);
}

/**
 * @brief       Remove breakpoint at the given given address and type
 *              If breakpoint doesn't exists, this method has no effect.@n
 *              Breakpoint type is useful when there are several types of breakpoints on the same address.
 *              bp_none disbales all breakpoints at the given address.
 *
 * @param[in]   type    Type of the breakpoint.
 * @param[in]   va      Virtual Address
 *
 * @return      Nothing.
 */
void Breakpoints::removeBP(BPXTYPE type, duint va)
{
    BPMAP bpList;

    // Get breakpoints list
    DbgGetBpList(type, &bpList);

    // Find breakpoint at address VA
    for(int i = 0; i < bpList.count; i++)
    {
        if(bpList.bp[i].addr == va)
        {
            removeBP(bpList.bp[i]);
        }
    }
    if(bpList.count)
        BridgeFree(bpList.bp);
}

void Breakpoints::removeBP(const QString & DLLName)
{
    BPMAP bpList;

    // Get breakpoints list
    DbgGetBpList(bp_dll, &bpList);

    // Find breakpoint at DLLName
    for(int i = 0; i < bpList.count; i++)
    {
        if(QString(bpList.bp[i].mod) == DLLName)
        {
            removeBP(bpList.bp[i]);
        }
    }
    if(bpList.count)
        BridgeFree(bpList.bp);
}

/**
 * @brief       Toggle the given breakpoint by disabling it when enabled.@n
 *              If breakpoint is initially active and enabled, it will be disabled.@n
 *              If breakpoint is initially active and disabled, it will stay disabled.@n
 *
 * @param[in]   bp  Breakpoint descriptor
 *
 * @return      Nothing.
 */
void Breakpoints::toggleBPByDisabling(const BRIDGEBP & bp)
{
    if(bp.enabled)
        disableBP(bp);
    else
        enableBP(bp);
}

/**
 * @brief       Toggle the given breakpoint by disabling it when enabled.@n
 *              If breakpoint is initially active and enabled, it will be disabled.@n
 *              If breakpoint is initially active and disabled, it will stay disabled.@n
 *              If breakpoint was previously removed, this method has no effect.@n
 *
 * @param[in]   type    Type of the breakpoint.
 * @param[in]   va      Virtual Address
 *
 * @return      Nothing.
 */
void Breakpoints::toggleBPByDisabling(BPXTYPE type, duint va)
{
    BPMAP bpList;

    // Get breakpoints list
    DbgGetBpList(type, &bpList);

    // Find breakpoint at address VA
    for(int i = 0; i < bpList.count; i++)
    {
        if(bpList.bp[i].addr == va)
        {
            toggleBPByDisabling(bpList.bp[i]);
        }
    }
    if(bpList.count)
        BridgeFree(bpList.bp);
}

void Breakpoints::toggleBPByDisabling(const QString & DLLName)
{
    BPMAP bpList;

    // Get breakpoints list
    DbgGetBpList(bp_dll, &bpList);

    // Find breakpoint at module name
    for(int i = 0; i < bpList.count; i++)
    {
        if(QString(bpList.bp[i].mod) == DLLName)
        {
            toggleBPByDisabling(bpList.bp[i]);
        }
    }
    if(bpList.count)
        BridgeFree(bpList.bp);
}

void Breakpoints::toggleAllBP(BPXTYPE type, bool bEnable)
{
    BPMAP bpList;

    // Get breakpoints list
    DbgGetBpList(type, &bpList);

    if(bEnable)
    {
        // Find breakpoint at address VA
        for(int i = 0; i < bpList.count; i++)
        {
            enableBP(bpList.bp[i]);
        }
    }
    else
    {
        // Find breakpoint at address VA
        for(int i = 0; i < bpList.count; i++)
        {
            disableBP(bpList.bp[i]);
        }
    }

    if(bpList.count)
        BridgeFree(bpList.bp);
}

/**
 * @brief       returns if a breakpoint is disabled or not
 *
 * @param[in]   type    Type of the breakpoint.
 * @param[in]   va      Virtual Address
 *
 * @return      enabled/disabled.
 */
BPXSTATE Breakpoints::BPState(BPXTYPE type, duint va)
{
    BPMAP bpList;
    BPXSTATE result = bp_non_existent;

    // Get breakpoints list
    DbgGetBpList(type, &bpList);

    // Find breakpoint at address VA
    for(int i = 0; i < bpList.count; i++)
    {
        if(bpList.bp[i].addr == va)
        {
            if(bpList.bp[i].enabled)
            {
                result = bp_enabled;
                break;
            }
            else
            {
                result = bp_disabled;
                break;
            }
        }
    }
    if(bpList.count)
        BridgeFree(bpList.bp);

    return result;
}

bool Breakpoints::BPTrival(BPXTYPE type, duint va)
{
    BPMAP bpList;
    bool trival = true;

    // Get breakpoints list
    DbgGetBpList(type, &bpList);

    // Find breakpoint at address VA
    for(int i = 0; i < bpList.count; i++)
    {
        BRIDGEBP & bp = bpList.bp[i];
        if(bp.addr == va)
        {
            trival = !(bp.breakCondition[0] || bp.logCondition[0] || bp.commandCondition[0] || bp.commandText[0] || bp.logText[0] || bp.name[0] || bp.fastResume || bp.silent);
            break;
        }
    }
    if(bpList.count)
        BridgeFree(bpList.bp);

    return trival;
}


/**
 * @brief       Toggle the given breakpoint by disabling it when enabled.@n
 *              If breakpoint is initially active and enabled, it will be disabled.@n
 *              If breakpoint is initially active and disabled, it will stay disabled.@n
 *              If breakpoint was previously removed, this method has no effect.@n
 *
 * @param[in]   type    Type of the breakpoint.
 * @param[in]   va      Virtual Address
 *
 * @return      Nothing.
 */
void Breakpoints::toggleBPByRemoving(BPXTYPE type, duint va)
{
    BPMAP bpList;
    bool normalWasRemoved = false;
    bool memoryWasRemoved = false;
    bool hardwareWasRemoved = false;

    // Get breakpoints list
    DbgGetBpList(type, &bpList);

    // Find breakpoints at address VA and remove them
    for(int i = 0; i < bpList.count; i++)
    {
        if(bpList.bp[i].addr == va)
        {
            removeBP(bpList.bp[i]);

            switch(bpList.bp[i].type)
            {
            case bp_normal:
                normalWasRemoved = true;
                break;
            case bp_memory:
                memoryWasRemoved = true;
                break;
            case bp_hardware:
                hardwareWasRemoved = true;
                break;
            default:
                break;
            }
        }
    }
    if(bpList.count)
        BridgeFree(bpList.bp);

    if((type == bp_none || type == bp_normal) && (normalWasRemoved == false))
    {
        setBP(bp_normal, va);
    }
    else if((type == bp_none || type == bp_memory) && (memoryWasRemoved == false))
    {
        setBP(bp_memory, va);
    }
    else if((type == bp_none || type == bp_hardware) && (hardwareWasRemoved == false))
    {
        setBP(bp_hardware, va);
    }
}

bool Breakpoints::editBP(BPXTYPE type, const QString & addrText, QWidget* widget, const QString & createCommand)
{
    BRIDGEBP bridgebp = {};
    bool found = false;
    if(type == bp_dll)
    {
        found = DbgFunctions()->GetBridgeBp(type, reinterpret_cast<duint>(addrText.toUtf8().constData()), &bridgebp);
    }
    else
    {
        found = DbgFunctions()->GetBridgeBp(type, (duint)addrText.toULongLong(nullptr, 16), &bridgebp);
    }

    if(!createCommand.isEmpty() && !found)
    {
        // Create a dummy BRIDGEBP to edit
        bridgebp.type = type;
        if(type == bp_dll)
        {
            strncpy_s(bridgebp.mod, addrText.toUtf8().constData(), _TRUNCATE);
        }
        else
        {
            bridgebp.addr = (duint)addrText.toULongLong(nullptr, 16);
        }
    }
    else if(!found)
    {
        // Fail if the breakpoint doesn't exist and we cannot create a new one
        return false;
    }

    EditBreakpointDialog dialog(widget, bridgebp);
    if(dialog.exec() != QDialog::Accepted)
        return false;

    auto bp = dialog.getBp();
    auto exec = [](const QString & command)
    {
        return DbgCmdExecDirect(command);
    };

    // Create the breakpoint if it didn't exist yet
    if(!createCommand.isEmpty() && !found)
    {
        if(!exec(createCommand))
        {
            return false;
        }
    }

    switch(type)
    {
    case bp_normal:
        exec(QString("SetBreakpointName %1, \"%2\"").arg(addrText).arg(bp.name));
        exec(QString("SetBreakpointCondition %1, \"%2\"").arg(addrText).arg(bp.breakCondition));
        exec(QString("SetBreakpointLog %1, \"%2\"").arg(addrText).arg(bp.logText));
        exec(QString("SetBreakpointLogCondition %1, \"%2\"").arg(addrText).arg(bp.logCondition));
        exec(QString("SetBreakpointCommand %1, \"%2\"").arg(addrText).arg(bp.commandText));
        exec(QString("SetBreakpointCommandCondition %1, \"%2\"").arg(addrText).arg(bp.commandCondition));
        exec(QString("ResetBreakpointHitCount %1, %2").arg(addrText).arg(ToPtrString(bp.hitCount)));
        exec(QString("SetBreakpointFastResume %1, %2").arg(addrText).arg(bp.fastResume));
        exec(QString("SetBreakpointSilent %1, %2").arg(addrText).arg(bp.silent));
        exec(QString("SetBreakpointSingleshoot %1, %2").arg(addrText).arg(bp.singleshoot));
        break;
    case bp_hardware:
        exec(QString("SetHardwareBreakpointName %1, \"%2\"").arg(addrText).arg(bp.name));
        exec(QString("SetHardwareBreakpointCondition %1, \"%2\"").arg(addrText).arg(bp.breakCondition));
        exec(QString("SetHardwareBreakpointLog %1, \"%2\"").arg(addrText).arg(bp.logText));
        exec(QString("SetHardwareBreakpointLogCondition %1, \"%2\"").arg(addrText).arg(bp.logCondition));
        exec(QString("SetHardwareBreakpointCommand %1, \"%2\"").arg(addrText).arg(bp.commandText));
        exec(QString("SetHardwareBreakpointCommandCondition %1, \"%2\"").arg(addrText).arg(bp.commandCondition));
        exec(QString("ResetHardwareBreakpointHitCount %1, %2").arg(addrText).arg(ToPtrString(bp.hitCount)));
        exec(QString("SetHardwareBreakpointFastResume %1, %2").arg(addrText).arg(bp.fastResume));
        exec(QString("SetHardwareBreakpointSilent %1, %2").arg(addrText).arg(bp.silent));
        exec(QString("SetHardwareBreakpointSingleshoot %1, %2").arg(addrText).arg(bp.singleshoot));
        break;
    case bp_memory:
        exec(QString("SetMemoryBreakpointName %1, \"\"%2\"\"").arg(addrText).arg(bp.name));
        exec(QString("SetMemoryBreakpointCondition %1, \"%2\"").arg(addrText).arg(bp.breakCondition));
        exec(QString("SetMemoryBreakpointLog %1, \"%2\"").arg(addrText).arg(bp.logText));
        exec(QString("SetMemoryBreakpointLogCondition %1, \"%2\"").arg(addrText).arg(bp.logCondition));
        exec(QString("SetMemoryBreakpointCommand %1, \"%2\"").arg(addrText).arg(bp.commandText));
        exec(QString("SetMemoryBreakpointCommandCondition %1, \"%2\"").arg(addrText).arg(bp.commandCondition));
        exec(QString("ResetMemoryBreakpointHitCount %1, %2").arg(addrText).arg(ToPtrString(bp.hitCount)));
        exec(QString("SetMemoryBreakpointFastResume %1, %2").arg(addrText).arg(bp.fastResume));
        exec(QString("SetMemoryBreakpointSilent %1, %2").arg(addrText).arg(bp.silent));
        exec(QString("SetMemoryBreakpointSingleshoot %1, %2").arg(addrText).arg(bp.singleshoot));
        break;
    case bp_dll:
        exec(QString("SetLibrarianBreakpointName \"%1\", \"\"%2\"\"").arg(addrText).arg(bp.name));
        exec(QString("SetLibrarianBreakpointCondition \"%1\", \"%2\"").arg(addrText).arg(bp.breakCondition));
        exec(QString("SetLibrarianBreakpointLog \"%1\", \"%2\"").arg(addrText).arg(bp.logText));
        exec(QString("SetLibrarianBreakpointLogCondition \"%1\", \"%2\"").arg(addrText).arg(bp.logCondition));
        exec(QString("SetLibrarianBreakpointCommand \"%1\", \"%2\"").arg(addrText).arg(bp.commandText));
        exec(QString("SetLibrarianBreakpointCommandCondition \"%1\", \"%2\"").arg(addrText).arg(bp.commandCondition));
        exec(QString("ResetLibrarianBreakpointHitCount \"%1\", %2").arg(addrText).arg(ToPtrString(bp.hitCount)));
        exec(QString("SetLibrarianBreakpointFastResume \"%1\", %2").arg(addrText).arg(bp.fastResume));
        exec(QString("SetLibrarianBreakpointSilent \"%1\", %2").arg(addrText).arg(bp.silent));
        exec(QString("SetLibrarianBreakpointSingleshoot \"%1\", %2").arg(addrText).arg(bp.singleshoot));
        break;
    case bp_exception:
        exec(QString("SetExceptionBreakpointName %1, \"%2\"").arg(addrText).arg(bp.name));
        exec(QString("SetExceptionBreakpointCondition %1, \"%2\"").arg(addrText).arg(bp.breakCondition));
        exec(QString("SetExceptionBreakpointLog %1, \"%2\"").arg(addrText).arg(bp.logText));
        exec(QString("SetExceptionBreakpointLogCondition %1, \"%2\"").arg(addrText).arg(bp.logCondition));
        exec(QString("SetExceptionBreakpointCommand %1, \"%2\"").arg(addrText).arg(bp.commandText));
        exec(QString("SetExceptionBreakpointCommandCondition %1, \"%2\"").arg(addrText).arg(bp.commandCondition));
        exec(QString("ResetExceptionBreakpointHitCount %1, %2").arg(addrText).arg(ToPtrString(bp.hitCount)));
        exec(QString("SetExceptionBreakpointFastResume %1, %2").arg(addrText).arg(bp.fastResume));
        exec(QString("SetExceptionBreakpointSilent %1, %2").arg(addrText).arg(bp.silent));
        exec(QString("SetExceptionBreakpointSingleshoot %1, %2").arg(addrText).arg(bp.singleshoot));
        break;
    default:
        return false;
    }
    GuiUpdateBreakpointsView();
    return true;
}
