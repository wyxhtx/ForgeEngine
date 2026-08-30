vcpkg_from_git(
    OUT_SOURCE_PATH SOURCE_PATH
    URL https://github.com/LTLA/subpar.git
    REF cbcc024b9bb1ba691fae00c545d323c1592cb872
    FETCH_REF master
)
file(COPY "${SOURCE_PATH}/include" DESTINATION "${CURRENT_PACKAGES_DIR}/")
file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
file(WRITE "${CURRENT_PACKAGES_DIR}/share/unofficial-subpar/unofficial-subpar-config.cmake" [[
if(NOT TARGET subpar)
    add_library(subpar INTERFACE IMPORTED)
    set_target_properties(subpar PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_LIST_DIR}/../../include")
endif()
]])
file(WRITE "${CURRENT_PACKAGES_DIR}/share/unofficial-subpar/unofficial-subpar-config-version.cmake" [[
set(PACKAGE_VERSION "0.5.0")
if(PACKAGE_VERSION VERSION_LESS PACKAGE_FIND_VERSION)
    set(PACKAGE_VERSION_COMPATIBLE FALSE)
else()
    set(PACKAGE_VERSION_COMPATIBLE TRUE)
    if(PACKAGE_VERSION VERSION_EQUAL PACKAGE_FIND_VERSION)
        set(PACKAGE_VERSION_EXACT TRUE)
    endif()
endif()
]])