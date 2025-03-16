# Get the root directory
get_filename_component(ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(RELEASE_DIR "${ROOT_DIR}/release")

# Clean up existing release directory
file(REMOVE_RECURSE "${RELEASE_DIR}")
file(MAKE_DIRECTORY "${RELEASE_DIR}")

# Setup pluginsdk directories
set(PLUGINSDK_DIR "${RELEASE_DIR}/pluginsdk")
file(MAKE_DIRECTORY "${PLUGINSDK_DIR}")
file(MAKE_DIRECTORY "${PLUGINSDK_DIR}/dbghelp")
file(MAKE_DIRECTORY "${PLUGINSDK_DIR}/DeviceNameResolver")
file(MAKE_DIRECTORY "${PLUGINSDK_DIR}/jansson")
file(MAKE_DIRECTORY "${PLUGINSDK_DIR}/lz4")
file(MAKE_DIRECTORY "${PLUGINSDK_DIR}/TitanEngine")
file(MAKE_DIRECTORY "${PLUGINSDK_DIR}/XEDParse")

# Setup release directories
file(MAKE_DIRECTORY "${RELEASE_DIR}/release")
file(MAKE_DIRECTORY "${RELEASE_DIR}/release/translations")
file(MAKE_DIRECTORY "${RELEASE_DIR}/release/x32")
file(MAKE_DIRECTORY "${RELEASE_DIR}/release/x64")

# Copy pluginsdk files
set(PLUGINSDK_DIR "${RELEASE_DIR}/pluginsdk")

# Copy directories
file(COPY "${ROOT_DIR}/src/dbg/dbghelp/" DESTINATION "${PLUGINSDK_DIR}/dbghelp")
file(COPY "${ROOT_DIR}/src/dbg/DeviceNameResolver/" DESTINATION "${PLUGINSDK_DIR}/DeviceNameResolver")
file(COPY "${ROOT_DIR}/src/dbg/jansson/" DESTINATION "${PLUGINSDK_DIR}/jansson")
file(COPY "${ROOT_DIR}/src/dbg/lz4/" DESTINATION "${PLUGINSDK_DIR}/lz4")
file(COPY "${ROOT_DIR}/src/dbg/TitanEngine/" DESTINATION "${PLUGINSDK_DIR}/TitanEngine")
file(COPY "${ROOT_DIR}/src/dbg/XEDParse/" DESTINATION "${PLUGINSDK_DIR}/XEDParse")

# Remove TitanEngine.txt
file(REMOVE "${PLUGINSDK_DIR}/TitanEngine/TitanEngine.txt")

# Copy headers
file(GLOB PLUGIN_HEADERS
    "${ROOT_DIR}/src/dbg/_plugin_types.h"
    "${ROOT_DIR}/src/dbg/_plugins.h"
    "${ROOT_DIR}/src/dbg/_scriptapi*.h"
    "${ROOT_DIR}/src/dbg/_dbgfunctions.h"
    "${ROOT_DIR}/src/bridge/bridge*.h"
)
file(COPY ${PLUGIN_HEADERS} DESTINATION "${PLUGINSDK_DIR}")

file(COPY "${ROOT_DIR}/bin/x32/x32bridge.lib" DESTINATION "${PLUGINSDK_DIR}")
file(COPY "${ROOT_DIR}/bin/x32/x32dbg.lib" DESTINATION "${PLUGINSDK_DIR}")
file(COPY "${ROOT_DIR}/bin/x64/x64bridge.lib" DESTINATION "${PLUGINSDK_DIR}")
file(COPY "${ROOT_DIR}/bin/x64/x64dbg.lib" DESTINATION "${PLUGINSDK_DIR}")

# Copy release files
set(RELEASE_MAIN_DIR "${RELEASE_DIR}/release")

# Handle deps_copied
function(handle_deps_copied arch)
    file(READ "${ROOT_DIR}/bin/${arch}/.deps_copied" DEPS_COPIED)
    string(REGEX REPLACE "\n" ";" DEPS_COPIED "${DEPS_COPIED}")
    foreach(DEP ${DEPS_COPIED})
        get_filename_component(reldir ${DEP} DIRECTORY)
        file(COPY "${ROOT_DIR}/bin/${arch}/${DEP}" DESTINATION "${RELEASE_MAIN_DIR}/${arch}/${reldir}")
    endforeach()
endfunction()

handle_deps_copied("x64")
handle_deps_copied("x32")

# Copy themes
file(COPY "${ROOT_DIR}/bin/themes/" DESTINATION "${RELEASE_MAIN_DIR}/themes")

# Copy main files
file(COPY "${ROOT_DIR}/bin/x96dbg.exe" DESTINATION "${RELEASE_MAIN_DIR}")
file(COPY "${ROOT_DIR}/bin/mnemdb.json" DESTINATION "${RELEASE_MAIN_DIR}")
file(COPY "${ROOT_DIR}/bin/errordb.txt" DESTINATION "${RELEASE_MAIN_DIR}")
file(COPY "${ROOT_DIR}/bin/exceptiondb.txt" DESTINATION "${RELEASE_MAIN_DIR}")
file(COPY "${ROOT_DIR}/bin/ntstatusdb.txt" DESTINATION "${RELEASE_MAIN_DIR}")
file(COPY "${ROOT_DIR}/bin/winconstants.txt" DESTINATION "${RELEASE_MAIN_DIR}")

# Copy translations
file(GLOB TRANSLATION_FILES "${ROOT_DIR}/bin/translations/*.qm")
file(COPY ${TRANSLATION_FILES} DESTINATION "${RELEASE_MAIN_DIR}/translations")

# Copy x32 files
file(COPY "${ROOT_DIR}/bin/x32/x32bridge.dll" DESTINATION "${RELEASE_MAIN_DIR}/x32")
file(COPY "${ROOT_DIR}/bin/x32/x32dbg.dll" DESTINATION "${RELEASE_MAIN_DIR}/x32")
file(COPY "${ROOT_DIR}/bin/x32/x32dbg.exe" DESTINATION "${RELEASE_MAIN_DIR}/x32")
file(COPY "${ROOT_DIR}/bin/x32/x32gui.dll" DESTINATION "${RELEASE_MAIN_DIR}/x32")
file(COPY "${ROOT_DIR}/bin/x32/loaddll.exe" DESTINATION "${RELEASE_MAIN_DIR}/x32")

# Copy x64 files
file(COPY "${ROOT_DIR}/bin/x64/x64bridge.dll" DESTINATION "${RELEASE_MAIN_DIR}/x64")
file(COPY "${ROOT_DIR}/bin/x64/x64dbg.dll" DESTINATION "${RELEASE_MAIN_DIR}/x64")
file(COPY "${ROOT_DIR}/bin/x64/x64dbg.exe" DESTINATION "${RELEASE_MAIN_DIR}/x64")
file(COPY "${ROOT_DIR}/bin/x64/x64gui.dll" DESTINATION "${RELEASE_MAIN_DIR}/x64")
file(COPY "${ROOT_DIR}/bin/x64/loaddll.exe" DESTINATION "${RELEASE_MAIN_DIR}/x64")

# Create commithash.txt
execute_process(
    COMMAND git rev-parse HEAD
    OUTPUT_FILE "${RELEASE_DIR}/commithash.txt"
    WORKING_DIRECTORY "${ROOT_DIR}"
)

# Copy PDB files
file(MAKE_DIRECTORY "${RELEASE_DIR}/pdb")
file(MAKE_DIRECTORY "${RELEASE_DIR}/pdb/x32")
file(MAKE_DIRECTORY "${RELEASE_DIR}/pdb/x64")

file(GLOB PDB_FILES "${ROOT_DIR}/bin/*.pdb")
file(COPY ${PDB_FILES} DESTINATION "${RELEASE_DIR}/pdb")

file(GLOB PDB_X32_FILES "${ROOT_DIR}/bin/x32/*.pdb")
file(COPY ${PDB_X32_FILES} DESTINATION "${RELEASE_DIR}/pdb/x32")

file(GLOB PDB_X64_FILES "${ROOT_DIR}/bin/x64/*.pdb")
file(COPY ${PDB_X64_FILES} DESTINATION "${RELEASE_DIR}/pdb/x64")
