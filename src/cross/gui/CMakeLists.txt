set(widgets_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../gui/Src")

add_subdirectory(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../zydis_wrapper
    ${CMAKE_CURRENT_BINARY_DIR}/zydis_wrapper
)

include(Qt.cmake)

set(hook_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Bridge.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Bridge.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Configuration.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Configuration.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MagicMenu.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MiscUtil.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MiscUtil.h
    ${CMAKE_CURRENT_SOURCE_DIR}/StringUtil.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/StringUtil.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Types.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Imports.h
)

set(widgets_SOURCES
    ${widgets_SOURCE_DIR}/BasicView/AbstractStdTable.cpp
    ${widgets_SOURCE_DIR}/BasicView/AbstractStdTable.h
    ${widgets_SOURCE_DIR}/BasicView/AbstractTableView.cpp
    ${widgets_SOURCE_DIR}/BasicView/AbstractTableView.h
    ${widgets_SOURCE_DIR}/BasicView/Disassembly.cpp
    ${widgets_SOURCE_DIR}/BasicView/Disassembly.h
    ${widgets_SOURCE_DIR}/BasicView/HexDump.cpp
    ${widgets_SOURCE_DIR}/BasicView/HexDump.h
    ${widgets_SOURCE_DIR}/BasicView/HistoryLineEdit.cpp
    ${widgets_SOURCE_DIR}/BasicView/HistoryLineEdit.h
    ${widgets_SOURCE_DIR}/BasicView/StdTable.cpp
    ${widgets_SOURCE_DIR}/BasicView/StdTable.h
    ${widgets_SOURCE_DIR}/Disassembler/Architecture.cpp
    ${widgets_SOURCE_DIR}/Disassembler/Architecture.h
    ${widgets_SOURCE_DIR}/Disassembler/QZydis.cpp
    ${widgets_SOURCE_DIR}/Disassembler/QZydis.h
    ${widgets_SOURCE_DIR}/Disassembler/ZydisTokenizer.cpp
    ${widgets_SOURCE_DIR}/Disassembler/ZydisTokenizer.h
    ${widgets_SOURCE_DIR}/Gui/BrowseDialog.cpp
    ${widgets_SOURCE_DIR}/Gui/BrowseDialog.h
    ${widgets_SOURCE_DIR}/Gui/BrowseDialog.ui
    ${widgets_SOURCE_DIR}/Gui/ColumnReorderDialog.cpp
    ${widgets_SOURCE_DIR}/Gui/ColumnReorderDialog.h
    ${widgets_SOURCE_DIR}/Gui/ColumnReorderDialog.ui
    ${widgets_SOURCE_DIR}/Gui/ComboBoxDialog.cpp
    ${widgets_SOURCE_DIR}/Gui/ComboBoxDialog.h
    ${widgets_SOURCE_DIR}/Gui/ComboBoxDialog.ui
    ${widgets_SOURCE_DIR}/Gui/DisassemblyPopup.cpp
    ${widgets_SOURCE_DIR}/Gui/DisassemblyPopup.h
    ${widgets_SOURCE_DIR}/Gui/LineEditDialog.cpp
    ${widgets_SOURCE_DIR}/Gui/LineEditDialog.h
    ${widgets_SOURCE_DIR}/Gui/LineEditDialog.ui
    ${widgets_SOURCE_DIR}/Memory/MemoryPage.cpp
    ${widgets_SOURCE_DIR}/Memory/MemoryPage.h
    ${widgets_SOURCE_DIR}/Utils/ActionHelpers.h
    ${widgets_SOURCE_DIR}/Utils/CachedFontMetrics.h
    ${widgets_SOURCE_DIR}/Utils/CodeFolding.cpp
    ${widgets_SOURCE_DIR}/Utils/CodeFolding.h
    ${widgets_SOURCE_DIR}/Utils/EncodeMap.cpp
    ${widgets_SOURCE_DIR}/Utils/EncodeMap.h
    ${widgets_SOURCE_DIR}/Utils/MenuBuilder.cpp
    ${widgets_SOURCE_DIR}/Utils/MenuBuilder.h
    ${widgets_SOURCE_DIR}/Utils/MethodInvoker.h
    ${widgets_SOURCE_DIR}/Utils/RichTextPainter.cpp
    ${widgets_SOURCE_DIR}/Utils/RichTextPainter.h
    ${widgets_SOURCE_DIR}/Utils/VaHistory.h
)

include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR})

add_library(widgets STATIC
    ${hook_SOURCES}
    ${widgets_SOURCES}
)

target_include_directories(widgets PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${widgets_SOURCE_DIR}
)

source_group(TREE ${widgets_SOURCE_DIR} FILES ${widgets_SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX hook FILES ${hook_SOURCES})

target_compile_definitions(widgets PRIVATE
    QT_NO_DEPRECATED_WARNINGS
)

target_link_libraries(widgets PUBLIC
    ${QT_LIBRARIES}
    zydis_wrapper
)

# https://doc.qt.io/qt-6/wasm.html#asyncify
if(EMSCRIPTEN)
    #target_link_options(${CMKR_TARGET} PUBLIC -sASYNCIFY -Os)
endif()