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
#include "line_number.hpp"
#include "reloc.hpp"
#include "string.hpp"
#include "symbol.hpp"
#include "file_header.hpp"
#include "import_library.hpp"
#include "section_header.hpp"
#include "auxiliaries/aux_file_name.hpp"
#include "auxiliaries/aux_function.hpp"
#include "auxiliaries/aux_function_delimiter.hpp"
#include "auxiliaries/aux_section.hpp"
#include "auxiliaries/aux_weak_external.hpp"

#pragma pack(push, COFF_STRUCT_PACKING)
namespace coff
{
    // Optional header and the header collection.
    //
	struct optional_header_t
	{
        // Only standard fields as described in the common object file format.
        //
		uint16_t                    magic;
		version_t                   linker_version;

		uint32_t                    size_code;
		uint32_t                    size_init_data;
		uint32_t                    size_uninit_data;

		uint32_t                    entry_point;
		uint32_t                    base_of_code;
		uint32_t                    base_of_data;
	};
	struct image_t
	{
		file_header_t               file_header;
		optional_header_t           optional_header;

		// Section getter
        //
        inline section_header_t* get_sections() { return ( section_header_t* ) ( ( uint8_t* ) &optional_header + file_header.size_optional_header ); }
        inline const section_header_t* get_sections() const { return const_cast< image_t*>( this )->get_sections(); }
        inline section_header_t* get_section( size_t n ) { return get_sections() + n; }
        inline const section_header_t* get_section( size_t n ) const { return get_sections() + n; }

        // Symbol table getter
        //
		inline symbol_t* get_symbols() { return ( symbol_t* ) ( ( uint8_t* ) this + file_header.ptr_symbols ); }
		inline const symbol_t* get_symbols() const { return const_cast< image_t* >( this )->get_symbols(); }
		inline symbol_t* get_symbol( size_t n ) { return get_symbols() + n; }
		inline const symbol_t* get_symbol( size_t n ) const { return get_symbols() + n; }

		// String table getter.
		//
		inline string_table_t* get_strings() { return ( string_table_t* ) ( get_symbols() + file_header.num_symbols ); }
		inline const string_table_t* get_strings() const { return const_cast< image_t* >( this )->get_strings(); }
	};
};
#pragma pack(pop)