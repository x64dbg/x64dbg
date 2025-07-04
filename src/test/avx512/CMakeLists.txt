cmake_minimum_required(VERSION 3.15)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
	message(FATAL_ERROR "In-tree builds are not supported. Run CMake from a separate directory: cmake -B build")
endif()

# Enable support for MSVC_RUNTIME_LIBRARY
cmake_policy(SET CMP0091 NEW)

project(avx512
	LANGUAGES
		C
		ASM_MASM
)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Target: avx512
set(avx512_SOURCES
	"main.c"
)

if(CMAKE_SIZEOF_VOID_P EQUAL 4) # x32
	list(APPEND avx512_SOURCES
		"avx512_x86.asm"
	)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8) # x64
	list(APPEND avx512_SOURCES
		"avx512_x64.asm"
	)
endif()

add_executable(avx512)

target_sources(avx512 PRIVATE ${avx512_SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${avx512_SOURCES})

target_link_options(avx512 PRIVATE
	"/safeseh:no"
	"/incremental:no"
)

get_directory_property(CMKR_VS_STARTUP_PROJECT DIRECTORY ${PROJECT_SOURCE_DIR} DEFINITION VS_STARTUP_PROJECT)
if(NOT CMKR_VS_STARTUP_PROJECT)
	set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT avx512)
endif()
