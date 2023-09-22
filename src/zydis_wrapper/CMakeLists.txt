set(zydis_wrapper_SOURCES
	Zydis/Zydis.c
	Zydis/Zydis.h
	zydis_wrapper.cpp
	zydis_wrapper.h
)

add_library(zydis_wrapper STATIC)

target_sources(zydis_wrapper PRIVATE ${zydis_wrapper_SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${zydis_wrapper_SOURCES})

target_compile_definitions(zydis_wrapper PUBLIC
	ZYCORE_STATIC_BUILD
	ZYDIS_STATIC_BUILD
)

target_compile_features(zydis_wrapper PUBLIC
	cxx_std_11
)

target_include_directories(zydis_wrapper PUBLIC
	.
)

target_include_directories(zydis_wrapper PRIVATE
	Zydis
)

