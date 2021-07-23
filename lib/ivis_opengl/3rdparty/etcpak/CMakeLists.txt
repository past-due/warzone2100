file(GLOB HEADERS "*.hpp")
file(GLOB SRC "*.cpp")

add_library(etcpak STATIC ${HEADERS} ${SRC})
set_property(TARGET etcpak PROPERTY FOLDER "3rdparty")
#if(WZ_TARGET_ADDITIONAL_PROPERTIES)
#	SET_TARGET_PROPERTIES(etcpak PROPERTIES ${WZ_TARGET_ADDITIONAL_PROPERTIES})
#endif()
target_include_directories(etcpak INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
