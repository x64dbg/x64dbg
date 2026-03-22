set(SCRIPTCMD_CALL_THREADS_RUNTIME_DIR "${X64DBG_TEST_OUTPUT_DIR}/scriptcmd_call_threads")
file(GLOB SCRIPTCMD_CALL_THREADS_TEST_SCRIPTS CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/test*.txt")

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

add_executable(test_scriptcmd_call_threads_target target.cpp)
target_compile_definitions(test_scriptcmd_call_threads_target PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_features(test_scriptcmd_call_threads_target PRIVATE cxx_std_17)
set_target_properties(test_scriptcmd_call_threads_target PROPERTIES OUTPUT_NAME scriptcmd_call_threads)
x64dbg_set_output_dir(test_scriptcmd_call_threads_target "${X64DBG_TEST_OUTPUT_DIR}")

set(SCRIPTCMD_CALL_THREADS_COPY_SCRIPT_COMMANDS
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SCRIPTCMD_CALL_THREADS_RUNTIME_DIR}"
)
foreach(script ${SCRIPTCMD_CALL_THREADS_TEST_SCRIPTS})
    get_filename_component(script_name "${script}" NAME)
    list(APPEND SCRIPTCMD_CALL_THREADS_COPY_SCRIPT_COMMANDS
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${script}"
            "${SCRIPTCMD_CALL_THREADS_RUNTIME_DIR}/${script_name}"
    )
endforeach()

add_custom_command(TARGET test_scriptcmd_call_threads_target POST_BUILD
    ${SCRIPTCMD_CALL_THREADS_COPY_SCRIPT_COMMANDS}
)

add_dependencies(x64dbg_tests test_scriptcmd_call_threads_target)
