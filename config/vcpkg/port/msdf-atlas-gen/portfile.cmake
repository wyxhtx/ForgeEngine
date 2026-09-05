# MSDF font atlas generator (Chlumsky), locked commit, external msdfgen via vcpkg
vcpkg_from_git(
    OUT_SOURCE_PATH SOURCE_PATH
    URL https://github.com/Chlumsky/msdf-atlas-gen.git
    REF 6148900d59423059bafde2f51a0cb303184404bd
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS -DMSDF_ATLAS_MSDFGEN_EXTERNAL=ON
            -DMSDF_ATLAS_USE_SKIA=OFF
            -DMSDF_ATLAS_NO_ARTERY_FONT=ON
            -DMSDF_ATLAS_BUILD_STANDALONE=OFF
            -DMSDF_ATLAS_INSTALL=ON
)
vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH "lib/cmake/msdf-atlas-gen")

file(INSTALL "${SOURCE_PATH}/LICENSE.txt" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")