add_library(minidump INTERFACE)
target_sources(minidump INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/MiniDisassembly.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MiniDisassembly.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MiniHexDump.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MiniHexDump.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MiniMemoryMap.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MiniMemoryMap.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MiniDump.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MiniDump.h
    ${CMAKE_CURRENT_SOURCE_DIR}/udmp-parser.h
    ${CMAKE_CURRENT_SOURCE_DIR}/udmp-utils.h
)
