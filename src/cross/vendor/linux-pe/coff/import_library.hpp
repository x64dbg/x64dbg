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
#include "file_header.hpp"
#include <string_view>

#pragma pack(push, COFF_STRUCT_PACKING)
namespace coff
{
	static constexpr uint32_t import_lib_magic = 0xFFFF0000;

	
	enum class import_type : uint16_t // :2
	{
		code =                      0,   // Executable code.
		data =                      1,   // Data.
		rdata =                     2,   // Specified as CONST in the .def file.
	};
	enum class import_name_type : uint16_t // :3
	{
		ordinal =                   0,   // The import is by ordinal. This indicates that the value in the Ordinal/Hint field of the import header 
		                                 // is the import's ordinal. If this constant is not specified, then the Ordinal/Hint field should always be 
		                                 // interpreted as the import's hint.
		name =                      1,   // The import name is identical to the public symbol name.
		name_no_prefix =            2,   // The import name is the public symbol name, but skipping the leading ?, @, or optionally _.
		name_no_undecorate =        3,   // The import name is the public symbol name, but skipping the leading ?, @, or optionally _, and truncating at the first @.
	};

	// Import library header.
	//
	struct import_header_t
	{
		uint32_t                    magic;
		version_t                   version;
		machine_id                  machine;
		uint32_t                    timedate_stamp;
		uint32_t                    data_size;
		union
		{
			uint16_t                hint;
			uint16_t                ordinal;
		};
		import_type                 type      : 2;
		import_name_type            name_type : 3;
		uint16_t                    reserved  : 11;

		const char* get_symbol_name() const { return ( const char* ) ( this + 1 ); }
		const char* get_library_name() const { return get_symbol_name() + strlen( get_symbol_name() ) + 1; }
	};
	static_assert( sizeof( import_header_t ) == 20, "Invalid enum bitfield." );
};
#pragma pack(pop)