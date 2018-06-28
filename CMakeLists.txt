cmake_minimum_required(VERSION 3.5)
project(wz2100)

OPTION(ENABLE_NLS "Native Language Support" ON)
OPTION(WZ_PORTABLE "Portable (Windows-only)" ON)
OPTION(ENABLE_WARNINGS "Enable warnings" ON)

if(WZ_PORTABLE AND NOT CMAKE_SYSTEM_NAME MATCHES "Windows")
	message( WARNING "Portable build is only supported on Windows; Ignoring WZ_PORTABLE option" )
	unset(WZ_PORTABLE CACHE)
endif()

if(ENABLE_WARNINGS)
	set(WZ_Wno_ "-Wno-error=")
else()
	set(WZ_Wno_ "-Wno-")
endif()

if(MSVC)
	set(CMAKE_CXX_STANDARD 14)
else()
	set(CMAKE_CXX_STANDARD 11)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

find_package(Qt5 COMPONENTS Core Widgets Script Gui REQUIRED)

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
  find_package(PkgConfig)
  if(${CMAKE_CROSSCOMPILING})
    pkg_check_modules(QT5ALL REQUIRED Qt5Widgets Qt5Core Qt5Script)
    link_directories(${QT5ALL_LIBRARY_DIRS})
  endif()
endif()

# Use "-fPIC" / "-fPIE" for all targets by default, including static libs
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# CMake doesn't add "-pie" by default for executables (CMake issue #14983)
INCLUDE(AddTargetLinkFlagsIfSupported)
CHECK_CXX_LINKER_FLAGS("${CMAKE_EXE_LINKER_FLAGS} -pie" LINK_FLAG_PIE_SUPPORTED)
if(LINK_FLAG_PIE_SUPPORTED AND NOT CMAKE_SYSTEM_NAME MATCHES "Windows")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie")
endif()

# Ensure all builds always have debug info built (MSVC)
if("${CMAKE_CXX_COMPILER_ID}" MATCHES "MSVC")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF" CACHE STRING "Flags used by the linker (Release builds)" FORCE)
endif()

include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)

