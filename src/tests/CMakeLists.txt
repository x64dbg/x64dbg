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
add_subdirectory(issue3808)
add_subdirectory(cmdline_init)
add_subdirectory(cmdline_setcommandline)
add_subdirectory(guard_page)
add_subdirectory(membp)
