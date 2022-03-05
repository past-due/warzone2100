# - Try to locate SDL2
# This module defines:
#
#  SDL2_INCLUDE_DIR
#  SDL2_LIBRARY
#  SDL2_FOUND
#  SDL2_VERSION_STRING, human-readable string containing the version of SDL
#
# Portions derived from FindSDL.cmake, distributed under the OSI-approved BSD 3-Clause License (https://cmake.org/licensing)
#

find_path(SPNG_INCLUDE_DIR NAMES spng.h)

find_library(SPNG_LIBRARY NAMES spng_static spng)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(SPNG REQUIRED_VARS SPNG_INCLUDE_DIR SPNG_LIBRARY)

mark_as_advanced(SPNG_INCLUDE_DIR SPNG_LIBRARY)

add_library(SPNG::SPNG UNKNOWN IMPORTED)
set_property(TARGET SPNG::SPNG
	APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${SPNG_INCLUDE_DIR}"
)
set_property(TARGET SPNG::SPNG
	APPEND PROPERTY IMPORTED_LOCATION "${SPNG_LIBRARY}"
)
