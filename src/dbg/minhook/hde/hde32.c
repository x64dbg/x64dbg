/*
 * Hacker Disassembler Engine 32 C
 * Copyright (c) 2008-2009, Vyacheslav Patkov.
 * All rights reserved.
 *
 */

#if defined(_M_IX86) || defined(__i386__)

#include "hde32.h"
#include "table32.h"

unsigned int hde32_disasm(const void* code, hde32s* hs)
{
    uint8_t x, c, *p = (uint8_t*)code, cflags, opcode, pref = 0;
    uint8_t* ht = hde32_table, m_mod, m_reg, m_rm, disp_size = 0;

    // Avoid using memset to reduce the footprint.
#ifndef _MSC_VER
    memset((LPBYTE)hs, 0, sizeof(hde32s));
#else
    __stosb((LPBYTE)hs, 0, sizeof(hde32s));
#endif

    for(x = 16; x; x--)
        switch(c = *p++)
        {
        case 0xf3:
            hs->p_rep = c;
            pref |= PRE_F3;
            break;
        case 0xf2:
            hs->p_rep = c;
            pref |= PRE_F2;
            break;
        case 0xf0:
            hs->p_lock = c;
            pref |= PRE_LOCK;
            break;
        case 0x26:
        case 0x2e:
        case 0x36:
        case 0x3e:
        case 0x64:
        case 0x65:
            hs->p_seg = c;
            pref |= PRE_SEG;
            break;
        case 0x66:
            hs->p_66 = c;
            pref |= PRE_66;
            break;
        case 0x67:
            hs->p_67 = c;
            pref |= PRE_67;
            break;
        default:
            goto pref_done;
        }
pref_done:

    hs->flags = (uint32_t)pref << 23;

    if(!pref)
        pref |= PRE_NONE;

    if((hs->opcode = c) == 0x0f)
    {
        hs->opcode2 = c = *p++;
        ht += DELTA_OPCODES;
    }
    else if(c >= 0xa0 && c <= 0xa3)
    {
        if(pref & PRE_67)
            pref |= PRE_66;
        else
            pref &= ~PRE_66;
    }

    opcode = c;
    cflags = ht[ht[opcode / 4] + (opcode % 4)];

    if(cflags == C_ERROR)
    {
        hs->flags |= F_ERROR | F_ERROR_OPCODE;
        cflags = 0;
        if((opcode & -3) == 0x24)
            cflags++;
    }

    x = 0;
    if(cflags & C_GROUP)
    {
        uint16_t t;
        t = *(uint16_t*)(ht + (cflags & 0x7f));
        cflags = (uint8_t)t;
        x = (uint8_t)(t >> 8);
    }

    if(hs->opcode2)
    {
        ht = hde32_table + DELTA_PREFIXES;
        if(ht[ht[opcode / 4] + (opcode % 4)] & pref)
            hs->flags |= F_ERROR | F_ERROR_OPCODE;
    }

    if(cflags & C_MODRM)
    {
        hs->flags |= F_MODRM;
        hs->modrm = c = *p++;
        hs->modrm_mod = m_mod = c >> 6;
        hs->modrm_rm = m_rm = c & 7;
        hs->modrm_reg = m_reg = (c & 0x3f) >> 3;

        if(x && ((x << m_reg) & 0x80))
            hs->flags |= F_ERROR | F_ERROR_OPCODE;

        if(!hs->opcode2 && opcode >= 0xd9 && opcode <= 0xdf)
        {
            uint8_t t = opcode - 0xd9;
            if(m_mod == 3)
            {
                ht = hde32_table + DELTA_FPU_MODRM + t * 8;
                t = ht[m_reg] << m_rm;
            }
            else
            {
                ht = hde32_table + DELTA_FPU_REG;
                t = ht[t] << m_reg;
            }
            if(t & 0x80)
                hs->flags |= F_ERROR | F_ERROR_OPCODE;
        }

        if(pref & PRE_LOCK)
        {
            if(m_mod == 3)
            {
                hs->flags |= F_ERROR | F_ERROR_LOCK;
            }
            else
            {
                uint8_t* table_end, op = opcode;
                if(hs->opcode2)
                {
                    ht = hde32_table + DELTA_OP2_LOCK_OK;
                    table_end = ht + DELTA_OP_ONLY_MEM - DELTA_OP2_LOCK_OK;
                }
                else
                {
                    ht = hde32_table + DELTA_OP_LOCK_OK;
                    table_end = ht + DELTA_OP2_LOCK_OK - DELTA_OP_LOCK_OK;
                    op &= -2;
                }
                for(; ht != table_end; ht++)
                    if(*ht++ == op)
                    {
                        if(!((*ht << m_reg) & 0x80))
                            goto no_lock_error;
                        else
                            break;
                    }
                hs->flags |= F_ERROR | F_ERROR_LOCK;
no_lock_error:
                ;
            }
        }

        if(hs->opcode2)
        {
            switch(opcode)
            {
            case 0x20:
            case 0x22:
                m_mod = 3;
                if(m_reg > 4 || m_reg == 1)
                    goto error_operand;
                else
                    goto no_error_operand;
            case 0x21:
            case 0x23:
                m_mod = 3;
                if(m_reg == 4 || m_reg == 5)
                    goto error_operand;
                else
                    goto no_error_operand;
            }
        }
        else
        {
            switch(opcode)
            {
            case 0x8c:
                if(m_reg > 5)
                    goto error_operand;
                else
                    goto no_error_operand;
            case 0x8e:
                if(m_reg == 1 || m_reg > 5)
                    goto error_operand;
                else
                    goto no_error_operand;
            }
        }

        if(m_mod == 3)
        {
            uint8_t* table_end;
            if(hs->opcode2)
            {
                ht = hde32_table + DELTA_OP2_ONLY_MEM;
                table_end = ht + sizeof(hde32_table) - DELTA_OP2_ONLY_MEM;
            }
            else
            {
                ht = hde32_table + DELTA_OP_ONLY_MEM;
                table_end = ht + DELTA_OP2_ONLY_MEM - DELTA_OP_ONLY_MEM;
            }
            for(; ht != table_end; ht += 2)
                if(*ht++ == opcode)
                {
                    if(*ht++ & pref && !((*ht << m_reg) & 0x80))
                        goto error_operand;
                    else
                        break;
                }
            goto no_error_operand;
        }
        else if(hs->opcode2)
        {
            switch(opcode)
            {
            case 0x50:
            case 0xd7:
            case 0xf7:
                if(pref & (PRE_NONE | PRE_66))
                    goto error_operand;
                break;
            case 0xd6:
                if(pref & (PRE_F2 | PRE_F3))
                    goto error_operand;
                break;
            case 0xc5:
                goto error_operand;
            }
            goto no_error_operand;
        }
        else
            goto no_error_operand;

error_operand:
        hs->flags |= F_ERROR | F_ERROR_OPERAND;
no_error_operand:

        c = *p++;
        if(m_reg <= 1)
        {
            if(opcode == 0xf6)
                cflags |= C_IMM8;
            else if(opcode == 0xf7)
                cflags |= C_IMM_P66;
        }

        switch(m_mod)
        {
        case 0:
            if(pref & PRE_67)
            {
                if(m_rm == 6)
                    disp_size = 2;
            }
            else if(m_rm == 5)
                disp_size = 4;
            break;
        case 1:
            disp_size = 1;
            break;
        case 2:
            disp_size = 2;
            if(!(pref & PRE_67))
                disp_size <<= 1;
        }

        if(m_mod != 3 && m_rm == 4 && !(pref & PRE_67))
        {
            hs->flags |= F_SIB;
            p++;
            hs->sib = c;
            hs->sib_scale = c >> 6;
            hs->sib_index = (c & 0x3f) >> 3;
            if((hs->sib_base = c & 7) == 5 && !(m_mod & 1))
                disp_size = 4;
        }

        p--;
        switch(disp_size)
        {
        case 1:
            hs->flags |= F_DISP8;
            hs->disp.disp8 = *p;
            break;
        case 2:
            hs->flags |= F_DISP16;
            hs->disp.disp16 = *(uint16_t*)p;
            break;
        case 4:
            hs->flags |= F_DISP32;
            hs->disp.disp32 = *(uint32_t*)p;
        }
        p += disp_size;
    }
    else if(pref & PRE_LOCK)
        hs->flags |= F_ERROR | F_ERROR_LOCK;

    if(cflags & C_IMM_P66)
    {
        if(cflags & C_REL32)
        {
            if(pref & PRE_66)
            {
                hs->flags |= F_IMM16 | F_RELATIVE;
                hs->imm.imm16 = *(uint16_t*)p;
                p += 2;
                goto disasm_done;
            }
            goto rel32_ok;
        }
        if(pref & PRE_66)
        {
            hs->flags |= F_IMM16;
            hs->imm.imm16 = *(uint16_t*)p;
            p += 2;
        }
        else
        {
            hs->flags |= F_IMM32;
            hs->imm.imm32 = *(uint32_t*)p;
            p += 4;
        }
    }

    if(cflags & C_IMM16)
    {
        if(hs->flags & F_IMM32)
        {
            hs->flags |= F_IMM16;
            hs->disp.disp16 = *(uint16_t*)p;
        }
        else if(hs->flags & F_IMM16)
        {
            hs->flags |= F_2IMM16;
            hs->disp.disp16 = *(uint16_t*)p;
        }
        else
        {
            hs->flags |= F_IMM16;
            hs->imm.imm16 = *(uint16_t*)p;
        }
        p += 2;
    }
    if(cflags & C_IMM8)
    {
        hs->flags |= F_IMM8;
        hs->imm.imm8 = *p++;
    }

    if(cflags & C_REL32)
    {
rel32_ok:
        hs->flags |= F_IMM32 | F_RELATIVE;
        hs->imm.imm32 = *(uint32_t*)p;
        p += 4;
    }
    else if(cflags & C_REL8)
    {
        hs->flags |= F_IMM8 | F_RELATIVE;
        hs->imm.imm8 = *p++;
    }

disasm_done:

    if((hs->len = (uint8_t)(p - (uint8_t*)code)) > 15)
    {
        hs->flags |= F_ERROR | F_ERROR_LENGTH;
        hs->len = 15;
    }

    return (unsigned int)hs->len;
}

#endif // defined(_M_IX86) || defined(__i386__)
