# ============================================================================
#  ForgeShader：编译全部管线着色器（GLSL → SPIR-V）
#
#  输入：src\assets\shader_src\<管线>\*.vert / *.frag   （GLSL 源，入库）
#  输出：src\assets\shader\<管线>\*Vert.spv / *Frag.spv （字节码，运行时加载）
#  工具：glslangValidator（Vulkan SDK，CMakeLists 已 find_program）
#  命名：vert → xxxVert.spv，frag → xxxFrag.spv（管线内同名）
#  触发：构建 ForgeEngine 前自动执行（CMakeLists add_dependencies）
#  增量：只重编"比源文件旧/缺失"的 spv（add_custom_command 依赖跟踪）
# ============================================================================

# 输入/输出根目录（相对项目根，与 fileInfo 资源路径一致）
set(FORGE_SHADER_BUILD_DIR "${CMAKE_SOURCE_DIR}/src/assets/shader_src")
set(FORGE_SHADER_OUT_DIR   "${CMAKE_SOURCE_DIR}/src/assets/shader")
set(FORGE_SHADER_LIST "")                       # 累积全部 spv 输出（供末尾目标依赖）

# ----------------------------------------------------------------------------
# 工具函数：注册一条"GLSL → SPV"编译规则
#   forge_compile_shader(<输入.vert/.frag> <输出.spv>)
#   每调一次 = add_custom_command 一条规则；CMake 自动做增量（源新于产物才重编）
# ----------------------------------------------------------------------------
function(forge_compile_shader INPUT_FILE OUTPUT_FILE)
  get_filename_component(OUT_DIR "${OUTPUT_FILE}" DIRECTORY)   # 取输出目录（shader\<管线>）
  file(MAKE_DIRECTORY "${OUT_DIR}")                            # 目录不存在则创建

  add_custom_command(
    OUTPUT  "${OUTPUT_FILE}"                                   # 本规则产物
    COMMAND "${GLSLANG_VALIDATOR}" -V -e main "${INPUT_FILE}" -o "${OUTPUT_FILE}"   # glslangValidator 编译
    DEPENDS "${INPUT_FILE}"                                    # 依赖源文件（变新才重跑）
    COMMENT "编译着色器：${INPUT_FILE}"
    VERBATIM)                                                  # 参数不做二次转义（防引号/分号坑）

  list(APPEND FORGE_SHADER_LIST "${OUTPUT_FILE}")              # 把 spv 收进列表
  set(FORGE_SHADER_LIST "${FORGE_SHADER_LIST}" PARENT_SCOPE)   # 函数内改的变量回传外层
endfunction()

# ============================================================================
# 22 条编译规则（9 条管线；model/terrain 各 4 条，其余 2 条）
# 分组 = 渲染管线名（与 shader_src/<管线>/ 目录对应）
# ============================================================================

# ---- dsScence：延迟场景主渲染（deferred 前向混合场景）----
forge_compile_shader(${FORGE_SHADER_BUILD_DIR}/dsScence/dsScence.vert ${FORGE_SHADER_OUT_DIR}/dsScence/dsScenceVert.spv)
forge_compile_shader(${FORGE_SHADER_BUILD_DIR}/dsScence/dsScence.frag ${FORGE_SHADER_OUT_DIR}/dsScence/dsScenceFrag.spv)

# ---- mine：挖矿/方块高亮（交互提示）----
forge_compile_shader(${FORGE_SHADER_BUILD_DIR}/mine/mine.vert ${FORGE_SHADER_OUT_DIR}/mine/mineVert.spv)
forge_compile_shader(${FORGE_SHADER_BUILD_DIR}/mine/mine.frag ${FORGE_SHADER_OUT_DIR}/mine/mineFrag.spv)

