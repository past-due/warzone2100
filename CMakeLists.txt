cmake_minimum_required(VERSION 3.5)
project(wz2100)

OPTION(ENABLE_NLS "Native Language Support" ON)
OPTION(WZ_PORTABLE "Portable (Windows-only)" ON)
OPTION(ENABLE_WARNINGS "Enable warnings" ON)
OPTION(WZ_ENABLE_WARNINGS_AS_ERRORS "Enable compiler flags that treat (most) warnings as errors" ON)

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

# CHECK_COMPILER_FLAGS_OUTPUT(	<compiler_flags>
#								COMPILER_TYPE <C | CXX>
#								[OUTPUT_FLAGS <output_compiler_flags>]
#								OUTPUT_VARIABLE <output_variable>
#								[APPEND]
#								[FORCE]
#								[QUIET <ALL | FAILURES | OFF> = FAILURES] )
#
function(CHECK_COMPILER_FLAGS_OUTPUT _compiler_flags)
	set(_options APPEND FORCE)
	set(_oneValueArgs COMPILER_TYPE OUTPUT_FLAGS OUTPUT_VARIABLE QUIET)
	set(_multiValueArgs)

	CMAKE_PARSE_ARGUMENTS(_parsedArguments "${_options}" "${_oneValueArgs}" "${_multiValueArgs}" ${ARGN})

	if(NOT DEFINED _compiler_flags)
		message( FATAL_ERROR "CHECK_COMPILER_FLAGS_OUTPUT requires compiler flags (first parameter)" )
	endif()

	if(DEFINED _parsedArguments_QUIET)
		if(NOT _parsedArguments_QUIET MATCHES "^(ALL|FAILURES|OFF)$")
			message( FATAL_ERROR "CHECK_COMPILER_FLAGS_OUTPUT invalid QUIET value - must be one of: (ALL, FAILURES, OFF)" )
		endif()
	else()
		set(_parsedArguments_QUIET "FAILURES")
	endif()

	if(NOT DEFINED _parsedArguments_COMPILER_TYPE)
		message( FATAL_ERROR "CHECK_COMPILER_FLAGS_OUTPUT missing required COMPILER_TYPE value - must be one of: (C, CXX)" )
	else()
		if(NOT _parsedArguments_COMPILER_TYPE MATCHES "^(C|CXX)$")
			message( FATAL_ERROR "CHECK_COMPILER_FLAGS_OUTPUT invalid COMPILER_TYPE value - must be one of: (C, CXX)" )
		endif()
	endif()

	if(NOT DEFINED _parsedArguments_OUTPUT_VARIABLE)
		message( FATAL_ERROR "CHECK_COMPILER_FLAGS_OUTPUT missing required OUTPUT_VARIABLE" )
	endif()

	if(DEFINED _parsedArguments_OUTPUT_FLAGS)
		set(_compiler_flags_output "${_parsedArguments_OUTPUT_FLAGS}")
	else()
		set(_compiler_flags_output "${_compiler_flags}")
	endif()

	set(_tmp_check_result)
	CHECK_COMPILER_FLAGS("${_compiler_flags}" COMPILER_TYPE "${_parsedArguments_COMPILER_TYPE}" RESULT_VARIABLE _tmp_check_result QUIET ALL)

	set(_compiler_type "${_parsedArguments_COMPILER_TYPE}")
	if(_tmp_check_result)
		if(NOT _parsedArguments_QUIET OR _parsedArguments_QUIET MATCHES "FAILURES")
			message( STATUS "Supports COMPILER_FLAG [${_compiler_type}]: ${_compiler_flags_output} ... YES${_check_cached_status}" )
		endif()
		if(_parsedArguments_APPEND)
			set(${_parsedArguments_OUTPUT_VARIABLE} "${${_parsedArguments_OUTPUT_VARIABLE}} ${_compiler_flags_output}" PARENT_SCOPE)
		else()
			set(${_parsedArguments_OUTPUT_VARIABLE} "${_compiler_flags_output}" PARENT_SCOPE)
		endif()
	else()
		if(NOT _parsedArguments_QUIET)
			message( STATUS "Supports COMPILER_FLAG [${_compiler_type}]: ${_compiler_flags_output} ... no${_check_cached_status}" )
		endif()
	endif()
endfunction()

