include_guard()

message(STATUS "Configuring Qt for Wine cross-compilation...")

set(QT_HOST_BIN_DIR ${CMAKE_CURRENT_LIST_DIR}/wine)

function(qt5_host_tool name)
    if (NOT TARGET Qt5::${name})
        add_executable(Qt5::${name} IMPORTED)

        set(imported_location ${QT_HOST_BIN_DIR}/${name})

        # FIXME run after deps.cmake to enable next
        #if(NOT EXISTS ${imported_location})
        #    message(FATAL_ERROR "Qt5 tool not found: ${imported_location}")
        #endif()

        set_target_properties(Qt5::${name} PROPERTIES
            IMPORTED_LOCATION ${imported_location}
        )
    endif()
endfunction()

qt5_host_tool(windeployqt)
qt5_host_tool(qmake)
qt5_host_tool(moc)
qt5_host_tool(rcc)
qt5_host_tool(uic)

