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
#include "nt_headers.hpp"
#include "directories/dir_debug.hpp"
#include "directories/dir_exceptions.hpp"
#include "directories/dir_export.hpp"
#include "directories/dir_iat.hpp"
#include "directories/dir_import.hpp"
#include "directories/dir_relocs.hpp"
#include "directories/dir_tls.hpp"
#include "directories/dir_load_config.hpp"
#include "directories/dir_resource.hpp"
#include "directories/dir_security.hpp"
#include "directories/dir_delay_load.hpp"

namespace win {
	static constexpr uint32_t img_npos = 0xFFFFFFFF;

	// Image wrapper
	//
	template<bool x64 = default_architecture>
	struct image_t {
		dos_header_t                dos_header;

		// Basic getters.
		//
		inline dos_header_t* get_dos_headers() { return &dos_header; }
		inline const dos_header_t* get_dos_headers() const { return &dos_header; }
		inline file_header_t* get_file_header() { return dos_header.get_file_header(); }
		inline const file_header_t* get_file_header() const { return dos_header.get_file_header(); }
		inline nt_headers_t<x64>* get_nt_headers() { return dos_header.get_nt_headers<x64>(); }
		inline const nt_headers_t<x64>* get_nt_headers() const { return dos_header.get_nt_headers<x64>(); }

		// Calculation of optional header checksum.
		//
		inline uint32_t compute_checksum( size_t file_len ) const
		{
			// Sum over each word.
			//
			uint32_t chksum = 0;
			const uint16_t* wdata = ( const uint16_t* ) this;
			for ( size_t n = 0; n != file_len / 2; n++ ) {
				uint32_t sum = wdata[ n ] + chksum;
				chksum = ( uint16_t ) sum + ( sum >> 16 );
			}

			// If there's a byte left append it.
			//
			uint16_t presult = chksum + ( chksum >> 16 );
			if ( file_len & 1 )
				presult += *( ( ( const char* ) this ) + file_len - 1 );

			// Adjust for the previous .checkum field (=0)
			//
			uint16_t* adjust_sum = ( uint16_t* ) &get_nt_headers()->optional_header.checksum;
			for ( size_t i = 0; i != 2; i++ ) {
				presult -= presult < adjust_sum[ i ];
				presult -= adjust_sum[ i ];
			}
			return presult + ( uint32_t ) file_len;
		}
		inline void update_checksum( size_t file_len )
		{
			get_nt_headers()->optional_header.checksum = compute_checksum( file_len );
		}

		// Directory getter
		//
		inline data_directory_t* get_directory( directory_id id )
		{
			auto nt_hdrs = get_nt_headers();
			if ( nt_hdrs->optional_header.num_data_directories <= id ) return nullptr;
			data_directory_t* dir = &nt_hdrs->optional_header.data_directories.entries[ id ];
			return dir->present() ? dir : nullptr;
		}
		inline const data_directory_t* get_directory( directory_id id ) const { return const_cast< image_t* >( this )->get_directory( id ); }

		// Gets the max raw offset referenced.
		//
		inline size_t get_raw_limit() const
		{
			// Initialize the length with the header size.
			//
			auto* nt_hdrs = get_nt_headers();
			size_t max_raw = nt_hdrs->optional_header.size_headers;

			// Calculate max length from the sections.
			//
			auto* scn = nt_hdrs->get_sections();
			for ( size_t i = 0; i != nt_hdrs->file_header.num_sections; i++ )
				max_raw = std::max<size_t>( scn[ i ].ptr_raw_data + scn[ i ].size_raw_data, max_raw );

			// If there is a security directory, which usually is at the end of the image unmapped, also consider that.
			//
			if ( auto dir = get_directory( directory_entry_security ) )
				max_raw = std::max<size_t>( dir->rva + dir->size, max_raw );
			return max_raw;
		}

