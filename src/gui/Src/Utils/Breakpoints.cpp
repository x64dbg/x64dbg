#include "Breakpoints.h"
#include "EditBreakpointDialog.h"

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
        wCmd = "bp " + QString("%1").arg(va, sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    }
    break;

    case bp_hardware:
    {
        wCmd = "bph " + QString("%1").arg(va, sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    }
    break;

    case bp_memory:
    {
        wCmd = "bpm " + QString("%1").arg(va, sizeof(dsint) * 2, 16, QChar('0')).toUpper();
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
        wCmd = "bphwe " + QString("%1").arg(bp.addr, sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    }
    else if(bp.type == bp_normal)
    {
        wCmd = "be " + QString("%1").arg(bp.addr, sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    }
    else if(bp.type == bp_memory)
    {
        wCmd = "bpme " + QString("%1").arg(bp.addr, sizeof(dsint) * 2, 16, QChar('0')).toUpper();
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
        wCmd = "bphwd " + QString("%1").arg(bp.addr, sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    }
    else if(bp.type == bp_normal)
    {
        wCmd = "bd " + QString("%1").arg(bp.addr, sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    }
    else if(bp.type == bp_memory)
    {
        wCmd = "bpmd " + QString("%1").arg(bp.addr, sizeof(dsint) * 2, 16, QChar('0')).toUpper();
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
    {
        wCmd = "bc " + QString("%1").arg(bp.addr, sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    }
    break;

    case bp_hardware:
    {
        wCmd = "bphc " + QString("%1").arg(bp.addr, sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    }
    break;

    case bp_memory:
    {
        wCmd = "bpmc " + QString("%1").arg(bp.addr, sizeof(dsint) * 2, 16, QChar('0')).toUpper();
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

void Breakpoints::editBP(BPXTYPE type, const QString & addrText, QWidget* widget)
{
    duint addr = addrText.toULongLong(nullptr, 16);
    BRIDGEBP bridgebp;
    if(!DbgFunctions()->GetBridgeBp(type, addr, &bridgebp))
        return;
    EditBreakpointDialog dialog(widget, bridgebp);
    if(dialog.exec() != QDialog::Accepted)
        return;
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
        exec(QString("ResetBreakpointHitCount %1, %2").arg(addrText).arg(bp.hitCount));
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
        exec(QString("ResetHardwareBreakpointHitCount %1, %2").arg(addrText).arg(bp.hitCount));
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
        exec(QString("ResetMemoryBreakpointHitCount %1, %2").arg(addrText).arg(bp.hitCount));
        exec(QString("SetMemoryBreakpointFastResume %1, %2").arg(addrText).arg(bp.fastResume));
        exec(QString("SetMemoryBreakpointSilent %1, %2").arg(addrText).arg(bp.silent));
        exec(QString("SetMemoryBreakpointSingleshoot %1, %2").arg(addrText).arg(bp.singleshoot));
        break;
    default:
        return;
    }
    GuiUpdateBreakpointsView();
}
