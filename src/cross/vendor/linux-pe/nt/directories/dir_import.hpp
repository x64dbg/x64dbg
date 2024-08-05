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
    struct import_directory_t
    {
        union
        {
            uint32_t  characteristics;                // 0 for terminating null import descriptor
            uint32_t  rva_original_first_thunk;       // RVA to original unbound IAT (PIMAGE_THUNK_DATA)
        };
        uint32_t      timedate_stamp;                 // 0 if not bound,
                                                      // -1 if bound, and real date    ime stamp
                                                      //     in IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT (new BIND)
                                                      // O.W. date/time stamp of DLL bound to (Old BIND)

        uint32_t      forwarder_chain;                // -1 if no forwarders
        uint32_t      rva_name;
        uint32_t      rva_first_thunk;                // RVA to IAT (if bound this IAT has actual addresses)
    };

    template<bool x64> struct directory_type<directory_id::directory_entry_import, x64, void> { using type = import_directory_t; };
};
#pragma pack(pop)