# Configure compiler warnings
if(MSVC)
	# MSVC

	# TODO: Enable /W4
	# NOTE: Do not use /Wall, as it enables *many* warnings that are off by default. MS recommends /W4 (max)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3")

	# Enable some additional MSVC warnings
	# C4701: Potentially uninitialized local variable 'name' used
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /w34701")

	# C4702: unreachable code
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /w34702")

	# Disable some warnings for WZ (for now)
	# C4456: declaration of 'identifier' hides previous local declaration
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4456")
	# C4459: declaration of 'identifier' hides global declaration
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4459")

	# Qt headers (at least 5.6.x) necessitate disabling some warnings
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4127")
else()
	# GCC, Clang, etc

	# Enable -Wall (if supported)
	check_c_compiler_flag(-Wall HAS_CFLAG_W_ALL)
	if (HAS_CFLAG_W_ALL)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
	endif()
	check_cxx_compiler_flag(-Wall HAS_CXXFLAG_W_ALL)
	if (HAS_CXXFLAG_W_ALL)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
	endif()

	# Enable -Werror (if supported)
	check_c_compiler_flag(-Werror HAS_CFLAG_W_ERROR)
	if (HAS_CFLAG_W_ERROR)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
	endif()
	check_cxx_compiler_flag(-Werror HAS_CXXFLAG_W_ERROR)
	if (HAS_CXXFLAG_W_ERROR)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
	endif()

	# Enable -Wcast-align (if supported)
	check_c_compiler_flag(-Wcast-align HAS_CFLAG_W_CAST_ALIGN)
	if (HAS_CFLAG_W_CAST_ALIGN)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wcast-align")
	endif()
	check_cxx_compiler_flag(-Wcast-align HAS_CXXFLAG_CAST_ALIGN)
	if (HAS_CXXFLAG_CAST_ALIGN)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wcast-align")
	endif()

	# Enable -Wwrite-strings (if supported)
	check_c_compiler_flag(-Wwrite-strings HAS_CFLAG_W_WRITE_STRINGS)
	if (HAS_CFLAG_W_WRITE_STRINGS)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wwrite-strings")
	endif()
	check_cxx_compiler_flag(-Wwrite-strings HAS_CXXFLAG_WRITE_STRINGS)
	if (HAS_CXXFLAG_WRITE_STRINGS)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wwrite-strings")
	endif()

	# Enable -Wpointer-arith (if supported)
	check_c_compiler_flag(-Wpointer-arith HAS_CFLAG_W_POINTER_ARITH)
	if (HAS_CFLAG_W_POINTER_ARITH)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wpointer-arith")
	endif()
	check_cxx_compiler_flag(-Wpointer-arith HAS_CXXFLAG_POINTER_ARITH)
	if (HAS_CXXFLAG_POINTER_ARITH)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wpointer-arith")
	endif()

	# Enable -Wstrict-prototypes for C (if supported)
	check_c_compiler_flag(-Wstrict-prototypes HAS_CFLAG_W_STRICT_PROTOTYPES)
	if (HAS_CFLAG_W_STRICT_PROTOTYPES)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wstrict-prototypes")
	endif()

	# Handle "sign-compare" (should be a warning at most - never an error)
	check_c_compiler_flag("-Werror ${WZ_Wno_}sign-compare -Wno-error=cpp" HAS_CFLAG_W_SIGN_COMPARE)
	if (HAS_CFLAG_W_SIGN_COMPARE)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WZ_Wno_}sign-compare")
	endif()
	check_cxx_compiler_flag("-Werror ${WZ_Wno_}sign-compare -Wno-error=cpp" HAS_CXXFLAG_W_SIGN_COMPARE)
	if (HAS_CXXFLAG_W_SIGN_COMPARE)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WZ_Wno_}sign-compare")
	endif()

	# Handle "format-security" (should be a warning at most - never an error)
	check_c_compiler_flag("-Werror ${WZ_Wno_}format-security -Wno-error=cpp" HAS_CFLAG_W_FORMAT_SECURITY)
	if (HAS_CFLAG_W_FORMAT_SECURITY)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WZ_Wno_}format-security")
	endif()
	check_cxx_compiler_flag("-Werror ${WZ_Wno_}format-security -Wno-error=cpp" HAS_CXXFLAG_W_FORMAT_SECURITY)
	if (HAS_CXXFLAG_W_FORMAT_SECURITY)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WZ_Wno_}format-security")
	endif()

	# Enable -fno-omit-frame-pointer (if supported)
	#check_c_compiler_flag(-fno-omit-frame-pointer HAS_CFLAG_FNO_OMIT_FRAME_POINTER)
	#if (HAS_CFLAG_FNO_OMIT_FRAME_POINTER)
	#	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-omit-frame-pointer")
	#endif()
	#check_cxx_compiler_flag(-fno-omit-frame-pointer HAS_CXXFLAG_FNO_OMIT_FRAME_POINTER)
	#if (HAS_CXXFLAG_FNO_OMIT_FRAME_POINTER)
	#	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer")
	#endif()

	# Enable stack protection, if supported by the compiler
	# Prefer -fstack-protector-strong if supported, fall-back to -fstack-protector
	check_c_compiler_flag(-fstack-protector-strong HAS_CFLAG_FSTACK_PROTECTOR_STRONG)
	if (HAS_CFLAG_FSTACK_PROTECTOR_STRONG)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector-strong")
	else()
		check_c_compiler_flag(-fstack-protector HAS_CFLAG_FSTACK_PROTECTOR)
		if (HAS_CFLAG_FSTACK_PROTECTOR)
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector")
		endif()
	endif()
	check_cxx_compiler_flag(-fstack-protector-strong HAS_CXXFLAG_FSTACK_PROTECTOR_STRONG)
	if (HAS_CXXFLAG_FSTACK_PROTECTOR_STRONG)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstack-protector-strong")
	else()
		check_cxx_compiler_flag(-fstack-protector HAS_CXXFLAG_FSTACK_PROTECTOR)
		if (HAS_CXXFLAG_FSTACK_PROTECTOR)
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstack-protector")
		endif()
	endif()

endif()

add_definitions("-DHAVE_CONFIG_H")

# CMAKE_CURRENT_BINARY_DIR should come before the current source directory
# so that any build products are preferentially included over in-source build
# products that might have been generated by a different compilation method / run
include_directories("${CMAKE_CURRENT_BINARY_DIR}")
include_directories(".")
include_directories("3rdparty")
include_directories("3rdparty/glm")
include_directories("3rdparty/miniupnp")

SET(STDC_HEADERS ON)
SET(HAVE_CFPREFERENCESCOPYAPPVALUE OFF)
SET(_MINIX OFF)
SET(_POSIX_1_SOURCE OFF)
SET(_POSIX_SOURCE OFF)
SET(_XOPEN_SOURCE)
SET(PACKAGE "warzone2100")
SET(PACKAGE_BUGREPORT "http://wz2100.net/")
SET(PACKAGE_NAME "Warzone 2100")
SET(PACKAGE_TARNAME "warzone2100")
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
  SET(_GNU_SOURCE ON)
endif()

