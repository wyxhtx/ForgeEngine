#===========================================================================
# reflect-data.cmake —— 反射数据生成（构建期）
#
# 链路：.spv ─[reflect-gen 工具]─▶ ${CMAKE_BINARY_DIR}/generated/ShaderReflectData.inc（类型 ＋ 数据）
# 引擎侧只 include 生成头并查表；解析器只存在于工具里（引擎内无解析器）
#===========================================================================
add_executable(reflect-gen "${CMAKE_SOURCE_DIR}/config/tool/reflect-gen/main.cpp")
target_include_directories(reflect-gen PRIVATE "${CMAKE_SOURCE_DIR}/src/code/vk")		#⇒ include "data/VkInfo.h"
target_compile_options(reflect-gen PRIVATE /utf-8 /W3)
target_include_directories(reflect-gen PRIVATE ${Vulkan_INCLUDE_DIRS})		#⇒ vulkan/vulkan_core.h
target_link_libraries(reflect-gen PRIVATE glm::glm)				#⇒ VkInfo.h 用到 glm::vec3
target_compile_definitions(reflect-gen PRIVATE _CRT_SECURE_NO_WARNINGS)	#⇒ getenv

#工具 exe 的真实落点：config/cmake/ForgeIntermediate.targets（Directory.Build.targets）把所有工程的 OutDir
#重定向到 bin/intermediate/<Platform>/<Config>/<工程名>/ ⇒ 不能用 $<TARGET_FILE>（它按 CMake 记录的 TargetPath 解析，会落空）
set(REFLECT_GEN_EXE "${CMAKE_BINARY_DIR}/bin/intermediate/x64/$<CONFIG>/reflect-gen/reflect-gen.exe")
set_target_properties(reflect-gen PROPERTIES		#与 Directory.Build.targets 的实际落点对齐 ⇒ 消除 MSB8012
    RUNTIME_OUTPUT_DIRECTORY_DEBUG          "${CMAKE_BINARY_DIR}/bin/intermediate/x64/Debug/reflect-gen"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE        "${CMAKE_BINARY_DIR}/bin/intermediate/x64/Release/reflect-gen"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/bin/intermediate/x64/RelWithDebInfo/reflect-gen")

set(REFLECT_DATA_INC "${CMAKE_BINARY_DIR}/generated/ShaderReflectData.inc")
file(GLOB_RECURSE REFLECT_SPV_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/assets/shader/*.spv")

add_custom_command(
    OUTPUT  "${REFLECT_DATA_INC}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/generated"
    COMMAND "${REFLECT_GEN_EXE}" --out "${REFLECT_DATA_INC}" --namespace spvStruct --verify ${REFLECT_SPV_FILES}		#--verify：与 SDK spirv-reflect 对拍（缺失则打印跳过）
    DEPENDS ${REFLECT_SPV_FILES} ${REFLECT_GEN_EXE} "${CMAKE_SOURCE_DIR}/config/tool/reflect-gen/main.cpp"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"		#CMake 会把工作目录写进规则；工具路径随之可解析
    COMMENT "生成反射数据：ShaderReflectData.inc（并对拍 spirv-reflect）"
    VERBATIM
)
add_custom_target(reflect-data DEPENDS "${REFLECT_DATA_INC}")
add_dependencies(reflect-data ForgeShader reflect-gen)		#先有 spv ＋ 先有工具
add_dependencies(ForgeEngine reflect-data)			#引擎编译前先有表
target_include_directories(ForgeEngine PRIVATE "${CMAKE_BINARY_DIR}/generated")