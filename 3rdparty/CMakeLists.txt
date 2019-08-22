cmake_minimum_required (VERSION 3.5)

add_subdirectory(micro-ecc)
set_property(TARGET microecc PROPERTY FOLDER "3rdparty")

add_subdirectory(fonts)

SET(UTF8PROC_INSTALL OFF CACHE BOOL "Enable installation of utf8proc" FORCE)
add_subdirectory(utf8proc)
set_property(TARGET utf8proc PROPERTY FOLDER "3rdparty")

# SHA2-library

set(SHA2_HEADERS
	sha/brg_endian.h
	sha/brg_types.h
	sha/sha2.h
)

set(SHA2_SOURCES
	sha/sha2.c
)

add_library(sha2 STATIC ${SHA2_HEADERS} ${SHA2_SOURCES})
set_property(TARGET sha2 PROPERTY FOLDER "3rdparty")
target_include_directories(sha2 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

# glad library

set(GLAD_HEADERS
	glad/include/glad/glad.h
	glad/include/KHR/khrplatform.h
)

set(GLAD_SOURCES
	glad/src/glad.c
)

add_library(glad STATIC ${GLAD_HEADERS} ${GLAD_SOURCES})
set_property(TARGET glad PROPERTY FOLDER "3rdparty")
target_link_libraries(glad PRIVATE ${CMAKE_DL_LIBS})
target_include_directories(glad PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/glad/include")
