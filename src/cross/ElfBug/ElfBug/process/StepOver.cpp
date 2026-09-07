#include <ElfBug/process/StepOver.h>
#include <ElfBug/process/Process.h>
#include <zydis_wrapper.h>

namespace ElfBug
{
    namespace
    {
        // IsRepeated in src/dbg/commands/cmd-debug-control.cpp. Only string ops
        // repeat under F3/F2; elsewhere the prefix is padding (`rep ret`, `pause`).
        bool isRepeatedStringOp(const Zydis & zydis)
        {
            switch(zydis.GetId())
            {
            case ZYDIS_MNEMONIC_INSB:
            case ZYDIS_MNEMONIC_INSW:
            case ZYDIS_MNEMONIC_INSD:
            case ZYDIS_MNEMONIC_OUTSB:
            case ZYDIS_MNEMONIC_OUTSW:
            case ZYDIS_MNEMONIC_OUTSD:
            case ZYDIS_MNEMONIC_MOVSB:
            case ZYDIS_MNEMONIC_MOVSW:
            case ZYDIS_MNEMONIC_MOVSD:
            case ZYDIS_MNEMONIC_MOVSQ:
            case ZYDIS_MNEMONIC_LODSB:
            case ZYDIS_MNEMONIC_LODSW:
            case ZYDIS_MNEMONIC_LODSD:
            case ZYDIS_MNEMONIC_LODSQ:
            case ZYDIS_MNEMONIC_STOSB:
            case ZYDIS_MNEMONIC_STOSW:
            case ZYDIS_MNEMONIC_STOSD:
            case ZYDIS_MNEMONIC_STOSQ:
            case ZYDIS_MNEMONIC_CMPSB:
            case ZYDIS_MNEMONIC_CMPSW:
            case ZYDIS_MNEMONIC_CMPSD:
            case ZYDIS_MNEMONIC_CMPSQ:
            case ZYDIS_MNEMONIC_SCASB:
            case ZYDIS_MNEMONIC_SCASW:
            case ZYDIS_MNEMONIC_SCASD:
            case ZYDIS_MNEMONIC_SCASQ:
                break;
            default:
                return false;
            }

            const auto* instr = zydis.GetInstr();
            constexpr auto repMask = ZYDIS_ATTRIB_HAS_REP | ZYDIS_ATTRIB_HAS_REPE | ZYDIS_ATTRIB_HAS_REPNE;
            return instr && (instr->info.attributes & repMask) != 0;
        }
    }

    StepOverKind ClassifyStepOver(const uint8* bytes, const size_t n, const ptr rip, ptr & nextAddr)
    {
        nextAddr = 0;
        if(!bytes || n == 0)
            return StepOverKind::None;

        Zydis zydis(true);
        if(!zydis.Disassemble(rip, bytes, n))
            return StepOverKind::None;

        const ptr next = rip + zydis.Size();

        if(zydis.IsCall())
        {
            nextAddr = next;
            return StepOverKind::Call;
        }

        if(isRepeatedStringOp(zydis))
        {
            nextAddr = next;
            return StepOverKind::Rep;
        }

        // Single-stepping sets EFLAGS.TF, which the tracee's own pushf would observe.
        switch(zydis.GetId())
        {
        case ZYDIS_MNEMONIC_PUSHF:
        case ZYDIS_MNEMONIC_PUSHFD:
        case ZYDIS_MNEMONIC_PUSHFQ:
            nextAddr = next;
            return StepOverKind::Pushf;
        default:
            break;
        }

        return StepOverKind::None;
    }

    StepOverKind Process::ClassifyStepOverAt(const ptr rip, ptr & nextAddr) const
    {
        nextAddr = 0;

        uint8 buffer[MAX_DISASM_BUFFER] = {};
        ptr bytesRead = 0;

        (void)MemRead(rip, buffer, sizeof(buffer), &bytesRead);
        if(bytesRead == 0)
            return StepOverKind::None;

        return ClassifyStepOver(buffer, static_cast<size_t>(bytesRead), rip, nextAddr);
    }
}
