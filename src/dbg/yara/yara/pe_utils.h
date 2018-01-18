#ifndef YR_PE_UTILS_H
#define YR_PE_UTILS_H

#include <yara/pe.h>

#define MAX_PE_SECTIONS              96


#define IS_64BITS_PE(pe) \
    (yr_le16toh(pe->header64->OptionalHeader.Magic) == IMAGE_NT_OPTIONAL_HDR64_MAGIC)


#define OptionalHeader(pe,field)                \
  (IS_64BITS_PE(pe) ?                           \
   pe->header64->OptionalHeader.field :         \
   pe->header->OptionalHeader.field)


//
// Imports are stored in a linked list. Each node (IMPORTED_DLL) contains the
// name of the DLL and a pointer to another linked list of
// IMPORT_EXPORT_FUNCTION structures containing the details of imported
// functions.
//

typedef struct _IMPORTED_DLL
{
    char* name;

    struct _IMPORT_EXPORT_FUNCTION* functions;
    struct _IMPORTED_DLL* next;

} IMPORTED_DLL, *PIMPORTED_DLL;


//
// This is used to track imported and exported functions. The "has_ordinal"
// field is only used in the case of imports as those are optional. Every export
// has an ordinal so we don't need the field there, but in the interest of
// keeping duplicate code to a minimum we use this function for both imports and
// exports.
//

typedef struct _IMPORT_EXPORT_FUNCTION
{
    char* name;
    uint8_t has_ordinal;
    uint16_t ordinal;

    struct _IMPORT_EXPORT_FUNCTION* next;

} IMPORT_EXPORT_FUNCTION, *PIMPORT_EXPORT_FUNCTION;


typedef struct _PE
{
    uint8_t* data;
    size_t data_size;

    union
    {
        PIMAGE_NT_HEADERS32 header;
        PIMAGE_NT_HEADERS64 header64;
    };

    YR_OBJECT* object;
    IMPORTED_DLL* imported_dlls;
    IMPORT_EXPORT_FUNCTION* exported_functions;

    uint32_t resources;

} PE;


#define fits_in_pe(pe, pointer, size) \
    ((size_t) size <= pe->data_size && \
     (uint8_t*) (pointer) >= pe->data && \
     (uint8_t*) (pointer) <= pe->data + pe->data_size - size)

#define struct_fits_in_pe(pe, pointer, struct_type) \
    fits_in_pe(pe, pointer, sizeof(struct_type))


PIMAGE_NT_HEADERS32 pe_get_header(
    uint8_t* data,
    size_t data_size);


PIMAGE_DATA_DIRECTORY pe_get_directory_entry(
    PE* pe,
    int entry);


PIMAGE_DATA_DIRECTORY pe_get_directory_entry(
    PE* pe,
    int entry);


int64_t pe_rva_to_offset(
    PE* pe,
    uint64_t rva);


char* ord_lookup(
    char* dll,
    uint16_t ord);


#if HAVE_LIBCRYPTO
#include <openssl/asn1.h>
time_t ASN1_get_time_t(ASN1_TIME* time);
#endif

#endif
