# ============================================================================
#  ForgeEngine 构建脚本（vcpkg manifest + CMake）
#
#  构建两条链：
#    cmake --preset vs     -> build\vs\     （VS2022 .sln，F5 调试）
#    cmake --preset ninja  -> build\ninja\  （命令行快速构建）
#  依赖：vcpkg.json（13 库）+ config\vcpkg\（triplet/port overlay）
# ============================================================================
cmake_minimum_required(VERSION 3.24)                                            # CMake 最低版本
project(ForgeEngine LANGUAGES CXX)                                              # 工程名 ForgeEngine



# ---- 语言标准 ----
set(CMAKE_CXX_STANDARD 20)                                                      # C++20
set(CMAKE_CXX_STANDARD_REQUIRED ON)                                             # 强制支持
set(CMAKE_CXX_EXTENSIONS OFF)                                                   # 纯标准，禁 GNU 扩展
set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/DNDEBUG /Od /Zi")                          # RelWithDebInfo：不优化+PDB，编译快、运行无 STL 检查、可断点
set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "/DEBUG /INCREMENTAL")                # RelWithDebInfo：增量链接WW



# 1. MSVC 全局配置
if(MSVC)
    add_compile_options(/utf-8 /W3)                                             # /utf-8：源码按 UTF-8 编译（防中文注释乱码）；/W3：警告级别 3
endif()



# 2. Vulkan SDK（系统级，走 VULKAN_SDK，不进 vcpkg）
find_package(Vulkan REQUIRED)                                                   # Vulkan 头/库（vulkan-1.lib）
find_program(GLSLANG_VALIDATOR glslangValidator HINTS "$ENV{VULKAN_SDK}/Bin")   # 着色器编译器
if(NOT GLSLANG_VALIDATOR)                                                       # 缺失则终止
    message(FATAL_ERROR "未找到 glslangValidator（请设 VULKAN_SDK 环境变量）")
endif()



# 3. vcpkg 官方包（版本由 vcpkg.json 的 builtin-baseline 锁定）
find_package(glfw3 CONFIG REQUIRED)                                             # GLFW:     框架库，lib
find_package(glm CONFIG REQUIRED)                                               # GLM:      数学库，header-only
find_package(VulkanMemoryAllocator CONFIG REQUIRED)                             # VMA:      显存库，header-only
find_package(Ktx CONFIG REQUIRED)                                               # KTX2:     纹理库，lib
find_path(TINYGLTF_INCLUDE_DIR tiny_gltf.h)                                     # tinygltf: 模型库，header-only，官方无 config -> find_path
if(NOT TINYGLTF_INCLUDE_DIR)
    message(FATAL_ERROR "找不到 tiny_gltf.h，请确认 vcpkg 已安装 tinygltf")
endif()
add_library(ForgeTinygltf INTERFACE)
target_include_directories(ForgeTinygltf INTERFACE "${TINYGLTF_INCLUDE_DIR}")
find_package(OpenAL CONFIG REQUIRED)                                            # OpenAL:   音频库，lib
find_package(FreeALUT CONFIG REQUIRED)                                          # ALUT:     音频封装库，lib
find_package(msdfgen CONFIG REQUIRED)         # msdfgen:       MSDF 距离场生成库，lib
find_package(msdf-atlas-gen CONFIG REQUIRED)  # msdf-atlas-gen: MSDF 图集生成库，lib



# 4. 自定义库（overlay ports，REF 锁 commit）
# 4.1 kmeans: 聚类算法库，header-only
find_path(KMEANS_INCLUDE_DIRS "kmeans/Details.hpp")                             
if(NOT KMEANS_INCLUDE_DIRS)
    message(FATAL_ERROR "找不到 kmeans/Details.hpp，请确认 vcpkg 已安装 kmeans")
endif()
add_library(ForgeKmeans INTERFACE)
target_include_directories(ForgeKmeans INTERFACE "${KMEANS_INCLUDE_DIRS}")

# 4.2 FastNoise2: 噪音库，lib
find_path(FASTNOISE2_INCLUDE_DIR FastNoise/FastNoise.h)   # 头目录               
find_library(FASTNOISE2_LIBRARY_RELEASE NAMES FastNoise)  # Release: FastNoise.lib
find_library(FASTNOISE2_LIBRARY_DEBUG NAMES FastNoiseD)   # Debug:   FastNoiseD.lib
if(NOT FASTNOISE2_INCLUDE_DIR OR NOT FASTNOISE2_LIBRARY_RELEASE OR NOT FASTNOISE2_LIBRARY_DEBUG)
    message(FATAL_ERROR "找不到 FastNoise2（FastNoise/FastNoise.h 或 FastNoise.lib/FastNoiseD.lib）")
endif()
add_library(ForgeFastNoise INTERFACE IMPORTED)            # 手工导入目标
set_target_properties(ForgeFastNoise PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${FASTNOISE2_INCLUDE_DIR}"
    INTERFACE_LINK_LIBRARIES "$<$<CONFIG:Debug>:${FASTNOISE2_LIBRARY_DEBUG}>;$<$<NOT:$<CONFIG:Debug>>:${FASTNOISE2_LIBRARY_RELEASE}>"   # Debug 链 D 库
    INTERFACE_COMPILE_DEFINITIONS "FASTNOISE_STATIC_LIB"                                                                                # 静态版必须（防 dllimport 冲突）
)




