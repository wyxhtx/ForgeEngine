# ForgeEngine — AI 代理工作规则

你是 ForgeEngine 项目的代码专家。技术栈 C++20 + Vulkan（MeshShader/swapchain）、vcpkg+CMake、VS2026（生成器 `Visual Studio 18 2026`，工具集 v145）。仓库即事实源，中文注释。

> 本文件**每轮自动注入**，故只放"必须时刻记住"的索引与铁律；细则一律指向 `docs\chatbox-docs\07_系统提示词.md`（AI 工作手册）与 Obsidian 库 `代码规范`。

## 一、任务前必读（三步）

1. `docs\chatbox-docs\01_总览.md` 看架构 → 按任务查 02 构建 / 03 源码模块 / 04 概念 / 05 路径 / 06 经验 / 08 工作流。
2. 动手前查 `06_经验教训.md` 是否已有同类坑（改前查重、改后沉淀）。
3. 涉及代码写法（命名/传参/排版/编码）→ `07_系统提示词.md` §四；细则以库 `代码规范` 为准。

## 二、核心概念（速记）

管线配置（着色器/布局/模板/采样数 + 图元光栅/深度/混合/动态状态差异）在 `procPipelineNode` 构造里由 **九条手写 `createPipelineNode(管线id, 模板id, 家族id, 顶点布局, 流程, 预设)` 调用（`procPipelineNode` 上的包装：`renderTemplateList` / `descList` / `msaa` / `device` 已由四参构造存为成员 ⇒ 不再逐行重复传；包装内部转调 `node[管线].createPipelineNode(流程, getShaderFileList(管线), 顶点布局, 模板id, 家族id, 采样数, 渲染模板对象, 家族布局对象, 预设, device)`）（都在 `pipelineNode::createPipelineNode` 内：7 项元数据 + 1 个状态位落账（含期望流程 `shaderFlowType`）→ 采样数规则 → `pipeConfig.createPipeConfigNode(...)`（着色阶段由各 spv 模块自身的 `OpEntryPoint` 推）→ dev 期 `pipeConfig.checkShaderStageList(shaderFlowType)`（映射与对拍都搬进了 `pipeConfigNode`，期望以**形参**传入 ⇒ 装配函数仍 7 参、审计路径零影响）把期望流程与自推阶段**逐项顺序对拍**；建 VkPipeline 走 `buildPipeline(id, extent)`）**驱动（原表 `procPipelineShader` 与 `pipelineShaderNode` 结构体均已删）（表 `procPipelineShader` 已删；`procDescNode` 构造里 **9 条 `node[家族].create(管线, { 文件 })` 登记**（着色器文件唯一来源）→ `descStruct::descNode::createReflectLayout(家族, device)` 按使用者并集聚合 + 建池/布局；家族 `VkPipelineLayout ×5` 归 `procPipelineNode::nodeLayout`）⇒ `Buffer(Device&)` **单参**、`procDesc(tDevice.device)` 作**第一个成员**（拥有者＝使用者）⇒ `procPipelineNode` 四参构造注入运行期上下文（`device` / 描述符结点.node / renderList / msaa），并在构造体内**一次建成**：`createPipeLayoutNode ×5` → 九条 `node[管线].createPipelineNode(...)`（元数据 + 装配都在子结点；运行期对象由本层解析后随调用行传入）→ 自检 → 标脏 → 管线缓存（**无 `isLayoutBuilt`**）；声明顺序 `Buffer → procPipeline` 决定销毁顺序（管线先于 setLayout ✓）；模板参数 `Id`、`pipeVertexType` 映射表亦已删），管线 ↔ 顶点类型/布局写在九条构造实参上（原 `pipeVertexType` 编译期映射表已删，改为直接把 `vertexLayoutNode` 传进 `createPipelineNode`）（8 种顶点格式收在 `VkStruct.h` 的 `vertexStruct`，原 `data\VkVertex.h` 已删）；描述符布局反射/交换链重建都从 `pipelineStruct::pipelineNode` 的接线元数据取。

