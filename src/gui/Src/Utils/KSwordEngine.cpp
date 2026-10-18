#include "KSwordEngine.h"
#include "StringUtil.h"
#include <QCoreApplication>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <memory>

namespace KSwordEngine
{
    namespace
    {
        unsigned long call(uint32_t command, const void* input, uint32_t inputBytes, void* output, uint32_t outputBytes)
        {
            KSWORD_DEBUGGER_CALL packet{KSWORD_DEBUGGER_API_VERSION, sizeof(packet), command, 0,
                reinterpret_cast<uintptr_t>(input), reinterpret_cast<uintptr_t>(output), inputBytes, outputBytes, 0, 0};
            const auto error = DbgKSwordCall(&packet);
            if(error == ERROR_SUCCESS && packet.bytesReturned != outputBytes)
                return ERROR_REVISION_MISMATCH;
            return error;
        }

        bool engineInfo(KSWORD_DEBUGGER_ENGINE_INFO& info)
        {
            return selected() && call(KSWORD_DEBUGGER_QUERY_ENGINE, nullptr, 0, &info, sizeof(info)) == ERROR_SUCCESS &&
                   info.version == KSWORD_DEBUGGER_ENGINE_INFO_VERSION && info.size == sizeof(info);
        }

        void reportError(QWidget* parent, unsigned long error)
        {
            if(error != ERROR_SUCCESS)
                QMessageBox::warning(parent, "KSword", QCoreApplication::translate("KSwordEngine", "Request failed (error %1). The menu will show the acknowledged engine state.").arg(error));
        }

        void setOptions(QWidget* parent, const KSWORD_DEBUGGER_OPTIONS& options)
        {
            KSWORD_DEBUGGER_OPTIONS actual{};
            reportError(parent, call(KSWORD_DEBUGGER_SET_OPTIONS, &options, sizeof(options), &actual, sizeof(actual)));
        }

        QString sessionName(uint32_t kind)
        {
            switch(kind)
            {
            case 1: return QCoreApplication::translate("KSwordEngine", "Live");
            case 2: return QCoreApplication::translate("KSwordEngine", "Minidump (read only)");
            case 3: return QCoreApplication::translate("KSwordEngine", "TTD replay");
            case 4: return QCoreApplication::translate("KSwordEngine", "Static");
            default: return QCoreApplication::translate("KSwordEngine", "No session");
            }
        }

        void showEngine(QWidget* parent)
        {
            KSWORD_DEBUGGER_ENGINE_INFO info{};
            if(!engineInfo(info)) { reportError(parent, ERROR_NOT_SUPPORTED); return; }
            const auto text = QCoreApplication::translate("KSwordEngine",
                "Session: %1\nProvider: %2\nPID: %3\nCapabilities: 0x%4\nHVM preference: %5; active in this session: %6\n"
                "Installed breakpoints: %7; Shadow write pages: %8\nFallbacks: %9; last error: %10\n\n"
                "The frontend hardware category is a request. Live execution may use real DR, EPT execution, or ShadowPage hidden INT3. Details query the installed binding.\n"
                "EPT data watches cover %11-byte pages; they cannot represent every byte-range DR breakpoint.\n"
                "Shadow writes change execution views. Ordinary reads show original code; numeric data writes require the explicit fallback policy.\n"
                "Live sessions retain the Windows debug-event transport and debug port. Replay uses provider-owned synthetic handles, logical breakpoints and session capabilities.")
                .arg(sessionName(info.sessionKind)).arg(info.provider == KSWORD_DEBUGGER_PROVIDER_DBGENG ? "DbgEng" : "TitanEngine + KSword")
                .arg(info.processId).arg(info.capabilities, 0, 16).arg(info.configuredHvm).arg(info.activeHvm)
                .arg(info.policy.activeBreakpoints).arg(info.policy.shadowWritePages).arg(info.policy.fallbackCount)
                .arg(info.policy.lastFallbackError).arg(info.eptDataGranularity);
            QMessageBox::information(parent, QCoreApplication::translate("KSwordEngine", "KSword engine and mechanisms"), text);
        }
    }

    bool selected() { return DbgGetDebugEngine() == DebugEngineKSword; }

