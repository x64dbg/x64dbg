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
#include "../symbol.hpp"

#pragma pack(push, COFF_STRUCT_PACKING)
namespace coff
{
    enum class weak_external_type : uint32_t
    {
        invalid =               0,
        no_library =            1,                          // No library search for sym1 should be performed.
        library =               2,                          // Library search for sym1 should be performed.
        alias =                 3,                          // Sym1 is just an alias to Sym2.
    };

    // Declare the data type.
    //
    struct aux_weak_external_t
    {
        uint32_t                sym_alias_idx;              // Index of sym2 that should be linked if sym1 does not exist.
        weak_external_type      type;                       // Type of the weak external.
        uint8_t                 _pad[ 10 ];
    };
    static_assert( sizeof( aux_weak_external_t ) == sizeof( symbol_t ), "Invalid auxiliary symbol." );
    
    // Declare the matching logic.
    //
    template<>
    inline bool symbol_t::valid_aux<aux_weak_external_t>() const
    {
        return storage_class == storage_class_id::weak_external &&
               section_index == special_section_id::symbol_undefined &&
               value == 0;
    }
};
#pragma pack(pop)