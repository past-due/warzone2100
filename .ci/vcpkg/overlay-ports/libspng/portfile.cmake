vcpkg_from_github(
     OUT_SOURCE_PATH SOURCE_PATH
     REPO randy408/libspng
     REF v0.7.2
     SHA512 924a8148aeb485ba229e99afc75dac794a7a281ca1b4c6f2993bc05d81f4d3e90ba6ad4beb76a8e78f1e0164c17ece5b10d1a4eca9cb5f3fb7a2de4e84010564
     HEAD_REF master
 )


set(_linkage_shared OFF)
set(_linkage_static OFF)
if(VCPKG_LIBRARY_LINKAGE MATCHES "dynamic")
	set(_linkage_shared ON)
else()
	set(_linkage_static ON)
endif()

vcpkg_cmake_configure(
	SOURCE_PATH "${SOURCE_PATH}"
	PREFER_NINJA
	OPTIONS
		-DSPNG_SHARED=${_linkage_shared}
		-DSPNG_STATIC=${_linkage_static}
		-DBUILD_EXAMPLES=OFF
)

vcpkg_cmake_install()
vcpkg_fixup_pkgconfig()
file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)

if (NOT DEFINED VCPKG_CMAKE_SYSTEM_NAME)
	file(MAKE_DIRECTORY "${CURRENT_PACKAGES_DIR}/bin")
	file(RENAME "${CURRENT_PACKAGES_DIR}/lib/spng.dll" "${CURRENT_PACKAGES_DIR}/bin/spng.dll")

	file(MAKE_DIRECTORY "${CURRENT_PACKAGES_DIR}/debug/bin")
	file(RENAME "${CURRENT_PACKAGES_DIR}/debug/lib/spng.dll" "${CURRENT_PACKAGES_DIR}/debug/bin/spng.dll")
endif()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
if(VCPKG_LIBRARY_LINKAGE STREQUAL "static")
     file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/bin" "${CURRENT_PACKAGES_DIR}/debug/bin")
endif()
