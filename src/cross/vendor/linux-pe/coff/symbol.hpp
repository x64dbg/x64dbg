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

#pragma pack(push, COFF_STRUCT_PACKING)
namespace coff
{
    // Special section indices.
    //
    enum special_section_id : uint16_t
    {
        symbol_undefined =   0,                                // External symbol
        symbol_absolute =    0xFFFF,                           // Absolute symbol, non-relocatable value.
        symbol_debug =       0xFFFE,                           // Misc. debugging info, not within a section.
    };

    // Storage class.
    //
    enum class storage_class_id : uint8_t
    {
        none =                    0,                           // None.
        auto_variable =           1,                           // Automatic variable.
        public_symbol =           2,                           // Public symbol, global.
        private_symbol =          3,                           // Private symbol, static.
        register_variable =       4,                           // Register variable.
        external_definition =     5,                           // External definition.
        label =                   6,                           // Label.
        undefined_label =         7,                           // Undefined label.
        struct_member =           8,                           // Struct field.
        function_argument =       9,                           // Function argument.
        struct_tag =              10,                          // Struct tag.
        union_member =            11,                          // Union field.
        union_tag =               12,                          // Union tag.
        type_definition =         13,                          // Type definition.
        undefined_static =        14,                          // Undefined static.
        enum_tag =                15,                          // Enumerator tag.
        enum_member =             16,                          // Enumerator entry.
        register_parameter =      17,                          // Register parameter.
        bitfield =                18,                          // Marks a bitfield.
        auto_argument =           19,                          // Automatic argument.
        end_of_block =            20,                          // Marks the end of a block.
        block_delimiter =         100,                         // Marks the beginning or the end of a block.
        function_delimiter =      101,                         // Marks the beginning or the end of a function
        struct_end =              102,                         // Marks the end of a structure definition.
        file_name =               103,                         // File name as symbol.
        line_number =             104,                         // a) Line number as symbol.
        section =                 104,                         // b) Section entry.
        alias_entry =             105,                         // a) Alias to another entry.
        weak_external =           105,                         // b) Weak external.
        hidden_ext_symbol =       106,                         // Hidden external symbol.
        clr_token =               107,                         // CLR token.
        phys_end_of_function =    255,                         // Marks physical end of a function.
    };

    // Type identifiers.
    //
    enum class base_type_id : uint16_t
    {
        none =                    0,
        t_void =                  1,
        t_char =                  2,
        t_short =                 3,
        t_int =                   4,
        t_long =                  5,
        t_float =                 6,
        t_double =                7,
        t_struct =                8,
        t_union =                 9,
        t_enum =                  10,
        t_enum_mem =              11,
        t_uchar =                 12,
        t_ushort =                13,
        t_uint =                  14,
        t_ulong =                 15,
    };
    enum class derived_type_id : uint16_t
    {
        none =                    0,                           // Not derived.
        pointer =                 1,                           // Pointer to base type.
        function =                2,                           // Function returning base type.
        c_array =                 3,                           // Array of base type.
    };

    // Symbol table entry.
    //
    struct symbol_t
    {
        string_t                 name;                         // Name of the symbol.
        int32_t                  value;                        // Value associated with the symbol, interp. depends on the type, usually address of the entry.
        uint16_t                 section_index;                // Special index or scn#+1.

        base_type_id             base_type    : 4;             // Base and derived type describing the symbol.
        derived_type_id          derived_type : 12;            //

        storage_class_id         storage_class;                // Storage class as described above.
        uint8_t                  num_auxiliary;                // Auxiliary data following this symbol.

        // Dynamic logic for auxiliary entries.
        //
        template<typename T>     bool valid_aux() const;

        // Checks whether or not this symbol has a section.
        //
        bool has_section() const
        {
            return section_index != symbol_debug &&
                   section_index != symbol_absolute &&
                   section_index != symbol_undefined;
        }
    };
    static_assert( sizeof( symbol_t ) == 18, "Invalid enum bitfield." );
};
#pragma pack(pop)