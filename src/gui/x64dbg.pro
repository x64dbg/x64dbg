#-------------------------------------------------
#
# Project created by QtCreator 2013-05-20T13:22:23
#
#-------------------------------------------------

##
## Pre-defined global variables
##

CONFIG(debug, debug|release) {
    DIR_SUFFIX = d
} else {
    DIR_SUFFIX =
}

!contains(QMAKE_HOST.arch, x86_64) {
    X64_BIN_DIR = ../../bin/x32$${DIR_SUFFIX}      # Relative BIN path, 32-bit
    X64_GEN_DIR = ../gui_build/out32$${DIR_SUFFIX} # QMake temporary generated files, placed inside the build folder. (OBJ, UI, MOC)
    TARGET = x32gui                  # Build x32gui
} else {
    X64_BIN_DIR = ../../bin/x64$${DIR_SUFFIX}      # Relative BIN path, 64-bit
    X64_GEN_DIR = ../gui_build/out64$${DIR_SUFFIX} # QMake temporary generated files, placed inside the build folder. (OBJ, UI, MOC)
    TARGET = x64gui                  # Build x64gui
}

##
## QMake output directories
##
DESTDIR = $${X64_BIN_DIR}
OBJECTS_DIR = $${X64_GEN_DIR}
MOC_DIR = $${X64_GEN_DIR}
RCC_DIR = $${X64_GEN_DIR}
UI_DIR = $${X64_GEN_DIR}

##
## QT libraries
##
QT += core gui network

# QT5 requires widgets
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

##
## Build flags
##
# Generate debug symbols in release mode
QMAKE_CXXFLAGS_RELEASE += -Zi   # Compiler
QMAKE_LFLAGS_RELEASE += /DEBUG  # Linker

# Build as a library
DEFINES += BUILD_LIB NOMINMAX
TEMPLATE = lib

##
## Includes
##
INCLUDEPATH += \
    ../ \
    Src \
    Src/Gui \
    Src/BasicView \
    Src/Disassembler \
    Src/Memory \
    Src/Bridge \
    Src/Global \
    Src/Utils \
    Src/ThirdPartyLibs/ldconvert \
    ../zydis_wrapper \
    ../zydis_wrapper/zydis/include

# Resources, sources, headers, and forms
RESOURCES += \
    resource.qrc

