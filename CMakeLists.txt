# ============================================================================
#  ForgeEngine 构建脚本（vcpkg + CMake）
#  依赖清单：vcpkg.json    构建入口：CMakePresets.json
# ============================================================================
cmake_minimum_required(VERSION 3.24)
project(Forge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ----------------------------------------------------------------------------
# 1. MSVC 全局配置
# ----------------------------------------------------------------------------
if(MSVC)
    
    # 源文件已统一 UTF-8；W3 警告级别
    add_compile_options(/utf-8 /W3)
endif()

# ----------------------------------------------------------------------------
# 2. Vulkan SDK（系统级，不走 vcpkg）
# ----------------------------------------------------------------------------
find_package(Vulkan REQUIRED)
find_program(GLSLANG_VALIDATOR glslangValidator HINTS "$ENV{VULKAN_SDK}/Bin")

if(NOT GLSLANG_VALIDATOR)
    message(FATAL_ERROR "未找到 glslangValidator（请设 VULKAN_SDK 环境变量）")
endif()

# ----------------------------------------------------------------------------
# 3. vcpkg 官方包
# ----------------------------------------------------------------------------
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(Ktx CONFIG REQUIRED)
# tinygltf 是 header-only，官方端口不生成 config，用 find_path 方式
find_path(TINYGLTF_INCLUDE_DIR tiny_gltf.h)
if(NOT TINYGLTF_INCLUDE_DIR)
    message(FATAL_ERROR "找不到 tiny_gltf.h，请确认 vcpkg 已安装 tinygltf")
endif()
add_library(ForgeTinygltf INTERFACE)
target_include_directories(ForgeTinygltf INTERFACE "${TINYGLTF_INCLUDE_DIR}")
find_package(OpenAL CONFIG REQUIRED)
find_package(FreeALUT CONFIG REQUIRED)

# ----------------------------------------------------------------------------
# 4. 自定义库（overlay ports）
# ----------------------------------------------------------------------------
# kmeans 是 header-only，自定义端口不生成 config，用 find_path 方式
find_path(KMEANS_INCLUDE_DIRS "kmeans/Details.hpp")
if(NOT KMEANS_INCLUDE_DIRS)
    message(FATAL_ERROR "找不到 kmeans/Details.hpp，请确认 vcpkg 已安装 kmeans")
endif()
add_library(ForgeKmeans INTERFACE)
target_include_directories(ForgeKmeans INTERFACE "${KMEANS_INCLUDE_DIRS}")
# FastNoise2 自定义端口无 config，用 find_path/find_library 手工目标
find_path(FASTNOISE2_INCLUDE_DIR FastNoise/FastNoise.h)
find_library(FASTNOISE2_LIBRARY_RELEASE NAMES FastNoise)
find_library(FASTNOISE2_LIBRARY_DEBUG NAMES FastNoiseD)
if(NOT FASTNOISE2_INCLUDE_DIR OR NOT FASTNOISE2_LIBRARY_RELEASE OR NOT FASTNOISE2_LIBRARY_DEBUG)
    message(FATAL_ERROR "找不到 FastNoise2（FastNoise/FastNoise.h 或 FastNoise.lib/FastNoiseD.lib）")
endif()
add_library(ForgeFastNoise INTERFACE IMPORTED)
set_target_properties(ForgeFastNoise PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${FASTNOISE2_INCLUDE_DIR}"
    INTERFACE_LINK_LIBRARIES "$<$<CONFIG:Debug>:${FASTNOISE2_LIBRARY_DEBUG}>;$<$<NOT:$<CONFIG:Debug>>:${FASTNOISE2_LIBRARY_RELEASE}>"
    INTERFACE_COMPILE_DEFINITIONS "FASTNOISE_STATIC_LIB"
)
# forge-stb-font 是 header-only，config 目录名与包名不匹配，用 find_path 方式
find_path(FORGE_STB_FONT_INCLUDE_DIRS "stb/stb_font_consolas_24_latin1.inl")
if(NOT FORGE_STB_FONT_INCLUDE_DIRS)
    message(FATAL_ERROR "找不到 stb/stb_font_consolas_24_latin1.inl，请确认 vcpkg 已安装 forge-stb-font")
endif()
add_library(ForgeStbFont INTERFACE)
target_include_directories(ForgeStbFont INTERFACE "${FORGE_STB_FONT_INCLUDE_DIRS}")

# ----------------------------------------------------------------------------
# 5. 主程序目标
# ----------------------------------------------------------------------------
add_executable(Forge src/code/vk/Main.cpp)
# 恢复旧工程 CharacterSet=Unicode（GetKeyboardLayoutName/LoadKeyboardLayout 需 W 版本）
target_compile_definitions(Forge PRIVATE UNICODE _UNICODE)
# 单 TU 包含全部实现（VMA/tinygltf/stb/引擎），节数超限，旧工程同样带 /bigobj
target_compile_options(Forge PRIVATE /bigobj)
target_compile_options(Forge PRIVATE $<$<NOT:$<CONFIG:Debug>>:/U_DEBUG>)  # 非Debug强制撤销_DEBUG，杜绝IDL串味
# 让 VS 解决方案资源管理器按磁盘目录显示源码树（纯显示，不影响编译）
file(GLOB_RECURSE FORGE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/code/*.h"
    "${CMAKE_SOURCE_DIR}/src/code/*.hpp"
    "${CMAKE_SOURCE_DIR}/src/code/*.cpp"
)
target_sources(Forge PRIVATE ${FORGE_SOURCES})
source_group(TREE "${CMAKE_SOURCE_DIR}/src/code" PREFIX "src/code" FILES ${FORGE_SOURCES})
set_property(TARGET Forge PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

target_include_directories(Forge PRIVATE
    src/code/vk
    src/code/forge
)

target_link_libraries(Forge PRIVATE
    glfw                                        # GLFW 窗口
    glm::glm                                    # glm 数学（header-only）
    Vulkan::Vulkan                              # Vulkan loader
    GPUOpen::VulkanMemoryAllocator              # VMA（header-only）
    KTX::ktx                                    # KTX 纹理
    ForgeTinygltf                               # glTF 模型加载
    OpenAL::OpenAL                              # OpenAL 音频
    FreeALUT::alut                              # freealut（官方或 fallback）
    ForgeKmeans                                 # 聚类（INTERFACE）
    ForgeFastNoise                              # 噪音（INTERFACE）
    ForgeStbFont                                # 字体数据（INTERFACE）
)

# ----------------------------------------------------------------------------
# 6. 调试工作目录（VS 生成器专属）
# ----------------------------------------------------------------------------
set_target_properties(Forge PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/output/x64/$<CONFIG>"
)

# ----------------------------------------------------------------------------
# 7. 着色器编译（替代原 bat）
# ----------------------------------------------------------------------------
include(config/cmake/ForgeShader.cmake)
add_dependencies(Forge ForgeShader)
