# This file is automatically generated from cmake.toml - DO NOT EDIT
# See https://github.com/build-cpp/cmkr for more information

cmake_minimum_required(VERSION 3.15)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
	message(FATAL_ERROR "In-tree builds are not supported. Run CMake from a separate directory: cmake -B build")
endif()

# Enable support for MSVC_RUNTIME_LIBRARY
cmake_policy(SET CMP0091 NEW)

set(CMKR_ROOT_PROJECT OFF)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
	set(CMKR_ROOT_PROJECT ON)

	# Bootstrap cmkr and automatically regenerate CMakeLists.txt
	include("cmake/cmkr.cmake" OPTIONAL RESULT_VARIABLE CMKR_INCLUDE_RESULT)
	if(CMKR_INCLUDE_RESULT)
		cmkr()
	endif()

	# Enable folder support
	set_property(GLOBAL PROPERTY USE_FOLDERS ON)

	# Create a configure-time dependency on cmake.toml to improve IDE support
	set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS cmake.toml)
	if(NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
		set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
	endif()

endif()

# Options
option(X64DBG_BUILD_IN_TREE "" ON)

# Variables
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include("cmake/VSToolchain.cmake")

project(x64dbg
	DESCRIPTION
		"An open-source x64/x32 debugger for windows."
)

include("cmake/VSFlags.cmake")

# Packages
find_package(Qt5 REQUIRED
	COMPONENTS
		Widgets
		WinExtras
)

# Target: zydis_wrapper
set(zydis_wrapper_SOURCES
	cmake.toml
	"src/zydis_wrapper/Zydis/Zydis.h"
	"src/zydis_wrapper/zydis/Zydis.c"
	"src/zydis_wrapper/zydis_wrapper.cpp"
	"src/zydis_wrapper/zydis_wrapper.h"
)

add_library(zydis_wrapper STATIC)

target_sources(zydis_wrapper PRIVATE ${zydis_wrapper_SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${zydis_wrapper_SOURCES})

target_compile_definitions(zydis_wrapper PUBLIC
	ZYCORE_STATIC_BUILD
	ZYDIS_STATIC_BUILD
)

target_include_directories(zydis_wrapper PUBLIC
	"src/zydis_wrapper"
)

target_include_directories(zydis_wrapper PRIVATE
	"src/zydis_wrapper/Zydis"
)

# Target: bridge
set(bridge_SOURCES
	cmake.toml
	"src/bridge/Utf8Ini.h"
	"src/bridge/_global.cpp"
	"src/bridge/_global.h"
	"src/bridge/bridgegraph.h"
	"src/bridge/bridgelist.h"
	"src/bridge/bridgemain.cpp"
	"src/bridge/bridgemain.h"
	"src/bridge/bridgemain_checker.c"
)

add_library(bridge SHARED)

target_sources(bridge PRIVATE ${bridge_SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${bridge_SOURCES})

target_compile_definitions(bridge PRIVATE
	BUILD_BRIDGE
)

if(CMAKE_SIZEOF_VOID_P EQUAL 4) # x86
	set_target_properties(bridge PROPERTIES
		OUTPUT_NAME
			x32bridge
	)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8) # x64
	set_target_properties(bridge PROPERTIES
		OUTPUT_NAME
			x64bridge
	)
endif()

# Target: btparser
set(btparser_SOURCES
	cmake.toml
	"src/dbg/btparser/btparser/ast.h"
	"src/dbg/btparser/btparser/helpers.h"
	"src/dbg/btparser/btparser/keywords.h"
	"src/dbg/btparser/btparser/lexer.cpp"
	"src/dbg/btparser/btparser/lexer.h"
	"src/dbg/btparser/btparser/operators.h"
	"src/dbg/btparser/btparser/parser.cpp"
	"src/dbg/btparser/btparser/parser.h"
	"src/dbg/btparser/btparser/testfiles.h"
)

add_library(btparser STATIC)

target_sources(btparser PRIVATE ${btparser_SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${btparser_SOURCES})

target_include_directories(btparser PUBLIC
	"src/dbg/btparser"
)

# Target: dbg
set(dbg_SOURCES
	cmake.toml
	"src/dbg/DeviceNameResolver/DeviceNameResolver.h"
	"src/dbg/GetPeArch.h"
	"src/dbg/TitanEngine/TitanEngine.h"
	"src/dbg/TraceRecord.cpp"
	"src/dbg/TraceRecord.h"
	"src/dbg/WinInet-Downloader/downslib.cpp"
	"src/dbg/WinInet-Downloader/downslib.h"
	"src/dbg/XEDParse/XEDParse.h"
	"src/dbg/_dbgfunctions.cpp"
	"src/dbg/_dbgfunctions.h"
	"src/dbg/_exports.cpp"
	"src/dbg/_exports.h"
	"src/dbg/_global.cpp"
	"src/dbg/_global.h"
	"src/dbg/_plugin_types.h"
	"src/dbg/_plugins.cpp"
	"src/dbg/_plugins.h"
	"src/dbg/_scriptapi.h"
	"src/dbg/_scriptapi_argument.cpp"
	"src/dbg/_scriptapi_argument.h"
	"src/dbg/_scriptapi_assembler.cpp"
	"src/dbg/_scriptapi_assembler.h"
	"src/dbg/_scriptapi_bookmark.cpp"
	"src/dbg/_scriptapi_bookmark.h"
	"src/dbg/_scriptapi_comment.cpp"
	"src/dbg/_scriptapi_comment.h"
	"src/dbg/_scriptapi_debug.cpp"
	"src/dbg/_scriptapi_debug.h"
	"src/dbg/_scriptapi_flag.cpp"
	"src/dbg/_scriptapi_flag.h"
	"src/dbg/_scriptapi_function.cpp"
	"src/dbg/_scriptapi_function.h"
	"src/dbg/_scriptapi_gui.cpp"
	"src/dbg/_scriptapi_gui.h"
	"src/dbg/_scriptapi_label.cpp"
	"src/dbg/_scriptapi_label.h"
	"src/dbg/_scriptapi_memory.cpp"
	"src/dbg/_scriptapi_memory.h"
	"src/dbg/_scriptapi_misc.cpp"
	"src/dbg/_scriptapi_misc.h"
	"src/dbg/_scriptapi_module.cpp"
	"src/dbg/_scriptapi_module.h"
	"src/dbg/_scriptapi_pattern.cpp"
	"src/dbg/_scriptapi_pattern.h"
	"src/dbg/_scriptapi_register.cpp"
	"src/dbg/_scriptapi_register.h"
	"src/dbg/_scriptapi_stack.cpp"
	"src/dbg/_scriptapi_stack.h"
	"src/dbg/_scriptapi_symbol.cpp"
	"src/dbg/_scriptapi_symbol.h"
	"src/dbg/addrinfo.cpp"
	"src/dbg/addrinfo.h"
	"src/dbg/analysis/AnalysisPass.cpp"
	"src/dbg/analysis/AnalysisPass.h"
	"src/dbg/analysis/BasicBlock.h"
	"src/dbg/analysis/CodeFollowPass.cpp"
	"src/dbg/analysis/CodeFollowPass.h"
	"src/dbg/analysis/FunctionPass.cpp"
	"src/dbg/analysis/FunctionPass.h"
	"src/dbg/analysis/LinearPass.cpp"
	"src/dbg/analysis/LinearPass.h"
	"src/dbg/analysis/advancedanalysis.cpp"
	"src/dbg/analysis/advancedanalysis.h"
	"src/dbg/analysis/analysis.cpp"
	"src/dbg/analysis/analysis.h"
	"src/dbg/analysis/analysis_nukem.cpp"
	"src/dbg/analysis/analysis_nukem.h"
	"src/dbg/analysis/controlflowanalysis.cpp"
	"src/dbg/analysis/controlflowanalysis.h"
	"src/dbg/analysis/exceptiondirectoryanalysis.cpp"
	"src/dbg/analysis/exceptiondirectoryanalysis.h"
	"src/dbg/analysis/linearanalysis.cpp"
	"src/dbg/analysis/linearanalysis.h"
	"src/dbg/analysis/recursiveanalysis.cpp"
	"src/dbg/analysis/recursiveanalysis.h"
	"src/dbg/analysis/xrefsanalysis.cpp"
	"src/dbg/analysis/xrefsanalysis.h"
	"src/dbg/animate.cpp"
	"src/dbg/animate.h"
	"src/dbg/argument.cpp"
	"src/dbg/argument.h"
	"src/dbg/assemble.cpp"
	"src/dbg/assemble.h"
	"src/dbg/bookmark.cpp"
	"src/dbg/bookmark.h"
	"src/dbg/breakpoint.cpp"
	"src/dbg/breakpoint.h"
	"src/dbg/bridgemain.h"
	"src/dbg/command.cpp"
	"src/dbg/command.h"
	"src/dbg/commandline.cpp"
	"src/dbg/commandline.h"
	"src/dbg/commandparser.cpp"
	"src/dbg/commandparser.h"
	"src/dbg/commands/cmd-all.h"
	"src/dbg/commands/cmd-analysis.cpp"
	"src/dbg/commands/cmd-analysis.h"
	"src/dbg/commands/cmd-breakpoint-control.cpp"
	"src/dbg/commands/cmd-breakpoint-control.h"
	"src/dbg/commands/cmd-conditional-breakpoint-control.cpp"
	"src/dbg/commands/cmd-conditional-breakpoint-control.h"
	"src/dbg/commands/cmd-debug-control.cpp"
	"src/dbg/commands/cmd-debug-control.h"
	"src/dbg/commands/cmd-general-purpose.cpp"
	"src/dbg/commands/cmd-general-purpose.h"
	"src/dbg/commands/cmd-gui.cpp"
	"src/dbg/commands/cmd-gui.h"
	"src/dbg/commands/cmd-memory-operations.cpp"
	"src/dbg/commands/cmd-memory-operations.h"
	"src/dbg/commands/cmd-misc.cpp"
	"src/dbg/commands/cmd-misc.h"
	"src/dbg/commands/cmd-operating-system-control.cpp"
	"src/dbg/commands/cmd-operating-system-control.h"
	"src/dbg/commands/cmd-plugins.cpp"
	"src/dbg/commands/cmd-plugins.h"
	"src/dbg/commands/cmd-script.cpp"
	"src/dbg/commands/cmd-script.h"
	"src/dbg/commands/cmd-searching.cpp"
	"src/dbg/commands/cmd-searching.h"
	"src/dbg/commands/cmd-thread-control.cpp"
	"src/dbg/commands/cmd-thread-control.h"
	"src/dbg/commands/cmd-tracing.cpp"
	"src/dbg/commands/cmd-tracing.h"
	"src/dbg/commands/cmd-types.cpp"
	"src/dbg/commands/cmd-types.h"
	"src/dbg/commands/cmd-undocumented.cpp"
	"src/dbg/commands/cmd-undocumented.h"
	"src/dbg/commands/cmd-user-database.cpp"
	"src/dbg/commands/cmd-user-database.h"
	"src/dbg/commands/cmd-variables.cpp"
	"src/dbg/commands/cmd-variables.h"
	"src/dbg/commands/cmd-watch-control.cpp"
	"src/dbg/commands/cmd-watch-control.h"
	"src/dbg/comment.cpp"
	"src/dbg/comment.h"
	"src/dbg/console.cpp"
	"src/dbg/console.h"
	"src/dbg/database.cpp"
	"src/dbg/database.h"
	"src/dbg/datainst_helper.cpp"
	"src/dbg/datainst_helper.h"
	"src/dbg/dbghelp_safe.cpp"
	"src/dbg/dbghelp_safe.h"
	"src/dbg/debugger.cpp"
	"src/dbg/debugger.h"
	"src/dbg/debugger_cookie.h"
	"src/dbg/debugger_tracing.h"
	"src/dbg/disasm_fast.cpp"
	"src/dbg/disasm_fast.h"
	"src/dbg/disasm_helper.cpp"
	"src/dbg/disasm_helper.h"
	"src/dbg/dynamicmem.h"
	"src/dbg/encodemap.cpp"
	"src/dbg/encodemap.h"
	"src/dbg/exception.cpp"
	"src/dbg/exception.h"
	"src/dbg/exhandlerinfo.cpp"
	"src/dbg/exhandlerinfo.h"
	"src/dbg/expressionfunctions.cpp"
	"src/dbg/expressionfunctions.h"
	"src/dbg/expressionparser.cpp"
	"src/dbg/expressionparser.h"
	"src/dbg/exprfunc.cpp"
	"src/dbg/exprfunc.h"
	"src/dbg/filehelper.cpp"
	"src/dbg/filehelper.h"
	"src/dbg/filemap.h"
	"src/dbg/formatfunctions.cpp"
	"src/dbg/formatfunctions.h"
	"src/dbg/function.cpp"
	"src/dbg/function.h"
	"src/dbg/handle.h"
	"src/dbg/handles.cpp"
	"src/dbg/handles.h"
	"src/dbg/historycontext.cpp"
	"src/dbg/historycontext.h"
	"src/dbg/jansson/jansson.h"
	"src/dbg/jansson/jansson_config.h"
	"src/dbg/jansson/jansson_x64dbg.h"
	"src/dbg/jit.cpp"
	"src/dbg/jit.h"
	"src/dbg/label.cpp"
	"src/dbg/label.h"
	"src/dbg/loop.cpp"
	"src/dbg/loop.h"
	"src/dbg/lz4/lz4.h"
	"src/dbg/lz4/lz4file.h"
	"src/dbg/lz4/lz4hc.h"
	"src/dbg/main.cpp"
	"src/dbg/memory.cpp"
	"src/dbg/memory.h"
	"src/dbg/mnemonichelp.cpp"
	"src/dbg/mnemonichelp.h"
	"src/dbg/module.cpp"
	"src/dbg/module.h"
	"src/dbg/msdia/cvconst.h"
	"src/dbg/msdia/dia2.h"
	"src/dbg/msdia/diacreate.cpp"
	"src/dbg/msdia/diacreate.h"
	"src/dbg/msgqueue.cpp"
	"src/dbg/msgqueue.h"
	"src/dbg/murmurhash.cpp"
	"src/dbg/murmurhash.h"
	"src/dbg/ntdll/ntdll.h"
	"src/dbg/patches.cpp"
	"src/dbg/patches.h"
	"src/dbg/patternfind.cpp"
	"src/dbg/patternfind.h"
	"src/dbg/pdbdiafile.cpp"
	"src/dbg/pdbdiafile.h"
	"src/dbg/pdbdiatypes.h"
	"src/dbg/plugin_loader.cpp"
	"src/dbg/plugin_loader.h"
	"src/dbg/reference.cpp"
	"src/dbg/reference.h"
	"src/dbg/serializablemap.h"
	"src/dbg/simplescript.cpp"
	"src/dbg/simplescript.h"
	"src/dbg/stackinfo.cpp"
	"src/dbg/stackinfo.h"
	"src/dbg/stringformat.cpp"
	"src/dbg/stringformat.h"
	"src/dbg/stringutils.cpp"
	"src/dbg/stringutils.h"
	"src/dbg/symbolinfo.cpp"
	"src/dbg/symbolinfo.h"
	"src/dbg/symbolsourcebase.cpp"
	"src/dbg/symbolsourcebase.h"
	"src/dbg/symbolsourcedia.cpp"
	"src/dbg/symbolsourcedia.h"
	"src/dbg/symbolundecorator.h"
	"src/dbg/syscalls.h"
	"src/dbg/taskthread.h"
	"src/dbg/tcpconnections.cpp"
	"src/dbg/tcpconnections.h"
	"src/dbg/thread.cpp"
	"src/dbg/thread.h"
	"src/dbg/threading.cpp"
	"src/dbg/threading.h"
	"src/dbg/types.cpp"
	"src/dbg/types.h"
	"src/dbg/typesparser.cpp"
	"src/dbg/typevisitor.cpp"
	"src/dbg/typevisitor.h"
	"src/dbg/value.cpp"
	"src/dbg/value.h"
	"src/dbg/variable.cpp"
	"src/dbg/variable.h"
	"src/dbg/watch.cpp"
	"src/dbg/watch.h"
	"src/dbg/x64dbg.cpp"
	"src/dbg/x64dbg.h"
	"src/dbg/xrefs.cpp"
	"src/dbg/xrefs.h"
)

add_library(dbg SHARED)

target_sources(dbg PRIVATE ${dbg_SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${dbg_SOURCES})

target_compile_definitions(dbg PRIVATE
	BUILD_DBG
)

target_include_directories(dbg PRIVATE
	"src/dbg"
	"src/dbg/analysis"
	"src/dbg/commands"
)

if(MSVC) # msvc
	target_link_libraries(dbg PUBLIC
		Delayimp
	)
endif()

if(NOT TARGET zydis_wrapper)
	message(FATAL_ERROR "Target \"zydis_wrapper\" referenced by \"dbg\" does not exist!")
endif()

if(NOT TARGET bridge)
	message(FATAL_ERROR "Target \"bridge\" referenced by \"dbg\" does not exist!")
endif()

if(NOT TARGET btparser)
	message(FATAL_ERROR "Target \"btparser\" referenced by \"dbg\" does not exist!")
endif()

target_link_libraries(dbg PRIVATE
	zydis_wrapper
	bridge
	btparser
	Psapi
	Shlwapi
	Ws2_32
	Wininet
)

if(CMAKE_SIZEOF_VOID_P EQUAL 4) # x86
	target_link_libraries(dbg PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/src/dbg/dbghelp/dbghelp_x86.lib"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/dbg/DeviceNameResolver/DeviceNameResolver_x86.lib"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/dbg/jansson/jansson_x86.lib"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/dbg/lz4/lz4_x86.lib"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/dbg/ntdll/ntdll_x86.lib"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/dbg/TitanEngine/TitanEngine_x86.lib"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/dbg/XEDParse/XEDParse_x86.lib"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/dbg/LLVMDemangle/LLVMDemangle_x86.lib"
	)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8) # x64
	target_link_libraries(dbg PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/src/dbg/dbghelp/dbghelp_x64.lib"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/dbg/DeviceNameResolver/DeviceNameResolver_x64.lib"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/dbg/jansson/jansson_x64.lib"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/dbg/lz4/lz4_x64.lib"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/dbg/ntdll/ntdll_x64.lib"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/dbg/TitanEngine/TitanEngine_x64.lib"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/dbg/XEDParse/XEDParse_x64.lib"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/dbg/LLVMDemangle/LLVMDemangle_x64.lib"
	)
endif()

if(MSVC) # msvc
	target_link_options(dbg PUBLIC
		"/DELAYLOAD:TitanEngine.dll"
	)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 4) # x86
	set_target_properties(dbg PROPERTIES
		OUTPUT_NAME
			x32dbg
	)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8) # x64
	set_target_properties(dbg PROPERTIES
		OUTPUT_NAME
			x64dbg
	)
endif()

# Target: gui
set(gui_SOURCES
	cmake.toml
	"src/gui/Src/BasicView/AbstractSearchList.h"
	"src/gui/Src/BasicView/AbstractStdTable.cpp"
	"src/gui/Src/BasicView/AbstractStdTable.h"
	"src/gui/Src/BasicView/AbstractTableView.cpp"
	"src/gui/Src/BasicView/AbstractTableView.h"
	"src/gui/Src/BasicView/Disassembly.cpp"
	"src/gui/Src/BasicView/Disassembly.h"
	"src/gui/Src/BasicView/HeaderButton.h"
	"src/gui/Src/BasicView/HexDump.cpp"
	"src/gui/Src/BasicView/HexDump.h"
	"src/gui/Src/BasicView/HistoryLineEdit.cpp"
	"src/gui/Src/BasicView/HistoryLineEdit.h"
	"src/gui/Src/BasicView/LabeledSplitter.cpp"
	"src/gui/Src/BasicView/LabeledSplitter.h"
	"src/gui/Src/BasicView/LabeledSplitterDetachedWindow.cpp"
	"src/gui/Src/BasicView/LabeledSplitterDetachedWindow.h"
	"src/gui/Src/BasicView/ReferenceView.cpp"
	"src/gui/Src/BasicView/ReferenceView.h"
	"src/gui/Src/BasicView/SearchListView.cpp"
	"src/gui/Src/BasicView/SearchListView.h"
	"src/gui/Src/BasicView/ShortcutEdit.cpp"
	"src/gui/Src/BasicView/ShortcutEdit.h"
	"src/gui/Src/BasicView/StdIconSearchListView.cpp"
	"src/gui/Src/BasicView/StdIconSearchListView.h"
	"src/gui/Src/BasicView/StdIconTable.cpp"
	"src/gui/Src/BasicView/StdIconTable.h"
	"src/gui/Src/BasicView/StdSearchListView.cpp"
	"src/gui/Src/BasicView/StdSearchListView.h"
	"src/gui/Src/BasicView/StdTable.cpp"
	"src/gui/Src/BasicView/StdTable.h"
	"src/gui/Src/BasicView/StdTableSearchList.cpp"
	"src/gui/Src/BasicView/StdTableSearchList.h"
	"src/gui/Src/Bridge/Bridge.cpp"
	"src/gui/Src/Bridge/Bridge.h"
	"src/gui/Src/Bridge/BridgeResult.cpp"
	"src/gui/Src/Bridge/BridgeResult.h"
	"src/gui/Src/Disassembler/Architecture.cpp"
	"src/gui/Src/Disassembler/Architecture.h"
	"src/gui/Src/Disassembler/QZydis.cpp"
	"src/gui/Src/Disassembler/QZydis.h"
	"src/gui/Src/Disassembler/ZydisTokenizer.cpp"
	"src/gui/Src/Disassembler/ZydisTokenizer.h"
	"src/gui/Src/Exports.h"
	"src/gui/Src/Gui/AboutDialog.cpp"
	"src/gui/Src/Gui/AboutDialog.h"
	"src/gui/Src/Gui/AboutDialog.ui"
	"src/gui/Src/Gui/AppearanceDialog.cpp"
	"src/gui/Src/Gui/AppearanceDialog.h"
	"src/gui/Src/Gui/AppearanceDialog.ui"
	"src/gui/Src/Gui/AssembleDialog.cpp"
	"src/gui/Src/Gui/AssembleDialog.h"
	"src/gui/Src/Gui/AssembleDialog.ui"
	"src/gui/Src/Gui/AttachDialog.cpp"
	"src/gui/Src/Gui/AttachDialog.h"
	"src/gui/Src/Gui/AttachDialog.ui"
	"src/gui/Src/Gui/BreakpointsView.cpp"
	"src/gui/Src/Gui/BreakpointsView.h"
	"src/gui/Src/Gui/BrowseDialog.cpp"
	"src/gui/Src/Gui/BrowseDialog.h"
	"src/gui/Src/Gui/BrowseDialog.ui"
	"src/gui/Src/Gui/CPUArgumentWidget.cpp"
	"src/gui/Src/Gui/CPUArgumentWidget.h"
	"src/gui/Src/Gui/CPUArgumentWidget.ui"
	"src/gui/Src/Gui/CPUDisassembly.cpp"
	"src/gui/Src/Gui/CPUDisassembly.h"
	"src/gui/Src/Gui/CPUDump.cpp"
	"src/gui/Src/Gui/CPUDump.h"
	"src/gui/Src/Gui/CPUInfoBox.cpp"
	"src/gui/Src/Gui/CPUInfoBox.h"
	"src/gui/Src/Gui/CPUMultiDump.cpp"
	"src/gui/Src/Gui/CPUMultiDump.h"
	"src/gui/Src/Gui/CPURegistersView.cpp"
	"src/gui/Src/Gui/CPURegistersView.h"
	"src/gui/Src/Gui/CPUSideBar.cpp"
	"src/gui/Src/Gui/CPUSideBar.h"
	"src/gui/Src/Gui/CPUStack.cpp"
	"src/gui/Src/Gui/CPUStack.h"
	"src/gui/Src/Gui/CPUWidget.cpp"
	"src/gui/Src/Gui/CPUWidget.h"
	"src/gui/Src/Gui/CPUWidget.ui"
	"src/gui/Src/Gui/CalculatorDialog.cpp"
	"src/gui/Src/Gui/CalculatorDialog.h"
	"src/gui/Src/Gui/CalculatorDialog.ui"
	"src/gui/Src/Gui/CallStackView.cpp"
	"src/gui/Src/Gui/CallStackView.h"
	"src/gui/Src/Gui/CloseDialog.cpp"
	"src/gui/Src/Gui/CloseDialog.h"
	"src/gui/Src/Gui/CloseDialog.ui"
	"src/gui/Src/Gui/CodepageSelectionDialog.cpp"
	"src/gui/Src/Gui/CodepageSelectionDialog.h"
	"src/gui/Src/Gui/CodepageSelectionDialog.ui"
	"src/gui/Src/Gui/ColumnReorderDialog.cpp"
	"src/gui/Src/Gui/ColumnReorderDialog.h"
	"src/gui/Src/Gui/ColumnReorderDialog.ui"
	"src/gui/Src/Gui/ComboBoxDialog.cpp"
	"src/gui/Src/Gui/ComboBoxDialog.h"
	"src/gui/Src/Gui/ComboBoxDialog.ui"
	"src/gui/Src/Gui/CommandLineEdit.cpp"
	"src/gui/Src/Gui/CommandLineEdit.h"
	"src/gui/Src/Gui/CustomizeMenuDialog.cpp"
	"src/gui/Src/Gui/CustomizeMenuDialog.h"
	"src/gui/Src/Gui/CustomizeMenuDialog.ui"
	"src/gui/Src/Gui/DebugStatusLabel.cpp"
	"src/gui/Src/Gui/DebugStatusLabel.h"
	"src/gui/Src/Gui/DisassemblerGraphView.cpp"
	"src/gui/Src/Gui/DisassemblerGraphView.h"
	"src/gui/Src/Gui/DisassemblyPopup.cpp"
	"src/gui/Src/Gui/DisassemblyPopup.h"
	"src/gui/Src/Gui/DisplayTypeDialog.cpp"
	"src/gui/Src/Gui/DisplayTypeDialog.h"
	"src/gui/Src/Gui/DisplayTypeDialog.ui"
	"src/gui/Src/Gui/EditBreakpointDialog.cpp"
	"src/gui/Src/Gui/EditBreakpointDialog.h"
	"src/gui/Src/Gui/EditBreakpointDialog.ui"
	"src/gui/Src/Gui/EditFloatRegister.cpp"
	"src/gui/Src/Gui/EditFloatRegister.h"
	"src/gui/Src/Gui/EditFloatRegister.ui"
	"src/gui/Src/Gui/ExceptionRangeDialog.cpp"
	"src/gui/Src/Gui/ExceptionRangeDialog.h"
	"src/gui/Src/Gui/ExceptionRangeDialog.ui"
	"src/gui/Src/Gui/FavouriteTools.cpp"
	"src/gui/Src/Gui/FavouriteTools.h"
	"src/gui/Src/Gui/FavouriteTools.ui"
	"src/gui/Src/Gui/FileLines.h"
	"src/gui/Src/Gui/GotoDialog.cpp"
	"src/gui/Src/Gui/GotoDialog.h"
	"src/gui/Src/Gui/GotoDialog.ui"
	"src/gui/Src/Gui/HandlesView.cpp"
	"src/gui/Src/Gui/HandlesView.h"
	"src/gui/Src/Gui/HexEditDialog.cpp"
	"src/gui/Src/Gui/HexEditDialog.h"
	"src/gui/Src/Gui/HexEditDialog.ui"
	"src/gui/Src/Gui/HexLineEdit.cpp"
	"src/gui/Src/Gui/HexLineEdit.h"
	"src/gui/Src/Gui/HexLineEdit.ui"
	"src/gui/Src/Gui/LineEditDialog.cpp"
	"src/gui/Src/Gui/LineEditDialog.h"
	"src/gui/Src/Gui/LineEditDialog.ui"
	"src/gui/Src/Gui/LocalVarsView.cpp"
	"src/gui/Src/Gui/LocalVarsView.h"
	"src/gui/Src/Gui/LogStatusLabel.cpp"
	"src/gui/Src/Gui/LogStatusLabel.h"
	"src/gui/Src/Gui/LogView.cpp"
	"src/gui/Src/Gui/LogView.h"
	"src/gui/Src/Gui/MainWindow.cpp"
	"src/gui/Src/Gui/MainWindow.h"
	"src/gui/Src/Gui/MainWindow.ui"
	"src/gui/Src/Gui/MemoryMapView.cpp"
	"src/gui/Src/Gui/MemoryMapView.h"
	"src/gui/Src/Gui/MessagesBreakpoints.cpp"
	"src/gui/Src/Gui/MessagesBreakpoints.h"
	"src/gui/Src/Gui/MessagesBreakpoints.ui"
	"src/gui/Src/Gui/MultiItemsSelectWindow.cpp"
	"src/gui/Src/Gui/MultiItemsSelectWindow.h"
	"src/gui/Src/Gui/NotepadView.cpp"
	"src/gui/Src/Gui/NotepadView.h"
	"src/gui/Src/Gui/NotesManager.cpp"
	"src/gui/Src/Gui/NotesManager.h"
	"src/gui/Src/Gui/PageMemoryRights.cpp"
	"src/gui/Src/Gui/PageMemoryRights.h"
	"src/gui/Src/Gui/PageMemoryRights.ui"
	"src/gui/Src/Gui/PatchDialog.cpp"
	"src/gui/Src/Gui/PatchDialog.h"
	"src/gui/Src/Gui/PatchDialog.ui"
	"src/gui/Src/Gui/PatchDialogGroupSelector.cpp"
	"src/gui/Src/Gui/PatchDialogGroupSelector.h"
	"src/gui/Src/Gui/PatchDialogGroupSelector.ui"
	"src/gui/Src/Gui/ReferenceManager.cpp"
	"src/gui/Src/Gui/ReferenceManager.h"
	"src/gui/Src/Gui/RegistersView.cpp"
	"src/gui/Src/Gui/RegistersView.h"
	"src/gui/Src/Gui/RichTextItemDelegate.cpp"
	"src/gui/Src/Gui/RichTextItemDelegate.h"
	"src/gui/Src/Gui/SEHChainView.cpp"
	"src/gui/Src/Gui/SEHChainView.h"
	"src/gui/Src/Gui/ScriptView.cpp"
	"src/gui/Src/Gui/ScriptView.h"
	"src/gui/Src/Gui/SelectFields.cpp"
	"src/gui/Src/Gui/SelectFields.h"
	"src/gui/Src/Gui/SelectFields.ui"
	"src/gui/Src/Gui/SettingsDialog.cpp"
	"src/gui/Src/Gui/SettingsDialog.h"
	"src/gui/Src/Gui/SettingsDialog.ui"
	"src/gui/Src/Gui/ShortcutsDialog.cpp"
	"src/gui/Src/Gui/ShortcutsDialog.h"
	"src/gui/Src/Gui/ShortcutsDialog.ui"
	"src/gui/Src/Gui/SimpleTraceDialog.cpp"
	"src/gui/Src/Gui/SimpleTraceDialog.h"
	"src/gui/Src/Gui/SimpleTraceDialog.ui"
	"src/gui/Src/Gui/SourceView.cpp"
	"src/gui/Src/Gui/SourceView.h"
	"src/gui/Src/Gui/SourceViewerManager.cpp"
	"src/gui/Src/Gui/SourceViewerManager.h"
	"src/gui/Src/Gui/StructWidget.cpp"
	"src/gui/Src/Gui/StructWidget.h"
	"src/gui/Src/Gui/SymbolView.cpp"
	"src/gui/Src/Gui/SymbolView.h"
	"src/gui/Src/Gui/SymbolView.ui"
	"src/gui/Src/Gui/SystemBreakpointScriptDialog.cpp"
	"src/gui/Src/Gui/SystemBreakpointScriptDialog.h"
	"src/gui/Src/Gui/SystemBreakpointScriptDialog.ui"
	"src/gui/Src/Gui/TabBar.cpp"
	"src/gui/Src/Gui/TabBar.h"
	"src/gui/Src/Gui/TabWidget.cpp"
	"src/gui/Src/Gui/TabWidget.h"
	"src/gui/Src/Gui/ThreadView.cpp"
	"src/gui/Src/Gui/ThreadView.h"
	"src/gui/Src/Gui/TimeWastedCounter.cpp"
	"src/gui/Src/Gui/TimeWastedCounter.h"
	"src/gui/Src/Gui/TypeWidget.cpp"
	"src/gui/Src/Gui/TypeWidget.h"
	"src/gui/Src/Gui/VirtualModDialog.cpp"
	"src/gui/Src/Gui/VirtualModDialog.h"
	"src/gui/Src/Gui/VirtualModDialog.ui"
	"src/gui/Src/Gui/WatchView.cpp"
	"src/gui/Src/Gui/WatchView.h"
	"src/gui/Src/Gui/WordEditDialog.cpp"
	"src/gui/Src/Gui/WordEditDialog.h"
	"src/gui/Src/Gui/WordEditDialog.ui"
	"src/gui/Src/Gui/XrefBrowseDialog.cpp"
	"src/gui/Src/Gui/XrefBrowseDialog.h"
	"src/gui/Src/Gui/XrefBrowseDialog.ui"
	"src/gui/Src/Gui/ZehSymbolTable.cpp"
	"src/gui/Src/Gui/ZehSymbolTable.h"
	"src/gui/Src/Imports.cpp"
	"src/gui/Src/Imports.h"
	"src/gui/Src/Memory/MemoryPage.cpp"
	"src/gui/Src/Memory/MemoryPage.h"
	"src/gui/Src/QHexEdit/ArrayCommand.cpp"
	"src/gui/Src/QHexEdit/ArrayCommand.h"
	"src/gui/Src/QHexEdit/QHexEdit.cpp"
	"src/gui/Src/QHexEdit/QHexEdit.h"
	"src/gui/Src/QHexEdit/QHexEditPrivate.cpp"
	"src/gui/Src/QHexEdit/QHexEditPrivate.h"
	"src/gui/Src/QHexEdit/XByteArray.cpp"
	"src/gui/Src/QHexEdit/XByteArray.h"
	"src/gui/Src/ThirdPartyLibs/ldconvert/ldconvert.h"
	"src/gui/Src/Tracer/TraceBrowser.cpp"
	"src/gui/Src/Tracer/TraceBrowser.h"
	"src/gui/Src/Tracer/TraceDump.cpp"
	"src/gui/Src/Tracer/TraceDump.h"
	"src/gui/Src/Tracer/TraceFileDump.cpp"
	"src/gui/Src/Tracer/TraceFileDump.h"
	"src/gui/Src/Tracer/TraceFileReader.cpp"
	"src/gui/Src/Tracer/TraceFileReader.h"
	"src/gui/Src/Tracer/TraceFileReaderInternal.h"
	"src/gui/Src/Tracer/TraceFileSearch.cpp"
	"src/gui/Src/Tracer/TraceFileSearch.h"
	"src/gui/Src/Tracer/TraceInfoBox.cpp"
	"src/gui/Src/Tracer/TraceInfoBox.h"
	"src/gui/Src/Tracer/TraceManager.cpp"
	"src/gui/Src/Tracer/TraceManager.h"
	"src/gui/Src/Tracer/TraceRegisters.cpp"
	"src/gui/Src/Tracer/TraceRegisters.h"
	"src/gui/Src/Tracer/TraceStack.cpp"
	"src/gui/Src/Tracer/TraceStack.h"
	"src/gui/Src/Tracer/TraceWidget.cpp"
	"src/gui/Src/Tracer/TraceWidget.h"
	"src/gui/Src/Tracer/TraceWidget.ui"
	"src/gui/Src/Tracer/TraceXrefBrowseDialog.cpp"
	"src/gui/Src/Tracer/TraceXrefBrowseDialog.h"
	"src/gui/Src/Tracer/TraceXrefBrowseDialog.ui"
	"src/gui/Src/Utils/ActionHelpers.h"
	"src/gui/Src/Utils/BackgroundFlickerThread.cpp"
	"src/gui/Src/Utils/BackgroundFlickerThread.h"
	"src/gui/Src/Utils/Breakpoints.cpp"
	"src/gui/Src/Utils/Breakpoints.h"
	"src/gui/Src/Utils/CachedFontMetrics.h"
	"src/gui/Src/Utils/CodeFolding.cpp"
	"src/gui/Src/Utils/CodeFolding.h"
	"src/gui/Src/Utils/CommonActions.cpp"
	"src/gui/Src/Utils/CommonActions.h"
	"src/gui/Src/Utils/Configuration.cpp"
	"src/gui/Src/Utils/Configuration.h"
	"src/gui/Src/Utils/EncodeMap.cpp"
	"src/gui/Src/Utils/EncodeMap.h"
	"src/gui/Src/Utils/FlickerThread.cpp"
	"src/gui/Src/Utils/FlickerThread.h"
	"src/gui/Src/Utils/HexValidator.cpp"
	"src/gui/Src/Utils/HexValidator.h"
	"src/gui/Src/Utils/LongLongValidator.cpp"
	"src/gui/Src/Utils/LongLongValidator.h"
	"src/gui/Src/Utils/MRUList.cpp"
	"src/gui/Src/Utils/MRUList.h"
	"src/gui/Src/Utils/MainWindowCloseThread.cpp"
	"src/gui/Src/Utils/MainWindowCloseThread.h"
	"src/gui/Src/Utils/MenuBuilder.cpp"
	"src/gui/Src/Utils/MenuBuilder.h"
	"src/gui/Src/Utils/MethodInvoker.h"
	"src/gui/Src/Utils/MiscUtil.cpp"
	"src/gui/Src/Utils/MiscUtil.h"
	"src/gui/Src/Utils/RichTextPainter.cpp"
	"src/gui/Src/Utils/RichTextPainter.h"
	"src/gui/Src/Utils/StringUtil.cpp"
	"src/gui/Src/Utils/StringUtil.h"
	"src/gui/Src/Utils/SymbolAutoCompleteModel.cpp"
	"src/gui/Src/Utils/SymbolAutoCompleteModel.h"
	"src/gui/Src/Utils/UpdateChecker.cpp"
	"src/gui/Src/Utils/UpdateChecker.h"
	"src/gui/Src/Utils/VaHistory.h"
	"src/gui/Src/Utils/ValidateExpressionThread.cpp"
	"src/gui/Src/Utils/ValidateExpressionThread.h"
	"src/gui/Src/main.cpp"
	"src/gui/Src/main.h"
	"src/gui/resource.qrc"
)

add_library(gui SHARED)

target_sources(gui PRIVATE ${gui_SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${gui_SOURCES})

target_compile_definitions(gui PRIVATE
	BUILD_LIB
	NOMINMAX
)

target_include_directories(gui PRIVATE
	src
	"src/gui/Src"
	"src/gui/Src/Gui"
	"src/gui/Src/BasicView"
	"src/gui/Src/Disassembler"
	"src/gui/Src/Memory"
	"src/gui/Src/Bridge"
	"src/gui/Src/Global"
	"src/gui/Src/Utils"
	"src/gui/Src/ThirdPartyLibs/ldconvert"
)

if(NOT TARGET zydis_wrapper)
	message(FATAL_ERROR "Target \"zydis_wrapper\" referenced by \"gui\" does not exist!")
endif()

if(NOT TARGET bridge)
	message(FATAL_ERROR "Target \"bridge\" referenced by \"gui\" does not exist!")
endif()

target_link_libraries(gui PRIVATE
	Qt5::Widgets
	Qt5::WinExtras
	zydis_wrapper
	bridge
	winmm
	wininet
)

if(CMAKE_SIZEOF_VOID_P EQUAL 4) # x86
	target_link_libraries(gui PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/src/gui/Src/ThirdPartyLibs/ldconvert/ldconvert_x86.lib"
	)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8) # x64
	target_link_libraries(gui PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/src/gui/Src/ThirdPartyLibs/ldconvert/ldconvert_x64.lib"
	)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 4) # x86
	set_target_properties(gui PROPERTIES
		OUTPUT_NAME
			x32gui
	)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8) # x64
	set_target_properties(gui PROPERTIES
		OUTPUT_NAME
			x64gui
	)
