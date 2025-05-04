#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    LogLevelDebug = 0,
    LogLevelInfo = 1,
    LogLevelWarning = 2,
    LogLevelError = 3,
} LogLevel;

typedef struct
{
    const char* file;
    uint32_t line;
    uint32_t column;
    size_t length;
} SourceLocation;

typedef struct
{
    SourceLocation location;
    const char* message;
    const char* description;
    const char* pretty;
} CompileError;

typedef struct
{
    SourceLocation location;
    const char* pretty;
} EvalError;

typedef void* TreeNode;

typedef struct
{
    const char* type_name;
    const char* variable_name;
    uint64_t offset;
    uint64_t size;
    uint32_t line;
    uint32_t color;
    const char* value;
    const char* comment;
    const char* reason;
    bool big_endian;
} VisitInfo;

typedef struct
{
    void* userdata;
    TreeNode root;
    const char* source;
    const char* filename;
    uint64_t base;
    uint64_t size;
    const char** defines_data;
    size_t defines_count;
    const char** includes_data;
    size_t includes_count;
    bool allow_dangerous_functions;

    void (*log_handler)(void* userdata, LogLevel level, const char* message);

    void (*compile_error)(void* userdata, const CompileError* error);

    void (*eval_error)(void* userdata, const EvalError* error);

    bool (*data_source)(void* userdata, uint64_t address, void* buffer, size_t size);

    TreeNode(*visit)(void* userdata, TreeNode parent, const VisitInfo* info);
} PatternRunArgs;

typedef enum
{
    PatternSuccess,
    PatternCompileError,
    PatternEvalError,
} PatternStatus;

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

PatternStatus PatternRun(const PatternRunArgs* args);

#ifdef __cplusplus
}
#endif // __cplusplus
