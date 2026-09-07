cmake_minimum_required(VERSION 3.15)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(X64DBG_TEST_ARCH x64)
    set(X64DBG_TEST_PLUGIN_SUFFIX .dp64)
else()
    set(X64DBG_TEST_ARCH x32)
    set(X64DBG_TEST_PLUGIN_SUFFIX .dp32)
endif()

set(X64DBG_TEST_OUTPUT_DIR "${PROJECT_SOURCE_DIR}/bin/${X64DBG_TEST_ARCH}/tests")

add_custom_target(x64dbg_tests ALL)

function(x64dbg_set_output_dir target dir)
    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${dir}"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${dir}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${dir}"
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${dir}"
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${dir}"
        LIBRARY_OUTPUT_DIRECTORY "${dir}"
        LIBRARY_OUTPUT_DIRECTORY_DEBUG "${dir}"
        LIBRARY_OUTPUT_DIRECTORY_RELEASE "${dir}"
        LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${dir}"
        LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL "${dir}"
        ARCHIVE_OUTPUT_DIRECTORY "${dir}"
        ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${dir}"
        ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${dir}"
        ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO "${dir}"
        ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL "${dir}"
    )
endfunction()

function(x64dbg_apply_test_defaults target)
    target_compile_definitions(${target} PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
    target_compile_features(${target} PRIVATE cxx_std_17)
endfunction()

function(x64dbg_add_test)
    set(options)
    set(oneValueArgs NAME SOURCE_DIR TARGET_SOURCE PLUGIN_SOURCE)
    set(multiValueArgs
        TARGET_COMPILE_DEFINITIONS
        TARGET_COMPILE_OPTIONS
        TARGET_LINK_LIBRARIES
        TARGET_LINK_OPTIONS
        PLUGIN_COMPILE_DEFINITIONS
        PLUGIN_COMPILE_OPTIONS
        PLUGIN_LINK_LIBRARIES
        PLUGIN_LINK_OPTIONS
    )
    cmake_parse_arguments(TEST "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT TEST_NAME)
        message(FATAL_ERROR "x64dbg_add_test requires NAME")
    endif()

    if(TEST_SOURCE_DIR)
        set(test_source_dir "${TEST_SOURCE_DIR}")
    else()
        set(test_source_dir "${CMAKE_CURRENT_LIST_DIR}/${TEST_NAME}")
    endif()
    if(NOT IS_DIRECTORY "${test_source_dir}")
        message(FATAL_ERROR "x64dbg_add_test(${TEST_NAME}) source dir does not exist: ${test_source_dir}")
    endif()

    if(NOT TEST_TARGET_SOURCE)
        set(TEST_TARGET_SOURCE target.cpp)
    endif()

    set(test_target_source_path "${test_source_dir}/${TEST_TARGET_SOURCE}")
    if(NOT EXISTS "${test_target_source_path}")
        message(FATAL_ERROR "x64dbg_add_test(${TEST_NAME}) missing target source: ${test_target_source_path}")
    endif()

    if(NOT TEST_PLUGIN_SOURCE AND EXISTS "${test_source_dir}/plugin.cpp")
        set(TEST_PLUGIN_SOURCE plugin.cpp)
    endif()

    set(test_runtime_dir "${X64DBG_TEST_OUTPUT_DIR}/${TEST_NAME}")
    set(test_target "test_${TEST_NAME}_target")

    add_executable(${test_target} "${test_target_source_path}")
    x64dbg_apply_test_defaults(${test_target})
    if(TEST_TARGET_COMPILE_DEFINITIONS)
        target_compile_definitions(${test_target} PRIVATE ${TEST_TARGET_COMPILE_DEFINITIONS})
    endif()
    if(TEST_TARGET_COMPILE_OPTIONS)
        target_compile_options(${test_target} PRIVATE ${TEST_TARGET_COMPILE_OPTIONS})
    endif()
    if(TEST_TARGET_LINK_LIBRARIES)
        target_link_libraries(${test_target} PRIVATE ${TEST_TARGET_LINK_LIBRARIES})
    endif()
    if(TEST_TARGET_LINK_OPTIONS)
        target_link_options(${test_target} PRIVATE ${TEST_TARGET_LINK_OPTIONS})
    endif()
    set_target_properties(${test_target} PROPERTIES OUTPUT_NAME "${TEST_NAME}")
    x64dbg_set_output_dir(${test_target} "${X64DBG_TEST_OUTPUT_DIR}")

    set(test_dependencies ${test_target})
    set(test_copy_target ${test_target})

    if(TEST_PLUGIN_SOURCE)
        set(test_plugin_source_path "${test_source_dir}/${TEST_PLUGIN_SOURCE}")
        if(NOT EXISTS "${test_plugin_source_path}")
            message(FATAL_ERROR "x64dbg_add_test(${TEST_NAME}) missing plugin source: ${test_plugin_source_path}")
        endif()

        set(test_plugin "test_${TEST_NAME}_plugin")
        add_library(${test_plugin} SHARED "${test_plugin_source_path}")
        target_include_directories(${test_plugin} PRIVATE
            "${PROJECT_SOURCE_DIR}/src/dbg"
            "${PROJECT_SOURCE_DIR}/src/bridge"
        )
        target_link_libraries(${test_plugin} PRIVATE
            bridge
            dbg
            ${TEST_PLUGIN_LINK_LIBRARIES}
        )
        x64dbg_apply_test_defaults(${test_plugin})
        target_compile_definitions(${test_plugin} PRIVATE
            X64DBG_TEST_NAME="${TEST_NAME}"
            ${TEST_PLUGIN_COMPILE_DEFINITIONS}
        )
        if(TEST_PLUGIN_COMPILE_OPTIONS)
            target_compile_options(${test_plugin} PRIVATE ${TEST_PLUGIN_COMPILE_OPTIONS})
        endif()
        if(TEST_PLUGIN_LINK_OPTIONS)
            target_link_options(${test_plugin} PRIVATE ${TEST_PLUGIN_LINK_OPTIONS})
        endif()
        set_target_properties(${test_plugin} PROPERTIES
            PREFIX ""
            SUFFIX "${X64DBG_TEST_PLUGIN_SUFFIX}"
            OUTPUT_NAME "${TEST_NAME}"
        )
        x64dbg_set_output_dir(${test_plugin} "${test_runtime_dir}")

        list(APPEND test_dependencies ${test_plugin})
        set(test_copy_target ${test_plugin})
    endif()

    file(GLOB test_scripts CONFIGURE_DEPENDS "${test_source_dir}/test*.txt")
    if(test_scripts)
        set(test_copy_commands
            COMMAND ${CMAKE_COMMAND} -E make_directory "${test_runtime_dir}"
        )
        foreach(test_script ${test_scripts})
            get_filename_component(test_script_name "${test_script}" NAME)
            list(APPEND test_copy_commands
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${test_script}"
                    "${test_runtime_dir}/${test_script_name}"
            )
        endforeach()

        add_custom_command(TARGET ${test_copy_target} POST_BUILD
            ${test_copy_commands}
        )
    endif()

    add_dependencies(x64dbg_tests ${test_dependencies})
endfunction()

function(x64dbg_add_membp_test)
    set(membp_driver_source_dir "${CMAKE_CURRENT_BINARY_DIR}/membp-driver")
    set(membp_driver_binary_dir "${membp_driver_source_dir}/build")
    file(MAKE_DIRECTORY "${membp_driver_source_dir}")
    file(WRITE "${membp_driver_source_dir}/CMakeLists.txt" "string(REPLACE \"/RTC1\" \"\" CMAKE_CXX_FLAGS_DEBUG \"\${CMAKE_CXX_FLAGS_DEBUG}\")\nset(MEMBP_TARGET_COMPILE_OPTIONS)\nif(MSVC)\n    list(APPEND MEMBP_TARGET_COMPILE_OPTIONS /GS- /GR- /Zl)\nelseif(CMAKE_CXX_COMPILER_ID MATCHES \"Clang|GNU\")\n    list(APPEND MEMBP_TARGET_COMPILE_OPTIONS -fno-stack-protector)\nendif()\n\nx64dbg_add_test(\n    NAME membp\n    SOURCE_DIR \"${CMAKE_CURRENT_LIST_DIR}/membp\"\n    TARGET_COMPILE_OPTIONS \${MEMBP_TARGET_COMPILE_OPTIONS}\n    TARGET_LINK_LIBRARIES kernel32\n    TARGET_LINK_OPTIONS\n        \"LINKER:/entry:start\"\n        \"LINKER:/incremental:no\"\n        \"LINKER:/nodefaultlib\"\n        \"LINKER:/merge:.mbd2=.mbc\"\n        \"LINKER:/merge:.mbd3=.mbc\"\n        \"LINKER:SHELL:/section:.mbc,ERW\"\n)\n")
    add_subdirectory("${membp_driver_source_dir}" "${membp_driver_binary_dir}")
endfunction()

set(X64DBG_STANDARD_TESTS
    commandparser
    issue3803
    issue3808
    issue3865
    cmdline_attach
    cmdline_init
    cmdline_setcommandline
    guard_page
    script_run_exit
    scriptcmd_call
    scriptcmd_call_threads
    swbp_stale
    trace_cmd_condition
    database_callbacks
)

foreach(test_name IN LISTS X64DBG_STANDARD_TESTS)
    x64dbg_add_test(NAME "${test_name}")
endforeach()

x64dbg_add_test(
    NAME attach_pause
    TARGET_LINK_LIBRARIES user32
)

x64dbg_add_membp_test()

# random_dll intentionally remains excluded while its scaffold stays disabled.