endif()

set_target_properties(gui PROPERTIES
	AUTOMOC
		ON
	AUTORCC
		ON
	AUTOUIC
		ON
)

set(CMKR_TARGET gui)
include("cmake/deps.cmake")

# Target: loaddll
set(loaddll_SOURCES
	cmake.toml
	"src/loaddll/loaddll.cpp"
)

add_executable(loaddll)

target_sources(loaddll PRIVATE ${loaddll_SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${loaddll_SOURCES})

if(CMAKE_SIZEOF_VOID_P EQUAL 4) # x86
	target_link_libraries(loaddll PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/src/dbg/ntdll/ntdll_x86.lib"
	)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8) # x64
	target_link_libraries(loaddll PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/src/dbg/ntdll/ntdll_x64.lib"
	)
endif()

if(MSVC) # msvc
	target_link_libraries(loaddll PRIVATE
		Delayimp
	)
endif()

if(MSVC) # msvc
	target_link_options(loaddll PRIVATE
		"/DELAYLOAD:user32.dll"
	)
endif()

set_target_properties(loaddll PROPERTIES
	MSVC_RUNTIME_LIBRARY
		"MultiThreaded$<$<CONFIG:Debug>:Debug>"
	WIN32_EXECUTABLE
		ON
)

get_directory_property(CMKR_VS_STARTUP_PROJECT DIRECTORY ${PROJECT_SOURCE_DIR} DEFINITION VS_STARTUP_PROJECT)
if(NOT CMKR_VS_STARTUP_PROJECT)
	set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT loaddll)
