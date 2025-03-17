include_guard()

message(STATUS "Configuring Qt for cross-compilation...")

# Qt
#set(AUTOMOC_EXECUTABLE /Users/admin/Projects/qtbase-wasm32-wasi/bin/moc)
#set(AUTORCC_EXECUTABLE /Users/admin/Projects/qtbase-wasm32-wasi/bin/rcc)
#set(AUTOUIC_EXECUTABLE /Users/admin/Projects/qtbase-wasm32-wasi/bin/uic)
set(QT_HOST_BIN_DIR /Users/admin/Projects/qtbase-wasm32-wasi/bin/)

function(qt5_host_tool name)
    if (NOT TARGET Qt5::${name})
        add_executable(Qt5::${name} IMPORTED)
        if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
            set(imported_location ${QT_HOST_BIN_DIR}/${name}.exe)
        else()
            set(imported_location ${QT_HOST_BIN_DIR}/${name})
        endif()
        if(NOT EXISTS ${imported_location})
            message(FATAL_ERROR "Qt5 tool not found: ${imported_location}")
        endif()
        set_target_properties(Qt5::${name} PROPERTIES
            IMPORTED_LOCATION ${imported_location}
        )
    endif()
endfunction()

qt5_host_tool(qmake)
qt5_host_tool(moc)
qt5_host_tool(rcc)
qt5_host_tool(uic)
