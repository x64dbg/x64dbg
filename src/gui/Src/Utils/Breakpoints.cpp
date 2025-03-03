#include "Breakpoints.h"
#include "EditBreakpointDialog.h"
#include "StringUtil.h"

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

static QString getBpIdentifier(const Breakpoints::Data & bp)
{
    if(!bp.module.isEmpty())
    {
        auto modbase = DbgModBaseFromName(bp.module.toUtf8().constData());
        if(!modbase)
            return QString("\"%1\":$%2").arg(bp.module.toUtf8().constData()).arg(ToHexString(bp.addr));
    }
    return ToPtrString(bp.addr);
}

/**
 * @brief       Enable breakpoint according to the given breakpoint descriptor.
 *
 * @param[in]   bp  Breakpoint descriptor
 *
 * @return      Nothing.
 */
void Breakpoints::enableBP(const Data & bp)
{
    QString cmd = "";

    if(bp.type == bp_hardware)
    {
        cmd = QString("bphwe \"%1\"").arg(getBpIdentifier(bp));
    }
    else if(bp.type == bp_normal)
    {
        cmd = QString("be \"%1\"").arg(getBpIdentifier(bp));
    }
    else if(bp.type == bp_memory)
    {
        cmd = QString("bpme \"%1\"").arg(getBpIdentifier(bp));
    }
    else if(bp.type == bp_dll)
    {
        cmd = QString("LibrarianEnableBreakPoint \"%1\"").arg(bp.module);
    }
    else if(bp.type == bp_exception)
    {
        cmd = QString("EnableExceptionBPX \"%1\"").arg(ToPtrString(bp.addr));
    }

    DbgCmdExecDirect(cmd);
}

/**
 * @brief       Disable breakpoint according to the given breakpoint descriptor.
 *
 * @param[in]   bp  Breakpoint descriptor
 *
 * @return      Nothing.
 */
void Breakpoints::disableBP(const Data & bp)
{
    QString cmd = "";

    if(bp.type == bp_hardware)
    {
        cmd = QString("bphwd \"%1\"").arg(getBpIdentifier(bp));
    }
    else if(bp.type == bp_normal)
    {
        cmd = QString("bd \"%1\"").arg(getBpIdentifier(bp));
    }
    else if(bp.type == bp_memory)
    {
        cmd = QString("bpmd \"%1\"").arg(getBpIdentifier(bp));
    }
    else if(bp.type == bp_dll)
    {
        cmd = QString("LibrarianDisableBreakPoint \"%1\"").arg(bp.module);
    }
    else if(bp.type == bp_exception)
    {
        cmd = QString("DisableExceptionBPX \"%1\"").arg(ToPtrString(bp.addr));
    }

    DbgCmdExecDirect(cmd);
}

/**
 * @brief       Remove breakpoint according to the given breakpoint descriptor.
 *
 * @param[in]   bp  Breakpoint descriptor
 *
 * @return      Nothing.
 */
void Breakpoints::removeBP(const Data & bp)
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
        cmd = QString("bcdll \"%1\"").arg(bp.module);
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
 * @brief       Toggle the given breakpoint by disabling it when enabled.@n
 *              If breakpoint is initially active and enabled, it will be disabled.@n
 *              If breakpoint is initially active and disabled, it will stay disabled.@n
 *
 * @param[in]   bp  Breakpoint descriptor
 *
 * @return      Nothing.
 */
void Breakpoints::toggleBPByDisabling(const Data & bp)
{
    if(bp.enabled)
        disableBP(bp);
    else
        enableBP(bp);
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
    BP_REF ref;
    DbgFunctions()->BpRefVa(&ref, type, va);
    bool enabled = false;
    if(!ref.GetField(bpf_enabled, enabled))
        return bp_non_existent;
    return enabled ? bp_enabled : bp_disabled;
}

bool Breakpoints::BPTrival(BPXTYPE type, duint va)
{
    BP_REF ref;
    DbgFunctions()->BpRefVa(&ref, type, va);
    Data bp(ref);
    return bp.breakCondition.isEmpty() &&
           bp.logText.isEmpty() &&
           bp.logCondition.isEmpty() &&
           bp.commandText.isEmpty() &&
           bp.commandCondition.isEmpty() &&
           bp.name.isEmpty() &&
           !bp.fastResume &&
           !bp.silent;
}

