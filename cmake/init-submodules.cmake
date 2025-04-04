function(init_submodule folder)
    set(full_path "${CMAKE_CURRENT_SOURCE_DIR}/${folder}")
    if(NOT EXISTS ${full_path})
        message(FATAL_ERROR "Submodule folder does not exist: ${full_path}")
    endif()
    file(GLOB files "${full_path}/*")
	if(NOT files)
        find_package(Git REQUIRED)
		message(STATUS "Submodule '${folder}' not initialized, running git...")
		execute_process(
			COMMAND "${GIT_EXECUTABLE}" rev-parse --show-toplevel
			WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
			OUTPUT_VARIABLE git_root
			OUTPUT_STRIP_TRAILING_WHITESPACE
			COMMAND_ERROR_IS_FATAL ANY
		)
		execute_process(
			COMMAND "${GIT_EXECUTABLE}" submodule update --init -- "${full_path}"
			WORKING_DIRECTORY "${git_root}"
			COMMAND_ERROR_IS_FATAL ANY
		)
	endif()
endfunction()

init_submodule(src/dbg/btparser)
init_submodule(deps)
