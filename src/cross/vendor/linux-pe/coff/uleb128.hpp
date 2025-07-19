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
#include <tuple>
#include <stdint.h>

namespace coff
{
    // Utility to encode/decode a ULEB128 value commonly seen in address signifiance tables.
    //
    inline void encode_uleb128( uint64_t value, std::vector<uint8_t>& out )
    {
        while( 1 )
        {
            // Push the value of the segment and shift the integer.
            //
            auto& segment = out.emplace_back( ( uint8_t ) ( value & 0x7F ) );
            value >>= 7;

            // If we reached zero, break.
            //
            if ( !value ) break;
            segment |= 0x80;
        }
    }
    
    template<typename It1, typename It2>
    inline std::pair<uint64_t, bool> decode_uleb128( It1& it, It2&& end )
    {
        uint64_t value = 0;
        for ( size_t bitcnt = 0; it < end; ++it, bitcnt += 7 )
        {
            // Read one segment and write it into value.
            //
            uint8_t segment = *it;
            value |= uint64_t( segment & 0x7F ) << bitcnt;

            // Make sure we did not overflow out of u64 range.
            //
            if ( ( value >> bitcnt ) != ( segment & 0x7F ) )
                return { value, false };

            // If stream is terminated, return the value.
            //
            if ( !( segment & 0x80 ) )
            {
                ++it;
                return { value, true };
            }
        }

        // Invalid stream.
        //
        return { value, false };
    }

    inline std::vector<uint8_t> encode_uleb128s( const std::vector<uint64_t>& values )
    {
        // Allocate a vector for the result and reserve the maximum size.
        //
        constexpr size_t max_size = ( 64 + 6 ) / 7;
        std::vector<uint8_t> result;
        result.reserve( values.size() * max_size );

        // Encode every value and return.
        //
        for ( uint64_t value : values )
            encode_uleb128( value, result );
        return result;
    }

    template<typename It1, typename It2>
    inline std::vector<uint64_t> decode_uleb128s( It1&& begin, It2&& end )
    {
        // Allocate a vector for the result and reserve an average result size assuming average integer is 22 bits.
        //
        constexpr size_t avg_size = ( 22 + 6 ) / 7;
        std::vector<uint64_t> result;
        result.reserve( ( end - begin ) * avg_size );

        // Read until we reach the end, indicate failure by returning null.
        //
        auto it = begin;
        while ( it != end )
        {
            auto [val, success] = decode_uleb128( it, end );
            if ( !success ) return {};
            result.emplace_back( val );
        }
        return result;
    }

};