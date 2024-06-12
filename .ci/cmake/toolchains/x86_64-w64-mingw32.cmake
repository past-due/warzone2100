SET(CMAKE_SYSTEM_NAME "Windows")
SET(CMAKE_SYSTEM_PROCESSOR "AMD64")
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    set(CMAKE_CROSSCOMPILING OFF CACHE BOOL "")
endif()

set(_WZ_MINGW_TRIPLET_PREFIX "x86_64")

find_program(CMAKE_C_COMPILER "${_WZ_MINGW_TRIPLET_PREFIX}-w64-mingw32-gcc")
find_program(CMAKE_CXX_COMPILER "${_WZ_MINGW_TRIPLET_PREFIX}-w64-mingw32-g++")
find_program(CMAKE_RC_COMPILER "${_WZ_MINGW_TRIPLET_PREFIX}-w64-mingw32-windres")
if(NOT CMAKE_RC_COMPILER)
    find_program(CMAKE_RC_COMPILER "windres")
endif()
