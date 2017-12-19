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
    QString wCmd = "";

    switch(type)
    {
    case bp_normal:
    {
        wCmd = "bp " + ToPtrString(va);
    }
    break;

    case bp_hardware:
    {
        wCmd = "bph " + ToPtrString(va);
    }
    break;

    case bp_memory:
    {
        wCmd = "bpm " + ToPtrString(va);
    }
    break;

    default:
    {

    }
    break;
    }

    DbgCmdExec(wCmd.toUtf8().constData());
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
    QString wCmd = "";

    if(bp.type == bp_hardware)
    {
        wCmd = QString("bphwe \"%1\"").arg(ToPtrString(bp.addr));
    }
    else if(bp.type == bp_normal)
    {
        wCmd = QString("be \"%1\"").arg(ToPtrString(bp.addr));
    }
    else if(bp.type == bp_memory)
    {
        wCmd = QString("bpme \"%1\"").arg(ToPtrString(bp.addr));
    }
    else if(bp.type == bp_dll)
    {
        wCmd = QString("LibrarianEnableBreakPoint \"%1\"").arg(QString(bp.mod));
    }
    else if(bp.type == bp_exception)
    {
        wCmd = QString("EnableExceptionBPX \"%1\"").arg(ToPtrString(bp.addr));
    }

    DbgCmdExec(wCmd.toUtf8().constData());
}

/**
 * @brief       Enable breakpoint that has been previously disabled according to its type and virtual address.
 *              If breakpoint was removed, this method has no effect.@n
 *              Breakpoint type is usefull when there are several types of breakpoints on the same address.
 *              bp_none enables all breakpoints at the given address.
 *
 * @param[in]   type    Type of the breakpoint.
 * @param[in]   va      Virtual Address
 *
 * @return      Nothing.
 */
void Breakpoints::enableBP(BPXTYPE type, duint va)
{
    BPMAP wBPList;

    // Get breakpoints list
    DbgGetBpList(type, &wBPList);

    // Find breakpoint at address VA
    for(int wI = 0; wI < wBPList.count; wI++)
    {
        if(wBPList.bp[wI].addr == va)
        {
            enableBP(wBPList.bp[wI]);
        }
    }
    if(wBPList.count)
        BridgeFree(wBPList.bp);
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
    QString wCmd = "";

    if(bp.type == bp_hardware)
    {
        wCmd = QString("bphwd \"%1\"").arg(ToPtrString(bp.addr));
    }
    else if(bp.type == bp_normal)
    {
        wCmd = QString("bd \"%1\"").arg(ToPtrString(bp.addr));
    }
    else if(bp.type == bp_memory)
    {
        wCmd = QString("bpmd \"%1\"").arg(ToPtrString(bp.addr));
    }
    else if(bp.type == bp_dll)
    {
        wCmd = QString("LibrarianDisableBreakPoint \"%1\"").arg(QString(bp.mod));
    }
    else if(bp.type == bp_exception)
    {
        wCmd = QString("DisableExceptionBPX \"%1\"").arg(ToPtrString(bp.addr));
    }

    DbgCmdExec(wCmd.toUtf8().constData());
}

/**
 * @brief       Disable breakpoint that has been previously enabled according to its type and virtual address.
 *              If breakpoint was removed, this method has no effect.@n
 *              Breakpoint type is usefull when there are several types of breakpoints on the same address.
 *              bp_none disbales all breakpoints at the given address.
 *
 * @param[in]   type    Type of the breakpoint.
 * @param[in]   va      Virtual Address
 *
 * @return      Nothing.
 */
void Breakpoints::disableBP(BPXTYPE type, duint va)
{
    BPMAP wBPList;

    // Get breakpoints list
    DbgGetBpList(type, &wBPList);

    // Find breakpoint at address VA
    for(int wI = 0; wI < wBPList.count; wI++)
    {
        if(wBPList.bp[wI].addr == va)
        {
            disableBP(wBPList.bp[wI]);
        }
    }
    if(wBPList.count)
        BridgeFree(wBPList.bp);
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
    QString wCmd = "";

    switch(bp.type)
    {
    case bp_normal:
        wCmd = QString("bc \"%1\"").arg(ToPtrString(bp.addr));
        break;

    case bp_hardware:
        wCmd = QString("bphc \"%1\"").arg(ToPtrString(bp.addr));
        break;

    case bp_memory:
        wCmd = QString("bpmc \"%1\"").arg(ToPtrString(bp.addr));
        break;

    case bp_dll:
        wCmd = QString("bcdll \"%1\"").arg(QString(bp.mod));
        break;

    case bp_exception:
        wCmd = QString("DeleteExceptionBPX \"%1\"").arg(ToPtrString(bp.addr));
        break;

    default:
        break;
    }

    DbgCmdExec(wCmd.toUtf8().constData());
}

/**
 * @brief       Remove breakpoint at the given given address and type
 *              If breakpoint doesn't exists, this method has no effect.@n
 *              Breakpoint type is usefull when there are several types of breakpoints on the same address.
 *              bp_none disbales all breakpoints at the given address.
 *
 * @param[in]   type    Type of the breakpoint.
 * @param[in]   va      Virtual Address
 *
 * @return      Nothing.
 */
void Breakpoints::removeBP(BPXTYPE type, duint va)
{
    BPMAP wBPList;

    // Get breakpoints list
    DbgGetBpList(type, &wBPList);

    // Find breakpoint at address VA
    for(int wI = 0; wI < wBPList.count; wI++)
    {
        if(wBPList.bp[wI].addr == va)
        {
            removeBP(wBPList.bp[wI]);
        }
    }
    if(wBPList.count)
        BridgeFree(wBPList.bp);
}

