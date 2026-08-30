vcpkg_from_git(
    OUT_SOURCE_PATH SOURCE_PATH
    URL https://github.com/libscran/kmeans.git
    REF 0f427abb2a66f5e820a64338543d838a5eca4c99
    FETCH_REF master
)
file(COPY "${SOURCE_PATH}/include" DESTINATION "${CURRENT_PACKAGES_DIR}/")
file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
file(WRITE "${CURRENT_PACKAGES_DIR}/share/kmeans/unofficial-kmeans-config.cmake" [[
if(NOT TARGET kmeans)
    add_library(kmeans INTERFACE IMPORTED)
    set_target_properties(kmeans PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_LIST_DIR}/../../include")
endif()
]])
file(WRITE "${CURRENT_PACKAGES_DIR}/share/kmeans/unofficial-kmeans-config-version.cmake" [[
set(PACKAGE_VERSION "1.0.0")
if(PACKAGE_VERSION VERSION_LESS PACKAGE_FIND_VERSION)
    set(PACKAGE_VERSION_COMPATIBLE FALSE)
else()
    set(PACKAGE_VERSION_COMPATIBLE TRUE)
endif()
]])