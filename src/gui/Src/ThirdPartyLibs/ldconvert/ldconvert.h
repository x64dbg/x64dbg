#pragma once

//Converts a long double to string.
//This function uses sprintf with the highest supported precision.
//pld: Pointer to an 80-bit (10 byte) long double.
//str: Buffer with at least 32 bytes of space.
extern "C" __declspec(dllimport) void ld2str(const void* pld, char* str);

//Converts a string to a long double.
//This function uses http://en.cppreference.com/w/c/string/byte/strtof
//str: The string to convert.
//pld: Pointer to an 80-bit (10 byte) long double.
extern "C" __declspec(dllimport) bool str2ld(const char* str, void* pld);

//Converts a long double to a double.
//pld: Pointer to an 80-bit (10 byte) long double.
//pd: Pointer to a 64-bit (8 byte) double.
extern "C" __declspec(dllexport) void ld2d(const void* pld, void* pd);

//Converts a double to a long double.
//pd: Pointer to a 64-bit (8 byte) double.
//pld: Pointer to an 80-bit (10 byte) long double.
extern "C" __declspec(dllexport) void d2ld(const void* pd, void* pld);