function(CHECK_COMPILER_FLAGS _compiler_flags)
	set(_options FORCE)
	set(_oneValueArgs COMPILER_TYPE RESULT_VARIABLE QUIET)
	set(_multiValueArgs )

	CMAKE_PARSE_ARGUMENTS(_parsedArguments "${_options}" "${_oneValueArgs}" "${_multiValueArgs}" ${ARGN})

	if(NOT DEFINED _compiler_flags)
		message( FATAL_ERROR "CHECK_COMPILER_FLAGS requires compiler flags (first parameter)" )
	endif()

	if(DEFINED _parsedArguments_QUIET)
		if(NOT _parsedArguments_QUIET MATCHES "^(ALL|FAILURES|OFF)$")
			message( FATAL_ERROR "CHECK_COMPILER_FLAGS invalid QUIET value - must be one of: (ALL, FAILURES, OFF)" )
		endif()
	else()
		set(_parsedArguments_QUIET "FAILURES")
	endif()

	if(NOT DEFINED _parsedArguments_COMPILER_TYPE)
		message( FATAL_ERROR "CHECK_COMPILER_FLAGS missing required COMPILER_TYPE value - must be one of: (C, CXX)" )
	else()
		if(NOT _parsedArguments_COMPILER_TYPE MATCHES "^(C|CXX)$")
			message( FATAL_ERROR "CHECK_COMPILER_FLAGS invalid COMPILER_TYPE value - must be one of: (C, CXX)" )
		endif()
	endif()

	if(NOT DEFINED _parsedArguments_RESULT_VARIABLE)
		message( FATAL_ERROR "CHECK_COMPILER_FLAGS missing required RESULT_VARIABLE" )
	endif()

	# Create a sanitized version of the input compiler flags string that can be used in a variable name
	string(REGEX REPLACE "[^a-zA-Z0-9\_]" "_" _cached_result_variable_flag_desc "${_compiler_flags}")

	set(_compiler_type "${_parsedArguments_COMPILER_TYPE}")
	if (NOT _parsedArguments_FORCE)
		set(_cached_result_variable_name "${_compiler_type}_COMPILER_FLAG_SUPPORTED_${_cached_result_variable_flag_desc}")
		if(DEFINED ${_cached_result_variable_name})
			set(_check_cached_status " (cached)")
		endif()
	else()
		# Generate a temporary variable name that isn't in the cache
		while(NOT _cached_result_variable_name)
			string(RANDOM LENGTH 12 _tmpRandom)
			set(_tmpRandom "_NOCACHE_tmp_rnd_${_tmpRandom}")
			if(NOT DEFINED "${_tmpRandom}")
				set(_cached_result_variable_name "${_tmpRandom}")
			endif()
		endwhile()
	endif()

	if(_compiler_type MATCHES "CXX")
		CHECK_CXX_COMPILER_FLAG("${_compiler_flags}" ${_cached_result_variable_name})
	elseif(_compiler_type MATCHES "C")
		CHECK_C_COMPILER_FLAG("${_compiler_flags}" ${_cached_result_variable_name})
	else()
		message( FATAL_ERROR "CHECK_COMPILER_FLAGS invalid COMPILER_TYPE value - must be one of: (C, CXX)" )
	endif()

	if(${_cached_result_variable_name})
		if(NOT _parsedArguments_QUIET OR _parsedArguments_QUIET MATCHES "FAILURES")
			message( STATUS "Supports COMPILER_FLAG [${_compiler_type}]: ${_compiler_flags} ... YES${_check_cached_status}" )
		endif()
		set(${_parsedArguments_RESULT_VARIABLE} ON PARENT_SCOPE)
	else()
		if(NOT _parsedArguments_QUIET)
			message( STATUS "Supports COMPILER_FLAG [${_compiler_type}]: ${_compiler_flags} ... no${_check_cached_status}" )
		endif()
		set(${_parsedArguments_RESULT_VARIABLE} OFF PARENT_SCOPE)
	endif()

	if (_parsedArguments_FORCE)
		# Unset the temporary cached variable that CHECK_<LANG>_COMPILER_FLAG populated
		unset(_cached_result_variable_name CACHE)
	endif()
endfunction()