SOURCES += \
    Src/Gui/CPURegistersView.cpp \
    Src/Tracer/TraceWidget.cpp \
    Src/main.cpp \
    Src/Gui/MainWindow.cpp \
    Src/Gui/CPUWidget.cpp \
    Src/Gui/CommandLineEdit.cpp \
    Src/BasicView/Disassembly.cpp \
    Src/BasicView/HexDump.cpp \
    Src/BasicView/AbstractTableView.cpp \
    Src/Disassembler/QBeaEngine.cpp \
    Src/Disassembler/ZydisTokenizer.cpp \
    Src/Memory/MemoryPage.cpp \
    Src/Bridge/Bridge.cpp \
    Src/BasicView/StdTable.cpp \
    Src/Gui/MemoryMapView.cpp \
    Src/Gui/LogView.cpp \
    Src/Gui/GotoDialog.cpp \
    Src/Gui/WordEditDialog.cpp \
    Src/Gui/CPUDisassembly.cpp \
    Src/Gui/LineEditDialog.cpp \
    Src/Gui/BreakpointsView.cpp \
    Src/Utils/Breakpoints.cpp \
    Src/Gui/CPUInfoBox.cpp \
    Src/Gui/CPUDump.cpp \
    Src/Gui/ScriptView.cpp \
    Src/Gui/CPUStack.cpp \
    Src/Gui/SymbolView.cpp \
    Src/Gui/RegistersView.cpp \
    Src/BasicView/SearchListView.cpp \
    Src/BasicView/ReferenceView.cpp \
    Src/Gui/ThreadView.cpp \
    Src/Gui/SettingsDialog.cpp \
    Src/Gui/ExceptionRangeDialog.cpp \
    Src/Utils/RichTextPainter.cpp \
    Src/Gui/TabBar.cpp \
    Src/Gui/TabWidget.cpp \
    Src/BasicView/HistoryLineEdit.cpp \
    Src/Utils/Configuration.cpp \
    Src/Gui/CPUSideBar.cpp \
    Src/Gui/AppearanceDialog.cpp \
    Src/Gui/CloseDialog.cpp \
    Src/Gui/HexEditDialog.cpp \
    Src/Gui/HexLineEdit.cpp \
    Src/QHexEdit/ArrayCommand.cpp \
    Src/QHexEdit/QHexEdit.cpp \
    Src/QHexEdit/QHexEditPrivate.cpp \
    Src/QHexEdit/XByteArray.cpp \
    Src/Gui/PatchDialog.cpp \
    Src/Gui/PatchDialogGroupSelector.cpp \
    Src/Utils/UpdateChecker.cpp \
    Src/Gui/CallStackView.cpp \
    Src/Gui/ShortcutsDialog.cpp \
    Src/BasicView/ShortcutEdit.cpp \
    Src/Gui/CalculatorDialog.cpp \
    Src/Gui/AttachDialog.cpp \
    Src/Gui/PageMemoryRights.cpp \
    Src/Gui/SelectFields.cpp \
    Src/Gui/ReferenceManager.cpp \
    Src/Bridge/BridgeResult.cpp \
    Src/Gui/SourceViewerManager.cpp \
    Src/Gui/SourceView.cpp \
    Src/Utils/ValidateExpressionThread.cpp \
    Src/Utils/MainWindowCloseThread.cpp \
    Src/Gui/TimeWastedCounter.cpp \
    Src/Utils/FlickerThread.cpp \
    Src/Gui/NotesManager.cpp \
    Src/Gui/NotepadView.cpp \
    Src/Gui/CPUMultiDump.cpp \
    Src/Gui/AssembleDialog.cpp \
    Src/Utils/StringUtil.cpp \
    Src/Gui/SEHChainView.cpp \
    Src/Gui/EditBreakpointDialog.cpp \
    Src/Gui/CPUArgumentWidget.cpp \
    Src/Gui/HandlesView.cpp \
    Src/Gui/EditFloatRegister.cpp \
    Src/Utils/HexValidator.cpp \
    Src/Utils/LongLongValidator.cpp \
    Src/Utils/MiscUtil.cpp \
    Src/Gui/XrefBrowseDialog.cpp \
    Src/Gui/CodepageSelectionDialog.cpp \
    Src/Gui/ColumnReorderDialog.cpp \
    Src/Utils/EncodeMap.cpp \
    Src/Utils/CodeFolding.cpp \
    Src/Gui/WatchView.cpp \
    Src/Gui/FavouriteTools.cpp \
    Src/Gui/BrowseDialog.cpp \
    Src/Gui/DisassemblerGraphView.cpp \
    Src/Gui/DisassemblyPopup.cpp \
    Src/Gui/VirtualModDialog.cpp \
    Src/BasicView/LabeledSplitter.cpp \
    Src/BasicView/LabeledSplitterDetachedWindow.cpp \
    Src/Gui/LogStatusLabel.cpp \
    Src/Gui/DebugStatusLabel.cpp \
    Src/Utils/MenuBuilder.cpp \
    Src/Gui/StructWidget.cpp \
    Src/Gui/CustomizeMenuDialog.cpp \
    Src/Gui/SimpleTraceDialog.cpp \
    Src/Utils/MRUList.cpp \
    Src/Gui/LocalVarsView.cpp \
    Src/Gui/MessagesBreakpoints.cpp \
    Src/Gui/AboutDialog.cpp \
    Src/Gui/BreakpointMenu.cpp \
    Src/Gui/ComboBoxDialog.cpp \
    Src/Utils/SymbolAutoCompleteModel.cpp \
    Src/Tracer/TraceBrowser.cpp \
    Src/Tracer/TraceFileReader.cpp \
    Src/Tracer/TraceFileSearch.cpp \
    Src/Gui/MultiItemsSelectWindow.cpp \
    Src/BasicView/AbstractStdTable.cpp \
    Src/Gui/ZehSymbolTable.cpp \
    Src/BasicView/StdSearchListView.cpp \
    Src/BasicView/StdTableSearchList.cpp


