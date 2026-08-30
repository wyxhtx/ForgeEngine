vcpkg_from_git(
    OUT_SOURCE_PATH SOURCE_PATH
    URL https://github.com/Auburn/FastNoise2.git
    REF 903c1f2d2f9d53ddce94cd223f32727d9ab3aeaa
    FETCH_REF v1.1.1
)

# 用上游 CMake 构建（FastNoise2 含 FastSIMD 编译单元，非纯 header）
vcpkg_find_acquire_program(GIT)
vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS -DFETCHCONTENT_FULLY_DISCONNECTED=OFF
            -DGIT_EXECUTABLE="${GIT}"
            -DFASTNOISE2_TOOLS=OFF
            -DFASTNOISE2_TESTS=OFF
)
vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/FastNoise2)

file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")