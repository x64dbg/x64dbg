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
#include <utility>
#include <string>

#include "../../img_common.hpp"
#include "../data_directories.hpp"

#pragma pack(push, WIN_STRUCT_PACKING)
namespace win
{
	namespace impl
	{
		inline constexpr char hexdigits[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

		inline constexpr void fmt_u8_0pad( char*& out, uint8_t value )
		{
			*out++ = hexdigits[ value >> 4 ];
			*out++ = hexdigits[ value & 0xF ];
		}

		template<typename T>
		inline constexpr void fmt_uhex_0pad( char*& out, T value )
		{
			if constexpr ( sizeof( T ) == 1 )
				return fmt_u8_0pad( out, value );
			for ( size_t n = 0; n != sizeof( T ); n++ )
				fmt_u8_0pad( out, uint8_t( value >> ( ( sizeof( T ) - n - 1 ) * 8 ) ) );
		}

		inline constexpr void fmt_udec( char*& out, uint32_t value )
		{
			size_t idx = 0;
			do
			{
				out[ idx++ ] = ( value % 10 ) + '0';
				value /= 10;
			}
			while ( value );

			size_t swap_count = idx / 2;
			for ( size_t i = 0; i != swap_count; i++ )
				std::swap( out[ i ], out[ idx - i - 1 ] );

			out += idx;
		}
	};

	enum class debug_directory_type_id : uint32_t
	{
		unknown =       0x00000000,
		coff =          0x00000001,
		codeview =      0x00000002,
		fpo =           0x00000003,
		misc =          0x00000004,
		exception =     0x00000005,
		fixup =         0x00000006,
		omap_to_src =   0x00000007,
		omap_from_src = 0x00000008,
		borland =       0x00000009,
		reserved10 =    0x0000000A,
		clsid =         0x0000000B,
		vc_feature =    0x0000000C,
		pogo =          0x0000000D,
		iltcg =         0x0000000E,
		mpx =           0x0000000F,
		repro =         0x00000010,
	};

	// Codeview headers
	//
	enum class cv_signature : uint32_t
	{
		cv41 =  0x3930424E, // 'NB09'
		pdb20 = 0x3031424E, // 'NB10'
		cv50 =  0x3131424E, // 'NB11'
		pdb70 = 0x53445352, // 'RSDS'
	};

	struct cv_header_t
	{
		cv_signature                signature;
	};

	struct cv_pdb20_t : cv_header_t
	{
		uint32_t                    offset;          // If not zero, stored within the image.
		uint32_t                    timedate_stamp;
		uint32_t                    age;

	};

	struct cv_pdb70_t : cv_header_t
	{
		guid_t                      guid;
		uint32_t                    age;
		char                        pdb_name[ VAR_LEN ];

		// Formats into the MSDL format.
		//
		inline std::string format() const
		{
			std::string result;
			result.resize( 8 + 8 + 16 + 10 );

			char* it = result.data();
			// u32 * 1 = 8 digits
			impl::fmt_uhex_0pad( it, guid.dword );
			// u16 * 2 = 8 digits
			for ( uint16_t v : guid.word )
				impl::fmt_uhex_0pad( it, v );
			// u8 * 8 = 16 digits
			for ( uint8_t v : guid.byte )
				impl::fmt_uhex_0pad( it, v );
			// u32 dec = max 10 digits
			impl::fmt_udec( it, age );

			result.resize( it - result.data() );
			return result;
		}
	};

	// Misc headers
	//
	enum class misc_debug_data_id : uint32_t
	{
		exe_name = 1,
	};

	struct misc_debug_data_t
	{
		misc_debug_data_id          data_type;
		uint32_t                    record_length;
		uint8_t                     is_unicode;
		uint8_t                     _pad[ 3 ];
		uint8_t                     data[ VAR_LEN ];
	};

	// Directory type.
	//
	struct debug_directory_entry_t
	{
		uint32_t                    characteristics;
		uint32_t                    timedate_stamp;
		ex_version_t                version;
		debug_directory_type_id     type;
		uint32_t                    size_raw_data;
		uint32_t                    rva_raw_data;
		uint32_t                    ptr_raw_data;
	};

	struct debug_directory_t
	{
		debug_directory_entry_t     entries[ VAR_LEN ];
	};

	template<bool x64> struct directory_type<directory_id::directory_entry_debug, x64, void> { using type = debug_directory_t; };
};
#pragma pack(pop)
