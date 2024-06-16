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
#include <string_view>
#include "../../img_common.hpp"
#include "../symbol.hpp"

#pragma pack(push, COFF_STRUCT_PACKING)
namespace coff
{
    enum class comdat_select_id : uint8_t
    {
        invalid =                     0,
        select_no_duplicates =        1,                      // Throw multiply defined symbol if matching entry found.
        select_any =                  2,                      // Pick any, discard rest.
        select_same_size =            3,                      // If all options have equivalent sizes pick any, else throw multiply defined symbol.
        select_exact_match =          4,                      // If all options have equivalent size and checksums pick any, else throw multiply defined symbol.
        select_associative =          5,                      // Inherits COMDAT state of another section associated, useful for discaring multiple data and code with the same logic.
        select_largest =              6                       // Picks the largest entry, if matching size arbitrarily chosen.
    };

    // Declare the data type.
    //
    struct aux_section_t
    {
        // Generic information.
        //
        uint32_t                      length;                 // Length of the data, same as size of raw data.
        uint16_t                      num_relocs;             // Number of relocation entries.
        uint16_t                      num_line_numbers;       // Number of line number entries.

        // COMDAT information, applicable if characteristics.lnk_comdat == true.
        //
        uint32_t                      checksum;               // Checksum of the data for COMDAT matching.
        uint16_t                      associative_section;    // One-based index into section table, only used if selection type is ::select_associative.
        comdat_select_id              comdat_select;          // COMDAT selection type.
        
        uint8_t                       _pad[ 3 ];
    };
    static_assert( sizeof( aux_section_t ) == sizeof( symbol_t ), "Invalid auxiliary symbol." );
    
    // Declare the matching logic.
    //
    template<>
    inline bool symbol_t::valid_aux<aux_section_t>() const
    {
        // Must also have matching names, but cannot be checked here.
        //
        return value == 0 &&
               storage_class == storage_class_id::private_symbol &&
               base_type == base_type_id::none &&
               derived_type == derived_type_id::none;
    }
};
#pragma pack(pop)