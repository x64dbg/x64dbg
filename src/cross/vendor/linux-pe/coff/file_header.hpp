// Copyright (c) 2020 Can Boluk
// All rights reserved.   
//    
// Redistribution and use in source and binary forms, with or without   
// modification, are permitted provided that the following conditions are met: 
//    
// 1. Redistributions of source code must retain the above copyright notice,   
//    this list of conditions and the following disclaimer.   
// 2. Redistributions in binary form must reproduce the above copyright   
//    notice, this list of conditions and the following disclaimer in the   
//    documentation and/or other materials provided with the distribution.   
// 3. Neither the name of the copyright holder nor the names of its contributors
//    may be used to endorse or promote products derived from this software 
//    without specific prior written permission.   
//    
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE   
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE  
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE   
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR   
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF   
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS   
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN   
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)   
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE  
// POSSIBILITY OF SUCH DAMAGE.        
//
#pragma once
#include "../img_common.hpp"

#pragma pack(push, WIN_STRUCT_PACKING)
namespace win
{
    // File target machine
    //
    enum class machine_id : uint16_t
    {
        unknown =                                        0x0000,
        target_host =                                    0x0001,             // Useful for indicating we want to interact with the host and not a WoW guest.
        i386 =                                           0x014C,             // Intel 386.
        r3000 =                                          0x0162,             // MIPS little-endian, 0x160 big-endian
        r4000 =                                          0x0166,             // MIPS little-endian
        r10000 =                                         0x0168,             // MIPS little-endian
        wcemipsv2 =                                      0x0169,             // MIPS little-endian WCE v2
        alpha =                                          0x0184,             // Alpha_AXP
        sh3 =                                            0x01A2,             // SH3 little-endian
        sh3dsp =                                         0x01A3,             
        sh3e =                                           0x01A4,             // SH3E little-endian
        sh4 =                                            0x01A6,             // SH4 little-endian
        sh5 =                                            0x01A8,             // SH5
        arm =                                            0x01C0,             // ARM Little-Endian
        thumb =                                          0x01C2,             // ARM Thumb/Thumb-2 Little-Endian
        armnt =                                          0x01C4,             // ARM Thumb-2 Little-Endian
        am33 =                                           0x01D3,             
        powerpc =                                        0x01F0,             // IBM PowerPC Little-Endian
        powerpcfp =                                      0x01F1,             
        ia64 =                                           0x0200,             // Intel 64
        mips16 =                                         0x0266,             // MIPS
        alpha64 =                                        0x0284,             // ALPHA64
        mipsfpu =                                        0x0366,             // MIPS
        mipsfpu16 =                                      0x0466,             // MIPS
        axp64 =                                          0x0284,             
        tricore =                                        0x0520,             // Infineon
        cef =                                            0x0CEF,             
        ebc =                                            0x0EBC,             // EFI Byte Code
        amd64 =                                          0x8664,             // AMD64 (K8)
        m32r =                                           0x9041,             // M32R little-endian
        arm64 =                                          0xAA64,             // ARM64 Little-Endian
        cee =                                            0xC0EE,
    };

    // File characteristics
    //
    union file_characteristics_t
    {
        uint16_t                 flags;
        struct                   
        {                        
            uint16_t             relocs_stripped                       : 1;  // Relocation info stripped from file.
            uint16_t             executable                            : 1;  // File is executable  (i.e. no unresolved external references).
            uint16_t             lines_stripped                        : 1;  // Line nunbers stripped from file.
            uint16_t             local_symbols_stripped                : 1;  // Local symbols stripped from file.
            uint16_t             aggressive_ws_trim                    : 1;  // Aggressively trim working set
            uint16_t             large_address_aware                   : 1;  // App can handle >2gb addresses
            uint16_t             _pad0                                 : 1;  
            uint16_t             bytes_reversed_lo                     : 1;  // Bytes of machine word are reversed.
            uint16_t             machine_32                            : 1;  // 32 bit word machine.
            uint16_t             debug_stripped                        : 1;  // Debugging info stripped from file in .DBG file
            uint16_t             runnable_from_swap                    : 1;  // If Image is on removable media, copy and run from the swap file.
            uint16_t             net_run_from_swap                     : 1;  // If Image is on Net, copy and run from the swap file.
            uint16_t             system_file                           : 1;  // System File.
            uint16_t             dll_file                              : 1;  // File is a DLL.
            uint16_t             up_system_only                        : 1;  // File should only be run on a UP machine
            uint16_t             bytes_reversed_hi                     : 1;  // Bytes of machine word are reversed.
        };
    };

    // File header
    //
    struct file_header_t
    {
        machine_id               machine;
        uint16_t                 num_sections;
        uint32_t                 timedate_stamp;
        uint32_t                 ptr_symbols;
        uint32_t                 num_symbols;
        uint16_t                 size_optional_header;
        file_characteristics_t   characteristics;
    };
};
namespace coff 
{ 
    using machine_id =                win::machine_id;
    using file_header_t =             win::file_header_t; 
    using file_characteristics_t =    win::file_characteristics_t;
};
#pragma pack(pop)