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
#include "../../img_common.hpp"
#include "../data_directories.hpp"

#pragma pack(push, WIN_STRUCT_PACKING)
namespace win
{
    union tls_characteristics_t
    {
        uint32_t                    flags;
        struct
        {
            uint32_t                _reserved0     : 20;
            uint32_t                alignment      : 4;
            uint32_t                _reserved1     : 8;
        };

        inline size_t get_alignment() const { return convert_alignment( alignment ); }
        inline bool set_alignment( size_t align ) { return alignment = reflect_alignment( align ); }
    };

    struct tls_directory_x64_t
    {
        uint64_t                    address_raw_data_start;
        uint64_t                    address_raw_data_end;
        uint64_t                    address_index;
        uint64_t                    address_callbacks;
        uint32_t                    size_zero_fill;
        tls_characteristics_t       characteristics;
    };

    struct tls_directory_x86_t
    {
        uint32_t                    address_raw_data_start;
        uint32_t                    address_raw_data_end;
        uint32_t                    address_index;
        uint32_t                    address_callbacks;
        uint32_t                    size_zero_fill;
        tls_characteristics_t       characteristics;
    };

    template<bool x64 = default_architecture>
    using tls_directory_t = std::conditional_t<x64, tls_directory_x64_t, tls_directory_x86_t>;

    template<bool x64> struct directory_type<directory_id::directory_entry_tls, x64, void> { using type = tls_directory_t<x64>; };
};
#pragma pack(pop)