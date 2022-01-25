vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO BinomialLLC/basis_universal
    REF 0b9cd40f3b42fcf44941a4d98ea767c0b10d4939 #tag/v1.15_rel2
    SHA512 b2d8cdb80d25e845a619bc34ea4aa53d83959dca9d3fbfed72dc865d76b73519cb38a63809cc5f542350fdf24c2f5d891d485e39355110f2b9006b077269be8b
    HEAD_REF master
)

vcpkg_configure_cmake(
    SOURCE_PATH ${SOURCE_PATH}
    PREFER_NINJA
)

vcpkg_install_cmake()

#vcpkg_fixup_cmake_targets(CONFIG_PATH lib/cmake/basisu)
if (WIN32)
    set(TOOL_NAME basisu.exe)
else()
    set(TOOL_NAME basisu)
endif()

file(INSTALL ${SOURCE_PATH}/LICENSE DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT} RENAME copyright)
file(COPY ${CURRENT_PACKAGES_DIR}/bin/${TOOL_NAME} DESTINATION ${CURRENT_PACKAGES_DIR}/tools/basisu)

vcpkg_copy_tool_dependencies(${CURRENT_PACKAGES_DIR}/tools/basisu)

# Remove unnecessary files
file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug)
file(REMOVE ${CURRENT_PACKAGES_DIR}/bin/${TOOL_NAME})

set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)

#if(VCPKG_LIBRARY_LINKAGE STREQUAL static)
#    file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/bin ${CURRENT_PACKAGES_DIR}/debug/bin)
#endif()

vcpkg_copy_pdbs()
