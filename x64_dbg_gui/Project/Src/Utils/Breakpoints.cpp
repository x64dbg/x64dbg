#include "Breakpoints.h"

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
void Breakpoints::setBP(BPXTYPE type, uint_t va)
{
    QString wCmd = "";

    switch(type)
    {
    case bp_normal:
    {
        wCmd = "bp " + QString("%1").arg(va, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    }
    break;

    case bp_hardware:
    {
        wCmd = "bph " + QString("%1").arg(va, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    }
    break;

    case bp_memory:
    {
        wCmd = "bpm " + QString("%1").arg(va, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
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
void Breakpoints::enableBP(BRIDGEBP & bp)
{
    QString wCmd = "";

    if(bp.type == bp_hardware)
    {
        wCmd = "bphwe " + QString("%1").arg(bp.addr, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    }
    else if(bp.type == bp_normal)
    {
        wCmd = "be " + QString("%1").arg(bp.addr, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    }
    else if(bp.type == bp_memory)
    {
        wCmd = "bpme " + QString("%1").arg(bp.addr, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
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
void Breakpoints::enableBP(BPXTYPE type, uint_t va)
{
    int wI = 0;
    BPMAP wBPList;

    // Get breakpoints list
    DbgGetBpList(type, &wBPList);

    // Find breakpoint at address VA
    for(wI = 0; wI < wBPList.count; wI++)
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
void Breakpoints::disableBP(BRIDGEBP & bp)
{
    QString wCmd = "";

    if(bp.type == bp_hardware)
    {
        wCmd = "bphwd " + QString("%1").arg(bp.addr, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    }
    else if(bp.type == bp_normal)
    {
        wCmd = "bd " + QString("%1").arg(bp.addr, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    }
    else if(bp.type == bp_memory)
    {
        wCmd = "bpmd " + QString("%1").arg(bp.addr, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
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
void Breakpoints::disableBP(BPXTYPE type, uint_t va)
{
    int wI = 0;
    BPMAP wBPList;

    // Get breakpoints list
    DbgGetBpList(type, &wBPList);

    // Find breakpoint at address VA
    for(wI = 0; wI < wBPList.count; wI++)
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
void Breakpoints::removeBP(BRIDGEBP & bp)
{
    QString wCmd = "";

    switch(bp.type)
    {
    case bp_normal:
    {
        wCmd = "bc " + QString("%1").arg(bp.addr, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    }
    break;

    case bp_hardware:
    {
        wCmd = "bphc " + QString("%1").arg(bp.addr, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    }
    break;

    case bp_memory:
    {
        wCmd = "bpmc " + QString("%1").arg(bp.addr, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
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
void Breakpoints::removeBP(BPXTYPE type, uint_t va)
{
    int wI = 0;
    BPMAP wBPList;

    // Get breakpoints list
    DbgGetBpList(type, &wBPList);

    // Find breakpoint at address VA
    for(wI = 0; wI < wBPList.count; wI++)
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
void Breakpoints::toggleBPByDisabling(BRIDGEBP & bp)
{
    if(bp.enabled == true)
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
void Breakpoints::toggleBPByDisabling(BPXTYPE type, uint_t va)
{
    int wI = 0;
    BPMAP wBPList;

    // Get breakpoints list
    DbgGetBpList(type, &wBPList);

    // Find breakpoint at address VA
    for(wI = 0; wI < wBPList.count; wI++)
    {
        if(wBPList.bp[wI].addr == va)
        {
            toggleBPByDisabling(wBPList.bp[wI]);
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
BPXSTATE Breakpoints::BPState(BPXTYPE type, uint_t va)
{
    int wI = 0;
    BPMAP wBPList;

    // Get breakpoints list
    DbgGetBpList(type, &wBPList);

    // Find breakpoint at address VA
    for(wI = 0; wI < wBPList.count; wI++)
    {
        if(wBPList.bp[wI].addr == va)
        {
            if(wBPList.bp[wI].enabled)
                return bp_enabled;
            else
                return bp_disabled;
        }
    }
    if(wBPList.count)
        BridgeFree(wBPList.bp);

    return bp_non_existent;
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
void Breakpoints::toggleBPByRemoving(BPXTYPE type, uint_t va)
{
    int wI = 0;
    BPMAP wBPList;
    bool wNormalWasRemoved = false;
    bool wMemoryWasRemoved = false;
    bool wHardwareWasRemoved = false;

    // Get breakpoints list
    DbgGetBpList(type, &wBPList);

    // Find breakpoints at address VA and remove them
    for(wI = 0; wI < wBPList.count; wI++)
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