# ---- model：模型 PBR（droid/temple 等 glTF）----
forge_compile_shader(${FORGE_SHADER_BUILD_DIR}/model/pbrDeferred.vert ${FORGE_SHADER_OUT_DIR}/model/pbrDeferredVert.spv)
forge_compile_shader(${FORGE_SHADER_BUILD_DIR}/model/pbrDeferred.frag ${FORGE_SHADER_OUT_DIR}/model/pbrDeferredFrag.spv)
forge_compile_shader(${FORGE_SHADER_BUILD_DIR}/model/pbrForward.vert  ${FORGE_SHADER_OUT_DIR}/model/pbrForwardVert.spv)
forge_compile_shader(${FORGE_SHADER_BUILD_DIR}/model/pbrForward.frag  ${FORGE_SHADER_OUT_DIR}/model/pbrForwardFrag.spv)

# ---- osMirror：镜面反射----
forge_compile_shader(${FORGE_SHADER_BUILD_DIR}/osMirror/osMirror.vert ${FORGE_SHADER_OUT_DIR}/osMirror/osMirrorVert.spv)
forge_compile_shader(${FORGE_SHADER_BUILD_DIR}/osMirror/osMirror.frag ${FORGE_SHADER_OUT_DIR}/osMirror/osMirrorFrag.spv)

# ---- osScence：正向场景（透明/天空盒等 forward 通道）----
forge_compile_shader(${FORGE_SHADER_BUILD_DIR}/osScence/osScence.vert ${FORGE_SHADER_OUT_DIR}/osScence/osScenceVert.spv)
forge_compile_shader(${FORGE_SHADER_BUILD_DIR}/osScence/osScence.frag ${FORGE_SHADER_OUT_DIR}/osScence/osScenceFrag.spv)

# ---- shadow：阴影贴图（深度 pass）----
forge_compile_shader(${FORGE_SHADER_BUILD_DIR}/shadow/shadow.vert ${FORGE_SHADER_OUT_DIR}/shadow/shadowVert.spv)
forge_compile_shader(${FORGE_SHADER_BUILD_DIR}/shadow/shadow.frag ${FORGE_SHADER_OUT_DIR}/shadow/shadowFrag.spv)

# ---- skybox：天空盒----
forge_compile_shader(${FORGE_SHADER_BUILD_DIR}/skybox/skybox.vert ${FORGE_SHADER_OUT_DIR}/skybox/skyboxVert.spv)
forge_compile_shader(${FORGE_SHADER_BUILD_DIR}/skybox/skybox.frag ${FORGE_SHADER_OUT_DIR}/skybox/skyboxFrag.spv)

# ---- terrain：地形（deferred + forward 双通道，含 mesh shader 任务）----
forge_compile_shader(${FORGE_SHADER_BUILD_DIR}/terrain/terrainDeferred.vert ${FORGE_SHADER_OUT_DIR}/terrain/terrainDeferredVert.spv)
forge_compile_shader(${FORGE_SHADER_BUILD_DIR}/terrain/terrainDeferred.frag ${FORGE_SHADER_OUT_DIR}/terrain/terrainDeferredFrag.spv)
forge_compile_shader(${FORGE_SHADER_BUILD_DIR}/terrain/terrainForward.vert  ${FORGE_SHADER_OUT_DIR}/terrain/terrainForwardVert.spv)
forge_compile_shader(${FORGE_SHADER_BUILD_DIR}/terrain/terrainForward.frag  ${FORGE_SHADER_OUT_DIR}/terrain/terrainForwardFrag.spv)

# ---- uiText：UI 文字----
forge_compile_shader(${FORGE_SHADER_BUILD_DIR}/uiText/uiText.vert ${FORGE_SHADER_OUT_DIR}/uiText/uiTextVert.spv)
forge_compile_shader(${FORGE_SHADER_BUILD_DIR}/uiText/uiText.frag ${FORGE_SHADER_OUT_DIR}/uiText/uiTextFrag.spv)

# ============================================================================
# 着色器编译目标：依赖全部 spv（构建 ForgeEngine 前由 add_dependencies 触发）
# ============================================================================
add_custom_target(ForgeShader DEPENDS ${FORGE_SHADER_LIST})