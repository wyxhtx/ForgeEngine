# 从 KTX-Software 仓库直接下载 1 个字体数据文件（raw URL，不 clone 整个仓库）
vcpkg_download_distfile(
    FONT_INL
    URLS "https://raw.githubusercontent.com/KhronosGroup/KTX-Software/v4.3.2/other_include/stb/stb_font_consolas_24_latin1.inl"
    FILENAME "stb_font_consolas_24_latin1.inl"
    SHA512 E9C6F9A47A42C8EC2E52D3D29F8535ADD014F32D5113D32B7ACD956D781C009B3F5F58D6D124DE8A14EEBCF5AD67C8F1DA9ACBFA44A1CCB1CD66C6E9255C9C3D
)
file(INSTALL "${FONT_INL}" DESTINATION "${CURRENT_PACKAGES_DIR}/include/stb")
file(WRITE "${CURRENT_PACKAGES_DIR}/share/${PORT}/copyright"
     "Forge custom port. See project repository for license details.\n")
file(WRITE "${CURRENT_PACKAGES_DIR}/share/unofficial-forge-stb-font/unofficial-forge-stb-font-config.cmake" [[
if(NOT TARGET forge-stb-font)
    add_library(forge-stb-font INTERFACE IMPORTED)
    set_target_properties(forge-stb-font PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_LIST_DIR}/../../include")
endif()
]])
file(WRITE "${CURRENT_PACKAGES_DIR}/share/unofficial-forge-stb-font/unofficial-forge-stb-font-config-version.cmake" [[
set(PACKAGE_VERSION "1.0.0")
if(PACKAGE_VERSION VERSION_LESS PACKAGE_FIND_VERSION)
    set(PACKAGE_VERSION_COMPATIBLE FALSE)
else()
    set(PACKAGE_VERSION_COMPATIBLE TRUE)
endif()
]])