# https://github.com/mrexodia/Qt5CMakeTemplate
# License: BSL-1.0

# Make the project look nicer in IDEs
set_property(GLOBAL PROPERTY AUTOGEN_SOURCE_GROUP "Generated Files")
set_property(GLOBAL PROPERTY AUTOGEN_TARGETS_FOLDER "CMakePredefinedTargets")

# Install Visual Studio runtime
include(InstallRequiredSystemLibraries)

# Helper function to enable moc/rcc/uic
function(target_qt target)
    set_target_properties(${target} PROPERTIES
        AUTOMOC
            ON
        AUTORCC
            ON
        AUTOUIC
            ON
    )
endfunction()

# Helper function to deploy Qt DLLs
function(target_windeployqt deploy_target)
    # Based on: https://stackoverflow.com/a/41199492/1806760
    # TODO: set VCINSTALLDIR environment variable to copy MSVC runtime DLLs
    if(Qt5_FOUND AND WIN32 AND TARGET Qt5::qmake AND NOT TARGET Qt5::windeployqt)
        get_target_property(_qt5_qmake_location Qt5::qmake IMPORTED_LOCATION)

        execute_process(
            COMMAND "${_qt5_qmake_location}" -query QT_INSTALL_PREFIX
            RESULT_VARIABLE return_code
            OUTPUT_VARIABLE qt5_install_prefix
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        set(imported_location "${qt5_install_prefix}/bin/windeployqt.exe")

        if(EXISTS ${imported_location})
            add_executable(Qt5::windeployqt IMPORTED)

            set_target_properties(Qt5::windeployqt PROPERTIES
                IMPORTED_LOCATION ${imported_location}
            )
        endif()
    endif()

    if(TARGET Qt5::windeployqt AND NOT TARGET ${deploy_target}-windeployqt)
        # Create a target that rebuilds when cmake is re-run
        file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/${deploy_target}-windeployqt.c" "static void foo() { }\n")
        add_library(${deploy_target}-windeployqt STATIC
            "${CMAKE_CURRENT_BINARY_DIR}/${deploy_target}-windeployqt.c"
        )
        set_target_properties(${deploy_target}-windeployqt PROPERTIES
            FOLDER "CMakePredefinedTargets"
        )

        # Execute windeployqt in a tmp directory after build
        add_custom_command(TARGET ${deploy_target}-windeployqt
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_CURRENT_BINARY_DIR}/${deploy_target}-windeployqt"
            COMMAND Qt5::windeployqt --no-compiler-runtime --dir "${CMAKE_CURRENT_BINARY_DIR}/${deploy_target}-windeployqt" "$<TARGET_FILE:${deploy_target}>"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_BINARY_DIR}/${deploy_target}-windeployqt" "$<TARGET_FILE_DIR:${deploy_target}>"
        )

        # Copy deployment directory during installation
        install(
            DIRECTORY
            "${CMAKE_CURRENT_BINARY_DIR}/${deploy_target}-windeployqt/"
            DESTINATION bin
        )
    endif()
endfunction()

