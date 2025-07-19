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
    struct image_named_import_t
    {
        uint16_t            hint;
        char                name[ 1 ];
    };

    struct image_thunk_data_x64_t
    {
        union
        {
            uint64_t        forwarder_string;
            uint64_t        function;
            uint64_t        address;                   // -> image_named_import_t
            struct
            {
                uint64_t    ordinal     : 16;
                uint64_t    _reserved0  : 47;
                uint64_t    is_ordinal  : 1;
            };
        };
    };

    struct image_thunk_data_x86_t
    {
        union
        {
            uint32_t        forwarder_string;
            uint32_t        function;
            uint32_t        address;                   // -> image_named_import_t
            struct
            {
                uint32_t    ordinal     : 16;
                uint32_t    _reserved0  : 15;
                uint32_t    is_ordinal  : 1;
            };
        };
    };

    struct bound_forwarder_ref_t
    {
        uint32_t            timedate_stamp;
        uint16_t            offset_module_name;
    };

    struct bound_import_descriptor_t
    {
        uint32_t            timedate_stamp;
        uint16_t            offset_module_name;
        uint16_t            num_module_forwarder_refs;
    };

    template<bool x64 = default_architecture>
    using image_thunk_data_t = std::conditional_t<x64, image_thunk_data_x64_t, image_thunk_data_x86_t>;

    template<bool x64> struct directory_type<directory_id::directory_entry_iat, x64, void> { using type = image_thunk_data_t<x64>; };
};
#pragma pack(pop)