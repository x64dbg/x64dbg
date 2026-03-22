set(SCRIPT_RUN_EXIT_RUNTIME_DIR "${X64DBG_TEST_OUTPUT_DIR}/script_run_exit")
file(GLOB SCRIPT_RUN_EXIT_TEST_SCRIPTS CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/test*.txt")

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

add_executable(test_script_run_exit_target target.cpp)
target_compile_definitions(test_script_run_exit_target PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_features(test_script_run_exit_target PRIVATE cxx_std_17)
set_target_properties(test_script_run_exit_target PROPERTIES OUTPUT_NAME script_run_exit)
x64dbg_set_output_dir(test_script_run_exit_target "${X64DBG_TEST_OUTPUT_DIR}")

set(SCRIPT_RUN_EXIT_COPY_SCRIPT_COMMANDS
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SCRIPT_RUN_EXIT_RUNTIME_DIR}"
)
foreach(script ${SCRIPT_RUN_EXIT_TEST_SCRIPTS})
    get_filename_component(script_name "${script}" NAME)
    list(APPEND SCRIPT_RUN_EXIT_COPY_SCRIPT_COMMANDS
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${script}"
            "${SCRIPT_RUN_EXIT_RUNTIME_DIR}/${script_name}"
    )
endforeach()

add_custom_command(TARGET test_script_run_exit_target POST_BUILD
    ${SCRIPT_RUN_EXIT_COPY_SCRIPT_COMMANDS}
)

add_dependencies(x64dbg_tests test_script_run_exit_target)