HEADERS += \
    Src/Gui/CPURegistersView.h \
    Src/Tracer/TraceWidget.h \
    Src/main.h \
    Src/Gui/MainWindow.h \
    Src/Gui/CPUWidget.h \
    Src/Gui/CommandLineEdit.h \
    Src/BasicView/Disassembly.h \
    Src/BasicView/HexDump.h \
    Src/BasicView/AbstractTableView.h \
    Src/Disassembler/QBeaEngine.h \
    Src/Disassembler/ZydisTokenizer.h \
    Src/Memory/MemoryPage.h \
    Src/Bridge/Bridge.h \
    Src/Exports.h \
    Src/Imports.h \
    Src/BasicView/StdTable.h \
    Src/Gui/MemoryMapView.h \
    Src/Gui/LogView.h \
    Src/Gui/GotoDialog.h \
    Src/Gui/RegistersView.h \
    Src/Gui/WordEditDialog.h \
    Src/Gui/CPUDisassembly.h \
    Src/Gui/LineEditDialog.h \
    Src/Gui/BreakpointsView.h \
    Src/Utils/Breakpoints.h \
    Src/Gui/CPUInfoBox.h \
    Src/Gui/CPUDump.h \
    Src/Gui/ScriptView.h \
    Src/Gui/CPUStack.h \
    Src/Gui/SymbolView.h \
    Src/BasicView/SearchListView.h \
    Src/BasicView/ReferenceView.h \
    Src/Gui/ThreadView.h \
    Src/Gui/SettingsDialog.h \
    Src/Gui/ExceptionRangeDialog.h \
    Src/Utils/RichTextPainter.h \
    Src/Gui/TabBar.h \
    Src/Gui/TabWidget.h \
    Src/BasicView/HistoryLineEdit.h \
    Src/Utils/Configuration.h \
    Src/Gui/CPUSideBar.h \
    Src/Gui/AppearanceDialog.h \
    Src/Gui/CloseDialog.h \
    Src/Gui/HexEditDialog.h \
    Src/Gui/HexLineEdit.h \
    Src/QHexEdit/ArrayCommand.h \
    Src/QHexEdit/QHexEdit.h \
    Src/QHexEdit/QHexEditPrivate.h \
    Src/QHexEdit/XByteArray.h \
    Src/Gui/PatchDialog.h \
    Src/Gui/PatchDialogGroupSelector.h \
    Src/Utils/UpdateChecker.h \
    Src/Gui/CallStackView.h \
    Src/Gui/ShortcutsDialog.h \
    Src/BasicView/ShortcutEdit.h \
    Src/Gui/CalculatorDialog.h \
    Src/Gui/AttachDialog.h \
    Src/Gui/PageMemoryRights.h \
    Src/Gui/SelectFields.h \
    Src/Gui/ReferenceManager.h \
    Src/Bridge/BridgeResult.h \
    Src/Gui/SourceViewerManager.h \
    Src/Gui/SourceView.h \
    Src/Utils/StringUtil.h \
    Src/Utils/ValidateExpressionThread.h \
    Src/Utils/MainWindowCloseThread.h \
    Src/Gui/TimeWastedCounter.h \
    Src/Utils/FlickerThread.h \
    Src/Gui/NotesManager.h \
    Src/Gui/NotepadView.h \
    Src/Utils/MenuBuilder.h \
    Src/Gui/CPUMultiDump.h \
    Src/Gui/AssembleDialog.h \
    Src/Gui/SEHChainView.h \
    Src/Gui/EditBreakpointDialog.h \
    Src/Gui/CPUArgumentWidget.h \
    Src/Gui/HandlesView.h \
    Src/Gui/EditFloatRegister.h \
    Src/Utils/HexValidator.h \
    Src/Utils/LongLongValidator.h \
    Src/Utils/MiscUtil.h \
    Src/Gui/XrefBrowseDialog.h \
    Src/Gui/CodepageSelectionDialog.h \
    Src/Utils/CachedFontMetrics.h \
    Src/Gui/ColumnReorderDialog.h \
    Src/Utils/EncodeMap.h \
    Src/Utils/CodeFolding.h \
    Src/Gui/WatchView.h \
    Src/Gui/FavouriteTools.h \
    Src/Gui/BrowseDialog.h \
    Src/Gui/DisassemblerGraphView.h \
    Src/Utils/ActionHelpers.h \
    Src/Gui/DisassemblyPopup.h \
    Src/Gui/VirtualModDialog.h \
    Src/BasicView/LabeledSplitter.h \
    Src/BasicView/LabeledSplitterDetachedWindow.h \
    Src/Gui/LogStatusLabel.h \
    Src/Gui/DebugStatusLabel.h \
    Src/Gui/CustomizeMenuDialog.h \
    Src/Gui/StructWidget.h \
    Src/Gui/SimpleTraceDialog.h \
    Src/Utils/MRUList.h \
    Src/Gui/LocalVarsView.h \
    Src/Gui/MessagesBreakpoints.h \
    Src/Gui/AboutDialog.h \
    Src/Gui/BreakpointMenu.h \
    Src/Gui/ComboBoxDialog.h \
    Src/Utils/VaHistory.h \
    Src/Utils/SymbolAutoCompleteModel.h \
    Src/Tracer/TraceBrowser.h \
    Src/Tracer/TraceFileReader.h \
    Src/Tracer/TraceFileReaderInternal.h \
    Src/Tracer/TraceFileSearch.h \
    Src/Gui/MultiItemsSelectWindow.h \
    Src/BasicView/AbstractStdTable.h \
    Src/Gui/ZehSymbolTable.h \
    Src/BasicView/AbstractSearchList.h \
    Src/BasicView/StdSearchListView.h \
    Src/Gui/FileLines.h \
    Src/BasicView/StdTableSearchList.h \
    Src/Utils/MethodInvoker.h
    

