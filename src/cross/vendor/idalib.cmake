set(IDASDK ${ida-sdk_SOURCE_DIR}/src)
if(NOT EXISTS ${IDASDK}/include/ida.hpp)
    message(FATAL_ERROR "Missing header file in IDA SDK: ${IDASDK}/include/ida.hpp")
endif()

set(IMPORTED_PROPERTY "IMPORTED_LOCATION")
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if(NOT CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
        message(FATAL_ERROR "Unsupported architecture ${CMAKE_SYSTEM_PROCESSOR} for platform ${CMAKE_SYSTEM_NAME}")
    endif()
    set(PLATFORM_DEFINE -D__LINUX__=1)
    set(IDA_KERNEL_LIB "${IDASDK}/lib/x64_linux_gcc_64/libida.so")
    set(IDA_LIB_LIB "${IDASDK}/lib/x64_linux_gcc_64/libidalib.so")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    if(NOT CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64")
        message(FATAL_ERROR "Unsupported architecture ${CMAKE_SYSTEM_PROCESSOR} for platform ${CMAKE_SYSTEM_NAME}")
    endif()
    set(PLATFORM_DEFINE -D__NT__=1)
    set(IDA_KERNEL_LIB "${IDASDK}/lib/x64_win_vc_64/ida.lib")
    set(IDA_LIB_LIB "${IDASDK}/lib/x64_win_vc_64/idalib.lib")
    set(IMPORTED_PROPERTY "IMPORTED_IMPLIB")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(PLATFORM_DEFINE -D__MAC__=1)
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
        set(IDA_KERNEL_LIB "${IDASDK}/lib/arm64_mac_clang_64/libida.dylib")
        set(IDA_LIB_LIB "${IDASDK}/lib/arm64_mac_clang_64/libidalib.dylib")
    elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
        set(IDA_KERNEL_LIB "${IDASDK}/lib/x64_mac_clang_64/libida.dylib")
        set(IDA_LIB_LIB "${IDASDK}/lib/x64_mac_clang_64/libidalib.dylib")
    else()
        message(FATAL_ERROR "Unsupported architecture ${CMAKE_SYSTEM_PROCESSOR} for platform ${CMAKE_SYSTEM_NAME}")
    endif()
else()
    message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME}")
endif()

# Parse symbols from a dylib
function(parse_dylib_symbols DYLIB_PATH OUTPUT_VAR)
    execute_process(
        COMMAND nm -gU ${DYLIB_PATH}
        OUTPUT_VARIABLE NM_OUTPUT
        COMMAND_ERROR_IS_FATAL ANY
    )

    set(SYMBOL_LIST "")

    # Split output into lines
    string(REPLACE "\n" ";" LINES "${NM_OUTPUT}")

    foreach(LINE ${LINES})
        # Match lines with format: "address T _symbolname"
        if(LINE MATCHES "^[0-9a-f]+ T _(.+)$")
            set(SYMBOL ${CMAKE_MATCH_1})
            list(APPEND SYMBOL_LIST ${SYMBOL})
        endif()
    endforeach()

    set(${OUTPUT_VAR} ${SYMBOL_LIST} PARENT_SCOPE)
endfunction()

# Parse both libraries
parse_dylib_symbols(${IDA_KERNEL_LIB} LIBIDALIB_SYMBOLS)
parse_dylib_symbols(${IDA_LIB_LIB} LIBIDA_SYMBOLS)

# Combine and remove duplicates
set(ALL_SYMBOLS ${LIBIDALIB_SYMBOLS} ${LIBIDA_SYMBOLS})
list(REMOVE_DUPLICATES ALL_SYMBOLS)
list(SORT ALL_SYMBOLS)
list(LENGTH ALL_SYMBOLS ALL_SYMBOLS_COUNT)

set(OUTPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/idasymbols.h")

# Generate the header file
file(WRITE ${OUTPUT_FILE} "#pragma once\n\n")
file(APPEND ${OUTPUT_FILE} "// Auto-generated from libidalib.dylib and libida.dylib\n")
file(APPEND ${OUTPUT_FILE} "// Total symbols: ${ALL_SYMBOLS_COUNT}\n\n")
file(APPEND ${OUTPUT_FILE} "#define IDA_SYMBOLS \\\n")

foreach(SYMBOL ${ALL_SYMBOLS})
    file(APPEND ${OUTPUT_FILE} "    X(${SYMBOL}) \\\n")
endforeach()

file(APPEND ${OUTPUT_FILE} "\n")

message(STATUS "Generated ${OUTPUT_FILE} with ${ALL_SYMBOLS_COUNT} symbols")

add_library(idaresolver SHARED
    idaresolver.cpp
)
target_include_directories(idaresolver PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

add_library(idasdk::headers INTERFACE IMPORTED)
target_include_directories(idasdk::headers INTERFACE ${IDASDK}/include)
target_compile_definitions(idasdk::headers INTERFACE __EA64__=1 ${PLATFORM_DEFINE})

add_library(idasdk::kernel SHARED IMPORTED)
set_target_properties(idasdk::kernel PROPERTIES
    ${IMPORTED_PROPERTY} ${IDA_KERNEL_LIB}
    INTERFACE_LINK_LIBRARIES idasdk::headers
)

add_library(idasdk::lib SHARED IMPORTED)
set_target_properties(idasdk::lib PROPERTIES
    ${IMPORTED_PROPERTY} ${IDA_LIB_LIB}
    INTERFACE_LINK_LIBRARIES idasdk::headers
    INTERFACE_LINK_LIBRARIES idasdk::kernel
)

target_link_libraries(idalib INTERFACE idaresolver idasdk::headers)
