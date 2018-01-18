/*
Copyright (c) 2014. The YARA Authors. All Rights Reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software without
specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef YR_MODULES_H
#define YR_MODULES_H

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "utils.h"
#include "limits.h"
#include "error.h"
#include "exec.h"
#include "types.h"
#include "object.h"
#include "libyara.h"

// Concatenation that macro-expands its arguments.

#define YR_CONCAT(arg1, arg2) _YR_CONCAT(arg1, arg2) // expands the arguments.
#define _YR_CONCAT(arg1, arg2) arg1 ## arg2  // do the actual concatenation.


#define module_declarations YR_CONCAT(MODULE_NAME, __declarations)
#define module_load YR_CONCAT(MODULE_NAME, __load)
#define module_unload YR_CONCAT(MODULE_NAME, __unload)
#define module_initialize YR_CONCAT(MODULE_NAME, __initialize)
#define module_finalize YR_CONCAT(MODULE_NAME, __finalize)

#define begin_declarations \
    int module_declarations(YR_OBJECT* module) { \
      YR_OBJECT* stack[64]; \
      int stack_top = 0; \
      stack[stack_top] = module;


#define end_declarations \
    return ERROR_SUCCESS; }


#define begin_struct(name) { \
    YR_OBJECT* structure; \
    FAIL_ON_ERROR(yr_object_create( \
        OBJECT_TYPE_STRUCTURE, \
        name, \
        stack[stack_top], \
        &structure)); \
    assertf( \
        stack_top < sizeof(stack)/sizeof(stack[0]) - 1, \
        "too many nested structures"); \
    stack[++stack_top] = structure; \
  }


#define begin_struct_array(name) { \
    YR_OBJECT* structure; \
    YR_OBJECT* array; \
    FAIL_ON_ERROR(yr_object_create( \
        OBJECT_TYPE_ARRAY, \
        name, \
        stack[stack_top], \
        &array)); \
    FAIL_ON_ERROR(yr_object_create( \
        OBJECT_TYPE_STRUCTURE, \
        name, \
        array, \
        &structure)); \
    assertf( \
        stack_top < sizeof(stack)/sizeof(stack[0]) - 1, \
        "too many nested structures"); \
    stack[++stack_top] = structure; \
  }


#define begin_struct_dictionary(name) { \
    YR_OBJECT* structure; \
    YR_OBJECT* array; \
    FAIL_ON_ERROR(yr_object_create( \
        OBJECT_TYPE_DICTIONARY, \
        name, \
        stack[stack_top], \
        &array)); \
    FAIL_ON_ERROR(yr_object_create( \
        OBJECT_TYPE_STRUCTURE, \
        name, \
        array, \
        &structure)); \
    assertf( \
        stack_top < sizeof(stack)/sizeof(stack[0]) - 1, \
        "too many nested structures"); \
    stack[++stack_top] = structure; \
  }


#define end_struct(name) { \
    assert(stack[stack_top]->type == OBJECT_TYPE_STRUCTURE); \
    assertf( \
        strcmp(stack[stack_top]->identifier, name) == 0, \
        "unbalanced begin_struct/end_struct"); \
    stack_top--; \
  }


#define end_struct_array(name) end_struct(name)


#define end_struct_dictionary(name) end_struct(name)


#define declare_integer(name) { \
    FAIL_ON_ERROR(yr_object_create( \
        OBJECT_TYPE_INTEGER, \
        name, \
        stack[stack_top], \
        NULL)); \
  }


#define declare_integer_array(name) { \
    YR_OBJECT* array; \
    FAIL_ON_ERROR(yr_object_create( \
        OBJECT_TYPE_ARRAY, \
        name, \
        stack[stack_top], \
        &array)); \
    FAIL_ON_ERROR(yr_object_create( \
        OBJECT_TYPE_INTEGER, \
        name, \
        array, \
        NULL)); \
  }


#define declare_integer_dictionary(name) { \
    YR_OBJECT* dict; \
    FAIL_ON_ERROR(yr_object_create( \
        OBJECT_TYPE_DICTIONARY, \
        name, \
        stack[stack_top], \
        &dict)); \
    FAIL_ON_ERROR(yr_object_create( \
        OBJECT_TYPE_INTEGER, \
        name, \
        dict, \
        NULL)); \
  }


#define declare_float(name) { \
    FAIL_ON_ERROR(yr_object_create( \
        OBJECT_TYPE_FLOAT, \
        name, \
        stack[stack_top], \
        NULL)); \
  }


#define declare_float_array(name) { \
    YR_OBJECT* array; \
    FAIL_ON_ERROR(yr_object_create( \
        OBJECT_TYPE_ARRAY, \
        name, \
        stack[stack_top], \
        &array)); \
    FAIL_ON_ERROR(yr_object_create( \
        OBJECT_TYPE_FLOAT, \
        name, \
        array, \
        NULL)); \
  }


#define declare_float_dictionary(name) { \
    YR_OBJECT* dict; \
    FAIL_ON_ERROR(yr_object_create( \
        OBJECT_TYPE_DICTIONARY, \
        name, \
        stack[stack_top], \
        &dict)); \
    FAIL_ON_ERROR(yr_object_create( \
        OBJECT_TYPE_FLOAT, \
        name, \
        dict, \
        NULL)); \
  }


#define declare_string(name) { \
    FAIL_ON_ERROR(yr_object_create( \
        OBJECT_TYPE_STRING, \
        name, \
        stack[stack_top], \
        NULL)); \
  }


#define declare_string_array(name) { \
    YR_OBJECT* array; \
    FAIL_ON_ERROR(yr_object_create( \
        OBJECT_TYPE_ARRAY, \
        name, \
        stack[stack_top], \
        &array)); \
    FAIL_ON_ERROR(yr_object_create( \
        OBJECT_TYPE_STRING, \
        name, \
        array, \
        NULL)); \
  }


#define declare_string_dictionary(name) { \
    YR_OBJECT* dict; \
    FAIL_ON_ERROR(yr_object_create( \
        OBJECT_TYPE_DICTIONARY, \
        name, \
        stack[stack_top], \
        &dict)); \
    FAIL_ON_ERROR(yr_object_create( \
        OBJECT_TYPE_STRING, \
        name, \
        dict, \
        NULL)); \
  }


#define declare_function(name, args_fmt, ret_fmt, func) { \
    YR_OBJECT* function; \
    FAIL_ON_ERROR(yr_object_function_create( \
        name, \
        args_fmt, \
        ret_fmt, \
        func, \
        stack[stack_top], \
        &function)); \
    }


#define define_function(func) \
    int func ( \
        YR_VALUE* __args, \
        YR_SCAN_CONTEXT* __context, \
        YR_OBJECT_FUNCTION* __function_obj)


#define sized_string_argument(n) \
    (__args[n-1].ss)

#define string_argument(n) \
    (sized_string_argument(n)->c_string)

#define integer_argument(n) \
    (__args[n-1].i)

#define float_argument(n) \
    (__args[n-1].d)

#define regexp_argument(n) \
    ((RE*)(__args[n-1].re))


#define module()        yr_object_get_root((YR_OBJECT*) __function_obj)
#define parent()        (__function_obj->parent)
#define scan_context()  (__context)


#define foreach_memory_block(iterator, block) \
  for (block = iterator->first(iterator); \
       block != NULL; \
       block = iterator->next(iterator)) \


#define first_memory_block(context) \
      (context)->iterator->first((context)->iterator)


#define is_undefined(object, ...) \
    yr_object_has_undefined_value(object, __VA_ARGS__)


#define get_object(object, ...) \
    yr_object_lookup(object, 0, __VA_ARGS__)


#define get_integer(object, ...) \
    yr_object_get_integer(object, __VA_ARGS__)


#define get_float(object, ...) \
    yr_object_get_float(object, __VA_ARGS__)


#define get_string(object, ...) \
    yr_object_get_string(object, __VA_ARGS__)


#define set_integer(value, object, ...) \
    yr_object_set_integer(value, object, __VA_ARGS__)


#define set_float(value, object, ...) \
    yr_object_set_float(value, object, __VA_ARGS__)


#define set_sized_string(value, len, object, ...) \
    yr_object_set_string(value, len, object, __VA_ARGS__)


#define set_string(value, object, ...) \
    set_sized_string(value, strlen(value), object, __VA_ARGS__)


#define return_integer(integer) { \
      assertf( \
          __function_obj->return_obj->type == OBJECT_TYPE_INTEGER, \
          "return type differs from function declaration"); \
      yr_object_set_integer( \
          (integer), \
          __function_obj->return_obj, \
          NULL); \
      return ERROR_SUCCESS; \
    }


#define return_float(double_) { \
      double d = (double) (double_); \
      assertf( \
          __function_obj->return_obj->type == OBJECT_TYPE_FLOAT, \
          "return type differs from function declaration"); \
      yr_object_set_float( \
          (d != (double) UNDEFINED) ? d : NAN, \
          __function_obj->return_obj, \
          NULL); \
      return ERROR_SUCCESS; \
    }


#define return_string(string) { \
      char* s = (char*) (string); \
      assertf( \
          __function_obj->return_obj->type == OBJECT_TYPE_STRING, \
          "return type differs from function declaration"); \
      yr_object_set_string( \
          (s != (char*) UNDEFINED) ? s : NULL, \
          (s != (char*) UNDEFINED) ? strlen(s) : 0, \
          __function_obj->return_obj, \
          NULL); \
      return ERROR_SUCCESS; \
    }


struct _YR_MODULE;


typedef int (*YR_EXT_INITIALIZE_FUNC)(
    struct _YR_MODULE* module);


typedef int (*YR_EXT_FINALIZE_FUNC)(
    struct _YR_MODULE* module);


typedef int (*YR_EXT_DECLARATIONS_FUNC)(
    YR_OBJECT* module_object);


typedef int (*YR_EXT_LOAD_FUNC)(
    YR_SCAN_CONTEXT* context,
    YR_OBJECT* module_object,
    void* module_data,
    size_t module_data_size);


typedef int (*YR_EXT_UNLOAD_FUNC)(
    YR_OBJECT* module_object);


typedef struct _YR_MODULE
{
    char* name;

    YR_EXT_DECLARATIONS_FUNC declarations;
    YR_EXT_LOAD_FUNC load;
    YR_EXT_UNLOAD_FUNC unload;
    YR_EXT_INITIALIZE_FUNC initialize;
    YR_EXT_FINALIZE_FUNC finalize;

} YR_MODULE;


typedef struct _YR_MODULE_IMPORT
{
    const char* module_name;
    void* module_data;
    size_t module_data_size;

} YR_MODULE_IMPORT;


int yr_modules_initialize(void);


int yr_modules_finalize(void);


int yr_modules_do_declarations(
    const char* module_name,
    YR_OBJECT* main_structure);


int yr_modules_load(
    const char* module_name,
    YR_SCAN_CONTEXT* context);


int yr_modules_unload_all(
    YR_SCAN_CONTEXT* context);

#endif
