include_guard()

# Change these defaults to point to your infrastructure if desired
set(CMKR_REPO "https://github.com/build-cpp/cmkr" CACHE STRING "cmkr git repository" FORCE)
set(CMKR_TAG "v0.2.44" CACHE STRING "cmkr git tag (this needs to be available forever)" FORCE)
set(CMKR_COMMIT_HASH "" CACHE STRING "cmkr git commit hash (optional)" FORCE)

# To bootstrap/generate a cmkr project: cmake -P cmkr.cmake
if(CMAKE_SCRIPT_MODE_FILE)
    set(CMAKE_BINARY_DIR "${CMAKE_BINARY_DIR}/build")
    set(CMAKE_CURRENT_BINARY_DIR "${CMAKE_BINARY_DIR}")
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}")
endif()

# Set these from the command line to customize for development/debugging purposes
set(CMKR_EXECUTABLE "" CACHE FILEPATH "cmkr executable")
set(CMKR_SKIP_GENERATION OFF CACHE BOOL "skip automatic cmkr generation")
set(CMKR_BUILD_TYPE "Debug" CACHE STRING "cmkr build configuration")
mark_as_advanced(CMKR_REPO CMKR_TAG CMKR_COMMIT_HASH CMKR_EXECUTABLE CMKR_SKIP_GENERATION CMKR_BUILD_TYPE)

# Disable cmkr if generation is disabled
if(DEFINED ENV{CI} OR CMKR_SKIP_GENERATION OR CMKR_BUILD_SKIP_GENERATION)
    message(STATUS "[cmkr] Skipping automatic cmkr generation")
    unset(CMKR_BUILD_SKIP_GENERATION CACHE)
    macro(cmkr)
    endmacro()
    return()
endif()

