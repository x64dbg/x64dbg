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
#include <stdlib.h>
#include <cstring>
#include "../img_common.hpp"

#pragma pack(push, COFF_STRUCT_PACKING)
namespace coff
{
    // String table.
    //
    union string_table_t
    {
        uint32_t                 size;
        char                     raw_data[ VAR_LEN ];

        // Resolves a string given the offset.
        //
        const char* begin() const { return size > 4 ? &raw_data[ 0 ] : nullptr; }
        const char* end() const { return size > 4 ? &raw_data[ size ] : nullptr; }
        std::string_view resolve( size_t offset ) const
        {
            // Fail if invalid offset.
            //
            if ( offset < 4 ) return {};

            // Search for the null terminator, return if found.
            //
            const char* start = begin() + offset;
            const char* lim = end();
            for ( const char* it = start; it < lim; it++ )
                if ( !*it )
                    return { start, ( size_t ) ( it - start ) };

            // Invalid string.
            //
            return {};
        }
    };

    // External reference to string table.
    //
    struct string_t
    {
        union
        {
            char                 short_name[ LEN_SHORT_STR ];  // Name as inlined string.
            struct
            {
                uint32_t         is_short;                     // If non-zero, name is inline'd into short_name, else has a long name.
                uint32_t         long_name_offset;             // Offset into string table.
            };
        };

        // Convert to string view given an optional string table.
        //
        std::string_view to_string( const string_table_t* tbl = nullptr ) const
        {
            if ( tbl && !is_short )
                return tbl->resolve( long_name_offset );
            size_t len = 0;
            while ( len != LEN_SHORT_STR && short_name[ len ] ) len++;
            return { short_name, len };
        }

        // Array lookup, only available for short strings.
        //
        char& operator[]( size_t n ) { return const_cast<char&>( to_string()[ n ] ); }
        const char& operator[]( size_t n ) const { return to_string()[ n ]; }

        // Basic comparison primitive.
        //
        bool equals( const char* str, const string_table_t* tbl = nullptr ) const { return to_string( tbl ) == str; }

        // Short string comparison primitive.
        //
        template<size_t N> requires( N <= ( LEN_SHORT_STR + 1 ) )
        bool equals_s( const char( &str )[ N ] ) const
        {
            // Compare with against empty string.
            //
            if constexpr ( N == 1 )
                return ( !is_short && !long_name_offset ) || ( is_short && !short_name[ 0 ] );

            // Can skip is short check since if string is not null, is short will be overwritten.
            //
            if constexpr ( N == ( LEN_SHORT_STR + 1 ) )
                return !memcmp( short_name, str, LEN_SHORT_STR );
            else
                return !memcmp( short_name, str, N );
        }
    };

    // Same as above but archive convention, used for section names.
    //
    struct scn_string_t
    {
        char                     short_name[ LEN_SHORT_STR ];

        // Convert to string view given an optional string table.
        //
        std::string_view to_string( const string_table_t* tbl = nullptr ) const
        {
            if ( tbl && short_name[ 0 ] == '/' )
            {
                char* end = ( char* ) std::end( short_name );
                return tbl->resolve( strtoll( short_name + 1, &end, 10 ) );
            }

            size_t len = 0;
            while ( len != LEN_SHORT_STR && short_name[ len ] ) len++;
            return { short_name, len };
        }

        // Array lookup, only available for short strings.
        //
        char& operator[]( size_t n ) { return const_cast<char&>( to_string()[ n ] ); }
        const char& operator[]( size_t n ) const { return to_string()[ n ]; }

        // Basic comparison primitive.
        //
        bool equals( const char* str, const string_table_t* tbl = nullptr ) const { return to_string( tbl ) == str; }

        // Short string comparison primitive.
        //
        template<size_t N> requires( N <= ( LEN_SHORT_STR + 1 ) )
        bool equals_s( const char( &str )[ N ] ) const
        {
            // Compare with against empty string.
            //
            if constexpr ( N == 1 )
                return !short_name[ 0 ];

            // Can skip is short check since if string is not null, is short will be overwritten.
            //
            if constexpr ( N == ( LEN_SHORT_STR + 1 ) )
                return !memcmp( short_name, str, LEN_SHORT_STR );
            else
                return !memcmp( short_name, str, N );
        }
    };
};
#pragma pack(pop)