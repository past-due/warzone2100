if(NOT CMAKE_SCRIPT_MODE_FILE)
 message(FATAL_ERROR "This script currently only supports being run via `cmake -P` script mode")
endif()
set(_fullPathToThisScript "${CMAKE_SCRIPT_MODE_FILE}")
get_filename_component(_scriptFolder "${_fullPathToThisScript}" DIRECTORY)

if((NOT DEFINED SOURCE_DIR) OR (SOURCE_DIR STREQUAL ""))
 message(FATAL_ERROR "SOURCE_DIR must be specified on command-line")
endif()

# Fix sentry_sync.h x86 compile on llvm-mingw
set(_sentry_sync_h_path "${SOURCE_DIR}/src/sentry_sync.h")
set(_sentry_sync_find_text "#if defined(__MINGW32__) && !defined(__MINGW64__)")
set(_sentry_sync_replace_text "#if defined(__MINGW32__) && !defined(__MINGW64__) && !defined(__clang__)")
file(READ "${_sentry_sync_h_path}" FILE_CONTENTS)
string(REPLACE "${_sentry_sync_find_text}" "${_sentry_sync_replace_text}" FILE_CONTENTS "${FILE_CONTENTS}")
file(WRITE "${_sentry_sync_h_path}" "${FILE_CONTENTS}")

##################################################
# Derived from vcpkg_apply_patches at:
# https://github.com/microsoft/vcpkg/blob/master/scripts/cmake/vcpkg_apply_patches.cmake
##################################################
function(sentry_apply_patches)
    # parse parameters such that semicolons in options arguments to COMMAND don't get erased
    cmake_parse_arguments(PARSE_ARGV 0 _ap "QUIET" "SOURCE_PATH" "PATCHES")

    find_package(Git REQUIRED)
    set(PATCHNUM 0)
    foreach(PATCH ${_ap_PATCHES})
        get_filename_component(ABSOLUTE_PATCH "${PATCH}" ABSOLUTE BASE_DIR "${_directoryOfThisScript}")
        message(STATUS "Applying patch ${PATCH}")
        set(LOGNAME patch-${TARGET_TRIPLET}-${PATCHNUM})
        execute_process(
            COMMAND ${GIT_EXECUTABLE} --work-tree=. apply "${ABSOLUTE_PATCH}" --ignore-whitespace --whitespace=nowarn --verbose
            OUTPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/${LOGNAME}-out.log
            ERROR_VARIABLE error
            WORKING_DIRECTORY ${_ap_SOURCE_PATH}
            RESULT_VARIABLE error_code
        )
        file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/${LOGNAME}-err.log" "${error}")

        if(error_code)
			if(NOT _ap_QUIET)
				message(FATAL_ERROR "Applying patch failed. ${error}")
			else()
				message(WARNING "Applying patch failed - may have already been applied, or may require updating for a new release of sentry-native. See: ${CMAKE_CURRENT_SOURCE_DIR}/${LOGNAME}-err.log")
			endif()
        endif()

        math(EXPR PATCHNUM "${PATCHNUM}+1")
    endforeach()
endfunction()

# Patch compat/mingw/dbghelp.h
sentry_apply_patches(
	QUIET
	SOURCE_PATH "${SOURCE_DIR}/external/crashpad"
	PATCHES
		"${_scriptFolder}/crashpad/dbghelp.h.patch"
)
