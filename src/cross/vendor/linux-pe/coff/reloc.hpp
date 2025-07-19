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

#pragma pack(push, COFF_STRUCT_PACKING)
namespace coff
{
    enum reloc_type : uint16_t
    {
        // AMD64:
        //
        rel_amd64_absolute =      0x0000,                      // The relocation is ignored.
        rel_amd64_addr64 =        0x0001,                      // The 64-bit VA of the relocation target.
        rel_amd64_addr32 =        0x0002,                      // The 32-bit VA of the relocation target.
        rel_amd64_addr32nb =      0x0003,                      // The 32-bit address without an image base (RVA).
        rel_amd64_rel32 =         0x0004,                      // The 32-bit relative address from the byte following the relocation.
        rel_amd64_rel32_1 =       0x0005,                      // The 32-bit address relative to byte distance 1 from the relocation.
        rel_amd64_rel32_2 =       0x0006,                      // The 32-bit address relative to byte distance 2 from the relocation.
        rel_amd64_rel32_3 =       0x0007,                      // The 32-bit address relative to byte distance 3 from the relocation.
        rel_amd64_rel32_4 =       0x0008,                      // The 32-bit address relative to byte distance 4 from the relocation.
        rel_amd64_rel32_5 =       0x0009,                      // The 32-bit address relative to byte distance 5 from the relocation.
        rel_amd64_section =       0x000A,                      // The 16-bit section index of the section that contains the target. This is used to support debugging information.
        rel_amd64_secrel =        0x000B,                      // The 32-bit offset of the target from the beginning of its section. This is used to support debugging information and static thread local storage.
        rel_amd64_secrel7 =       0x000C,                      // A 7-bit unsigned offset from the base of the section that contains the target.
        rel_amd64_token =         0x000D,                      // CLR tokens.
        rel_amd64_srel32 =        0x000E,                      // A 32-bit signed span-dependent value emitted into the object.
        rel_amd64_pair =          0x000F,                      // A pair that must immediately follow every span-dependent value.
        rel_amd64_sspan32 =       0x0010,                      // A 32-bit signed span-dependent value that is applied at link time.

        // I386:
        //
        rel_i386_absolute =       0x0000,                      // The relocation is ignored.
        rel_i386_dir16 =          0x0001,                      // Not supported.
        rel_i386_rel16 =          0x0002,                      // Not supported.
        rel_i386_dir32 =          0x0006,                      // The target's 32-bit VA.
        rel_i386_dir32nb =        0x0007,                      // The target's 32-bit RVA.
        rel_i386_seg12 =          0x0009,                      // Not supported.
        rel_i386_section =        0x000A,                      // The 16-bit section index of the section that contains the target. This is used to support debugging information.
        rel_i386_secrel =         0x000B,                      // The 32-bit offset of the target from the beginning of its section. This is used to support debugging information and static thread local storage.
        rel_i386_token =          0x000C,                      // The CLR token.
        rel_i386_secrel7 =        0x000D,                      // A 7-bit offset from the base of the section that contains the target.
        rel_i386_rel32 =          0x0014,                      // The 32-bit relative displacement to the target. This supports the x86 relative branch and call instructions.
    };

    // Relocation entry.
    //
    struct reloc_t
    {
        uint32_t                  virtual_address;             // Virtual address of the relocated data.
        uint32_t                  symbol_index;                // Symbol index.
        reloc_type                type;                        // Type of the relocation applied.
    };
};
#pragma pack(pop)