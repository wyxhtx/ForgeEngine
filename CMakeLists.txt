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
# 顶层只保留两类节点：src\（代码/资产/着色器源）与 docs\（文档）
# 根文件（.gitignore/CMakePresets.json/README.md/vcpkg.json）、cmd\、config\ 不再注入工程树；文件仍在磁盘，可直接编辑
# 文档（排除 chatbox-sync 的同步副本，避免与 chatbox-docs 出现同名两份）
file(GLOB_RECURSE FORGE_DOCS_FILES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/docs/*.md"
)
list(FILTER FORGE_DOCS_FILES EXCLUDE REGEX "/chatbox-sync/")
source_group(TREE "${CMAKE_SOURCE_DIR}/docs" PREFIX "docs" FILES ${FORGE_DOCS_FILES})
target_sources(ForgeEngine PRIVATE ${FORGE_DOCS_FILES})

# 资产
file(GLOB_RECURSE FORGE_ASSET_FILES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/assets/*"
)
list(FILTER FORGE_ASSET_FILES EXCLUDE REGEX "/(shader|shader_src|image)/")
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

# PCH 中间文件（CMake 自动生成的 cmake_pch.cxx/.hxx）默认落在英文节点 Source Files / Precompile Header File；改归顶层内部节点（中文名排序在最下方，不挤在 src 里）
source_group("构建中间文件" FILES
    "${CMAKE_BINARY_DIR}/CMakeFiles/ForgeEngine.dir/cmake_pch.cxx"
    "${CMAKE_BINARY_DIR}/CMakeFiles/ForgeEngine.dir/Debug/cmake_pch.hxx"
    "${CMAKE_BINARY_DIR}/CMakeFiles/ForgeEngine.dir/Release/cmake_pch.hxx"
    "${CMAKE_BINARY_DIR}/CMakeFiles/ForgeEngine.dir/RelWithDebInfo/cmake_pch.hxx"
)



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

# VS 输出/中间目录（对齐 VS 手写工程：bin\output\x64\<Config> / bin\intermediate\x64\<Config>）
set_target_properties(ForgeEngine PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"                         # F5 工作目录=项目根
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/output/x64/$<CONFIG>"     # exe 输出位
)
configure_file(config/cmake/ForgeIntermediate.targets "${CMAKE_BINARY_DIR}/Directory.Build.targets" COPYONLY)   # 中间目录=bin\intermediate\x64\<Config>（VS 生成器无原生项，改用 MSBuild 末尾导入覆盖）

# 着色器编译
include(config/cmake/ForgeShader.cmake)     # 编译规则
add_dependencies(ForgeEngine ForgeShader)   # 先编译着色器


# ---- 着色器源（GLSL）在解决方案中显示 ----
# 只显示 src/assets/shader_src 下的 GLSL 源；shader\（spv 产物）不显示，避免 VS 出现 "CMake Rules" 节点
# 显式限定扩展名：当前只有 .vert/.frag；.mesh/.task/.comp/.glsl 为预留（将来加网格/计算着色器自动显示）
# 这些文件为 None 项类型：仅显示，不参与编译
file(GLOB_RECURSE FORGE_SHADER_SRC_FILES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/assets/shader_src/*.vert"
    "${CMAKE_SOURCE_DIR}/src/assets/shader_src/*.frag"
    "${CMAKE_SOURCE_DIR}/src/assets/shader_src/*.mesh"
    "${CMAKE_SOURCE_DIR}/src/assets/shader_src/*.task"
    "${CMAKE_SOURCE_DIR}/src/assets/shader_src/*.comp"
    "${CMAKE_SOURCE_DIR}/src/assets/shader_src/*.glsl"
)
if (FORGE_SHADER_SRC_FILES)
  source_group(TREE "${CMAKE_SOURCE_DIR}/src/assets" PREFIX "src/assets" FILES ${FORGE_SHADER_SRC_FILES})
  target_sources(ForgeEngine PRIVATE ${FORGE_SHADER_SRC_FILES})
endif()