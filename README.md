# ForgeEngine（Forge）

无限体素世界 3D 沙盒游戏引擎。C++20 + Vulkan（VK_EXT_MESH_SHADER）。

## 构建
cmake --preset vs && cmake --build --preset release
.\build\vs\bin\output\x64\Release\ForgeEngine.exe

## 调试指令
cd 进项目自动加载（cmd\forgeengine.ps1），完整指令表见 docs/调试指令速查.md。
常用：`fe-br` 构建+运行 · `fe-config` 配置速览 · `fe-st` git 状态

## 目录
- config/ 构建配置（cmake/vcpkg 端口）
- cmd/ 调试指令脚本（cd 进项目自动加载）
- docs/ 文档（构建体系/调试速查/端口说明）
- src/assets 资源（shader/image/model/music + *_src 源）
- src/code 源码（forge 游戏层 / vk 渲染层）
- build/ 构建产物（忽略）· storage/ 运行数据（忽略）
