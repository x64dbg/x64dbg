set(MEMBP_RUNTIME_DIR "${X64DBG_TEST_OUTPUT_DIR}/membp")
file(GLOB MEMBP_TEST_SCRIPTS CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/test*.txt")

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

add_executable(test_membp_target target.cpp)
target_compile_definitions(test_membp_target PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_features(test_membp_target PRIVATE cxx_std_17)
target_compile_options(test_membp_target PRIVATE /O2 /GS-)
set_target_properties(test_membp_target PROPERTIES OUTPUT_NAME membp)
x64dbg_set_output_dir(test_membp_target "${X64DBG_TEST_OUTPUT_DIR}")

target_link_libraries(test_membp_target PRIVATE kernel32)
target_link_options(test_membp_target PRIVATE
    "/entry:start"
    "/subsystem:console"
    "/incremental:no"
    "/merge:.mbd2=.mbc"
    "/merge:.mbd3=.mbc"
    "/section:.mbc,ERW"
)

add_library(test_membp_plugin SHARED plugin.cpp)
target_include_directories(test_membp_plugin PRIVATE
    "${PROJECT_SOURCE_DIR}/src/dbg"
    "${PROJECT_SOURCE_DIR}/src/bridge"
)
target_link_libraries(test_membp_plugin PRIVATE
    bridge
    dbg
)
target_compile_definitions(test_membp_plugin PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_features(test_membp_plugin PRIVATE cxx_std_17)
set_target_properties(test_membp_plugin PROPERTIES
    PREFIX ""
    SUFFIX "${X64DBG_TEST_PLUGIN_SUFFIX}"
    OUTPUT_NAME "MembpRegression"
)
x64dbg_set_output_dir(test_membp_plugin "${MEMBP_RUNTIME_DIR}")

set(MEMBP_COPY_SCRIPT_COMMANDS
    COMMAND ${CMAKE_COMMAND} -E make_directory "${MEMBP_RUNTIME_DIR}"
)
foreach(script ${MEMBP_TEST_SCRIPTS})
    get_filename_component(script_name "${script}" NAME)
    list(APPEND MEMBP_COPY_SCRIPT_COMMANDS
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${script}"
            "${MEMBP_RUNTIME_DIR}/${script_name}"
    )
endforeach()

add_custom_command(TARGET test_membp_plugin POST_BUILD
    ${MEMBP_COPY_SCRIPT_COMMANDS}
)

add_dependencies(x64dbg_tests test_membp_target test_membp_plugin)