FORMS += \
    Src/Gui/MainWindow.ui \
    Src/Gui/CPUWidget.ui \
    Src/Gui/GotoDialog.ui \
    Src/Gui/WordEditDialog.ui \
    Src/Gui/LineEditDialog.ui \
    Src/Gui/SymbolView.ui \
    Src/Gui/SettingsDialog.ui \
    Src/Gui/ExceptionRangeDialog.ui \
    Src/Gui/AppearanceDialog.ui \
    Src/Gui/CloseDialog.ui \
    Src/Gui/HexEditDialog.ui \
    Src/Gui/HexLineEdit.ui \
    Src/Gui/PatchDialog.ui \
    Src/Gui/PatchDialogGroupSelector.ui \
    Src/Gui/ShortcutsDialog.ui \
    Src/Gui/CalculatorDialog.ui \
    Src/Gui/AttachDialog.ui \
    Src/Gui/PageMemoryRights.ui \
    Src/Gui/SelectFields.ui \
    Src/Gui/AssembleDialog.ui \
    Src/Gui/EditBreakpointDialog.ui \
    Src/Gui/CPUArgumentWidget.ui \
    Src/Gui/EditFloatRegister.ui \
    Src/Gui/XrefBrowseDialog.ui \
    Src/Gui/CodepageSelectionDialog.ui \
    Src/Gui/ColumnReorderDialog.ui \
    Src/Gui/FavouriteTools.ui \
    Src/Gui/BrowseDialog.ui \
    Src/Gui/VirtualModDialog.ui \
    Src/Gui/CustomizeMenuDialog.ui \
    Src/Gui/StructWidget.ui \
    Src/Gui/SimpleTraceDialog.ui \
    Src/Gui/MessagesBreakpoints.ui \
    Src/Gui/AboutDialog.ui \
    Src/Gui/ComboBoxDialog.ui \
    Src/Tracer/TraceWidget.ui

##
## Libraries
##
LIBS += -luser32 -ladvapi32 -lwinmm -lshell32

!contains(QMAKE_HOST.arch, x86_64) {
    # Windows x86 (32bit) specific build
    LIBS += -L"$$PWD/../zydis_wrapper/bin/x32$${DIR_SUFFIX}" -lzydis_wrapper
    LIBS += -L"$$PWD/Src/ThirdPartyLibs/ldconvert" -lldconvert_x86
    LIBS += -L"$${X64_BIN_DIR}" -lx32bridge
} else {
    # Windows x64 (64bit) specific build
    LIBS += -L"$$PWD/../zydis_wrapper/bin/x64$${DIR_SUFFIX}" -lzydis_wrapper
    LIBS += -L"$$PWD/Src/ThirdPartyLibs/ldconvert" -lldconvert_x64
    LIBS += -L"$${X64_BIN_DIR}" -lx64bridge
}
