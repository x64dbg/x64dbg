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
#include <type_traits>
#include <stdint.h>
#include <cstddef>

#define WIN_STRUCT_PACKING                4 // Structure packings of the variants.
#define COFF_STRUCT_PACKING               1 //
#define LEN_SHORT_STR                     8 // Common short string length used in COFF and it's variants.

// If your compiler does not support zero-len arrays, define VAR_LEN as 1 before including linuxpe.
//
#ifndef VAR_LEN
    #ifndef _MSC_VER
        #define VAR_LEN 1
    #else
        #pragma warning(disable:4200)
        #define VAR_LEN
    #endif
#endif

#pragma pack(push, 1)
namespace win
{
    // Default image architecture
    //
    static constexpr bool default_architecture = sizeof( void* ) == 8;

    // NT versioning
    //
    union version_t
    {
        uint16_t                    identifier;
        struct
        {
            uint8_t                 major;
            uint8_t                 minor;
        };
    };
    union ex_version_t
    {
        uint32_t                    identifier;
        struct
        {
            uint16_t                major;
            uint16_t                minor;
        };
    };

    struct guid_t
    {
        uint32_t                    dword;
        uint16_t                    word[ 2 ];
        uint8_t                     byte[ 8 ];
    };

    // Common alignment helpers
    // - Both return 0 on failure.
    //
    static constexpr size_t convert_alignment( uint8_t align_flag /*: 4*/ )
    {
        if ( align_flag == 0 ) 
            return 16;
        if ( align_flag >= 0xF ) 
            return 0;
        return 1ull << ( align_flag - 1 );
    }
    static constexpr uint8_t reflect_alignment( size_t alignment )
    {
        for ( uint8_t align_flag = 1; align_flag != 0xF; align_flag++ )
            if ( ( 1ull << ( align_flag - 1 ) ) >= alignment )
                return align_flag;
        return 0;
    }
};
namespace coff 
{ 
    using version_t =                win::version_t;
    using ex_version_t =             win::ex_version_t;
};
#pragma pack(pop)