# Configure compiler warnings for WZ
# NOTE: This should be called after the 3rdparty/ libs are added, so it only affects WZ code & frameworks
macro(CONFIGURE_WZ_COMPILER_WARNINGS)
	if(MSVC)
		# MSVC

		# TODO: Enable /W4
		# NOTE: Do not use /Wall, as it enables *many* warnings that are off by default. MS recommends /W4 (max)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")

		# Enable some additional MSVC warnings (if compiling at warning level 3)
		# C4204: nonstandard extension used : non-constant aggregate initializer
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /w34204")
		# C4512: 'class' : assignment operator could not be generated
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /w34512")

		# Enable some additional MSVC warnings (that are off by default)
		# C4191: unsafe conversion from 'type of expression' to 'type required'
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /w34191")
		# C4263: 'function': member function does not override any base class virtual member function
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /w34263")
		# C4264: 'virtual_function': no override available for virtual member function from base 'class'; function is hidden
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /w34264")
		# C4265: 'class': class has virtual functions, but destructor is not virtual
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /w34265")
		# C4266: 'function' : no override available for virtual member function from base 'type'; function is hidden
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /w34266")
		# C4905: wide string literal cast to 'LPSTR'
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /w14905")
		# C4906: string literal cast to 'LPWSTR'
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /w14906")

		# Disable some warnings for WZ (permanently)
		# C4800: 'type' : forcing value to bool 'true' or 'false' (performance warning)
		# This warning is no longer generated in Visual Studio 2017+
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4800")

		# Disable some warnings for WZ (FIXME)
		# C4100: unreferenced formal parameter
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4100")
		# C4456: declaration of 'identifier' hides previous local declaration
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4456")
		# C4459: declaration of 'identifier' hides global declaration
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4459")
		# C4702: unreachable code
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4702")
		# C4245: 'conversion' : conversion from 'type1' to 'type2', signed/unsigned mismatch
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4245")
		# C4701: Potentially uninitialized local variable 'name' used
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4701")

		# Qt headers (at least 5.6.x) necessitate disabling some warnings
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4127")
	else()
		# GCC, Clang, etc
		# Comments are provided next to each warning option detailing expected compiler support (from GCC 3.4+, Clang 3.2+ - earlier versions may / may not support these options)

		set(_supported_c_compiler_flags "")
		set(_supported_cxx_compiler_flags "")

		# Enable -Wpedantic (if supported)
		check_compiler_flags_output("-Werror -Wpedantic -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wpedantic" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror -Wpedantic -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wpedantic" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# Enable -Wall (if supported)
		check_compiler_flags_output("-Werror -Wall -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wall" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror -Wall -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wall" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# Enable -Wextra (if supported)
		check_compiler_flags_output("-Werror -Wextra -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wextra" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror -Wextra -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wextra" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# -Wcast-align				(GCC 3.4+, Clang 3.2+)
		check_compiler_flags_output("-Werror -Wcast-align -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wcast-align" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror -Wcast-align -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wcast-align" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# -Wcast-qual				(GCC 3.4+, Clang 3.2+ (no-op until 3.6+))
		check_compiler_flags_output("-Werror -Wcast-qual -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wcast-qual" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror -Wcast-qual -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wcast-qual" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

#		# NOTE: Currently conflicts with json.hpp
#		# -Wctor-dtor-privacy		(GCC 3.4+, Clang 3.2+ (no-op through at least 6.0))
#		check_compiler_flags_output("-Werror -Wctor-dtor-privacy -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wctor-dtor-privacy" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
#		check_compiler_flags_output("-Werror -Wctor-dtor-privacy -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wctor-dtor-privacy" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# -Winit-self				(GCC 3.4+, Clang 3.2+ (no-op through at least 6.0))
		check_compiler_flags_output("-Werror -Winit-self -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Winit-self" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror -Winit-self -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Winit-self" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

#		# NOTE: Currently disabled
#		# -Wmissing-declarations	(GCC 3.4+, Clang 3.2+)
#		check_compiler_flags_output("-Werror -Wmissing-declarations -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wmissing-declarations" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
#		check_compiler_flags_output("-Werror -Wmissing-declarations -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wmissing-declarations" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

#		# NOTE: Currently disabled because of miniupnpc (pending fix)
#		# -Wmissing-include-dirs	(GCC 4.0+, Clang 3.2+ (no-op through at least 6.0))
#		check_compiler_flags_output("-Werror -Wmissing-include-dirs -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wmissing-include-dirs" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
#		check_compiler_flags_output("-Werror -Wmissing-include-dirs -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wmissing-include-dirs" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# -Wnoexcept				(GCC 4.6+) [C++-only]
		# check_compiler_flags_output("-Werror -Wnoexcept -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wnoexcept" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror -Wnoexcept -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wnoexcept" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# -Woverloaded-virtual		(GCC 3.4+, Clang 3.2+) [C++-only]
		# check_compiler_flags_output("-Werror -Woverloaded-virtual -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Woverloaded-virtual" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror -Woverloaded-virtual -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Woverloaded-virtual" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# -Wstrict-null-sentinel	(GCC 4.0+) [C++-only]
		check_compiler_flags_output("-Werror -Wstrict-null-sentinel -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wstrict-null-sentinel" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# -Wwrite-strings			(GCC 3.4+, Clang 3.2+)
		check_compiler_flags_output("-Werror -Wwrite-strings -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wwrite-strings" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror -Wwrite-strings -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wwrite-strings" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# -Wpointer-arith			(GCC 3.4+, Clang 3.2+)
		check_compiler_flags_output("-Werror -Wpointer-arith -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wpointer-arith" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror -Wpointer-arith -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wpointer-arith" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# -Wstrict-prototypes		(GCC 3.4+, Clang 3.2+ (no-op through at least 6.0)) [C-only]
		check_compiler_flags_output("-Werror -Wstrict-prototypes -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wstrict-prototypes" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)

#		# -Wmissing-noreturn		(GCC 3.4+, Clang 3.3+)
#		check_compiler_flags_output("-Werror -Wmissing-noreturn -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wmissing-noreturn" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
#		check_compiler_flags_output("-Werror -Wmissing-noreturn -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wmissing-noreturn" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

#		# -Wundef					(GCC 3.4+, Clang (supported, but min version unclear))
#		check_compiler_flags_output("-Werror -Wundef -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wundef" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
#		check_compiler_flags_output("-Werror -Wundef -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wundef" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# -Wnon-virtual-dtor		(GCC 3.4+, Clang 3.2+) [C++ only]
		check_compiler_flags_output("-Werror -Wnon-virtual-dtor -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wnon-virtual-dtor" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

#		# FUTURE-TODO: Enable -Wshadow (lots of warnings to fix, some due to 3rdparty dependencies?)
#		# -Wshadow					(GCC 3.4+, Clang 3.2+)
#		check_compiler_flags_output("-Werror -Wshadow -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wshadow" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
#		check_compiler_flags_output("-Werror -Wshadow -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wshadow" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

#		# FUTURE-TODO: Enable -Wuseless-cast (large number of warnings to fix)
#		# -Wuseless-cast			(GCC 4.8+)
#		check_compiler_flags_output("-Werror -Wuseless-cast -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wuseless-cast" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
#		check_compiler_flags_output("-Werror -Wuseless-cast -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wuseless-cast" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# -Wnull-dereference		(GCC 6.0+, Clang 3.2+)
		check_compiler_flags_output("-Werror -Wnull-dereference -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wnull-dereference" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror -Wnull-dereference -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wnull-dereference" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# -Wduplicated-cond			(GCC 6.0+)
		check_compiler_flags_output("-Werror -Wduplicated-cond -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wduplicated-cond" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror -Wduplicated-cond -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wduplicated-cond" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# -Wrestrict				(GCC 7.0+)
		check_compiler_flags_output("-Werror -Wrestrict -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wrestrict" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror -Wrestrict -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wrestrict" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		if (WZ_ENABLE_WARNINGS_AS_ERRORS)
			# Enable -Werror (if supported)
#			check_compiler_flags_output("-Werror" COMPILER_TYPE C   OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
#			check_compiler_flags_output("-Werror" COMPILER_TYPE CXX OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)
		else()
			message( STATUS "WZ will *NOT* enable -Werror" )
		endif()
		message(STATUS "CMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}")

		# NOTE: From this point on, checking compiler flags must also specify -Wno-error=cpp (or the check will fail)

		# Enable -Wlogical-op (if supported) (WARNING-ONLY)
		check_compiler_flags_output("-Werror ${WZ_Wno_}logical-op -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "${WZ_Wno_}logical-op" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror ${WZ_Wno_}logical-op -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "${WZ_Wno_}logical-op" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# Handle "sign-compare" (should be a warning at most - never an error)
		check_compiler_flags_output("-Werror ${WZ_Wno_}sign-compare -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wno-sign-compare" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror ${WZ_Wno_}sign-compare -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wno-sign-compare" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# Handle "format-security" (should be a warning at most - never an error)
		check_compiler_flags_output("-Werror ${WZ_Wno_}format-security -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "${WZ_Wno_}format-security" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror ${WZ_Wno_}format-security -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "${WZ_Wno_}format-security" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		# Disable -Wunused-parameter (FIXME)
		check_compiler_flags_output("-Werror -Wno-unused-parameter -Wno-error=cpp" COMPILER_TYPE C   OUTPUT_FLAGS "-Wno-unused-parameter" OUTPUT_VARIABLE _supported_c_compiler_flags APPEND)
		check_compiler_flags_output("-Werror -Wno-unused-parameter -Wno-error=cpp" COMPILER_TYPE CXX OUTPUT_FLAGS "-Wno-unused-parameter" OUTPUT_VARIABLE _supported_cxx_compiler_flags APPEND)

		message( STATUS "_supported_c_compiler_flags=${_supported_c_compiler_flags}" )
		message( STATUS "_supported_cxx_compiler_flags=${_supported_cxx_compiler_flags}" )

		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${_supported_c_compiler_flags}")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${_supported_cxx_compiler_flags}")

	endif()
endmacro(CONFIGURE_WZ_COMPILER_WARNINGS)

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
CONFIGURE_WZ_COMPILER_WARNINGS()
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
