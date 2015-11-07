/*
Copyright (c) 2014. The YARA Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/


#ifndef YR_RULES_H
#define YR_RULES_H

#include "types.h"
#include "utils.h"


#define CALLBACK_MSG_RULE_MATCHING              1
#define CALLBACK_MSG_RULE_NOT_MATCHING          2
#define CALLBACK_MSG_SCAN_FINISHED              3
#define CALLBACK_MSG_IMPORT_MODULE              4

#define CALLBACK_CONTINUE   0
#define CALLBACK_ABORT      1
#define CALLBACK_ERROR      2


#define yr_rule_tags_foreach(rule, tag_name) \
    for (tag_name = rule->tags; \
         tag_name != NULL && *tag_name != '\0'; \
         tag_name += strlen(tag_name) + 1)


#define yr_rule_metas_foreach(rule, meta) \
    for (meta = rule->metas; !META_IS_NULL(meta); meta++)


#define yr_rule_strings_foreach(rule, string) \
    for (string = rule->strings; !STRING_IS_NULL(string); string++)


#define yr_string_matches_foreach(string, match) \
    for (match = STRING_MATCHES(string).head; match != NULL; match = match->next)


#define yr_rules_foreach(rules, rule) \
    for (rule = rules->rules_list_head; !RULE_IS_NULL(rule); rule++)



YR_API int yr_rules_scan_mem(
    YR_RULES* rules,
    uint8_t* buffer,
    size_t buffer_size,
    int flags,
    YR_CALLBACK_FUNC callback,
    void* user_data,
    int timeout);


YR_API int yr_rules_scan_file(
    YR_RULES* rules,
    const char* filename,
    int flags,
    YR_CALLBACK_FUNC callback,
    void* user_data,
    int timeout);


YR_API int yr_rules_scan_proc(
    YR_RULES* rules,
    int pid,
    int flags,
    YR_CALLBACK_FUNC callback,
    void* user_data,
    int timeout);


YR_API int yr_rules_save(
    YR_RULES* rules,
    const char* filename);


YR_API int yr_rules_load(
    const char* filename,
    YR_RULES** rules);


YR_API int yr_rules_destroy(
    YR_RULES* rules);


YR_API int yr_rules_define_integer_variable(
    YR_RULES* rules,
    const char* identifier,
    int64_t value);


YR_API int yr_rules_define_boolean_variable(
    YR_RULES* rules,
    const char* identifier,
    int value);


YR_API int yr_rules_define_float_variable(
    YR_RULES* rules,
    const char* identifier,
    double value);


YR_API int yr_rules_define_string_variable(
    YR_RULES* rules,
    const char* identifier,
    const char* value);


YR_API void yr_rules_print_profiling_info(
    YR_RULES* rules);

#endif
