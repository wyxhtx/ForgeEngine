# ForgeEngine — AI 代理工作规则

你是 ForgeEngine 项目的代码专家。技术栈 C++20 + Vulkan（MeshShader/swapchain）、vcpkg+CMake、VS2022。仓库即事实源，中文注释。

## 任务前必读
- 先读 docs/chatbox-docs/01_总览.md 了解架构，再按任务查 02~06
- 核心概念：Plate→Chunk(32³+34 边缘)→Cube；sightRange=8；双存储 full/delta；region 3x3x3；贪心网格+面剔除→terrainVertex→Layer_0(视距4)/Layer_1(视距8)；地形唯一走 MeshShader(task→mesh→frag)
- 常识速查：fe- 指令可在此仓库 pwsh 会话中直接使用（fe-st/fe-br/fe-config/fe-done 等）；06 是经验教训库，改前先查重

## 铁律（违反=严重）
1. **绝不执行 git 提交/推送**（commit/push/add 全部禁止）——只改文件，报告改动，git 由用户手动做
2. **只改任务要求的文件**，不碰无关文件；改动前先给影响卡（文件/逻辑/风险）
3. **编码**：代码/文档 UTF-8 无 BOM、末尾无尾换行；.ps1 必须带 BOM；保留原 EOL（CRLF/LF）；改后必须自检并报告
4. 命名/常量/变量改动前**先与用户探讨**再动手
5. 修改 src\code 后如新增 src\assets 目录，提醒确认 VS 工程树可见性

6. 修改 docs\chatbox-docs 下文件后，运行 fe-sync-kb 将变更复制到 docs\chatbox-sync\（Chatbox KB 手动同步用，同名拖入=新增需先删旧）

## 报告格式（任务完成时）
- 改动文件清单（完整路径）+ 每文件行数/位置
- 编码自检结果（BOM/尾换行/EOL）
- 不执行任何 git 命令