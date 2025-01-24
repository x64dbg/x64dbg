# Visual Studio generator specific flags
if (CMAKE_GENERATOR MATCHES "Visual Studio")
    # HACK: DO NOT this to add compiler flags/definitions, use target_compile_options on a
    # target instead https://cmake.org/cmake/help/latest/command/target_compile_options.html

    # Enable multiprocessor compilation
    add_compile_options(/MP)
endif()

# TODO: support other toolchains
if(MSVC)
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/DEBUG:FULL /INCREMENTAL:NO /OPT:REF /OPT:ICF" CACHE STRING "")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "/DEBUG:FULL /INCREMENTAL:NO /OPT:REF /OPT:ICF" CACHE STRING "")
endif()

# Make the project look nicer in IDEs
set_property(GLOBAL PROPERTY AUTOGEN_SOURCE_GROUP "Generated Files")
set_property(GLOBAL PROPERTY AUTOGEN_TARGETS_FOLDER "CMakePredefinedTargets")
set_property(GLOBAL PROPERTY AUTOMOC_SOURCE_GROUP "Generated Files")
set_property(GLOBAL PROPERTY AUTOMOC_TARGETS_FOLDER "CMakePredefinedTargets")
set_property(GLOBAL PROPERTY AUTORCC_SOURCE_GROUP "Generated Files")
set_property(GLOBAL PROPERTY AUTORCC_TARGETS_FOLDER "CMakePredefinedTargets")
