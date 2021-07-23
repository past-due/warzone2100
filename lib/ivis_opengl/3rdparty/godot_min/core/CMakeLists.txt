file(GLOB_RECURSE HEADERS "*.h")
file(GLOB_RECURSE SRC "*.cpp")


add_library(godot_min STATIC ${HEADERS} ${SRC})
set_property(TARGET godot_min PROPERTY FOLDER "3rdparty")
#if(WZ_TARGET_ADDITIONAL_PROPERTIES)
#	SET_TARGET_PROPERTIES(etcpak PROPERTIES ${WZ_TARGET_ADDITIONAL_PROPERTIES})
#endif()
target_include_directories(godot_min PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../")
target_include_directories(godot_min INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/../")