void Breakpoints::removeBP(const QString & DLLName)
{
    BPMAP wBPList;

    // Get breakpoints list
    DbgGetBpList(bp_dll, &wBPList);

    // Find breakpoint at DLLName
    for(int wI = 0; wI < wBPList.count; wI++)
    {
        if(QString(wBPList.bp[wI].mod) == DLLName)
        {
            removeBP(wBPList.bp[wI]);
        }
    }
    if(wBPList.count)
        BridgeFree(wBPList.bp);
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
    BPMAP wBPList;

    // Get breakpoints list
    DbgGetBpList(type, &wBPList);

    // Find breakpoint at address VA
    for(int wI = 0; wI < wBPList.count; wI++)
    {
        if(wBPList.bp[wI].addr == va)
        {
            toggleBPByDisabling(wBPList.bp[wI]);
        }
    }
    if(wBPList.count)
        BridgeFree(wBPList.bp);
}

void Breakpoints::toggleBPByDisabling(const QString & DLLName)
{
    BPMAP wBPList;

    // Get breakpoints list
    DbgGetBpList(bp_dll, &wBPList);

    // Find breakpoint at module name
    for(int wI = 0; wI < wBPList.count; wI++)
    {
        if(QString(wBPList.bp[wI].mod) == DLLName)
        {
            toggleBPByDisabling(wBPList.bp[wI]);
        }
    }
    if(wBPList.count)
        BridgeFree(wBPList.bp);
}

void Breakpoints::toggleAllBP(BPXTYPE type, bool bEnable)
{
    BPMAP wBPList;

    // Get breakpoints list
    DbgGetBpList(type, &wBPList);

    if(bEnable)
    {
        // Find breakpoint at address VA
        for(int wI = 0; wI < wBPList.count; wI++)
        {
            enableBP(wBPList.bp[wI]);
        }
    }
    else
    {
        // Find breakpoint at address VA
        for(int wI = 0; wI < wBPList.count; wI++)
        {
            disableBP(wBPList.bp[wI]);
        }
    }

    if(wBPList.count)
        BridgeFree(wBPList.bp);
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
    BPMAP wBPList;
    BPXSTATE result = bp_non_existent;

    // Get breakpoints list
    DbgGetBpList(type, &wBPList);

    // Find breakpoint at address VA
    for(int wI = 0; wI < wBPList.count; wI++)
    {
        if(wBPList.bp[wI].addr == va)
        {
            if(wBPList.bp[wI].enabled)
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
    if(wBPList.count)
        BridgeFree(wBPList.bp);

    return result;
}

bool Breakpoints::BPTrival(BPXTYPE type, duint va)
{
    BPMAP wBPList;
    bool trival = true;

    // Get breakpoints list
    DbgGetBpList(type, &wBPList);

    // Find breakpoint at address VA
    for(int wI = 0; wI < wBPList.count; wI++)
    {
        BRIDGEBP & bp = wBPList.bp[wI];
        if(bp.addr == va)
        {
            trival = !(bp.breakCondition[0] || bp.logCondition[0] || bp.commandCondition[0] || bp.commandText[0] || bp.logText[0] || bp.name[0] || bp.fastResume || bp.silent);
            break;
        }
    }
    if(wBPList.count)
        BridgeFree(wBPList.bp);

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
    BPMAP wBPList;
    bool wNormalWasRemoved = false;
    bool wMemoryWasRemoved = false;
    bool wHardwareWasRemoved = false;

    // Get breakpoints list
    DbgGetBpList(type, &wBPList);

    // Find breakpoints at address VA and remove them
    for(int wI = 0; wI < wBPList.count; wI++)
    {
        if(wBPList.bp[wI].addr == va)
        {
            removeBP(wBPList.bp[wI]);

            switch(wBPList.bp[wI].type)
            {
            case bp_normal:
                wNormalWasRemoved = true;
                break;
            case bp_memory:
                wMemoryWasRemoved = true;
                break;
            case bp_hardware:
                wHardwareWasRemoved = true;
                break;
            default:
                break;
            }
        }
    }
    if(wBPList.count)
        BridgeFree(wBPList.bp);

    if((type == bp_none || type == bp_normal) && (wNormalWasRemoved == false))
    {
        setBP(bp_normal, va);
    }
    else if((type == bp_none || type == bp_memory) && (wMemoryWasRemoved == false))
    {
        setBP(bp_memory, va);
    }
    else if((type == bp_none || type == bp_hardware) && (wHardwareWasRemoved == false))
    {
        setBP(bp_hardware, va);
    }
}

bool Breakpoints::editBP(BPXTYPE type, const QString & addrText, QWidget* widget)
{
    BRIDGEBP bridgebp;
    if(type != bp_dll)
    {
        duint addr = addrText.toULongLong(nullptr, 16);
        if(!DbgFunctions()->GetBridgeBp(type, addr, &bridgebp))
            return false;
    }
    else if(!DbgFunctions()->GetBridgeBp(type, reinterpret_cast<duint>(addrText.toUtf8().constData()), &bridgebp))
        return false;
    EditBreakpointDialog dialog(widget, bridgebp);
    if(dialog.exec() != QDialog::Accepted)
        return false;
    auto bp = dialog.getBp();
    auto exec = [](const QString & command)
    {
        DbgCmdExecDirect(command.toUtf8().constData());
    };
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