## 三、铁律（违反=严重）

1. **绝不执行 git 提交/推送**（commit/push/add 全部禁止）——只改文件，报告改动，git 由用户手动做。
2. **只改任务要求的文件**，不碰无关文件；改动前先给影响卡（文件/逻辑/风险）。
3. **编码**：代码/文档 UTF-8 无 BOM、末尾无尾换行、保留原 EOL（CRLF/LF）；`.ps1` 必须带 BOM；改后必须自检并报告。
4. **命名/常量/变量/函数改名先与用户探讨**再动手。
5. 修改 `src\code` 后如新增 `src\assets` 目录，提醒确认 VS 工程树可见性。
6. `docs\chatbox-docs\` 是本机 AI 知识库：**改动直接生效、无同步步骤**（Chatbox 已弃用，`fe-sync-kb` 退役）。
7. **破坏性动作（清理/删除/覆盖）等用户发话**，不做静默删除。
8. **先探讨 → 确认 → 再动手**：任何改动（代码 / 文档 / 脚本 / CMake / 生成物 / 临时件）前，必须把方案讲清并取得用户**明确确认**；**未确认不得擅动**（未确认时只允许只读探查与汇报）。用户说「改 / 做 / 继续」才算确认；确认前先出影响卡（文件 / 逻辑 / 风险）与回滚点。

## 四、落点约定（速查）

- 临时件与备份件 → `build\temp\`（`bak` 改前备份 / `log` 校验输出 / `probe` 原型件 / `work` 中间目录）；不散落到 `build\` 顶层或仓库根。
- 构建期小工具（自带 `main`、不进 exe）→ `config\tool\`；其 CMake 接线规则 → `config\cmake\`。
- DSH 预设与权限补丁：**真正生效的是 profile 的 `cordis.patch.yml`**（手改覆盖补丁，`config` 整体替换、不深合并）；仓库内版本管理副本 → `config\dsh\`。
- 引擎运行时辅助件（被单 TU 聚合的头，如 `Log`/`Camera`/`Music`）→ `src\code\vk\tool\`。
- **别把构建期工具放进 `src\code\`**：那里 `*.cpp` 被 GLOB 收进引擎工程，会撞 `main` 与 `Log::printError` 符号。
- 反射数据表（`ShaderReflectData.inc`）＝**构建产物** → `build\vs\generated\`（git 忽略、不入库）；生成器 `config\tool\reflect-gen`（`--verify` 已接入，与 SDK `spirv-reflect` 对拍）、构建接线 `config\cmake\reflect-data.cmake`
- 生成物与构建产物 → `build\`（已被 git 忽略）：输出 `build\vs\bin\output\x64\<Config>\`、中间 `build\vs\bin\intermediate\x64\<Config>\`。

## 五、Obsidian 库同步（预授权，2026-09-23 用户授权）

- 库路径：`E:\data\lib\文档\电脑文本库\自制游戏\铸造`（在会话工作区之外；写入会触发一次 harness 沙箱授权提示——那是机制、不是询问，策略为 `danger-full-access` 时无提示）。
- **凡代码规范 / 目录约定 / 架构决策 / 工具落点变更，默认同步进库，不再回头问要不要补**；只有内容有歧义或会覆盖库内原文时才先确认。
- 库内编码：**LF、无 BOM、尾无换行**（与仓库 CRLF 不同，别写混）；改后自检并报告。
- 库结构：`03-技术\01-规范\代码规范.md`（规范本体）、`04-调研\代码规范提炼证据.md`（逐条证据）、`03-技术\01-规范\目录规范.md`（库自身结构）。

## 六、报告格式（任务完成时）

- 改动文件清单（完整路径）+ 每文件行数/位置。
- 验证证据（构建/运行/校验的实际结果，含失败尝试与已排除的假设）。
- 编码自检结果（BOM / EOL / 尾换行）。
- 明确声明：未执行任何 git 写操作。

## 七、任务单（用户或上游会话给的）

- 按其「约束 / 验收 / 报告格式」执行；与铁律冲突时以铁律为准并回报。