# ============================================================
# ktx：改编自官方 vcpkg port
# 改动：官方用 cmake tar 解压（Windows 无法处理包内符号链接）
#       这里改用 7z 解压，其余构建逻辑与官方完全一致
# ============================================================
vcpkg_download_distfile(
    KTX_ARCHIVE
    URLS "https://github.com/KhronosGroup/KTX-Software/archive/v${VERSION}.tar.gz"
    FILENAME "KhronosGroup-KTX-Software-v${VERSION}.tar.gz"
    SHA512 19514da2d5b021d7fd1e24251dfd27d0e032018bdb84c7f76328de0ad431aeff12a77e7b3c857a1933a0b258a83ffd4b77cd053672702cc6f7132afcd1fa253e
)

# ---- 7z 解压（替代官方 cmake tar，解决符号链接问题）----
set(SOURCE_PATH "${CURRENT_BUILDTREES_DIR}/src/ktx-${VERSION}-forge")
file(REMOVE_RECURSE "${SOURCE_PATH}")
file(MAKE_DIRECTORY "${CURRENT_BUILDTREES_DIR}/src")
vcpkg_find_acquire_program(7Z)
vcpkg_execute_required_process(
    COMMAND "${7Z}" x "${KTX_ARCHIVE}" -o${CURRENT_BUILDTREES_DIR}/src -aoa -y
    WORKING_DIRECTORY "${CURRENT_BUILDTREES_DIR}/src"
    LOGNAME "7z-extract-gz-${TARGET_TRIPLET}"
)
# 7z 对 .tar.gz 只解 gzip 层（得到 .tar），需再解一次 tar
file(GLOB KTX_TAR "${CURRENT_BUILDTREES_DIR}/src/*.tar")
# tar 内含符号链接（如 NOTICE.md）：无管理员/开发者模式权限时 7z 创建符号链接
# 失败并返回非零退出码，但普通文件已全部解出；NOTICE.md 不参与构建与安装，故忽略退出码。
execute_process(
    COMMAND "${7Z}" x "${KTX_TAR}" -o${CURRENT_BUILDTREES_DIR}/src -aoa -y
    WORKING_DIRECTORY "${CURRENT_BUILDTREES_DIR}/src"
    RESULT_VARIABLE _ktx_extract_rc
    OUTPUT_FILE "${CURRENT_BUILDTREES_DIR}/7z-extract-tar-${TARGET_TRIPLET}-out.log"
    ERROR_FILE  "${CURRENT_BUILDTREES_DIR}/7z-extract-tar-${TARGET_TRIPLET}-err.log"
)
if(NOT _ktx_extract_rc EQUAL 0)
    message(WARNING "7z 解压 tar 返回码 ${_ktx_extract_rc}（预计为符号链接权限错误，继续构建）")
endif()
file(GLOB KTX_SRC_DIR "${CURRENT_BUILDTREES_DIR}/src/KTX-*")
if(NOT KTX_SRC_DIR)
    message(FATAL_ERROR "7z 解压失败：未在 src 下找到 KTX 源码目录")
endif()
file(RENAME "${KTX_SRC_DIR}" "${SOURCE_PATH}")

# 符号链接缺失补偿：NOTICE.md 若因符号链接创建失败而缺失，补占位文件保持源码树完整
if(NOT EXISTS "${SOURCE_PATH}/NOTICE.md")
    file(WRITE "${SOURCE_PATH}/NOTICE.md" "See LICENSE.md for license details.\n")
endif()
file(GLOB KTX_SRC_DIR "${CURRENT_BUILDTREES_DIR}/src/KTX-*")
if(NOT KTX_SRC_DIR)
    message(FATAL_ERROR "7z 解压失败：未在 src 下找到 KTX 源码目录")
endif()
file(RENAME "${KTX_SRC_DIR}" "${SOURCE_PATH}")

# ---- 应用官方补丁 ----
vcpkg_apply_patches(SOURCE_PATH "${SOURCE_PATH}"
    PATCHES
        0001-Use-vcpkg-zstd.patch
        0003-mkversion.patch
        0004-quirks.patch
        0005-no-vendored-libs.patch
        0006-fix-ios-install.patch
        ktxread-libtool.diff
)

# ---- 清理 vendored 第三方（同官方）----
file(GLOB third_party "${SOURCE_PATH}/external/*" "${SOURCE_PATH}/external/basisu/zstd" "${SOURCE_PATH}/other_include/*")
list(FILTER third_party EXCLUDE REGEX "/(astc-encoder|basisu|dfdutils|etcdec|imageio|glm|lodepng|SDL_gesture)$")
file(REMOVE_RECURSE ${third_party})

# ---- MSYS（同官方：Windows 构建需要 bash/dos2unix）----
vcpkg_list(SET OPTIONS)
if(VCPKG_TARGET_IS_WINDOWS)
    vcpkg_acquire_msys(MSYS_ROOT
        PACKAGES bash
        DIRECT_PACKAGES
            "https://repo.msys2.org/msys/x86_64/util-linux-2.40.2-2-x86_64.pkg.tar.zst"
            bf45b16cd470f8d82a9fe03842a09da2e6c60393c11f4be0bab354655072c7a461afc015b9c07f9f5c87a0e382cd867e4f079ede0d42f1589aa99ebbb3f76309
            "https://mirror.msys2.org/msys/x86_64/dos2unix-7.5.2-1-x86_64.pkg.tar.zst"
            e5e949f01b19c82630131e338a4642da75e42f84220f5af4a97a11dd618e363396567b233d2adab79e05422660a0000abcbbabcd17efcadf37f07fe7565f041e
    )
    vcpkg_add_to_path("${MSYS_ROOT}/usr/bin")
    vcpkg_list(APPEND OPTIONS "-DBASH_EXECUTABLE=${MSYS_ROOT}/usr/bin/bash.exe")
endif()

# ---- Vulkan feature ----
vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_OPTIONS
    FEATURES
        tools   KTX_FEATURE_TOOLS
        vulkan  KTX_FEATURE_VK_UPLOAD
)

# ---- CMake 构建（同官方）----
vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DKTX_GIT_VERSION_FULL=v${VERSION}-vcpkg
        -DKTX_FEATURE_TESTS=OFF
        -DKTX_FEATURE_LOADTEST_APPS=OFF
        ${FEATURE_OPTIONS}
        ${OPTIONS}
    OPTIONS_DEBUG
        -DKTX_FEATURE_TOOLS=OFF
    DISABLE_PARALLEL_CONFIGURE
)
vcpkg_cmake_install()
vcpkg_copy_pdbs()
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/ktx)

if(tools IN_LIST FEATURES)
    vcpkg_copy_tools(
        TOOL_NAMES ktx toktx ktxsc ktxinfo ktx2ktx2 ktx2check
        AUTO_CLEAN
    )
endif()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

file(GLOB LICENSE_FILES "${SOURCE_PATH}/LICENSES/*")
file(COPY ${LICENSE_FILES} DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}/LICENSES")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE.md")