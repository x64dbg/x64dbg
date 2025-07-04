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
    // Declare the data type.
    //
    struct aux_function_delimiter_t
    {
        uint32_t                _pad0;
        uint16_t                line_number;                // Line number
        uint16_t                _pad1;              
        uint32_t                _pad2;              
        uint32_t                sym_next_bf_idx;            // Symbol table index for the next .bf index, if zero last entry.
        uint16_t                _pad;
    };
    static_assert( sizeof( aux_function_delimiter_t ) == sizeof( symbol_t ), "Invalid auxiliary symbol." );
    
    // Declare the matching logic.
    //
    template<>
    inline bool symbol_t::valid_aux<aux_function_delimiter_t>() const
    {
        return ( name.equals_s( ".bf" ) || name.equals_s( ".ef" ) ) &&
               storage_class == storage_class_id::function_delimiter;
    }
};
#pragma pack(pop)