endif()

# Target: exe
set(exe_SOURCES
	cmake.toml
	"src/exe/LoadResourceString.h"
	"src/exe/crashdump.cpp"
	"src/exe/crashdump.h"
	"src/exe/icon.h"
	"src/exe/manifest.xml"
	"src/exe/resource.h"
	"src/exe/resource.rc"
	"src/exe/signaturecheck.cpp"
	"src/exe/signaturecheck.h"
	"src/exe/strings.h"
	"src/exe/strings_utf8.rc"
	"src/exe/x64dbg_exe.cpp"
)

if(CMAKE_SIZEOF_VOID_P EQUAL 4) # x86
	list(APPEND exe_SOURCES
		"src/exe/icon32.rc"
	)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8) # x64
	list(APPEND exe_SOURCES
		"src/exe/icon64.rc"
	)
endif()

add_executable(exe)

target_sources(exe PRIVATE ${exe_SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${exe_SOURCES})

if(NOT TARGET bridge)
	message(FATAL_ERROR "Target \"bridge\" referenced by \"exe\" does not exist!")
endif()

target_link_libraries(exe PRIVATE
	bridge
	Wintrust
)

if(MSVC) # msvc
	target_link_libraries(exe PRIVATE
		Delayimp
	)
endif()

if(MSVC) # msvc
	target_link_options(exe PRIVATE
		"/DEF:${CMAKE_SOURCE_DIR}/src/exe/signaturecheck.def"
		"/DELAYLOAD:user32.dll"
		"/DELAYLOAD:wintrust.dll"
		"/DELAYLOAD:$<TARGET_FILE_NAME:bridge>"
	)
endif()

set_target_properties(exe PROPERTIES
	MSVC_RUNTIME_LIBRARY
		"MultiThreaded$<$<CONFIG:Debug>:Debug>"
	WIN32_EXECUTABLE
		ON
)

if(CMAKE_SIZEOF_VOID_P EQUAL 4) # x86
	set_target_properties(exe PROPERTIES
		OUTPUT_NAME
			x32dbg
		PDB_NAME
			x32dbg_exe
	)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8) # x64
	set_target_properties(exe PROPERTIES
		OUTPUT_NAME
			x64dbg
		PDB_NAME
			x64dbg_exe
	)