# 5. 主程序目标（ForgeEngine.exe；游戏逻辑在 src\code\forge）
add_executable(ForgeEngine src/code/vk/Main.cpp)                                                # 单 TU（Main.cpp 内含全部实现）
target_compile_definitions(ForgeEngine PRIVATE UNICODE _UNICODE)                                # Unicode（WinAPI W 版）
target_compile_options(ForgeEngine PRIVATE /bigobj)                                             # 单 TU 节数超限需大目标
target_compile_options(ForgeEngine PRIVATE $<$<NOT:$<CONFIG:Debug>>:/U_DEBUG>)                  # 非 Debug 撤 _DEBUG（防 LNK2038）
target_compile_options(ForgeEngine PRIVATE $<$<CONFIG:Release>:/Zi>)                            # Release 产 PDB（/O2 下可断点/看调用栈）
target_link_options   (ForgeEngine PRIVATE $<$<CONFIG:Release>:/DEBUG>)                         # Release 链接带调试信息
# target_compile_options(ForgeEngine PRIVATE $<$<CONFIG:Release>:/d1reportTime>)                # 临时诊断：编译相位耗时报告
target_precompile_headers(ForgeEngine PRIVATE "${CMAKE_SOURCE_DIR}/src/code/vk/External.h")     # PCH：系统+三方头只编一次



# ---- 解决方案资源管理器显示 ----
# 命令
file(GLOB FORGE_CMD_FILES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/cmd/*.ps1"
)
source_group("cmd" FILES ${FORGE_CMD_FILES})
target_sources(ForgeEngine PRIVATE ${FORGE_CMD_FILES})

# 配置
file(GLOB_RECURSE FORGE_CONFIG_FILES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/config/*.cmake"
    "${CMAKE_SOURCE_DIR}/config/*.json"
    "${CMAKE_SOURCE_DIR}/config/*.patch"
    "${CMAKE_SOURCE_DIR}/config/*.diff"
)
source_group(TREE "${CMAKE_SOURCE_DIR}/config" PREFIX "config" FILES ${FORGE_CONFIG_FILES})
target_sources(ForgeEngine PRIVATE ${FORGE_CONFIG_FILES})

set(FORGE_ROOT_FILES
    "${CMAKE_SOURCE_DIR}/.gitignore"
    "${CMAKE_SOURCE_DIR}/CMakePresets.json"
    "${CMAKE_SOURCE_DIR}/README.md"
    "${CMAKE_SOURCE_DIR}/vcpkg.json"
)
source_group("config" FILES ${FORGE_ROOT_FILES})
target_sources(ForgeEngine PRIVATE ${FORGE_ROOT_FILES})

# 文档
file(GLOB_RECURSE FORGE_DOCS_FILES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/docs/*.md"
)
source_group(TREE "${CMAKE_SOURCE_DIR}/docs" PREFIX "docs" FILES ${FORGE_DOCS_FILES})
target_sources(ForgeEngine PRIVATE ${FORGE_DOCS_FILES})

# 资产
file(GLOB_RECURSE FORGE_ASSET_FILES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/assets/*"
)
list(FILTER FORGE_ASSET_FILES EXCLUDE REGEX "/(shader|shader_src)/")
source_group(TREE "${CMAKE_SOURCE_DIR}/src/assets" PREFIX "src/assets" FILES ${FORGE_ASSET_FILES})
target_sources(ForgeEngine PRIVATE ${FORGE_ASSET_FILES})

# 源码
file(GLOB_RECURSE FORGE_SOURCES CONFIGURE_DEPENDS                               
    "${CMAKE_SOURCE_DIR}/src/code/*.h"
    "${CMAKE_SOURCE_DIR}/src/code/*.hpp"
    "${CMAKE_SOURCE_DIR}/src/code/*.cpp"
)
target_sources(ForgeEngine PRIVATE ${FORGE_SOURCES})
source_group(TREE "${CMAKE_SOURCE_DIR}/src/code" PREFIX "src/code" FILES ${FORGE_SOURCES})



# 运行库类型: 全静态 MT/MTd
set_property(TARGET ForgeEngine PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# 头文件搜索路径
target_include_directories(ForgeEngine PRIVATE
    src/code/vk      # Vulkan 渲染层
    src/code/forge   # 游戏层
)

# 库链接清单
target_link_libraries(ForgeEngine PRIVATE
    glfw                                       # GLFW:                  框架库，    lib
    glm::glm                                   # GLM:                   数学库，    header-only
    Vulkan::Vulkan                             # Vulkan:                vk库，      dll
    GPUOpen::VulkanMemoryAllocator             # VMA:                   显存库，    header-only
    KTX::ktx                                   # KTX2:                  纹理库，    lib
    ForgeTinygltf                              # tinygltf:              模型库，    header-only
    OpenAL::OpenAL                             # OpenAL:                音频库，    lib
    FreeALUT::alut                             # ALUT:                  音频封装库，lib
    ForgeKmeans                                # kmeans:                聚类算法库，header-only
    ForgeFastNoise                             # FastNoise2:            噪音库，    lib
    msdfgen::msdfgen-full           # msdfgen:       MSDF 距离场库，     lib
    msdf-atlas-gen::msdf-atlas-gen  # msdf-atlas-gen: MSDF 图集库，       lib
)

# VS 工作目录
set_target_properties(ForgeEngine PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"                         # F5 工作目录=项目根
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/output/x64/$<CONFIG>"     # exe 输出位
)

# 着色器编译
include(config/cmake/ForgeShader.cmake)     # 编译规则
add_dependencies(ForgeEngine ForgeShader)   # 先编译着色器
