if(CMAKE_SCRIPT_MODE_FILE)
    set(GUI_DLL ${CMAKE_ARGV3})
    set(DEPS_DIR ${CMAKE_ARGV4})
    set(WINDEPLOYQT ${CMAKE_ARGV5})
    get_filename_component(GUI_DIR ${GUI_DLL} DIRECTORY)

    # Check if we already copied the dependencies
    if(EXISTS "${GUI_DIR}/.deps_copied")
        return()
    endif()

    message(STATUS "Copying dependencies from ${DEPS_DIR} to ${GUI_DIR}")

    execute_process(COMMAND ${WINDEPLOYQT} --no-compiler-runtime ${GUI_DLL})

    function(copy_dep relfile)
        if(EXISTS ${relfile})
            message(STATUS "Skipping ${relfile}")
            return()
        endif()
        message(STATUS "Copying ${relfile}")
        get_filename_component(reldir ${relfile} DIRECTORY)
        get_filename_component(relfile ${relfile} NAME)
        file(COPY ${DEPS_DIR}/${relfile} DESTINATION ${GUI_DIR}/${reldir})
    endfunction()

    file(GLOB DEPS RELATIVE ${DEPS_DIR} "${DEPS_DIR}/*.dll")
    foreach(DEP ${DEPS})
        if(NOT DEP MATCHES "^(Qt5|msvc)")
            copy_dep(${DEP})
        endif()
    endforeach()

    copy_dep(GleeBug/TitanEngine.dll)
    copy_dep(StaticEngine/TitanEngine.dll)

    file(TOUCH "${GUI_DIR}/.deps_copied")

    return()
endif()

if(NOT WIN32)
    message(STATUS "copy_dependencies is only supported on Windows")
    return()
endif()

if(NOT TARGET Qt5::windeployqt AND Qt5_FOUND AND TARGET Qt5::qmake)
    get_target_property(_qt5_qmake_location Qt5::qmake IMPORTED_LOCATION)

    execute_process(
        COMMAND "${_qt5_qmake_location}" -query QT_INSTALL_PREFIX
        RESULT_VARIABLE return_code
        OUTPUT_VARIABLE qt5_install_prefix
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    set(imported_location "${qt5_install_prefix}/bin/windeployqt.exe")
    if(NOT EXISTS ${imported_location})
        message(FATAL_ERROR "Qt5 tool not found: ${imported_location}")
    endif()

    add_executable(Qt5::windeployqt IMPORTED)

    set_target_properties(Qt5::windeployqt PROPERTIES
        IMPORTED_LOCATION ${imported_location}
    )
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(DEPS_DIR ${CMAKE_SOURCE_DIR}/deps/x64)
else()
    set(DEPS_DIR ${CMAKE_SOURCE_DIR}/deps/x32)
endif()

add_custom_target(deps
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_LIST_DIR}/deps.cmake $<TARGET_FILE:gui> ${DEPS_DIR} $<TARGET_FILE:Qt5::windeployqt>
)