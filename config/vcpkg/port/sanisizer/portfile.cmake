vcpkg_from_git(
    OUT_SOURCE_PATH SOURCE_PATH
    URL https://github.com/LTLA/sanisizer.git
    REF 32a024ae1926231ceaa02ed6ff104c6df6d524dc
    FETCH_REF master
)
file(COPY "${SOURCE_PATH}/include" DESTINATION "${CURRENT_PACKAGES_DIR}/")
file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
file(WRITE "${CURRENT_PACKAGES_DIR}/share/unofficial-sanisizer/unofficial-sanisizer-config.cmake" [[
if(NOT TARGET sanisizer)
    add_library(sanisizer INTERFACE IMPORTED)
    set_target_properties(sanisizer PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_LIST_DIR}/../../include")
endif()
]])
file(WRITE "${CURRENT_PACKAGES_DIR}/share/unofficial-sanisizer/unofficial-sanisizer-config-version.cmake" [[
set(PACKAGE_VERSION "0.2.0")
if(PACKAGE_VERSION VERSION_LESS PACKAGE_FIND_VERSION)
    set(PACKAGE_VERSION_COMPATIBLE FALSE)
else()
    set(PACKAGE_VERSION_COMPATIBLE TRUE)
    if(PACKAGE_VERSION VERSION_EQUAL PACKAGE_FIND_VERSION)
        set(PACKAGE_VERSION_EXACT TRUE)
    endif()
endif()
]])