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
#include <string>
#include <string_view>
#include <iterator>
#include "../../img_common.hpp"
#include "../data_directories.hpp"

#pragma pack(push, WIN_STRUCT_PACKING)
namespace win
{
    // Resource type identifiers
    //
    enum class resource_id : uint16_t
    {
        cursor =                    1,
        bitmap =                    2,
        icon =                      3,
        menu =                      4,
        dialog =                    5,
        string =                    6,
        font_dir =                  7,
        font =                      8,
        accelerator =               9,
        rcdata =                    10,
        message_table =             11,
        group_cursor =              12,
        group_icon =                14,
        version =                   16,
        dlg_include =               17,
        plug_play =                 19,
        vxd =                       20,
        ani_cursor =                21,
        ani_icon =                  22,
        html =                      23,
        manifest =                  24,
    };

    // String entry
    //
    struct rsrc_string_t
    {
        uint16_t                    length;
        wchar_t                     name[ VAR_LEN ];
        inline std::wstring_view view() const { return { name, length }; }
    };

    // Data entry
    //
    struct rsrc_data_t
    {
        uint32_t                    rva_data;
        uint32_t                    size_data;
        uint32_t                    code_page;
        uint32_t                    reserved;
    };

    // Generic entry
    //
    struct rsrc_generic_t
    {
        union
        {
            struct
            {
                uint32_t            offset_name     : 31;
                uint32_t            is_named        : 1;
            };
            uint16_t                identifier;
        };
        uint32_t                    offset          : 31;
        uint32_t                    is_directory    : 1;
    };

    // Directory entry
    //
    struct rsrc_directory_t
    {
        uint32_t                    characteristics;
        uint32_t                    timedate_stamp;
        ex_version_t                version;
        uint16_t                    num_named_entries;
        uint16_t                    num_id_entries;
        rsrc_generic_t              entries[ VAR_LEN ];

        inline size_t                        num_entries() const { return ( size_t ) num_named_entries + num_id_entries; }

        // (Tree root) Helper to resolve entry referenced by rsrc_generic_t::offset
        //
        template<typename T> inline T*       at( uint32_t offset ) { return ( T* ) ( ( char* ) this + offset ); }
        template<typename T> inline const T* at( uint32_t offset ) const { return ( T* ) ( ( char* ) this + offset ); }

        // (Tree root) Explicit optional field accessors
        //
        inline rsrc_data_t*                  as_data( const rsrc_generic_t& entry ) { return !entry.is_directory ? at<rsrc_data_t>( entry.offset ) : nullptr; }
        inline rsrc_string_t*                get_name( const rsrc_generic_t& entry ) { return entry.is_named ? at<rsrc_string_t>( entry.offset_name ) : nullptr; }
        inline rsrc_directory_t*             as_directory( const rsrc_generic_t& entry ) { return entry.is_directory ? at<rsrc_directory_t>( entry.offset ) : nullptr; }
        inline const rsrc_data_t*            as_data( const rsrc_generic_t& entry ) const { return !entry.is_directory ? at<rsrc_data_t>( entry.offset ) : nullptr; }
        inline const rsrc_string_t*          get_name( const rsrc_generic_t& entry ) const { return entry.is_named ? at<rsrc_string_t>( entry.offset_name ) : nullptr; }
        inline const rsrc_directory_t*       as_directory( const rsrc_generic_t& entry ) const { return entry.is_directory ? at<rsrc_directory_t>( entry.offset ) : nullptr; }
    };

    // Iterator type propagating reference to tree root
    //
    enum rsrc_directory_depth : int32_t
    {
        rsrc_null =          -1,
        rsrc_type_directory = 0,
        rsrc_name_directory = 1,
        rsrc_lang_directory = 2,
        rsrc_node           = 3
    };
    template<bool C>
    struct base_rsrc_iterator_t
    {
        template<typename T> using M = std::conditional_t<C, std::add_const_t<T>, T>;
        
        // Declare tree traits
        //
        using entry_t =             M<rsrc_generic_t>;
        using data_t =              M<rsrc_data_t>;
        using directory_t =         M<rsrc_directory_t>;

        // Declare container traits
        //
        using iterator =            base_rsrc_iterator_t<C>;
        using const_iterator =      base_rsrc_iterator_t<true>;

        // Declare iterator traits
        //
        using iterator_category =   std::bidirectional_iterator_tag;
        using difference_type =     int32_t;
        using reference =           base_rsrc_iterator_t<C>;
        using pointer =             void*;

        // Tree root, current level and the current entry
        //
        directory_t*                root = nullptr;
        directory_t*                level = nullptr;
        size_t                      idx = 0;
        int32_t                     depth = rsrc_type_directory;

        // Implement bidirectional iteration and basic comparison
        //
        inline iterator&                   operator++() { idx++; return *this; }
        inline iterator&                   operator--() { idx--; return *this; }
        inline iterator                    operator++( int ) { auto s = *this; operator++(); return s; }
        inline iterator                    operator--( int ) { auto s = *this; operator--(); return s; }
        inline bool                        operator==( const iterator& other ) const { return root == other.root && level == other.level && idx == other.idx; }
        inline bool                        operator!=( const iterator& other ) const { return root != other.root || level != other.level || idx != other.idx; }

