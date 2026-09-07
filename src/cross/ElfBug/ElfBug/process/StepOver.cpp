#include <ElfBug/process/StepOver.h>
#include <ElfBug/process/Process.h>
#include <zydis_wrapper.h>

namespace ElfBug
{
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

        if(zydis.IsRepeated())
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
