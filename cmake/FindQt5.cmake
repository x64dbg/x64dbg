# Documentation: https://cmake.org/cmake/help/latest/manual/cmake-developer.7.html#find-modules

if(Qt5_FOUND)
    return()
endif()

find_package(Qt5 COMPONENTS ${Qt5_FIND_COMPONENTS} QUIET CONFIG)

if(Qt5_FOUND)
    if(NOT Qt5_FIND_QUIETLY)
        message(STATUS "Qt5 found: ${Qt5_DIR}")
    endif()
    return()
endif()

if(Qt5_FIND_REQUIRED AND MSVC)
    message(STATUS "Downloading Qt5...")
    # Fix warnings about DOWNLOAD_EXTRACT_TIMESTAMP
    if(POLICY CMP0135)
        cmake_policy(SET CMP0135 NEW)
    endif()
    include(FetchContent)
    set(FETCHCONTENT_QUIET OFF)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        FetchContent_Declare(Qt5
            URL "https://github.com/x64dbg/deps/releases/download/dependencies/qt5.12.12-msvc2017_64.7z"
            URL_HASH SHA256=bf302d02366dc09e112a1146e202429717c2097446edba8da3c3168d9a9996b4
        )
    else()
        FetchContent_Declare(Qt5
            URL "https://github.com/x64dbg/deps/releases/download/dependencies/qt5.12.12-msvc2017.7z"
            URL_HASH SHA256=0ea8bf7ea3ac5d7dfeb87af4ce471eefdadbf0809524fda2b0ca07aaf4654cfb
        )
    endif()
    FetchContent_MakeAvailable(Qt5)
    unset(FETCHCONTENT_QUIET)
    set(Qt5_ROOT ${qt5_SOURCE_DIR})
    find_package(Qt5 COMPONENTS ${Qt5_FIND_COMPONENTS} CONFIG REQUIRED)
endif()