        // Implement tree interface
        //
        inline bool                        is_null() const { return !level; }
        inline entry_t&                    entry() { return level->entries[ idx ]; }
        inline const entry_t&              entry() const { return level->entries[ idx ]; }
        inline bool                        has_name() const { return entry().is_named; }
        inline std::wstring_view           name() const { return has_name() ? root->get_name( entry() )->view() : L""; }
        inline bool                        has_id() const { return !entry().is_named; }
        inline uint16_t                    id() const { return has_id() ? entry().identifier : 0; }
        inline resource_id                 rid() const { return ( resource_id ) id(); }
        inline bool                        is_data() const { return !entry().is_directory; }
        inline data_t*                     data() { return root->as_data( entry() ); }
        inline const data_t*               data() const { return root->as_data( entry() ); }
        inline bool                        is_directory() const { return entry().is_directory; }
        inline directory_t*                directory() { return root->as_directory( entry() ); }
        inline const directory_t*          directory() const { return root->as_directory( entry() ); }
        inline explicit operator bool() const { return !is_null(); }

        // Implement iterator interface
        //
        inline iterator&                   operator*() { return *this; }
        inline const_iterator&             operator*() const { return *this; }
        inline data_t*                     operator->() { return data(); }
        inline const data_t*               operator->() const { return data(); }
        inline operator const const_iterator&() const { return *( const_iterator* ) this; }

        // Implement container interface
        // - operator[] is used for lookups.
        //
        inline bool                        empty() const { return size() == 0; }
        inline size_t                      size() const { return !is_null() && is_directory() ? directory()->num_entries() : 0; }
        inline iterator                    at( size_t n ) { return { .root = root, .level = directory(), .idx = n, .depth = depth + 1 }; }
        inline const_iterator              at( size_t n ) const { return const_cast< iterator* >( this )->at( n ); }
        inline iterator                    begin() { return at( 0 ); }
        inline iterator                    end() { return at( size() ); }
        inline const_iterator              begin() const { return at( 0 ); }
        inline const_iterator              end() const { return at( size() ); }
        inline iterator                    front() { return at( 0 ); }
        inline iterator                    back() { return at( size() - 1 ); }
        inline const_iterator              front() const { return at( 0 ); }
        inline const_iterator              back() const { return at( size() - 1 ); }

        // Implement lookups.
        //
        template<typename T>
        inline iterator                    find_if( T&& fn ) const
        {
            for ( auto it = begin(); it.idx != size(); ++it )
                if ( fn( it ) ) return it;
            return { .root = root, .level = nullptr, .idx = 0, .depth = rsrc_null };
        }
        inline iterator                    find( uint16_t u_id ) const { return find_if( [ & ] ( const auto& it ) { return it.has_id() && it.id() == u_id; } ); }
        inline iterator                    find( resource_id r_id ) const { return find( ( uint16_t ) r_id ); }
        inline iterator                    find( std::wstring_view name ) const { return find_if( [ & ] ( const auto& it ) { return it.has_name() && it.name() == name; } ); }
        template<typename T> inline auto   operator[]( T&& v ) const { return find( std::forward<T>( v ) ); }
    };

    // Resource directory, which is essentially the root of the tree.
    // - Type Directory
    //    - Name Directory
    //       - Lang Directory
    //
    struct resource_directory_t
    {
        // Container traits.
        //
        using iterator =            base_rsrc_iterator_t<false>;
        using const_iterator =      base_rsrc_iterator_t<true>;
        using value_type =          iterator;
        
        // Root entry.
        //
        rsrc_directory_t            type_directory;

        // Implement container interface
        // - operator[] is used for lookups.
        //
        inline bool                        empty() const { return size() == 0; }
        inline size_t                      size() const { return type_directory.num_entries(); }
        inline iterator                    at( size_t n ) { return { .root = &type_directory, .level = &type_directory, .idx = n, .depth = rsrc_type_directory }; }
        inline const_iterator              at( size_t n ) const { return const_cast< resource_directory_t* >( this )->at( n ); }
        inline iterator                    begin() { return at( 0 ); }
        inline iterator                    end()   { return at( size() ); }
        inline const_iterator              begin() const { return at( 0 ); }
        inline const_iterator              end()   const { return at( size() ); }
        inline iterator                    front() { return at( 0 ); }
        inline iterator                    back() { return at( size() - 1 ); }
        inline const_iterator              front() const { return at( 0 ); }
        inline const_iterator              back() const { return at( size() - 1 ); }

        // Implement lookups.
        //
        template<typename T>
        inline const_iterator              find_if( T&& fn ) const
        {
            for ( auto it = begin(); it.idx != size(); ++it )
                if ( fn( it ) )  return it;
            return { .root = &type_directory, .level = nullptr, .idx = 0, .depth = rsrc_null };
        }
        inline const_iterator              find( uint16_t u_id ) const { return find_if( [ & ] ( const auto& it ) { return it.has_id() && it.id() == u_id; } ); }
        inline const_iterator              find( resource_id r_id ) const { return find( ( uint16_t ) r_id ); }
        inline const_iterator              find( std::wstring_view name ) const { return find_if( [ & ] ( const auto& it ) { return it.has_name() && it.name() == name; } ); }
        template<typename T> inline auto   find( T&& v ) { return acquire( ( ( const resource_directory_t* ) this )->find( std::forward<T>( v ) ) ); }
        template<typename T> inline auto   find_if( T&& fn ) { return acquire( ( ( const resource_directory_t* ) this )->find_if( std::forward<T>( fn ) ) ); }
        template<typename T> inline auto   operator[]( T&& v ) { return acquire( ( ( const resource_directory_t* ) this )->find( std::forward<T>( v ) ) ); }
        template<typename T> inline auto   operator[]( T&& v ) const { return find( std::forward<T>( v ) ); }
        inline iterator                    acquire( const const_iterator& i ) { return *( iterator* ) &i; }
    };

    template<bool x64> struct directory_type<directory_id::directory_entry_resource, x64, void> { using type = resource_directory_t; };
};
#pragma pack(pop)