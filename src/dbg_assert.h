/**
 * Code originally from:
 *
 * (c) 2015 Alexandro Sanchez Bach. All rights reserved.
 * https://github.com/AlexAltea/nucleus/blob/9103d7eb52cc861cb6e317f0007246bd03d663a2/nucleus/assert.h
 * https://github.com/AlexAltea/nucleus/blob/9103d7eb52cc861cb6e317f0007246bd03d663a2/LICENSE
 * Released under GPL v2 license. Read LICENSE for more details.
 *
 */

#pragma once

#include <cassert>

// Count arguments
#define __DBG_ARGUMENT_EXPAND(x) x
#define __DBG_ARGUMENT_COUNT(...) \
    __DBG_ARGUMENT_EXPAND(__DBG_ARGUMENT_EXTRACT(__VA_ARGS__, 8, 7, 6, 5, 4, 3, 2, 1, 0))
#define __DBG_ARGUMENT_EXTRACT(a1, a2, a3, a4, a5, a6, a7, a8, N, ...) N

// Dispatching macros
#define __DBG_MACRO_DISPATCH(function, ...) \
    __DBG_MACRO_SELECT(function, __DBG_ARGUMENT_COUNT(__VA_ARGS__))
#define __DBG_MACRO_SELECT(function, argc) \
    __DBG_MACRO_CONCAT(function, argc)
#define __DBG_MACRO_CONCAT(a, b) a##b

// Trigger an exception if the debugger is not currently active
#define ASSERT_DEBUGGING(message) ASSERT_TRUE(IsFileBeingDebugged(), message)

// Trigger an exception if expression is false
#define ASSERT_TRUE(...) \
    __DBG_MACRO_DISPATCH(ASSERT_TRUE, __VA_ARGS__)(__VA_ARGS__)
#define ASSERT_TRUE1(expr) assert(expr)
#define ASSERT_TRUE2(expr, message) assert((expr) && (message))

// Trigger an exception if expression is true
#define ASSERT_FALSE(...) \
    __DBG_MACRO_DISPATCH(ASSERT_FALSE, __VA_ARGS__)(__VA_ARGS__)
#define ASSERT_FALSE1(expr) assert(!(expr))
#define ASSERT_FALSE2(expr, message) assert(!(expr) && (message))

// Trigger an exception if expression is zero
#define ASSERT_ZERO(...) \
    __DBG_MACRO_DISPATCH(ASSERT_ZERO, __VA_ARGS__)(__VA_ARGS__)
#define ASSERT_ZERO1(expr) assert(expr)
#define ASSERT_ZERO2(expr, message) assert(((expr) == 0) && (message))

// Trigger an exception if expression is non-zero
#define ASSERT_NONZERO(...) \
    __DBG_MACRO_DISPATCH(ASSERT_TRUE, __VA_ARGS__)(__VA_ARGS__)
#define ASSERT_NONZERO1(expr) ASSERT_NONZERO(expr)
#define ASSERT_NONZERO2(expr, message) assert(((expr) != 0) && (message))

// Trigger an exception if expression is a nullptr
#define ASSERT_NULL(...) \
    __DBG_MACRO_DISPATCH(ASSERT_NULL, __VA_ARGS__)(__VA_ARGS__)
#define ASSERT_NULL1(expr) assert(expr)
#define ASSERT_NULL2(expr, message) assert(((expr) == nullptr) && (message))

// Trigger an exception if expression is not a nullptr
#define ASSERT_NONNULL(...) \
    __DBG_MACRO_DISPATCH(ASSERT_NONNULL, __VA_ARGS__)(__VA_ARGS__)
#define ASSERT_NONNULL1(expr) assert(expr)
#define ASSERT_NONNULL2(expr, message) assert(((expr) != nullptr) && (message))

// Trigger an exception
#define ASSERT_ALWAYS(...) \
    __DBG_MACRO_DISPATCH(ASSERT_ALWAYS, __VA_ARGS__)(__VA_ARGS__)
#define ASSERT_ALWAYS0(...) assert(0)
#define ASSERT_ALWAYS1(message) assert(0 && (message))