endif()

get_directory_property(CMKR_VS_STARTUP_PROJECT DIRECTORY ${PROJECT_SOURCE_DIR} DEFINITION VS_STARTUP_PROJECT)
if(NOT CMKR_VS_STARTUP_PROJECT)
	set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT exe)
endif()

set(CMKR_TARGET exe)
# Make the executable the main target
add_dependencies(exe dbg gui loaddll deps)

# Target: headless
set(headless_SOURCES
	cmake.toml
	"src/headless/headless.cpp"
	"src/headless/signaturecheck.cpp"
	"src/headless/stringutils.cpp"
	"src/headless/stringutils.h"
	"src/headless/tostring.h"
)

add_executable(headless)

target_sources(headless PRIVATE ${headless_SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${headless_SOURCES})

if(NOT TARGET bridge)
	message(FATAL_ERROR "Target \"bridge\" referenced by \"headless\" does not exist!")
endif()

target_link_libraries(headless PRIVATE
	bridge
)

if(MSVC) # msvc
	target_link_options(headless PRIVATE
		"/DEF:${CMAKE_SOURCE_DIR}/src/exe/signaturecheck.def"
	)
endif()

get_directory_property(CMKR_VS_STARTUP_PROJECT DIRECTORY ${PROJECT_SOURCE_DIR} DEFINITION VS_STARTUP_PROJECT)
if(NOT CMKR_VS_STARTUP_PROJECT)
	set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT headless)
