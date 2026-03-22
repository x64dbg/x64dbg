set(SCRIPTCMD_CALL_RUNTIME_DIR "${X64DBG_TEST_OUTPUT_DIR}/scriptcmd_call")
file(GLOB SCRIPTCMD_CALL_TEST_SCRIPTS CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/test*.txt")

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
    )
endfunction()

add_executable(test_scriptcmd_call_target target.cpp)
target_compile_definitions(test_scriptcmd_call_target PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_features(test_scriptcmd_call_target PRIVATE cxx_std_17)
set_target_properties(test_scriptcmd_call_target PROPERTIES OUTPUT_NAME scriptcmd_call)
x64dbg_set_output_dir(test_scriptcmd_call_target "${X64DBG_TEST_OUTPUT_DIR}")

add_library(test_scriptcmd_call_plugin SHARED plugin.cpp)
target_include_directories(test_scriptcmd_call_plugin PRIVATE
    "${PROJECT_SOURCE_DIR}/src/dbg"
    "${PROJECT_SOURCE_DIR}/src/bridge"
)
target_link_libraries(test_scriptcmd_call_plugin PRIVATE
    bridge
    dbg
)
target_compile_definitions(test_scriptcmd_call_plugin PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_features(test_scriptcmd_call_plugin PRIVATE cxx_std_17)
set_target_properties(test_scriptcmd_call_plugin PROPERTIES
    PREFIX ""
    SUFFIX "${X64DBG_TEST_PLUGIN_SUFFIX}"
    OUTPUT_NAME "ScriptCmdCallPlugin"
)
x64dbg_set_output_dir(test_scriptcmd_call_plugin "${SCRIPTCMD_CALL_RUNTIME_DIR}")

set(SCRIPTCMD_CALL_COPY_SCRIPT_COMMANDS
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SCRIPTCMD_CALL_RUNTIME_DIR}"
)
foreach(script ${SCRIPTCMD_CALL_TEST_SCRIPTS})
    get_filename_component(script_name "${script}" NAME)
    list(APPEND SCRIPTCMD_CALL_COPY_SCRIPT_COMMANDS
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${script}"
            "${SCRIPTCMD_CALL_RUNTIME_DIR}/${script_name}"
    )
endforeach()

add_custom_command(TARGET test_scriptcmd_call_plugin POST_BUILD
    ${SCRIPTCMD_CALL_COPY_SCRIPT_COMMANDS}
)

add_dependencies(x64dbg_tests test_scriptcmd_call_target test_scriptcmd_call_plugin)
