/*
Copyright (c) 2013. The YARA Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef _ELF_H
#define _ELF_H

#include <stdint.h>


// 32-bit ELF base types

typedef uint32_t elf32_addr_t;
typedef uint16_t elf32_half_t;
typedef uint32_t elf32_off_t;
typedef uint32_t elf32_word_t;

// 64-bit ELF base types

typedef uint64_t elf64_addr_t;
typedef uint16_t elf64_half_t;
typedef uint64_t elf64_off_t;
typedef uint32_t elf64_word_t;
typedef uint64_t elf64_xword_t;

#define ELF_MAGIC       0x464C457F

#define ELF_ET_NONE     0x0000  // no type
#define ELF_ET_REL      0x0001  // relocatable
#define ELF_ET_EXEC     0x0002  // executeable
#define ELF_ET_DYN      0x0003  // Shared-Object-File
#define ELF_ET_CORE     0x0004  // Corefile
#define ELF_ET_LOPROC   0xFF00  // Processor-specific
#define ELF_ET_HIPROC   0x00FF  // Processor-specific

#define ELF_EM_NONE         0x0000  // no type
#define ELF_EM_M32          0x0001  // AT&T WE 32100
#define ELF_EM_SPARC        0x0002  // SPARC
#define ELF_EM_386          0x0003  // Intel 80386
#define ELF_EM_68K          0x0004  // Motorola 68000
#define ELF_EM_88K          0x0005  // Motorola 88000
#define ELF_EM_860          0x0007  // Intel 80860
#define ELF_EM_MIPS         0x0008  // MIPS I Architecture
#define ELF_EM_MIPS_RS3_LE  0x000A  // MIPS RS3000 Little-endian
#define ELF_EM_PPC          0x0014  // PowerPC
#define ELF_EM_PPC64        0x0015  // 64-bit PowerPC
#define ELF_EM_ARM          0x0028  // ARM
#define ELF_EM_X86_64       0x003E  // AMD/Intel x86_64
#define ELF_EM_AARCH64      0x00B7  // 64-bit ARM

#define ELF_CLASS_NONE  0x0000
#define ELF_CLASS_32    0x0001  // 32bit file
#define ELF_CLASS_64    0x0002  // 64bit file

#define ELF_DATA_NONE   0x0000
#define ELF_DATA_2LSB   0x0001
#define ELF_DATA_2MSB   0x002


#define ELF_SHT_NULL         0     // Section header table entry unused
#define ELF_SHT_PROGBITS     1     // Program data
#define ELF_SHT_SYMTAB       2     // Symbol table
#define ELF_SHT_STRTAB       3     // String table
#define ELF_SHT_RELA         4     // Relocation entries with addends
#define ELF_SHT_HASH         5     // Symbol hash table
#define ELF_SHT_DYNAMIC      6     // Dynamic linking information
#define ELF_SHT_NOTE         7     // Notes
#define ELF_SHT_NOBITS       8     // Program space with no data (bss)
#define ELF_SHT_REL          9     // Relocation entries, no addends
#define ELF_SHT_SHLIB        10    // Reserved
#define ELF_SHT_DYNSYM       11    // Dynamic linker symbol table
#define ELF_SHT_NUM          12    // Number of defined types

#define ELF_SHF_WRITE        0x1   // Section is writable
#define ELF_SHF_ALLOC        0x2   // Section is present during execution
#define ELF_SHF_EXECINSTR    0x4   // Section contains executable instructions

#define ELF_SHN_LORESERVE    0xFF00

#define ELF_PT_NULL          0     // The array element is unused
#define ELF_PT_LOAD          1     // Loadable segment
#define ELF_PT_DYNAMIC       2     // Segment contains dynamic linking info
#define ELF_PT_INTERP        3     // Contains interpreter pathname
#define ELF_PT_NOTE          4     // Location & size of auxiliary info
#define ELF_PT_SHLIB         5     // Reserved, unspecified semantics
#define ELF_PT_PHDR          6     // Location and size of program header table
#define ELF_PT_TLS           7     // Thread-Local Storage
#define ELF_PT_GNU_EH_FRAME  0x6474e550
#define ELF_PT_GNU_STACK     0x6474e551

#define ELF_PF_X             0x1   // Segment is executable
#define ELF_PF_W             0x2   // Segment is writable
#define ELF_PF_R             0x4   // Segment is readable

#define ELF_PN_XNUM          0xffff

#pragma pack(push,1)

typedef struct
{
    uint32_t magic;
    uint8_t _class;
    uint8_t data;
    uint8_t version;
    uint8_t pad[8];
    uint8_t nident;

} elf_ident_t;


typedef struct
{
    elf_ident_t     ident;
    elf32_half_t    type;
    elf32_half_t    machine;
    elf32_word_t    version;
    elf32_addr_t    entry;
    elf32_off_t     ph_offset;
    elf32_off_t     sh_offset;
    elf32_word_t    flags;
    elf32_half_t    header_size;
    elf32_half_t    ph_entry_size;
    elf32_half_t    ph_entry_count;
    elf32_half_t    sh_entry_size;
    elf32_half_t    sh_entry_count;
    elf32_half_t    sh_str_table_index;

} elf32_header_t;


typedef struct
{
    elf_ident_t     ident;
    elf64_half_t    type;
    elf64_half_t    machine;
    elf64_word_t    version;
    elf64_addr_t    entry;
    elf64_off_t     ph_offset;
    elf64_off_t     sh_offset;
    elf64_word_t    flags;
    elf64_half_t    header_size;
    elf64_half_t    ph_entry_size;
    elf64_half_t    ph_entry_count;
    elf64_half_t    sh_entry_size;
    elf64_half_t    sh_entry_count;
    elf64_half_t    sh_str_table_index;

} elf64_header_t;


typedef struct
{
    elf32_word_t    type;
    elf32_off_t     offset;
    elf32_addr_t    virt_addr;
    elf32_addr_t    phys_addr;
    elf32_word_t    file_size;
    elf32_word_t    mem_size;
    elf32_word_t    flags;
    elf32_word_t    alignment;

} elf32_program_header_t;


typedef struct
{
    elf64_word_t    type;
    elf64_word_t    flags;
    elf64_off_t     offset;
    elf64_addr_t    virt_addr;
    elf64_addr_t    phys_addr;
    elf64_xword_t   file_size;
    elf64_xword_t   mem_size;
    elf64_xword_t   alignment;

} elf64_program_header_t;


typedef struct
{
    elf32_word_t    name;
    elf32_word_t    type;
    elf32_word_t    flags;
    elf32_addr_t    addr;
    elf32_off_t     offset;
    elf32_word_t    size;
    elf32_word_t    link;
    elf32_word_t    info;
    elf32_word_t    align;
    elf32_word_t    entry_size;

} elf32_section_header_t;


typedef struct
{
    elf64_word_t    name;
    elf64_word_t    type;
    elf64_xword_t   flags;
    elf64_addr_t    addr;
    elf64_off_t     offset;
    elf64_xword_t   size;
    elf64_word_t    link;
    elf64_word_t    info;
    elf64_xword_t   align;
    elf64_xword_t   entry_size;

} elf64_section_header_t;


#pragma pack(pop)

#endif