bool Breakpoints::editBP(BPXTYPE type, const QString & module, duint address, QWidget* widget, const QString & createCommand)
{
    QString addrText;
    BP_REF ref;
    switch(type)
    {
    case bp_dll:
        addrText = module;
        DbgFunctions()->BpRefDll(&ref, module.toUtf8().constData());
        break;
    case bp_exception:
        addrText = ToHexString(address);
        DbgFunctions()->BpRefException(&ref, address);
        break;
    default:
        if(!module.isEmpty())
        {
            addrText = QString("\"%1\":$%2").arg(module).arg(ToHexString(address));
            if(!DbgFunctions()->BpRefRva(&ref, type, module.toUtf8().constData(), address))
                return false;
        }
        else
        {
            addrText = ToHexString(address);
            if(!DbgFunctions()->BpRefVa(&ref, type, address))
                return false;
        }
        break;
    }

    Data bp(ref);
    auto found = !bp.error;
    if(!createCommand.isEmpty() && !found)
    {
        // Create a dummy to edit
        bp.type = type;
        if(type == bp_dll)
        {
            bp.module = module;
        }
        else if(type == bp_exception)
        {
            bp.addr = address;
        }
        else
        {
            bp.module = module;
            bp.addr = address;
        }
    }
    else if(!found)
    {
        // Fail if the breakpoint doesn't exist and we cannot create a new one
        return false;
    }

    EditBreakpointDialog dialog(widget, bp);
    if(dialog.exec() != QDialog::Accepted)
        return false;

    // Get the updated structure from the dialog
    bp = dialog.getBp();
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
        exec(QString("SetBreakpointName %1, \"%2\"").arg(addrText).arg(DbgCmdEscape(bp.name)));
        exec(QString("SetBreakpointCondition %1, \"%2\"").arg(addrText).arg(DbgCmdEscape(bp.breakCondition)));
        exec(QString("SetBreakpointLog %1, \"%2\"").arg(addrText).arg(DbgCmdEscape(bp.logText)));
        exec(QString("SetBreakpointLogCondition %1, \"%2\"").arg(addrText).arg(DbgCmdEscape(bp.logCondition)));
        exec(QString("SetBreakpointLogFile %1, \"%2\"").arg(addrText).arg(bp.logFile));
        exec(QString("SetBreakpointCommand %1, \"%2\"").arg(addrText).arg(DbgCmdEscape(bp.commandText)));
        exec(QString("SetBreakpointCommandCondition %1, \"%2\"").arg(addrText).arg(DbgCmdEscape(bp.commandCondition)));
        exec(QString("ResetBreakpointHitCount %1, %2").arg(addrText).arg(ToPtrString(bp.hitCount)));
        exec(QString("SetBreakpointFastResume %1, %2").arg(addrText).arg(bp.fastResume));
        exec(QString("SetBreakpointSilent %1, %2").arg(addrText).arg(bp.silent));
        exec(QString("SetBreakpointSingleshoot %1, %2").arg(addrText).arg(bp.singleshoot));
        break;
    case bp_hardware:
        exec(QString("SetHardwareBreakpointName %1, \"%2\"").arg(addrText).arg(DbgCmdEscape(bp.name)));
        exec(QString("SetHardwareBreakpointCondition %1, \"%2\"").arg(addrText).arg(DbgCmdEscape(bp.breakCondition)));
        exec(QString("SetHardwareBreakpointLog %1, \"%2\"").arg(addrText).arg(DbgCmdEscape(bp.logText)));
        exec(QString("SetHardwareBreakpointLogCondition %1, \"%2\"").arg(addrText).arg(DbgCmdEscape(bp.logCondition)));
        exec(QString("SetHardwareBreakpointLogFile %1, \"%2\"").arg(addrText).arg(bp.logFile));
        exec(QString("SetHardwareBreakpointCommand %1, \"%2\"").arg(addrText).arg(DbgCmdEscape(bp.commandText)));
        exec(QString("SetHardwareBreakpointCommandCondition %1, \"%2\"").arg(addrText).arg(DbgCmdEscape(bp.commandCondition)));
        exec(QString("ResetHardwareBreakpointHitCount %1, %2").arg(addrText).arg(ToPtrString(bp.hitCount)));
        exec(QString("SetHardwareBreakpointFastResume %1, %2").arg(addrText).arg(bp.fastResume));
        exec(QString("SetHardwareBreakpointSilent %1, %2").arg(addrText).arg(bp.silent));
        exec(QString("SetHardwareBreakpointSingleshoot %1, %2").arg(addrText).arg(bp.singleshoot));
        break;
    case bp_memory:
        exec(QString("SetMemoryBreakpointName %1, \"\"%2\"\"").arg(addrText).arg(DbgCmdEscape(bp.name)));
        exec(QString("SetMemoryBreakpointCondition %1, \"%2\"").arg(addrText).arg(DbgCmdEscape(bp.breakCondition)));
        exec(QString("SetMemoryBreakpointLog %1, \"%2\"").arg(addrText).arg(DbgCmdEscape(bp.logText)));
        exec(QString("SetMemoryBreakpointLogCondition %1, \"%2\"").arg(addrText).arg(DbgCmdEscape(bp.logCondition)));
        exec(QString("SetMemoryBreakpointLogFile %1, \"%2\"").arg(addrText).arg(bp.logFile));
        exec(QString("SetMemoryBreakpointCommand %1, \"%2\"").arg(addrText).arg(DbgCmdEscape(bp.commandText)));
        exec(QString("SetMemoryBreakpointCommandCondition %1, \"%2\"").arg(addrText).arg(DbgCmdEscape(bp.commandCondition)));
        exec(QString("ResetMemoryBreakpointHitCount %1, %2").arg(addrText).arg(ToPtrString(bp.hitCount)));
        exec(QString("SetMemoryBreakpointFastResume %1, %2").arg(addrText).arg(bp.fastResume));
        exec(QString("SetMemoryBreakpointSilent %1, %2").arg(addrText).arg(bp.silent));
        exec(QString("SetMemoryBreakpointSingleshoot %1, %2").arg(addrText).arg(bp.singleshoot));
        break;
    case bp_dll:
        exec(QString("SetLibrarianBreakpointName \"%1\", \"\"%2\"\"").arg(addrText).arg(DbgCmdEscape(bp.name)));
        exec(QString("SetLibrarianBreakpointCondition \"%1\", \"%2\"").arg(addrText).arg(DbgCmdEscape(bp.breakCondition)));
        exec(QString("SetLibrarianBreakpointLog \"%1\", \"%2\"").arg(addrText).arg(DbgCmdEscape(bp.logText)));
        exec(QString("SetLibrarianBreakpointLogCondition \"%1\", \"%2\"").arg(addrText).arg(DbgCmdEscape(bp.logCondition)));
        exec(QString("SetLibrarianBreakpointLogFile \"%1\", \"%2\"").arg(addrText).arg(bp.logFile));
        exec(QString("SetLibrarianBreakpointCommand \"%1\", \"%2\"").arg(addrText).arg(DbgCmdEscape(bp.commandText)));
        exec(QString("SetLibrarianBreakpointCommandCondition \"%1\", \"%2\"").arg(addrText).arg(DbgCmdEscape(bp.commandCondition)));
        exec(QString("ResetLibrarianBreakpointHitCount \"%1\", %2").arg(addrText).arg(ToPtrString(bp.hitCount)));
        exec(QString("SetLibrarianBreakpointFastResume \"%1\", %2").arg(addrText).arg(bp.fastResume));
        exec(QString("SetLibrarianBreakpointSilent \"%1\", %2").arg(addrText).arg(bp.silent));
        exec(QString("SetLibrarianBreakpointSingleshoot \"%1\", %2").arg(addrText).arg(bp.singleshoot));
        break;
    case bp_exception:
        exec(QString("SetExceptionBreakpointName %1, \"%2\"").arg(addrText).arg(DbgCmdEscape(bp.name)));
        exec(QString("SetExceptionBreakpointCondition %1, \"%2\"").arg(addrText).arg(DbgCmdEscape(bp.breakCondition)));
        exec(QString("SetExceptionBreakpointLog %1, \"%2\"").arg(addrText).arg(DbgCmdEscape(bp.logText)));
        exec(QString("SetExceptionBreakpointLogCondition %1, \"%2\"").arg(addrText).arg(DbgCmdEscape(bp.logCondition)));
        exec(QString("SetExceptionBreakpointLogFile %1, \"%2\"").arg(addrText).arg(bp.logFile));
        exec(QString("SetExceptionBreakpointCommand %1, \"%2\"").arg(addrText).arg(DbgCmdEscape(bp.commandText)));
        exec(QString("SetExceptionBreakpointCommandCondition %1, \"%2\"").arg(addrText).arg(DbgCmdEscape(bp.commandCondition)));
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

Breakpoints::Data::Data(const BP_REF & key)
    : ref(key)
{
    read();
}

void Breakpoints::Data::getField(BP_FIELD field, QString & value)
{
    auto callback = [](const char* str, void* userdata)
    {
        *(QString*)userdata = QString::fromUtf8(str);
    };
    if(!DbgFunctions()->BpGetFieldText(&ref, field, callback, &value))
    {
        error = true;
        return;
    }
}

void Breakpoints::Data::getField(BP_FIELD field, duint & value)
{
    if(!ref.GetField(field, value))
        error = true;
}

void Breakpoints::Data::getField(BP_FIELD field, bool & value)
{
    if(!ref.GetField(field, value))
        error = true;
}

bool Breakpoints::Data::read()
{
    getField(bpf_type, type);
    if(error)
        return false;
    getField(bpf_address, addr);
    getField(bpf_module, module);
    getField(bpf_active, active);
    getField(bpf_enabled, enabled);
    getField(bpf_hwsize, hwSize);
    getField(bpf_typeex, typeEx);
    getField(bpf_breakcondition, breakCondition);
    getField(bpf_logtext, logText);
    getField(bpf_logcondition, logCondition);
    getField(bpf_commandtext, commandText);
    getField(bpf_commandcondition, commandCondition);
    getField(bpf_name, name);
    getField(bpf_hitcount, hitCount);
    getField(bpf_logfile, logFile);
    getField(bpf_singleshoot, singleshoot);
    getField(bpf_silent, silent);
    getField(bpf_fastresume, fastResume);
    return !error;
}