    QString mechanismName(uint32_t mechanism)
    {
        switch(mechanism)
        {
        case KSWORD_DEBUGGER_MECHANISM_DR: return QCoreApplication::translate("KSwordEngine", "Hardware DR");
        case KSWORD_DEBUGGER_MECHANISM_EPT_EXECUTE: return QCoreApplication::translate("KSwordEngine", "EPT execute");
        case KSWORD_DEBUGGER_MECHANISM_SHADOW_INT3: return QCoreApplication::translate("KSwordEngine", "Shadow INT3");
        case KSWORD_DEBUGGER_MECHANISM_SHADOW_LONG_INT3: return QCoreApplication::translate("KSwordEngine", "Shadow long INT3");
        case KSWORD_DEBUGGER_MECHANISM_SHADOW_UD2: return QCoreApplication::translate("KSwordEngine", "Shadow UD2");
        case KSWORD_DEBUGGER_MECHANISM_INT3: return "INT3";
        case KSWORD_DEBUGGER_MECHANISM_LONG_INT3: return QCoreApplication::translate("KSwordEngine", "Long INT3");
        case KSWORD_DEBUGGER_MECHANISM_UD2: return "UD2";
        case KSWORD_DEBUGGER_MECHANISM_PAGE_GUARD: return "PAGE_GUARD";
        case KSWORD_DEBUGGER_MECHANISM_REPLAY_CODE: return QCoreApplication::translate("KSwordEngine", "Replay code");
        case KSWORD_DEBUGGER_MECHANISM_REPLAY_DATA: return QCoreApplication::translate("KSwordEngine", "Replay data");
        default: return QCoreApplication::translate("KSwordEngine", "Mechanism pending");
        }
    }

    bool breakpointInfo(duint address, BPXTYPE type, KSWORD_DEBUGGER_BREAKPOINT_INFO& info, DWORD* error)
    {
        KSWORD_DEBUGGER_BREAKPOINT_QUERY query{KSWORD_DEBUGGER_ENGINE_INFO_VERSION, sizeof(query), address, static_cast<uint32_t>(type), 0};
        const auto result = call(KSWORD_DEBUGGER_QUERY_BREAKPOINT, &query, sizeof(query), &info, sizeof(info));
        if(error) *error = result;
        return result == ERROR_SUCCESS && info.version == KSWORD_DEBUGGER_ENGINE_INFO_VERSION && info.size == sizeof(info);
    }

    QString breakpointLabel(duint address, BPXTYPE type)
    {
        if(!selected() || (type != bp_normal && type != bp_hardware && type != bp_memory)) return QString();
        KSWORD_DEBUGGER_BREAKPOINT_INFO info{}; DWORD error = 0;
        if(breakpointInfo(address, type, info, &error)) return mechanismName(info.mechanism);
        return error == ERROR_NOT_FOUND ? QCoreApplication::translate("KSwordEngine", "Not installed") : QCoreApplication::translate("KSwordEngine", "Query unavailable");
    }

    void showBreakpoint(QWidget* parent, duint address, BPXTYPE type)
    {
        KSWORD_DEBUGGER_BREAKPOINT_INFO info{}; DWORD error = 0;
        if(!breakpointInfo(address, type, info, &error))
        {
            if(error == ERROR_NOT_FOUND)
                QMessageBox::information(parent, "KSword", QCoreApplication::translate("KSwordEngine", "No installed engine binding at %1 for frontend category %2. The record may be disabled, inactive or retired.").arg(ToPtrString(address)).arg(type));
            else reportError(parent, error);
            return;
        }
        QString physical = QCoreApplication::translate("KSwordEngine", "Physical arming was not queried");
        if(info.flags & KSWORD_DEBUGGER_BP_PHYSICAL_STATE_KNOWN)
            physical = info.flags & KSWORD_DEBUGGER_BP_PHYSICAL_ARMED ? QCoreApplication::translate("KSwordEngine", "Execution breakpoint bytes armed") :
                       QCoreApplication::translate("KSwordEngine", "Execution bytes temporarily restored; logical binding retained");
        const auto text = QCoreApplication::translate("KSwordEngine",
            "Address: %1\nFrontend category: %2\nActual mechanism: %3\nRequested/effective coverage: %4 / %5 bytes\n"
            "Access mask (read=1, write=2, execute=4): %6\nFrontend slot: %7\nBinding fallback error: %8\n"
            "Original code unchanged: %9\nWindows debug-event transport: %10\n%11")
            .arg(ToPtrString(address)).arg(type).arg(mechanismName(info.mechanism)).arg(info.requestedBytes).arg(info.effectiveBytes)
            .arg(info.access).arg(info.slot == UINT32_MAX ? QString("-") : QString::number(info.slot)).arg(info.fallbackError)
            .arg(!!(info.flags & KSWORD_DEBUGGER_BP_ORIGINAL_UNCHANGED)).arg(!!(info.flags & KSWORD_DEBUGGER_BP_WINDOWS_TRANSPORT)).arg(physical);
        QMessageBox::information(parent, QCoreApplication::translate("KSwordEngine", "KSword installed breakpoint"), text);
    }

    void showAddress(QWidget* parent, duint address)
    {
        const auto types = DbgGetBpxTypeAt(address);
        if(types & bp_hardware) showBreakpoint(parent, address, bp_hardware);
        else if(types & bp_normal) showBreakpoint(parent, address, bp_normal);
        else if(types & bp_memory) showBreakpoint(parent, address, bp_memory);
        else QMessageBox::information(parent, "KSword", QCoreApplication::translate("KSwordEngine", "No frontend breakpoint at %1.").arg(ToPtrString(address)));
    }

