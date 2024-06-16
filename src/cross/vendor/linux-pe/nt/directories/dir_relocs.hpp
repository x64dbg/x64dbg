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
#include "../../img_common.hpp"
#include "../data_directories.hpp"

#pragma pack(push, WIN_STRUCT_PACKING)
namespace win
{
    enum reloc_type_id : uint16_t
    {
        rel_based_absolute =          0,
        rel_based_high =              1,
        rel_based_low =               2,
        rel_based_high_low =          3,
        rel_based_high_adj =          4,
        rel_based_ia64_imm64 =        9,
        rel_based_dir64 =             10,
    };

    struct reloc_entry_t
    {
        uint16_t                    offset  : 12;
        reloc_type_id               type    : 4;
    };
    static_assert( sizeof( reloc_entry_t ) == 2, "Enum bitfield is not supported." );

    struct reloc_block_t
    {
        uint32_t                    base_rva;
        uint32_t                    size_block;
        reloc_entry_t               entries[ VAR_LEN ];

        inline reloc_block_t* next() { return ( reloc_block_t* ) ( ( char* ) this + this->size_block ); }
        inline const reloc_block_t* next() const { return const_cast< reloc_block_t* >( this )->next(); }
        inline size_t num_entries() const { return ( reloc_entry_t* ) next() - &entries[ 0 ]; }

        inline reloc_entry_t* begin() { return &entries[ 0 ]; }
        inline const reloc_entry_t* begin() const { return &entries[ 0 ]; }
        inline reloc_entry_t* end() { return ( reloc_entry_t* ) next(); }
        inline const reloc_entry_t* end() const { return ( const reloc_entry_t* ) next(); }
    };

    struct reloc_directory_t
    {
        reloc_block_t               first_block;
    };

    template<bool x64> struct directory_type<directory_id::directory_entry_basereloc, x64, void> { using type = reloc_directory_t; };
};
#pragma pack(pop)