		// Section mapping
		//
		inline section_header_t* rva_to_section( uint32_t rva )
		{
			auto nt_hdrs = get_nt_headers();
			for ( size_t i = 0; i != nt_hdrs->file_header.num_sections; i++ ) {
				auto section = nt_hdrs->get_section( i );
				if ( section->virtual_address <= rva && rva < ( section->virtual_address + section->virtual_size ) )
					return section;
			}
			return nullptr;
		}
		inline section_header_t* fo_to_section( uint32_t offset )
		{
			auto nt_hdrs = get_nt_headers();
			for ( size_t i = 0; i != nt_hdrs->file_header.num_sections; i++ ) {
				auto section = nt_hdrs->get_section( i );
				if ( section->ptr_raw_data <= offset && offset < ( section->ptr_raw_data + section->size_raw_data ) )
					return section;
			}
			return nullptr;
		}
		inline const section_header_t* rva_to_section( uint32_t rva ) const { return const_cast< image_t* >( this )->rva_to_section( rva ); }
		inline const section_header_t* fo_to_section( uint32_t offset ) const { return const_cast< image_t* >( this )->fo_to_section( offset ); }

		// RVA mappings.
		// - Conversions using pointers are only safe on raw views.
		//
		template<typename T = uint8_t>
		inline T* rva_to_ptr( uint32_t rva, size_t length = 1 )
		{
			// Find the section, try mapping to header if none found.
			//
			auto scn = rva_to_section( rva );
			if ( !scn ) {
				uint32_t rva_hdr_end = get_nt_headers()->optional_header.size_headers;
				if ( rva < rva_hdr_end && ( rva + length ) <= rva_hdr_end )
					return ( T* ) ( ( uint8_t* ) &dos_header + rva );
				return nullptr;
			}

			// Apply the boundary check.
			//
			size_t offset = rva - scn->virtual_address;
			if ( ( offset + length ) > scn->size_raw_data )
				return nullptr;

			// Return the final pointer.
			//
			return ( T* ) ( ( uint8_t* ) &dos_header + scn->ptr_raw_data + offset );
		}
		template<typename T = uint8_t>
		inline const T* rva_to_ptr( uint32_t rva, size_t length = 1 ) const { return const_cast< image_t* >( this )->template rva_to_ptr<const T>( rva, length ); }
		inline uint32_t rva_to_fo( uint32_t rva, size_t length = 1 ) const { return ptr_to_raw( rva_to_ptr( rva, length ) ); }

		// RAW offset mappings.
		// - Conversions using pointers are only safe on mapped views.
		//
		template<typename T = uint8_t>
		inline T* fo_to_ptr( uint32_t offset, size_t length = 1 )
		{
			// Find the section, try mapping to header if none found.
			//
			auto scn = fo_to_section( offset );
			if ( !scn ) {
				uint32_t rva_hdr_end = get_nt_headers()->optional_header.size_headers;
				if ( offset < rva_hdr_end && ( offset + length ) <= rva_hdr_end )
					return ( T* ) ( ( uint8_t* ) &dos_header + offset );
				return nullptr;
			}

			// Apply the boundary check.
			//
			size_t soffset = offset - scn->ptr_raw_data;
			if ( ( soffset + length ) > scn->virtual_size )
				return nullptr;

			// Return the final pointer.
			//
			return ( T* ) ( ( uint8_t* ) &dos_header + scn->virtual_address + soffset );
		}
		template<typename T = uint8_t>
		inline const T* fo_to_ptr( uint32_t offset, size_t length = 1 ) const { return const_cast< image_t* >( this )->template fo_to_ptr<const T>( offset, length ); }
		inline uint32_t fo_to_rva( uint32_t offset, size_t length = 1 ) const { return ptr_to_raw( fo_to_ptr( offset, length ) ); }

		// Raw offset to pointer mapping, no boundary checks by default so this can
		// be used to translate RVA as well if image is mapped.
		// - If length is given, should not be used for RVA translation on mapped images.
		//
		template<typename T = uint8_t>
		inline T* raw_to_ptr( uint32_t offset, size_t length = 0 )
		{
			// Do a basic boundary check if length is given.
			//
			if ( length != 0 && ( offset + length ) > get_raw_limit() )
				return nullptr;

			// Return the final pointer.
			//
			return ( T* ) ( ( uint8_t* ) &dos_header + offset );
		}
		template<typename T = void>
		inline const T* raw_to_ptr( uint32_t rva, size_t length = 0 ) const { return const_cast< image_t* >( this )->template raw_to_ptr<const T>( rva, length ); }
		inline uint32_t ptr_to_raw( const void* ptr ) const { return ptr ? uint32_t( uintptr_t( ptr ) - uintptr_t( &dos_header ) ) : img_npos; }
	};
	using image_x64_t = image_t<true>;
	using image_x86_t = image_t<false>;
};
