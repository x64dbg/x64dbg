option(ENABLE_SANITIZERS "Enable sanitizers" OFF)
if(ENABLE_SANITIZERS)
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT MATCHES "^MSVC$")
      # NOTE: There is bug in Clang-CL that makes address sanitizers not work for all projects.
      # The main issue is the 'world' project, which has WINDOWS_EXPORT_ALL_SYMBOLS.
      # This issue will likely be fixed in a later version of Clang-CL, but for now you should
      # configure with -DCMAKE_C_COMPILER=clang.exe -DCMAKE_CXX_COMPILER=clang++.exe to enable
      # UB sanitizers.
      message(WARNING "Enabling Clang-CL sanitizers (Clang works better)...")
      add_compile_options(-fsanitize=address,undefined)

      # Reference: https://devblogs.microsoft.com/cppblog/addresssanitizer-asan-for-windows-with-msvc/
      if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(ASAN_LIB_SUFFIX "x86_64")
      else()
        set(ASAN_LIB_SUFFIX "i386")
      endif()
      set(ASAN_LINKER_FLAGS "/wholearchive:clang_rt.asan-${ASAN_LIB_SUFFIX}.lib /wholearchive:clang_rt.asan_cxx-${ASAN_LIB_SUFFIX}.lib")
    else()
      message(STATUS "Enabling Clang sanitizers...")
      add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
      set(ASAN_LINKER_FLAGS "-fsanitize=address,undefined")
    endif()

    # NOTE: Only set linker flags for executables and shared libraries
    # the add_link_options command would add flags to static libraries as well
    # which causes issues with symbols being defined in multiple places.
    set(CMAKE_EXE_LINKER_FLAGS "${ASAN_LINKER_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS "${ASAN_LINKER_FLAGS}")

    if(WIN32)
      # NOTE: The sanitizer library only supports the static release runtime
      set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
    endif()
  elseif(MSVC)
    # Reference: https://learn.microsoft.com/en-us/cpp/build/reference/fsanitize
    message(WARNING "Enabling MSVC sanitizers (Clang has better support)...")
    add_compile_options(/fsanitize=address)
    add_link_options(/INCREMENTAL:NO)
  else()
    message (FATAL_ERROR "Unsupported compiler for sanitizers: ${CMAKE_CXX_COMPILER_ID}")
  endif()
endif()

# Visual Studio generator specific flags
if (CMAKE_GENERATOR MATCHES "Visual Studio")
    # HACK: DO NOT this to add compiler flags/definitions, use target_compile_options on a
    # target instead https://cmake.org/cmake/help/latest/command/target_compile_options.html

    # Enable multiprocessor compilation
    add_compile_options(/MP)
endif()

if(MSVC)
    # Generate PDB files for release builds
    add_link_options($<$<CONFIG:Release,MinSizeRel>:/DEBUG:FULL>)
    # Disable incremental linking
    add_link_options(
        $<$<CONFIG:Release,MinSizeRel,RelWithDebInfo>:/INCREMENTAL:NO>
        $<$<CONFIG:Release,MinSizeRel,RelWithDebInfo>:/OPT:REF>
        $<$<CONFIG:Release,MinSizeRel,RelWithDebInfo>:/OPT:ICF>
    )
    # Enable big objects (unity build)
    add_compile_options(/bigobj)
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

# Workaround for RC files to be treated as UTF-8 with llvm-rc
if(CMAKE_RC_COMPILER MATCHES "[\\\\/]llvm-rc(.exe)?$")
    message(STATUS "Detected llvm-rc, applying UTF-8 workaround")
    # https://github.com/llvm/llvm-project/issues/63426#issuecomment-2769972658
  # https://stackoverflow.com/a/38346103/1806760
  set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> /C 65001 /fo<OBJECT> <SOURCE>")
endif()
