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
#include <vector>
#include <iterator>
#include <string_view>
#include <cstring>
#include <stdlib.h>
#include <type_traits>
#include <unordered_map>
#include "../img_common.hpp"

#pragma pack(push, COFF_STRUCT_PACKING)
namespace ar
{
	// Magic constants.
	//
	constexpr uint64_t format_magic =     0x0A3E686372613C21; // "!<arch>\n"
	constexpr uint16_t entry_terminator = 0x0A60;             // "`\n"

	// Stupid string integers.
	//
	template<size_t B, size_t N>
	struct string_integer
	{
		char string[ N ];

		// Integer to string.
		//
		string_integer( uint64_t integer )
		{
			// Handle zero:
			//
			if ( !integer )
			{
				string[ 0 ] = '0';
				memset( string + 1, ' ', N - 1 );
				return;
			}
			
			// Until all characters are written:
			//
			static constexpr char dictionary[] = "0123456789ABCDEF";
			char* it = std::end( string );
			while ( integer )
			{
				*--it = dictionary[ integer % B ];
				integer /= B;

				// Overflow, malformed.
				//
				if ( it == std::begin( string ) )
				{
					it = std::end( string );
					break;
				}
			}

			// Move to leftmost and right pad with spaces.
			//
			size_t len = ( size_t ) ( std::end( string ) - it );
			memmove( string, it, len );
			memset( string + len, ' ', N - len );
		}
		string_integer( const string_integer& ) = default;
		string_integer& operator=( const string_integer& ) = default;

		// String to integer.
		//
		uint64_t get() const
		{
			char* it = ( char* ) string;
			while ( it != std::end( string ) && *it && *it != ' ' )
				++it;
			return strtoull( string, &it, B );
		}
		operator uint64_t() const { return get(); }
	};

	// Big endian integers.
	//
	template<typename T>
	struct big_endian_t
	{
		uint8_t bytes[ sizeof( T ) ];

		big_endian_t( T val )
		{
			for( size_t i = 0; i != sizeof( T ); i++ )
				bytes[ sizeof( T ) - ( i + 1 ) ] = ( val >> ( 8 * i ) ) & 0xFF;
		}

		T get() const
		{
			T value = 0;
			for ( size_t i = 0; i != sizeof( T ); i++ )
				value |= bytes[ sizeof( T ) - ( i + 1 ) ] << ( 8 * i );
			return value;
		}
		operator T() const { return get(); }
	};

	// File entry.
	//
	struct entry_t
	{
		union
		{
			char                        identifier[ 16 ];
			struct
			{
				char                    _pad;
				string_integer<10, 15>  long_string_offset;
			};
		};
		string_integer<10, 12>          modify_timestamp;
		string_integer<10, 6>           owner_id;
		string_integer<10, 6>           group_id;
		string_integer<8,  8>           mode;
		string_integer<10, 10>          length;
		uint16_t                        terminator;

		// Data getter.
		//
		uint8_t* data() { return ( uint8_t* ) ( this + 1 ); };
		const uint8_t* data() const { return ( const uint8_t* ) ( this + 1 ); };
		uint8_t* begin() { return data(); };
		const uint8_t* begin() const { return data(); };
		uint8_t* end() { return data() + length; };
		const uint8_t* end() const { return data() + length; };

		// Forward to next.
		//
		entry_t* next() { return ( entry_t* ) ( data() + ( ( length + 1 ) & ~1 ) ); }
		const entry_t* next() const { return ( const entry_t* ) ( data() + ( ( length + 1 ) & ~1 ) ); }

		// System-V extension properties.
		//
		bool is_symbol_table() const { return identifier[ 0 ] == '/' && identifier[ 1 ] == ' '; }
		bool is_string_table() const { return identifier[ 0 ] == '/' && identifier[ 1 ] == '/' && identifier[ 2 ] == ' '; }
		bool has_long_name() const { return identifier[ 0 ] == '/' && '0' <= identifier[ 1 ] && identifier[ 1 ] <= '9'; }

		// Convert identifier to string view.
		//
		std::string_view to_string( const entry_t* string_table = nullptr ) const
		{
			const char* begin = std::begin( identifier );
			const char* end = std::end( identifier );
			char terminator = '/';

			if ( has_long_name() )
			{
				begin = ( const char* ) string_table->begin() + long_string_offset;
				end = ( const char* ) string_table->end();
				terminator = '\n';
				if ( begin >= end ) return {};
			}

			auto it = begin;
			while ( it != end && *it != terminator && *it ) it++;
			if ( it != begin && it[ -1 ] == '/' ) --it;
			return { begin, ( size_t ) ( it - begin ) };
		}
	};

	// Archive header.
	//
	struct header_t
	{
		uint64_t               magic;
		entry_t                first_entry;
	};