# Disable cmkr if no cmake.toml file is found
if(NOT CMAKE_SCRIPT_MODE_FILE AND NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake.toml")
    message(AUTHOR_WARNING "[cmkr] Not found: ${CMAKE_CURRENT_SOURCE_DIR}/cmake.toml")
    macro(cmkr)
    endmacro()
    return()
endif()

# Convert a Windows native path to CMake path
if(CMKR_EXECUTABLE MATCHES "\\\\")
    string(REPLACE "\\" "/" CMKR_EXECUTABLE_CMAKE "${CMKR_EXECUTABLE}")
    set(CMKR_EXECUTABLE "${CMKR_EXECUTABLE_CMAKE}" CACHE FILEPATH "" FORCE)
    unset(CMKR_EXECUTABLE_CMAKE)
endif()

# Helper macro to execute a process (COMMAND_ERROR_IS_FATAL ANY is 3.19 and higher)
function(cmkr_exec)
    execute_process(COMMAND ${ARGV} RESULT_VARIABLE CMKR_EXEC_RESULT)
    if(NOT CMKR_EXEC_RESULT EQUAL 0)
        message(FATAL_ERROR "cmkr_exec(${ARGV}) failed (exit code ${CMKR_EXEC_RESULT})")
    endif()
endfunction()

# Windows-specific hack (CMAKE_EXECUTABLE_PREFIX is not set at the moment)
if(WIN32)
    set(CMKR_EXECUTABLE_NAME "cmkr.exe")
else()
    set(CMKR_EXECUTABLE_NAME "cmkr")
endif()

# Use cached cmkr if found
if(DEFINED ENV{CMKR_CACHE})
    set(CMKR_DIRECTORY_PREFIX "$ENV{CMKR_CACHE}")
    string(REPLACE "\\" "/" CMKR_DIRECTORY_PREFIX "${CMKR_DIRECTORY_PREFIX}")
    if(NOT CMKR_DIRECTORY_PREFIX MATCHES "\\/$")
        set(CMKR_DIRECTORY_PREFIX "${CMKR_DIRECTORY_PREFIX}/")
    endif()
    # Build in release mode for the cache
    set(CMKR_BUILD_TYPE "Release")
else()
    set(CMKR_DIRECTORY_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/_cmkr_")
endif()
set(CMKR_DIRECTORY "${CMKR_DIRECTORY_PREFIX}${CMKR_TAG}")
set(CMKR_CACHED_EXECUTABLE "${CMKR_DIRECTORY}/bin/${CMKR_EXECUTABLE_NAME}")

# Helper function to check if a string starts with a prefix
# Cannot use MATCHES, see: https://github.com/build-cpp/cmkr/issues/61
function(cmkr_startswith str prefix result)
    string(LENGTH "${prefix}" prefix_length)
    string(LENGTH "${str}" str_length)
    if(prefix_length LESS_EQUAL str_length)
        string(SUBSTRING "${str}" 0 ${prefix_length} str_prefix)
        if(prefix STREQUAL str_prefix)
            set("${result}" ON PARENT_SCOPE)
            return()
        endif()
    endif()
    set("${result}" OFF PARENT_SCOPE)
endfunction()

# Handle upgrading logic
if(CMKR_EXECUTABLE AND NOT CMKR_CACHED_EXECUTABLE STREQUAL CMKR_EXECUTABLE)
    cmkr_startswith("${CMKR_EXECUTABLE}" "${CMAKE_CURRENT_BINARY_DIR}/_cmkr" CMKR_STARTSWITH_BUILD)
    cmkr_startswith("${CMKR_EXECUTABLE}" "${CMKR_DIRECTORY_PREFIX}" CMKR_STARTSWITH_CACHE)
    if(CMKR_STARTSWITH_BUILD)
        if(DEFINED ENV{CMKR_CACHE})
            message(AUTHOR_WARNING "[cmkr] Switching to cached cmkr: '${CMKR_CACHED_EXECUTABLE}'")
            if(EXISTS "${CMKR_CACHED_EXECUTABLE}")
                set(CMKR_EXECUTABLE "${CMKR_CACHED_EXECUTABLE}" CACHE FILEPATH "Full path to cmkr executable" FORCE)
            else()
                unset(CMKR_EXECUTABLE CACHE)
            endif()
        else()
            message(AUTHOR_WARNING "[cmkr] Upgrading '${CMKR_EXECUTABLE}' to '${CMKR_CACHED_EXECUTABLE}'")
            unset(CMKR_EXECUTABLE CACHE)
        endif()
    elseif(DEFINED ENV{CMKR_CACHE} AND CMKR_STARTSWITH_CACHE)
        message(AUTHOR_WARNING "[cmkr] Upgrading cached '${CMKR_EXECUTABLE}' to '${CMKR_CACHED_EXECUTABLE}'")
        unset(CMKR_EXECUTABLE CACHE)
    endif()
endif()

if(CMKR_EXECUTABLE AND EXISTS "${CMKR_EXECUTABLE}")
    message(VERBOSE "[cmkr] Found cmkr: '${CMKR_EXECUTABLE}'")
elseif(CMKR_EXECUTABLE AND NOT CMKR_EXECUTABLE STREQUAL CMKR_CACHED_EXECUTABLE)
    message(FATAL_ERROR "[cmkr] '${CMKR_EXECUTABLE}' not found")
elseif(NOT CMKR_EXECUTABLE AND EXISTS "${CMKR_CACHED_EXECUTABLE}")
    set(CMKR_EXECUTABLE "${CMKR_CACHED_EXECUTABLE}" CACHE FILEPATH "Full path to cmkr executable" FORCE)
    message(STATUS "[cmkr] Found cached cmkr: '${CMKR_EXECUTABLE}'")
else()
    set(CMKR_EXECUTABLE "${CMKR_CACHED_EXECUTABLE}" CACHE FILEPATH "Full path to cmkr executable" FORCE)
    message(VERBOSE "[cmkr] Bootstrapping '${CMKR_EXECUTABLE}'")

    message(STATUS "[cmkr] Fetching cmkr...")
    if(EXISTS "${CMKR_DIRECTORY}")
        cmkr_exec("${CMAKE_COMMAND}" -E rm -rf "${CMKR_DIRECTORY}")
    endif()
    find_package(Git QUIET REQUIRED)
    cmkr_exec("${GIT_EXECUTABLE}"
        clone
        --config advice.detachedHead=false
        --branch ${CMKR_TAG}
        --depth 1
        ${CMKR_REPO}
        "${CMKR_DIRECTORY}"
    )
    if(CMKR_COMMIT_HASH)
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" checkout -q "${CMKR_COMMIT_HASH}"
            RESULT_VARIABLE CMKR_EXEC_RESULT
            WORKING_DIRECTORY "${CMKR_DIRECTORY}"
        )
        if(NOT CMKR_EXEC_RESULT EQUAL 0)
            message(FATAL_ERROR "Tag '${CMKR_TAG}' hash is not '${CMKR_COMMIT_HASH}'")
        endif()
    endif()
    message(STATUS "[cmkr] Building cmkr (using system compiler)...")
    cmkr_exec("${CMAKE_COMMAND}"
        --no-warn-unused-cli
        "${CMKR_DIRECTORY}"
        "-B${CMKR_DIRECTORY}/build"
        "-DCMAKE_BUILD_TYPE=${CMKR_BUILD_TYPE}"
        "-DCMAKE_UNITY_BUILD=ON"
        "-DCMAKE_INSTALL_PREFIX=${CMKR_DIRECTORY}"
        "-DCMKR_GENERATE_DOCUMENTATION=OFF"
    )
    cmkr_exec("${CMAKE_COMMAND}"
        --build "${CMKR_DIRECTORY}/build"
        --config "${CMKR_BUILD_TYPE}"
        --parallel
    )
    cmkr_exec("${CMAKE_COMMAND}"
        --install "${CMKR_DIRECTORY}/build"
        --config "${CMKR_BUILD_TYPE}"
        --prefix "${CMKR_DIRECTORY}"
        --component cmkr
    )
    if(NOT EXISTS ${CMKR_EXECUTABLE})
        message(FATAL_ERROR "[cmkr] Failed to bootstrap '${CMKR_EXECUTABLE}'")
    endif()
    cmkr_exec("${CMKR_EXECUTABLE}" version)
    message(STATUS "[cmkr] Bootstrapped ${CMKR_EXECUTABLE}")
endif()
execute_process(COMMAND "${CMKR_EXECUTABLE}" version
    RESULT_VARIABLE CMKR_EXEC_RESULT
)
if(NOT CMKR_EXEC_RESULT EQUAL 0)
    message(FATAL_ERROR "[cmkr] Failed to get version, try clearing the cache and rebuilding")
endif()

# Use cmkr.cmake as a script
if(CMAKE_SCRIPT_MODE_FILE)
    if(NOT EXISTS "${CMAKE_SOURCE_DIR}/cmake.toml")
        execute_process(COMMAND "${CMKR_EXECUTABLE}" init
            RESULT_VARIABLE CMKR_EXEC_RESULT
        )
        if(NOT CMKR_EXEC_RESULT EQUAL 0)
            message(FATAL_ERROR "[cmkr] Failed to bootstrap cmkr project. Please report an issue: https://github.com/build-cpp/cmkr/issues/new")
        else()
            message(STATUS "[cmkr] Modify cmake.toml and then configure using: cmake -B build")
        endif()
    else()
        execute_process(COMMAND "${CMKR_EXECUTABLE}" gen
            RESULT_VARIABLE CMKR_EXEC_RESULT
        )
        if(NOT CMKR_EXEC_RESULT EQUAL 0)
            message(FATAL_ERROR "[cmkr] Failed to generate project.")
        else()
            message(STATUS "[cmkr] Configure using: cmake -B build")
        endif()
    endif()
endif()

# This is the macro that contains black magic
macro(cmkr)
    # When this macro is called from the generated file, fake some internal CMake variables
    get_source_file_property(CMKR_CURRENT_LIST_FILE "${CMAKE_CURRENT_LIST_FILE}" CMKR_CURRENT_LIST_FILE)
    if(CMKR_CURRENT_LIST_FILE)
        set(CMAKE_CURRENT_LIST_FILE "${CMKR_CURRENT_LIST_FILE}")
        get_filename_component(CMAKE_CURRENT_LIST_DIR "${CMAKE_CURRENT_LIST_FILE}" DIRECTORY)
    endif()

    # File-based include guard (include_guard is not documented to work)
    get_source_file_property(CMKR_INCLUDE_GUARD "${CMAKE_CURRENT_LIST_FILE}" CMKR_INCLUDE_GUARD)
    if(NOT CMKR_INCLUDE_GUARD)
        set_source_files_properties("${CMAKE_CURRENT_LIST_FILE}" PROPERTIES CMKR_INCLUDE_GUARD TRUE)

        file(SHA256 "${CMAKE_CURRENT_LIST_FILE}" CMKR_LIST_FILE_SHA256_PRE)

        # Generate CMakeLists.txt
        cmkr_exec("${CMKR_EXECUTABLE}" gen
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        )

        file(SHA256 "${CMAKE_CURRENT_LIST_FILE}" CMKR_LIST_FILE_SHA256_POST)

        # Delete the temporary file if it was left for some reason
        set(CMKR_TEMP_FILE "${CMAKE_CURRENT_SOURCE_DIR}/CMakerLists.txt")
        if(EXISTS "${CMKR_TEMP_FILE}")
            file(REMOVE "${CMKR_TEMP_FILE}")
        endif()

        if(NOT CMKR_LIST_FILE_SHA256_PRE STREQUAL CMKR_LIST_FILE_SHA256_POST)
            # Copy the now-generated CMakeLists.txt to CMakerLists.txt
            # This is done because you cannot include() a file you are currently in
            configure_file(CMakeLists.txt "${CMKR_TEMP_FILE}" COPYONLY)

            # Add the macro required for the hack at the start of the cmkr macro
            set_source_files_properties("${CMKR_TEMP_FILE}" PROPERTIES
                CMKR_CURRENT_LIST_FILE "${CMAKE_CURRENT_LIST_FILE}"
            )

            # 'Execute' the newly-generated CMakeLists.txt
            include("${CMKR_TEMP_FILE}")

            # Delete the generated file
            file(REMOVE "${CMKR_TEMP_FILE}")

            # Do not execute the rest of the original CMakeLists.txt
            return()
        endif()
        # Resume executing the unmodified CMakeLists.txt
    endif()
endmacro()