INCLUDE (CheckIncludeFiles)
CHECK_INCLUDE_FILES(alloca.h HAVE_ALLOCA_H)
CHECK_INCLUDE_FILES(inttypes.h HAVE_INTTYPES_H)
CHECK_INCLUDE_FILES(memory.h HAVE_MEMORY_H)
CHECK_INCLUDE_FILES(stdint.h HAVE_STDINT_H)
CHECK_INCLUDE_FILES(stdlib.h HAVE_STDLIB_H)
CHECK_INCLUDE_FILES(strings.h HAVE_STRINGS_H)
CHECK_INCLUDE_FILES(string.h HAVE_STRING_H)
CHECK_INCLUDE_FILES("sys/stat.h" HAVE_SYS_STAT_H)
CHECK_INCLUDE_FILES("sys/types.h" HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILES("sys/ucontext.h" HAVE_SYS_UCONTEXT_H)
CHECK_INCLUDE_FILES(unistd.h HAVE_UNISTD_H)

INCLUDE (CheckFunctionExists)
CHECK_FUNCTION_EXISTS(gettext HAVE_GETTEXT)
CHECK_FUNCTION_EXISTS(iconv HAVE_ICONV)
CHECK_FUNCTION_EXISTS(strlcat HAVE_SYSTEM_STRLCAT)
CHECK_FUNCTION_EXISTS(strlcpy HAVE_SYSTEM_STRLCPY)
CHECK_FUNCTION_EXISTS(strlcat HAVE_VALID_STRLCAT)
CHECK_FUNCTION_EXISTS(strlcpy HAVE_VALID_STRLCPY)
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/src/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

SET(UPNPC_BUILD_STATIC ON CACHE BOOL "miniupnpc - Build static library" FORCE)
SET(UPNPC_BUILD_SHARED OFF CACHE BOOL "miniupnpc - Build shared library" FORCE)
SET(UPNPC_BUILD_TESTS OFF CACHE BOOL "miniupnpc - Build tests" FORCE)
SET(UPNPC_BUILD_SAMPLE OFF CACHE BOOL "miniupnpc - Build samples" FORCE)

add_subdirectory(3rdparty/miniupnp/miniupnpc)
add_subdirectory(3rdparty/micro-ecc)
add_subdirectory(3rdparty/sha2)
add_subdirectory(3rdparty/fonts)
add_subdirectory(3rdparty/utf8proc)
add_subdirectory(lib)
add_subdirectory(src)
add_subdirectory(po)
add_subdirectory(data)
add_subdirectory(doc)
add_subdirectory(icons)
add_subdirectory(pkg)

# Install base text / info files
set(ROOT_FILES ChangeLog AUTHORS COPYING.NONGPL COPYING COPYING.README README.md)
if(CMAKE_SYSTEM_NAME MATCHES "Windows")
	# Target system is Windows
	# Must convert the ROOT_FILES to Windows line endings, and rename with ".txt" at the end
	set(_new_ROOT_FILES)
	foreach(rfile ${ROOT_FILES})
		get_filename_component(_rfile_filename ${rfile} NAME)
		# Read in the file
		file(READ ${rfile} _contents)
		# Strip all CRs
		string(REPLACE "\r" "" _contents ${_contents})
		# Convert all LFs to CRLFs
		string(REPLACE "\n" "\r\n" _contents ${_contents})
		# Write out the converted file
		set(_rfile_newfilename "${CMAKE_CURRENT_BINARY_DIR}/${_rfile_filename}.txt")
		file(WRITE "${_rfile_newfilename}" ${_contents})
		list(APPEND _new_ROOT_FILES "${_rfile_newfilename}")
	endforeach()
	set(ROOT_FILES ${_new_ROOT_FILES})
else()
	# Just copy the files to the build directory
	foreach(rfile ${ROOT_FILES})
		get_filename_component(_rfile_filename ${rfile} NAME)
		configure_file(${rfile} "${CMAKE_CURRENT_BINARY_DIR}/${_rfile_filename}" COPYONLY)
	endforeach()
endif()
foreach(rfile ${ROOT_FILES})
	install(FILES ${rfile}
			COMPONENT Core
			DESTINATION "."
	)
endforeach()

# Add "dist" target alias (using CPack package_source)
get_filename_component(_cmake_path ${CMAKE_COMMAND} PATH)
find_program(CPACK_COMMAND cpack ${_cmake_path})
unset(_cmake_path)
if(CPACK_COMMAND)
	add_custom_target(dist
		COMMAND ${CPACK_COMMAND} --config ${CMAKE_CURRENT_BINARY_DIR}/CPackSourceConfig.cmake
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	)
endif()