	// Archive view wrapping the AR format.
	//
	template<bool constant = true>
	struct view
	{
		// Typedefs.
		//
		using archive_type = std::conditional_t<constant, const header_t, header_t>;
		using entry_type =   std::conditional_t<constant, const entry_t, entry_t>;

		// Declare iterator.
		//
		struct iterator
		{
			// Declare iterator traits
			//
			using iterator_category =   std::forward_iterator_tag;
			using difference_type =     int64_t;
			using reference =           std::pair<std::string_view, entry_type&>;
			using pointer =             const void*;

			// Stores current location, the limit and the string table.
			//
			entry_type*                 str_table;
			entry_type*                 at;
			const void*                 limit;

			// Implement forward iteration and basic comparison.
			//
			iterator& operator++() { at = at->next(); return *this; }
			iterator operator++( int ) { auto s = *this; operator++(); return s; }
			bool operator==( const iterator& other ) const 
			{
				if ( !at )       return !other.at || other.at >= other.limit || other.at->terminator != entry_terminator;
				if ( !other.at ) return !at || at >= limit || at->terminator != entry_terminator;
				else             return at == other.at;
			}
			bool operator<( const iterator& other ) const
			{
				if ( !at )       return false;
				if ( !other.at ) return !operator==( other );
				else             return at < other.at;
			}
			bool operator!=( const iterator& other ) const { return !operator==( other ); }

			// Implement iterator interface and the name helper.
			//
			std::string_view to_string() const { return at->to_string( str_table ); }
			reference operator*() const { return reference{ to_string(), *at }; }
			entry_type* operator->() const { return at; }
		};
		using const_iterator = iterator;

		// Holds the archive header and the region limit.
		//
		archive_type*            archive;
		const void*              limit;

		// Holds special tables discovered during construction.
		//
		entry_type*              string_table = nullptr;
		std::vector<entry_type*> symbol_tables = {};
		entry_type*              first_entry = nullptr;

		// Constructed by pointer and the region size.
		//
		view( const void* _archive, size_t size ) : archive( ( archive_type* ) _archive ), limit( ( uint8_t* ) _archive + size )
		{
			// Validate magic.
			//
			if ( archive->magic != format_magic )
				limit = &archive->first_entry;

			// Resolve special entries and assign the first non-special entry.
			//
			for ( entry_type* it = &archive->first_entry; it < limit; it = it->next() )
			{
				if ( it->is_string_table() && !string_table )
				{
					string_table = it;
				}
				else if ( it->is_symbol_table() )
				{
					symbol_tables.emplace_back( it );
				}
				else
				{
					first_entry = it;
					break;
				}
			}
		}
		view( view&& ) noexcept = default;
		view( const view& ) = default;
		view& operator=( view&& ) noexcept = default;
		view& operator=( const view& ) = default;

		// Make iterable.
		//
		iterator begin() const { return { string_table, first_entry, limit }; }
		iterator end() const { return { nullptr, nullptr, nullptr }; }

		// Parser for the System V symbol table.
		//
		std::unordered_multimap<std::string_view, iterator> read_symbols() const
		{
			// Get the table descriptor.
			//
			if ( symbol_tables.empty() ) return {};
			auto* table = symbol_tables.front();
			const uint8_t* it = table->begin();
			const uint8_t* end = table->end();

			// Read entry count.
			//
			if ( ( it + 4 ) > end ) return {};
			uint32_t entry_count = *( const big_endian_t<uint32_t>* ) it;
			it += 4;

			// Reference the offset table.
			//
			if ( ( it + 4 * entry_count ) > end ) return {};
			const big_endian_t<uint32_t>* offsets = ( const big_endian_t<uint32_t>* ) it;
			it += 4 * entry_count;

			// Read the entries one by one.
			//
			std::unordered_multimap<std::string_view, iterator> entries;
			for ( size_t n = 0; n != entry_count; n++ )
			{
				// Read a zero-terminated string.
				//
				const uint8_t* str_begin = it;
				while ( it != end && *it ) it++;
				if ( it == end ) return {}; // Malformed entry.
				std::string_view string{ ( const char* ) str_begin, ( size_t ) ( it++ - str_begin ) };

				// Read the entry.
				//
				entry_type* entry = ( entry_type* ) ( ( uint8_t* ) archive + offsets[ n ].get() );
				if ( entry >= limit || entry->terminator != entry_terminator ) return {}; // Malformed entry.
				iterator entry_iterator = { string_table, entry, limit };
				entries.emplace( std::move( string ), std::move( entry_iterator ) );
			}
			return entries;
		}
	};
	template<typename T> view( T*, size_t ) -> view<std::is_const_v<T>>;
};
#pragma pack(pop)