# ============================================================================
#  ForgeShader：编译全部管线着色器（GLSL → SPIR-V）
#  替代旧工程 0_shader_build 下的 *.bat 脚本
# ============================================================================
set(FORGE_SHADER_BUILD_DIR "${CMAKE_SOURCE_DIR}/src/assets/0_shader_build")
set(FORGE_SHADER_OUT_DIR   "${CMAKE_SOURCE_DIR}/src/assets/0_shader")
set(FORGE_SHADER_LIST "")

function(forge_compile_shader INPUT_FILE OUTPUT_FILE)
  get_filename_component(OUT_DIR "${OUTPUT_FILE}" DIRECTORY)
  file(MAKE_DIRECTORY "${OUT_DIR}")
  add_custom_command(
    OUTPUT  "${OUTPUT_FILE}"
    COMMAND "${GLSLANG_VALIDATOR}" -V -e main "${INPUT_FILE}" -o "${OUTPUT_FILE}"
    DEPENDS "${INPUT_FILE}"
    COMMENT "编译着色器：${INPUT_FILE}"
    VERBATIM)
  list(APPEND FORGE_SHADER_LIST "${OUTPUT_FILE}")
  set(FORGE_SHADER_LIST "${FORGE_SHADER_LIST}" PARENT_SCOPE)
endfunction()

# dsScence
forge_compile_shader(${FORGE_SHADER_BUILD_DIR}/dsScence/dsScence.vert ${FORGE_SHADER_OUT_DIR}/dsScence/dsScenceVert.spv)
forge_compile_shader(${FORGE_SHADER_BUILD_DIR}/dsScence/dsScence.frag ${FORGE_SHADER_OUT_DIR}/dsScence/dsScenceFrag.spv)

# mine
forge_compile_shader(${FORGE_SHADER_BUILD_DIR}/mine/mine.vert ${FORGE_SHADER_OUT_DIR}/mine/mineVert.spv)
forge_compile_shader(${FORGE_SHADER_BUILD_DIR}/mine/mine.frag ${FORGE_SHADER_OUT_DIR}/mine/mineFrag.spv)

# model
forge_compile_shader(${FORGE_SHADER_BUILD_DIR}/model/pbrDeferred.vert ${FORGE_SHADER_OUT_DIR}/model/pbrDeferredVert.spv)
forge_compile_shader(${FORGE_SHADER_BUILD_DIR}/model/pbrDeferred.frag ${FORGE_SHADER_OUT_DIR}/model/pbrDeferredFrag.spv)
forge_compile_shader(${FORGE_SHADER_BUILD_DIR}/model/pbrForward.vert  ${FORGE_SHADER_OUT_DIR}/model/pbrForwardVert.spv)
forge_compile_shader(${FORGE_SHADER_BUILD_DIR}/model/pbrForward.frag  ${FORGE_SHADER_OUT_DIR}/model/pbrForwardFrag.spv)

# osMirror
forge_compile_shader(${FORGE_SHADER_BUILD_DIR}/osMirror/osMirror.vert ${FORGE_SHADER_OUT_DIR}/osMirror/osMirrorVert.spv)
forge_compile_shader(${FORGE_SHADER_BUILD_DIR}/osMirror/osMirror.frag ${FORGE_SHADER_OUT_DIR}/osMirror/osMirrorFrag.spv)

# osScence
forge_compile_shader(${FORGE_SHADER_BUILD_DIR}/osScence/osScence.vert ${FORGE_SHADER_OUT_DIR}/osScence/osScenceVert.spv)
forge_compile_shader(${FORGE_SHADER_BUILD_DIR}/osScence/osScence.frag ${FORGE_SHADER_OUT_DIR}/osScence/osScenceFrag.spv)

# shadow
forge_compile_shader(${FORGE_SHADER_BUILD_DIR}/shadow/shadow.vert ${FORGE_SHADER_OUT_DIR}/shadow/shadowVert.spv)
forge_compile_shader(${FORGE_SHADER_BUILD_DIR}/shadow/shadow.frag ${FORGE_SHADER_OUT_DIR}/shadow/shadowFrag.spv)

# skybox
forge_compile_shader(${FORGE_SHADER_BUILD_DIR}/skybox/skybox.vert ${FORGE_SHADER_OUT_DIR}/skybox/skyboxVert.spv)
forge_compile_shader(${FORGE_SHADER_BUILD_DIR}/skybox/skybox.frag ${FORGE_SHADER_OUT_DIR}/skybox/skyboxFrag.spv)

# terrain
forge_compile_shader(${FORGE_SHADER_BUILD_DIR}/terrain/terrainDeferred.vert ${FORGE_SHADER_OUT_DIR}/terrain/terrainDeferredVert.spv)
forge_compile_shader(${FORGE_SHADER_BUILD_DIR}/terrain/terrainDeferred.frag ${FORGE_SHADER_OUT_DIR}/terrain/terrainDeferredFrag.spv)
forge_compile_shader(${FORGE_SHADER_BUILD_DIR}/terrain/terrainForward.vert  ${FORGE_SHADER_OUT_DIR}/terrain/terrainForwardVert.spv)
forge_compile_shader(${FORGE_SHADER_BUILD_DIR}/terrain/terrainForward.frag  ${FORGE_SHADER_OUT_DIR}/terrain/terrainForwardFrag.spv)

# uiText
forge_compile_shader(${FORGE_SHADER_BUILD_DIR}/uiText/uiText.vert ${FORGE_SHADER_OUT_DIR}/uiText/uiTextVert.spv)
forge_compile_shader(${FORGE_SHADER_BUILD_DIR}/uiText/uiText.frag ${FORGE_SHADER_OUT_DIR}/uiText/uiTextFrag.spv)

add_custom_target(ForgeShader DEPENDS ${FORGE_SHADER_LIST})