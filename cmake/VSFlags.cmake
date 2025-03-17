# Visual Studio generator specific flags
if (CMAKE_GENERATOR MATCHES "Visual Studio")
    # HACK: DO NOT this to add compiler flags/definitions, use target_compile_options on a
    # target instead https://cmake.org/cmake/help/latest/command/target_compile_options.html

    # Enable multiprocessor compilation
    add_compile_options(/MP)
endif()

# Generate PDB files for release builds
if(MSVC)
    add_link_options(
        $<$<CONFIG:Release>:/DEBUG:FULL>
        $<$<CONFIG:Release>:/INCREMENTAL:NO>
        $<$<CONFIG:Release>:/OPT:REF>
        $<$<CONFIG:Release>:/OPT:ICF>
    )
endif()

# Make the project look nicer in IDEs
set_property(GLOBAL PROPERTY AUTOGEN_SOURCE_GROUP "Generated Files")
set_property(GLOBAL PROPERTY AUTOGEN_TARGETS_FOLDER "CMakePredefinedTargets")
set_property(GLOBAL PROPERTY AUTOMOC_SOURCE_GROUP "Generated Files")
set_property(GLOBAL PROPERTY AUTOMOC_TARGETS_FOLDER "CMakePredefinedTargets")
set_property(GLOBAL PROPERTY AUTORCC_SOURCE_GROUP "Generated Files")
set_property(GLOBAL PROPERTY AUTORCC_TARGETS_FOLDER "CMakePredefinedTargets")

# Build to the right output directory
if(X64DBG_BUILD_IN_TREE)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/x64$<$<CONFIG:Debug>:d>")
    else()
        set(OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/x32$<$<CONFIG:Debug>:d>")
    endif()

    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIRECTORY})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIRECTORY})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_DIRECTORY})
endif()