    void addMenu(QMenu* menu, QWidget* parent)
    {
        // This is invoked only for the engine actually loaded by BridgeInit.
        auto infoAction = menu->addAction(QCoreApplication::translate("KSwordEngine", "Engine capabilities and mechanisms..."));
        QObject::connect(infoAction, &QAction::triggered, parent, [parent] { showEngine(parent); });
        menu->addSeparator();
        auto hvm = menu->addAction(QCoreApplication::translate("KSwordEngine", "Use HVM for live debugging")); hvm->setCheckable(true);
        QObject::connect(hvm, &QAction::triggered, parent, [parent](bool checked)
        {
            const uint32_t value = checked ? 1U : 0U; KSWORD_DEBUGGER_BACKEND_STATUS status{};
            reportError(parent, call(KSWORD_DEBUGGER_USE_HVM, &value, sizeof(value), &status, sizeof(status)));
        });
        struct Toggle { QAction* action; uint32_t KSWORD_DEBUGGER_OPTIONS::* member; };
        auto toggles = std::make_shared<QList<Toggle>>();
        auto option = [&](const char* title, uint32_t KSWORD_DEBUGGER_OPTIONS::* member)
        {
            auto action = menu->addAction(QCoreApplication::translate("KSwordEngine", title)); action->setCheckable(true);
            toggles->append({action, member});
            QObject::connect(action, &QAction::triggered, parent, [parent, member](bool checked)
            {
                KSWORD_DEBUGGER_ENGINE_INFO info{};
                if(!engineInfo(info)) { reportError(parent, ERROR_NOT_SUPPORTED); return; }
                auto options = info.policy.options; options.*member = checked ? 1U : 0U; setOptions(parent, options);
            });
        };
        option(QT_TRANSLATE_NOOP("KSwordEngine", "Prefer hidden Shadow execution breakpoints (stealth policy)"), &KSWORD_DEBUGGER_OPTIONS::mode);
        option(QT_TRANSLATE_NOOP("KSwordEngine", "Shadow execution patches"), &KSWORD_DEBUGGER_OPTIONS::shadowMemoryWrites);
        option(QT_TRANSLATE_NOOP("KSwordEngine", "Allow explicit data/native fallback"), &KSWORD_DEBUGGER_OPTIONS::allowFallback);
        option(QT_TRANSLATE_NOOP("KSwordEngine", "Allow Windows context fallback"), &KSWORD_DEBUGGER_OPTIONS::nativeContextFallback);
        option(QT_TRANSLATE_NOOP("KSwordEngine", "Allow Windows suspend fallback"), &KSWORD_DEBUGGER_OPTIONS::nativeSuspendFallback);
        auto budget = menu->addAction(QCoreApplication::translate("KSwordEngine", "Shadow page limit..."));
        QObject::connect(budget, &QAction::triggered, parent, [parent]
        {
            KSWORD_DEBUGGER_ENGINE_INFO info{};
            if(!engineInfo(info)) { reportError(parent, ERROR_NOT_SUPPORTED); return; }
            bool ok = false;
            const int value = QInputDialog::getInt(parent, "KSword", QCoreApplication::translate("KSwordEngine", "Maximum owned Shadow pages"), info.policy.options.maxShadowPages, 1, 32, 1, &ok);
            if(ok) { auto options = info.policy.options; options.maxShadowPages = value; setOptions(parent, options); }
        });
        auto restore = menu->addAction(QCoreApplication::translate("KSwordEngine", "Restore owned Shadow writes"));
        QObject::connect(restore, &QAction::triggered, parent, [parent]
        {
            KSWORD_DEBUGGER_SHADOW_RESTORE request{KSWORD_DEBUGGER_API_VERSION, sizeof(request), 0, 0};
            KSWORD_DEBUGGER_BACKEND_STATUS status{};
            reportError(parent, call(KSWORD_DEBUGGER_RESTORE_SHADOW_WRITES, &request, sizeof(request), &status, sizeof(status)));
            GuiUpdateAllViews();
        });
        QObject::connect(menu, &QMenu::aboutToShow, parent, [hvm, toggles, budget, restore]
        {
            KSWORD_DEBUGGER_ENGINE_INFO info{};
            const bool available = engineInfo(info);
            const bool mutablePolicy = available && info.provider == KSWORD_DEBUGGER_PROVIDER_NATIVE && info.policy.canChangeOptions != 0;
            hvm->setChecked(available && info.configuredHvm != 0); hvm->setEnabled(mutablePolicy);
            for(const auto& toggle : *toggles)
            { toggle.action->setChecked(available && info.policy.options.*(toggle.member) != 0); toggle.action->setEnabled(mutablePolicy); }
            budget->setEnabled(mutablePolicy);
            restore->setEnabled(available && info.activeHvm != 0 && info.policy.shadowWritePages != 0 && !DbgIsRunning());
        });
    }
}
