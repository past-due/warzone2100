###############################################
# WZ core / default exception handler (writes local logs & dumps)
file(GLOB HEADERS "*.h")
set(SRC "dumpinfo.cpp" "exceptionhandler.cpp")

add_library(exception-handler STATIC ${HEADERS} ${SRC})
set_property(TARGET exception-handler PROPERTY FOLDER "lib")
include(WZTargetConfiguration)
WZ_TARGET_CONFIGURATION(exception-handler)

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
	if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU" AND ${CMAKE_CROSSCOMPILING})
		# Cross-compiling for Windows with MINGW
		target_sources(exception-handler PRIVATE "exchndl_mingw.cpp")
		target_link_libraries(exception-handler PRIVATE bfd iberty)
	else()
		# Compiling with MSVC (etc) on Windows
		target_sources(exception-handler PRIVATE "3rdparty/StackWalker.cpp" "exchndl_win.cpp")
		target_link_libraries(exception-handler PRIVATE dbghelp version shlwapi)
		if (MINGW AND NOT MSVC)
			# Disable some warnings
			set_source_files_properties("3rdparty/StackWalker.cpp" PROPERTIES COMPILE_FLAGS "-Wno-unknown-pragmas -Wno-switch")
			# Define _MSC_VER
			set_source_files_properties("3rdparty/StackWalker.cpp" PROPERTIES COMPILE_DEFINITIONS "_MSC_VER=1900")
		endif()
		set_property(SOURCE "3rdparty/StackWalker.h" "3rdparty/StackWalker.cpp" APPEND PROPERTY COMPILE_DEFINITIONS "WIN32_LEAN_AND_MEAN" "NOCRYPT" "NOMINMAX" "WIN32_EXTRA_LEAN")
	endif()
endif()

target_link_libraries(exception-handler PRIVATE framework launchinfo)

###############################################
# WZ Sentry.io crash handling provider support
# To build, define CMAKE variable SENTRY_IO_DSN

if(DEFINED SENTRY_IO_DSN AND NOT SENTRY_IO_DSN STREQUAL "")
	include(FetchContent)
	FetchContent_Declare(
		sentrynative
		URL "https://github.com/getsentry/sentry-native/releases/download/0.4.11/sentry-native.zip"
		URL_HASH SHA512=3d66295526c0cd068a0b7c2180dbeae7594775b3eb3199a74da7efe634cbc19cff6eba8d44b1479037c43c6eb58a9ba4fdaf8da6995767f27c585a0d42582c37
		PATCH_COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/sentry/CMakeLists.txt" "<SOURCE_DIR>/CMakeLists.txt"
	)
	FetchContent_GetProperties(sentrynative)
	set(SENTRY_BUILD_SHARED_LIBS OFF CACHE BOOL "Sentry build shared libs" FORCE)
	set(SENTRY_EXPORT_SYMBOLS OFF CACHE BOOL "Export symbols" FORCE)
	set(SENTRY_BUILD_RUNTIMESTATIC OFF CACHE BOOL "Build sentry-native with static runtime" FORCE)
	if(CMAKE_SYSTEM_NAME MATCHES "Darwin|Linux")
		set(SENTRY_BACKEND "breakpad" CACHE STRING
  "The sentry backend responsible for reporting crashes, can be either 'none', 'inproc', 'breakpad' or 'crashpad'." FORCE)
	endif()
	if(NOT sentrynative_POPULATED)
		FetchContent_Populate(sentrynative)
		add_subdirectory("${sentrynative_SOURCE_DIR}" "${sentrynative_BINARY_DIR}")
	endif()
	message(STATUS "Enabling crash-handling backend: sentry-native ($CACHE{SENTRY_BACKEND})")

	include(CheckCXXCompilerFlag)
	if(CACHE{SENTRY_BACKEND} STREQUAL "crashpad")
		set_target_properties(crashpad_client PROPERTIES CXX_EXTENSIONS ON)
		set_target_properties(crashpad_compat PROPERTIES CXX_EXTENSIONS ON)
		set_target_properties(crashpad_getopt PROPERTIES CXX_EXTENSIONS ON)
		set_target_properties(crashpad_handler PROPERTIES CXX_EXTENSIONS ON)
		set_target_properties(crashpad_handler_lib PROPERTIES CXX_EXTENSIONS ON)
		set_target_properties(crashpad_minidump PROPERTIES CXX_EXTENSIONS ON)
		set_target_properties(crashpad_snapshot PROPERTIES CXX_EXTENSIONS ON)
		set_target_properties(crashpad_snapshot PROPERTIES CXX_EXTENSIONS ON)
		set_target_properties(crashpad_tools PROPERTIES CXX_EXTENSIONS ON)
		set_target_properties(crashpad_util PROPERTIES CXX_EXTENSIONS ON)
		set_target_properties(crashpad_zlib PROPERTIES CXX_EXTENSIONS ON)
		set_target_properties(mini_chromium PROPERTIES CXX_EXTENSIONS ON)

		CHECK_CXX_COMPILER_FLAG(-Wno-pedantic COMPILER_SUPPORTS_WNO_PEDANTIC)
		if(COMPILER_SUPPORTS_WNO_PEDANTIC)
			target_compile_options(crashpad_client PRIVATE -Wno-pedantic)
			target_compile_options(crashpad_compat PRIVATE -Wno-pedantic)
			target_compile_options(crashpad_getopt PRIVATE -Wno-pedantic)
			target_compile_options(crashpad_handler PRIVATE -Wno-pedantic)
			target_compile_options(crashpad_handler_lib PRIVATE -Wno-pedantic)
			target_compile_options(crashpad_minidump PRIVATE -Wno-pedantic)
			target_compile_options(crashpad_snapshot PRIVATE -Wno-pedantic)
			target_compile_options(crashpad_snapshot PRIVATE -Wno-pedantic)
			target_compile_options(crashpad_tools PRIVATE -Wno-pedantic)
			target_compile_options(crashpad_util PRIVATE -Wno-pedantic)
			target_compile_options(crashpad_zlib PRIVATE -Wno-pedantic)
			target_compile_options(mini_chromium PRIVATE -Wno-pedantic)
		endif()
	endif()

	# Piggy-back on the exception-handler target to get these libraries / defines over to the main target # TODO: Refactor later?
	target_link_libraries(exception-handler INTERFACE sentry)
	target_compile_definitions(exception-handler INTERFACE WZ_CRASHHANDLING_PROVIDER_SENTRY)
	target_compile_definitions(exception-handler INTERFACE SENTRY_BUILD_STATIC=1)
	file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/wz-sentry-config")
	set(WZ_CRASHHANDLING_PROVIDER_SENTRY_DSN "${SENTRY_IO_DSN}")
	configure_file("3rdparty/sentry/wz-sentry-config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/wz-sentry-config/wz-sentry-config.h" @ONLY)
	target_include_directories(exception-handler INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/wz-sentry-config")
endif()
