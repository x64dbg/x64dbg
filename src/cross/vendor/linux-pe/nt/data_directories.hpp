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

#pragma pack(push, WIN_STRUCT_PACKING)
namespace win
{
    static constexpr uint32_t NUM_DATA_DIRECTORIES =    16;

    // Directory indices
    //
    enum directory_id : uint8_t
    {
        directory_entry_export =                        0,                // Export Directory
        directory_entry_import =                        1,                // Import Directory
        directory_entry_resource =                      2,                // Resource Directory
        directory_entry_exception =                     3,                // Exception Directory
        directory_entry_security =                      4,                // Security Directory
        directory_entry_basereloc =                     5,                // Base Relocation Table
        directory_entry_debug =                         6,                // Debug Directory
        directory_entry_copyright =                     7,                // (X86 usage)
        directory_entry_architecture =                  7,                // Architecture Specific Data
        directory_entry_globalptr =                     8,                // RVA of GP
        directory_entry_tls =                           9,                // TLS Directory
        directory_entry_load_config =                   10,               // Load Configuration Directory
        directory_entry_bound_import =                  11,               // Bound Import Directory in headers
        directory_entry_iat =                           12,               // Import Address Table
        directory_entry_delay_import =                  13,               // Delay Load Import Descriptors
        directory_entry_com_descriptor =                14,               // COM Runtime descriptor
        directory_reserved0 =                           15,               // -
    };

    // Declare generic mapping for indices
    //
    template<directory_id id, bool x64, typename = void>
    struct directory_type { using type = char; };
    template<directory_id id, bool x64>
    using directory_type_t = typename directory_type<id, x64>::type;

    // Generic directory descriptors
    //
    struct data_directory_t
    {
        uint32_t                    rva;
        uint32_t                    size;
        inline bool present() const { return size; }
    };
    struct raw_data_directory_t
    {
        uint32_t                    ptr_raw_data;
        uint32_t                    size;
        inline bool present() const { return size; }
    };

    // Data directories
    //
    struct data_directories_x86_t
    {
        union
        {
            struct
            {
                data_directory_t      export_directory;
                data_directory_t      import_directory;
                data_directory_t      resource_directory;
                data_directory_t      exception_directory;
                raw_data_directory_t  security_directory;  // File offset instead of RVA!
                data_directory_t      basereloc_directory;
                data_directory_t      debug_directory;
                data_directory_t      copyright_directory;
                data_directory_t      globalptr_directory;
                data_directory_t      tls_directory;
                data_directory_t      load_config_directory;
                data_directory_t      bound_import_directory;
                data_directory_t      iat_directory;
                data_directory_t      delay_import_directory;
                data_directory_t      com_descriptor_directory;
                data_directory_t      _reserved0;
            };
            data_directory_t          entries[ NUM_DATA_DIRECTORIES ];
        };
    };
    struct data_directories_x64_t
    {
        union
        {
            struct
            {
                data_directory_t      export_directory;
                data_directory_t      import_directory;
                data_directory_t      resource_directory;
                data_directory_t      exception_directory;
                raw_data_directory_t  security_directory;  // File offset instead of RVA!
                data_directory_t      basereloc_directory;
                data_directory_t      debug_directory;
                data_directory_t      architecture_directory;
                data_directory_t      globalptr_directory;
                data_directory_t      tls_directory;
                data_directory_t      load_config_directory;
                data_directory_t      bound_import_directory;
                data_directory_t      iat_directory;
                data_directory_t      delay_import_directory;
                data_directory_t      com_descriptor_directory;
                data_directory_t      _reserved0;
            };
            data_directory_t          entries[ NUM_DATA_DIRECTORIES ];
        };
    };
    template<bool x64 = default_architecture>
    using data_directories_t = std::conditional_t<x64, data_directories_x64_t, data_directories_x86_t>;
};
#pragma pack(pop)