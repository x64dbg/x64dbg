#ifndef _LZ4FILE_H
#define _LZ4FILE_H

typedef enum _LZ4_STATUS
{
    LZ4_SUCCESS,
    LZ4_FAILED_OPEN_INPUT,
    LZ4_FAILED_OPEN_OUTPUT,
    LZ4_NOT_ENOUGH_MEMORY,
    LZ4_INVALID_ARCHIVE,
    LZ4_CORRUPTED_ARCHIVE
} LZ4_STATUS;

#if defined (__cplusplus)
extern "C"
{
#endif

__declspec(dllimport) LZ4_STATUS LZ4_compress_file(const char* input_filename, const char* output_filename);
__declspec(dllimport) LZ4_STATUS LZ4_compress_fileW(const wchar_t* input_filename, const wchar_t* output_filename);
__declspec(dllimport) LZ4_STATUS LZ4_decompress_file(const char* input_filename, const char* output_filename);
__declspec(dllimport) LZ4_STATUS LZ4_decompress_fileW(const wchar_t* input_filename, const wchar_t* output_filename);

#if defined (__cplusplus)
}
#endif

#endif //_LZ4FILE_H