endif()

set(CMKR_TARGET headless)
add_dependencies(headless exe)

# Target: launcher
if(CMAKE_SIZEOF_VOID_P EQUAL 4) # x86
	set(launcher_SOURCES
		cmake.toml
		"src/exe/icon.rc"
		"src/exe/resource.rc"
		"src/exe/strings_utf8.rc"
		"src/launcher/x64dbg_launcher.cpp"
	)

	add_executable(launcher)

	target_sources(launcher PRIVATE ${launcher_SOURCES})
	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${launcher_SOURCES})

	target_link_libraries(launcher PRIVATE
		Shlwapi
		Comctl32
	)

	set_target_properties(launcher PROPERTIES
		MSVC_RUNTIME_LIBRARY
			"MultiThreaded$<$<CONFIG:Debug>:Debug>"
		OUTPUT_NAME
			x96dbg
		WIN32_EXECUTABLE
			ON
	)

	if(X64DBG_BUILD_IN_TREE) # X64DBG_BUILD_IN_TREE
		set_target_properties(launcher PROPERTIES
			RUNTIME_OUTPUT_DIRECTORY
				"${PROJECT_SOURCE_DIR}/bin"
		)
	endif()

	get_directory_property(CMKR_VS_STARTUP_PROJECT DIRECTORY ${PROJECT_SOURCE_DIR} DEFINITION VS_STARTUP_PROJECT)
	if(NOT CMKR_VS_STARTUP_PROJECT)
		set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT launcher)
	endif()

	set(CMKR_TARGET launcher)
	add_dependencies(exe launcher)

endif()
