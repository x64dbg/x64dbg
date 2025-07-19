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
#include "string.hpp"

#pragma pack(push, WIN_STRUCT_PACKING)
namespace coff
{
    // Section characteristics
    //
    union section_characteristics_t
    {
        uint32_t flags;
        struct
        {
            uint32_t _pad0                              : 5;
            uint32_t cnt_code                           : 1;            // Section contains code.
            uint32_t cnt_init_data                      : 1;            // Section contains initialized data.
            uint32_t cnt_uninit_data                    : 1;            // Section contains uninitialized data.
            uint32_t _pad1                              : 1;
            uint32_t lnk_info                           : 1;            // Section contains comments or some other type of information.
            uint32_t _pad2                              : 1;
            uint32_t lnk_remove                         : 1;            // Section contents will not become part of image.
            uint32_t lnk_comdat                         : 1;            // Section contents comdat.
            uint32_t _pad3                              : 1;
            uint32_t no_defer_spec_exc                  : 1;            // Reset speculative exceptions handling bits in the TLB entries for this section.
            uint32_t mem_far                            : 1;
            uint32_t _pad4                              : 1;
            uint32_t mem_purgeable                      : 1;
            uint32_t mem_locked                         : 1;
            uint32_t mem_preload                        : 1;
            uint32_t alignment                          : 4;            // Alignment calculated as: n ? 1 << ( n - 1 ) : 16 
            uint32_t lnk_nreloc_ovfl                    : 1;            // Section contains extended relocations.
            uint32_t mem_discardable                    : 1;            // Section can be discarded.
            uint32_t mem_not_cached                     : 1;            // Section is not cachable.
            uint32_t mem_not_paged                      : 1;            // Section is not pageable.
            uint32_t mem_shared                         : 1;            // Section is shareable.
            uint32_t mem_execute                        : 1;            // Section is executable.
            uint32_t mem_read                           : 1;            // Section is readable.
            uint32_t mem_write                          : 1;            // Section is writeable.
        };

        inline size_t get_alignment() const { return win::convert_alignment( alignment ); }
        inline bool set_alignment( size_t align ) { return alignment = win::reflect_alignment( align ); }
    };

    // Section header
    //
    struct section_header_t
    {
        scn_string_t                name;
        union
        {
            uint32_t                physical_address;
            uint32_t                virtual_size;
        };
        uint32_t                    virtual_address;
        
        uint32_t                    size_raw_data;
        uint32_t                    ptr_raw_data;
        
        uint32_t                    ptr_relocs;
        uint32_t                    ptr_line_numbers;
        uint16_t                    num_relocs;
        uint16_t                    num_line_numbers;
        
        section_characteristics_t   characteristics;

        // Characteristics NT checks based on the name as well as the real flag
        //
        bool is_paged() const
        {
            return !characteristics.mem_not_paged && ( *( uint32_t* ) &name.short_name == 'ade.' || *( uint32_t* ) &name.short_name == 'EGAP' );
        }
        bool is_discardable() const
        {
            return characteristics.mem_discardable || *( uint32_t* ) &name.short_name == 'TINI';
        }
    };
};
namespace win
{ 
    using section_header_t =             coff::section_header_t;
    using section_characteristics_t =    coff::section_characteristics_t;
};
#pragma pack(pop)