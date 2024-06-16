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
#include "data_directories.hpp"

#pragma pack(push, WIN_STRUCT_PACKING)
namespace win
{
    // Magic numbers.
    //
    static constexpr uint16_t OPT_HDR32_MAGIC =            0x010B;
    static constexpr uint16_t OPT_HDR64_MAGIC =            0x020B;

    // Subsystems
    //
    enum class subsystem_id : uint16_t
    {
        unknown =                                          0x0000,          // Unknown subsystem.
        native =                                           0x0001,          // Image doesn't require a subsystem.
        windows_gui =                                      0x0002,          // Image runs in the Windows GUI subsystem.
        windows_cui =                                      0x0003,          // Image runs in the Windows character subsystem
        os2_cui =                                          0x0005,          // image runs in the OS/2 character subsystem.
        posix_cui =                                        0x0007,          // image runs in the Posix character subsystem.
        native_windows =                                   0x0008,          // image is a native Win9x driver.
        windows_ce_gui =                                   0x0009,          // Image runs in the Windows CE subsystem.
        efi_application =                                  0x000A,          //
        efi_boot_service_driver =                          0x000B,          //
        efi_runtime_driver =                               0x000C,          //
        efi_rom =                                          0x000D,
        xbox =                                             0x000E,
        windows_boot_application =                         0x0010,
        xbox_code_catalog =                                0x0011,
    };
    
    // DLL characteristics
    //
    union dll_characteristics_t
    {
        uint16_t                    flags;
        struct
        {
            uint16_t                _pad0                   : 5;
            uint16_t                high_entropy_va         : 1;            // Image can handle a high entropy 64-bit virtual address space.
            uint16_t                dynamic_base            : 1;            // DLL can move.
            uint16_t                force_integrity         : 1;            // Code Integrity Image
            uint16_t                nx_compat               : 1;            // Image is NX compatible
            uint16_t                no_isolation            : 1;            // Image understands isolation and doesn't want it
            uint16_t                no_seh                  : 1;            // Image does not use SEH.  No SE handler may reside in this image
            uint16_t                no_bind                 : 1;            // Do not bind this image.
            uint16_t                appcontainer            : 1;            // Image should execute in an AppContainer
            uint16_t                wdm_driver              : 1;            // Driver uses WDM model
            uint16_t                guard_cf                : 1;            // Image supports Control Flow Guard.
            uint16_t                terminal_server_aware   : 1;
        };
    };

    // Optional header
    //
    struct optional_header_x64_t
    {
        // Standard fields.
        uint16_t                    magic;
        version_t                   linker_version;

        uint32_t                    size_code;
        uint32_t                    size_init_data;
        uint32_t                    size_uninit_data;
        
        uint32_t                    entry_point;
        uint32_t                    base_of_code;

        // NT additional fields.
        uint64_t                    image_base;
        uint32_t                    section_alignment;
        uint32_t                    file_alignment;
        
        ex_version_t                os_version;
        ex_version_t                img_version;
        ex_version_t                subsystem_version;
        uint32_t                    win32_version_value;
        
        uint32_t                    size_image;
        uint32_t                    size_headers;
        
        uint32_t                    checksum;
        subsystem_id                subsystem;
        dll_characteristics_t       characteristics;
        
        uint64_t                    size_stack_reserve;
        uint64_t                    size_stack_commit;
        uint64_t                    size_heap_reserve;
        uint64_t                    size_heap_commit;
        
        uint32_t                    ldr_flags;

        uint32_t                    num_data_directories;
        data_directories_x64_t      data_directories;
    };
    struct optional_header_x86_t
    {
        // Standard fields.
        uint16_t                    magic;
        version_t                   linker_version;

        uint32_t                    size_code;
        uint32_t                    size_init_data;
        uint32_t                    size_uninit_data;

        uint32_t                    entry_point;
        uint32_t                    base_of_code;
        uint32_t                    base_of_data;

        // NT additional fields.
        uint32_t                    image_base;
        uint32_t                    section_alignment;
        uint32_t                    file_alignment;

        ex_version_t                os_version;
        ex_version_t                img_version;
        ex_version_t                subsystem_version;
        uint32_t                    win32_version_value;

        uint32_t                    size_image;
        uint32_t                    size_headers;

        uint32_t                    checksum;
        subsystem_id                subsystem;
        dll_characteristics_t       characteristics;

        uint32_t                    size_stack_reserve;
        uint32_t                    size_stack_commit;
        uint32_t                    size_heap_reserve;
        uint32_t                    size_heap_commit;

        uint32_t                    ldr_flags;

        uint32_t                    num_data_directories;
        data_directories_x86_t      data_directories;

        inline bool has_directory( const data_directory_t* dir ) const { return &data_directories.entries[ num_data_directories ] < dir && dir->present(); }
        inline bool has_directory( directory_id id ) const { return has_directory( &data_directories.entries[ id ] ); }
    };
    template<bool x64 = default_architecture>
    using optional_header_t = std::conditional_t<x64, optional_header_x64_t, optional_header_x86_t>;
};
#pragma pack(pop)