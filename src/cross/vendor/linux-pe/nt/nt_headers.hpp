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
#include "../coff/file_header.hpp"
#include "../coff/section_header.hpp"
#include "optional_header.hpp"

#pragma pack(push, WIN_STRUCT_PACKING)
namespace win
{
    // Magic constants
    //
    static constexpr uint16_t DOS_HDR_MAGIC =           0x5A4D;            // "MZ"
    static constexpr uint32_t NT_HDR_MAGIC =            0x00004550;        // "PE\x0\x0"

    // NT headers
    //
    template<bool x64 = default_architecture>
    struct nt_headers_t
    {
        uint32_t                    signature;
        file_header_t               file_header;
        optional_header_t<x64>      optional_header;

        // Section getters
        //
        inline section_header_t* get_sections() { return ( section_header_t* ) ( ( uint8_t* ) &optional_header + file_header.size_optional_header ); }
        inline section_header_t* get_section( size_t n ) { return n >= file_header.num_sections ? nullptr : get_sections() + n; }
        inline const section_header_t* get_sections() const { return const_cast< nt_headers_t* >( this )->get_sections(); }
        inline const section_header_t* get_section( size_t n ) const { return const_cast< nt_headers_t* >( this )->get_section( n ); }

        // Section iterator
        //
        template<typename T>
        struct proxy 
        { 
            T* base;
            uint16_t count;
            T* begin() const { return base; }
            T* end() const { return base + count; }
        };
        inline proxy<section_header_t> sections() { return { get_sections(), file_header.num_sections }; }
        inline proxy<const section_header_t> sections() const { return { get_sections(), file_header.num_sections }; }
    };
    using nt_headers_x64_t = nt_headers_t<true>;
    using nt_headers_x86_t = nt_headers_t<false>;

    // DOS header
    //
    struct dos_header_t
    {
        uint16_t                    e_magic;
        uint16_t                    e_cblp;
        uint16_t                    e_cp;
        uint16_t                    e_crlc;
        uint16_t                    e_cparhdr;
        uint16_t                    e_minalloc;
        uint16_t                    e_maxalloc;
        uint16_t                    e_ss;
        uint16_t                    e_sp;
        uint16_t                    e_csum;
        uint16_t                    e_ip;
        uint16_t                    e_cs;
        uint16_t                    e_lfarlc;
        uint16_t                    e_ovno;
        uint16_t                    e_res[ 4 ];
        uint16_t                    e_oemid;
        uint16_t                    e_oeminfo;
        uint16_t                    e_res2[ 10 ];
        uint32_t                    e_lfanew;

        inline file_header_t* get_file_header() { return &get_nt_headers<>()->file_header; }
        inline const file_header_t* get_file_header() const { return &get_nt_headers<>()->file_header; }
        template<bool x64 = default_architecture> inline nt_headers_t<x64>* get_nt_headers() { return ( nt_headers_t<x64>* ) ( ( uint8_t* ) this + e_lfanew ); }
        template<bool x64 = default_architecture> inline const nt_headers_t<x64>* get_nt_headers() const { return const_cast< dos_header_t* >( this )->template get_nt_headers<x64>(); }
    };
};
#pragma pack(pop)