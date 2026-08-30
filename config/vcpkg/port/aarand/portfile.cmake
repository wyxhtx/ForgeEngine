vcpkg_from_git(
    OUT_SOURCE_PATH SOURCE_PATH
    URL https://github.com/LTLA/aarand.git
    REF e8ba3436d6a713deab943be81b7be0fce609e565
    FETCH_REF master
)
file(COPY "${SOURCE_PATH}/include" DESTINATION "${CURRENT_PACKAGES_DIR}/")
file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
file(WRITE "${CURRENT_PACKAGES_DIR}/share/aarand/unofficial-aarand-config.cmake" [[
if(NOT TARGET aarand)
    add_library(aarand INTERFACE IMPORTED)
    set_target_properties(aarand PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_LIST_DIR}/../../include")
endif()
]])
file(WRITE "${CURRENT_PACKAGES_DIR}/share/aarand/unofficial-aarand-config-version.cmake" [[
set(PACKAGE_VERSION "1.1.0")
if(PACKAGE_VERSION VERSION_LESS PACKAGE_FIND_VERSION)
    set(PACKAGE_VERSION_COMPATIBLE FALSE)
else()
    set(PACKAGE_VERSION_COMPATIBLE TRUE)
endif()
]])