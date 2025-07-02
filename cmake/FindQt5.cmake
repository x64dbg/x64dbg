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
            URL "https://github.com/x64dbg/deps/releases/download/2025.07.02/qt5.12.12-msvc2017_64.7z"
            URL_HASH SHA256=770490bf09514982c8192ebde9a1fac8821108ba42b021f167bac54e85ada48a
        )
    else()
        FetchContent_Declare(Qt5
            URL "https://github.com/x64dbg/deps/releases/download/2025.07.02/qt5.12.12-msvc2017.7z"
            URL_HASH SHA256=3ff2a58e5ed772be475643cd7bb2df3e5499d7169d794ddf1ed5df5c5e862cb6
        )
    endif()
    FetchContent_MakeAvailable(Qt5)
    unset(FETCHCONTENT_QUIET)
    set(Qt5_ROOT ${qt5_SOURCE_DIR})
    find_package(Qt5 COMPONENTS ${Qt5_FIND_COMPONENTS} CONFIG REQUIRED)
endif()
