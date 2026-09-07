# ForgeEngine 构建体系说明（vcpkg + CMake）

## 一、体系概览

```
vcpkg.json      （依赖清单，13 依赖直列）
   ↓ manifest 模式
vcpkg           （下载 + 锁定版本 + 编译 + 安装；删依赖行 → reconfigure 自动卸载）
   ↓ 装到 build\vcpkg_installed\
CMakeLists.txt  （find_package + find_path + PCH + 构建定义）
   ↓ VS 生成器
build\vs\ForgeEngine.sln → ForgeEngine.exe
```

- **依赖分三类**：官方直用（8 个，baseline 锁版本）/ 覆盖官方（3 个：ktx、tinygltf、freetype，overlay 修正上游 port）/ 纯自定义（6 个：aarand、subpar、sanisizer、kmeans、fastnoise2、msdf-atlas-gen，REF 锁 commit）/ 系统级 Vulkan SDK（不走 vcpkg）
- **配置三档**：Debug / Release / RelWithDebInfo——RelWithDebInfo 重定义为快速日常档（/DNDEBUG /Od /Zi，链接 /DEBUG /INCREMENTAL，带 PDB 可断点）
- **构建工作流**：`cmake --preset vs` → `cmake --build --preset debug` → VS 打开 `build\vs\ForgeEngine.sln` F5 调试；日常迭代用 RelWithDebInfo 配置

## 二、目录结构

```
ForgeEngine\
├─ build\
│  ├─ ninja\                        ninja 工程
│  ├─ vcpkg_installed\              依赖安装目录
│  └─ vs\                           VS 工程 + exe
├─ cmd\
│  └─ forgeEngineCmdList.ps1        调试指令文件，项目路径下自动加载
├─ config\
│  ├─ cmake\
│  │  └─ ForgeShader.cmake          着色器编译规则
│  └─ vcpkg\
│     ├─ port\                      9 个端口   （6 纯自定义 REF 锁 commit + 3 覆盖官方）
│     │  ├─ aarand\                 随机数     （kmeans 依赖）
│     │  ├─ fastnoise2\             噪音       （需编译 FastSIMD）
│     │  ├─ freetype\               覆盖官方   （补丁集：UWP/brotli-static/bzip2/fix-exports）
│     │  ├─ kmeans\                 聚类       （区块并行分区）
│     │  ├─ ktx\                    覆盖官方   （7z 解压，修符号链接）
│     │  ├─ msdf-atlas-gen\         MSDF 图集  （纯自定义，外链官方 msdfgen）
│     │  ├─ sanisizer\              数据清洗   （kmeans 依赖）
│     │  ├─ subpar\                 并行工具   （kmeans 依赖）
│     │  └─ tinygltf\               覆盖官方   （修正 SHA512）
│     └─ triplet\
│        └─ x64-windows-static-forgeengine.cmake   全静态 + Debug 宏定制
├─ docs\
│  ├─ ForgeEngine构建体系说明.md     本文件     （构建/依赖/维护全说明）
│  ├─ 端口说明.md                    自定义端口速查
│  └─ 调试指令速查.md                fe- 指令表
├─ src\assets\
│  ├─ font\                         SourceHanSansSC（OTF + OFL 授权，MSDF 字形源）
│  ├─ image\                        ktx2                （入库）
│  ├─ image_src\                    png/jpeg + 转换bat  （入库）
│  ├─ model\                        glTF                （入库）
│  ├─ music\                        wav                 （入库）
│  ├─ shader\                       spv                 （忽略，构建现编）
│  └─ shader_src\                   GLSL                （入库）
├─ src\code\
│  ├─ forge\                        游戏层
│  └─ vk\                           Vulkan 渲染层
├─ .gitignore
├─ CMakeLists.txt                   构建定义
├─ CMakePresets.json                preset 配置
├─ README.md
└─ vcpkg.json                       依赖清单
```

## 三、配置文件详解

### 3.1 vcpkg.json —— 依赖清单

```json
{
  "name": "forgeengine",
  "version": "1.0.0",
  "description": "ForgeEngine",
  "dependencies": [
    "glfw3",
    "glm",
    "vulkan-headers",
    "vulkan-memory-allocator",
    { "name": "ktx", "features": ["vulkan"] },
    "tinygltf",
    "stb",
    { "name": "msdfgen", "default-features": false, "features": ["extensions"] },
    "openal-soft",
    "freealut",
    "kmeans",
    "fastnoise2",
    "msdf-atlas-gen"
  ],
  "builtin-baseline": "61d43ee6f0cc440dc97983d36e0c85e80fe429d4"
}
```

- 官方直用 8 个：版本由 `builtin-baseline`（vcpkg 仓库 commit）锁定
- 覆盖官方 3（ktx/tinygltf/freetype）：overlay 修正上游 port，版本/补丁见各自 vcpkg.json/portfile
- 纯自定义 6：版本由各自 portfile 的 `REF` 锁定（含直列 kmeans/fastnoise2/msdf-atlas-gen）
- `ktx` 必须开 `vulkan` feature（否则 ktxvulkan.h API 缺失）
- `msdfgen` 开 `default-features:false + extensions`（外链给 msdf-atlas-gen 用）
- 间接依赖自动递归：kmeans→aarand/sanisizer/subpar；msdf-atlas-gen→msdfgen[extensions]+freetype；freetype→brotli/bzip2/libpng/zlib
- 删库 = vcpkg.json 删一行 → reconfigure（manifest 自动卸载已装包，实测；forgeengine-stb-font 已于 P2 后移除）

### 3.2 CMakePresets.json —— 一键配置入口

- `vs` 预设：VS2022 生成器，输出 `build\vs\`，F5 调试全功能
- `ninja` 预设：备用命令行快速构建，输出 `build\ninja\`
- 关键变量：
  - `CMAKE_TOOLCHAIN_FILE`：vcpkg 入口（manifest 模式自动装依赖）
  - `VCPKG_TARGET_TRIPLET`：`x64-windows-static-forgeengine`
  - `VCPKG_OVERLAY_TRIPLETS` / `VCPKG_OVERLAY_PORTS`：自定义目录
  - `VCPKG_INSTALLED_DIR`：`build\vcpkg_installed`（依赖统一位置，不进项目根）

### 3.3 CMakeLists.txt —— 构建定义

顶部：C++20 标准 + CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo" + RelWithDebInfo flags（快速日常档）

| 区块 | 内容 |
|---|---|
| 1 MSVC 全局 | `/utf-8 /W3`（源码已转码；_ITERATOR_DEBUG_LEVEL 在 triplet 设，防 LNK2038） |
| 2 Vulkan SDK | `find_package(Vulkan)` + glslangValidator 查找 |
| 3 find_package 群 | 官方 CONFIG（glfw3/glm/VMA/Ktx/OpenAL/ALUT/msdfgen）+ 自产 CONFIG（msdf-atlas-gen）；tinygltf 官方无 config → find_path 封装 ForgeTinygltf |
| 4 自定义手工目标 | kmeans（find_path → ForgeKmeans）、fastnoise2（find_path+find_library → ForgeFastNoise，Debug 链 FastNoiseD.lib + FASTNOISE_STATIC_LIB） |
| 5 主程序 | `add_executable(ForgeEngine src/code/vk/Main.cpp)`（单 TU）+ target_precompile_headers(External.h)（PCH）+ /bigobj + UNICODE + 非 Debug 撤 _DEBUG + Release /Zi /DEBUG（PDB）+ 解决方案显示分组（GLOB）+ 链接全部依赖（含 msdfgen::msdfgen-full、msdf-atlas-gen::msdf-atlas-gen） |
| 6 调试工作目录 | `VS_DEBUGGER_WORKING_DIRECTORY` = 项目根（资源相对路径可解析） |
| 7 着色器 | include ForgeShader.cmake + `add_dependencies(ForgeEngine ForgeShader)` |

### 3.4 triplet —— 库编译的统一配置

```cmake
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE static)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_C_FLAGS_DEBUG "${VCPKG_C_FLAGS_DEBUG} /D_ITERATOR_DEBUG_LEVEL=1")
set(VCPKG_CXX_FLAGS_DEBUG "${VCPKG_CXX_FLAGS_DEBUG} /D_ITERATOR_DEBUG_LEVEL=1")
```

- 全静态：无 DLL，删除原 SetDllDirectoryA 和 dll 目录
- Debug 宏与 Main.h 一致，避免 LNK2038（C/CXX 必须成对设置）

### 3.5 解决方案显示分组（纯显示，不影响编译）

CMakeLists 用 GLOB + source_group 把工程文件分组显示在 VS 解决方案资源管理器：

| 组 | 内容 | 方式 |
|---|---|---|
| src\code | 源码树（按磁盘目录）| GLOB_RECURSE + TREE |
| src\assets | 资产目录树（含 font\）| GLOB_RECURSE + TREE |
| config | ForgeShader/triplet/9 端口 + 根 4 文件 | GLOB + TREE / FILES |
| docs | 3 份 md | GLOB |
| cmd | forgeEngineCmdList.ps1 | GLOB |

全部经 target_sources 加入（非源码不编译），GLOB 带 CONFIGURE_DEPENDS（新增文件自动刷新）。

## 四、自定义库说明书（config\vcpkg\port\）

### 总览

| port | 上游 | 是什么 | 装到 include 下 | 被谁用 |
|---|---|---|---|---|
| aarand | LTLA/aarand | 随机数生成 | `aarand\` | kmeans 依赖 |
| subpar | LTLA/subpar | 并行化工具 | `subpar\` | kmeans 依赖 |
| sanisizer | LTLA/sanisizer | 数值安全检查 | `sanisizer\` | kmeans 依赖 |
| kmeans | libscran/kmeans | k-means 聚类 | `kmeans\` | **项目直接使用**（区块分区） |
| fastnoise2 | Auburn/FastNoise2 | 模块化噪音（需编译） | `FastNoise\` | **项目直接使用**（地形生成） |
| msdf-atlas-gen | Chlumsky/msdf-atlas-gen | MSDF 图集生成（外链官方 msdfgen） | `msdf-atlas-gen\` | **项目直接使用**（UI 文本字形图集） |
| freetype | freetype/freetype | 字体轮廓解析（覆盖官方） | `freetype\` | msdf-atlas-gen 依赖（间接） |
| ktx | KTX-Software/KTX | KTX2 纹理（覆盖官方） | `ktx\` | **项目直接使用**（纹理） |
| tinygltf | syoyo/tinygltf | glTF 模型加载（覆盖官方） | `tinygltf\` | **项目直接使用**（模型） |

> msdfgen 走官方 port（非 overlay）：msdf-atlas-gen 与 vcpkg.json 直列均引它；forgeengine-stb-font（旧位图字库）已于 P2 MSDF 重构后删除。

### 三种配方模式

```
① 纯头文件库（aarand/subpar/sanisizer/kmeans）：`vcpkg_from_git` + `file(COPY include)` + 手写 config
② 需编译库（fastnoise2/msdf-atlas-gen）：`vcpkg_from_git` + `vcpkg_cmake_configure` + `vcpkg_cmake_install`（msdf-atlas-gen 另做 config_fixup）
③ 覆盖官方（ktx/tinygltf/freetype）：改上游 port（7z 解压修符号链接 / 修 SHA512 / 补丁集）
```

### 各库版本锁定（REF = commit SHA / FETCH_REF = tag 或分支）

| port | REF | FETCH_REF |
|---|---|---|
| aarand | e8ba3436d6a713deab943be81b7be0fce609e565 | master |
| subpar | cbcc024b9bb1ba691fae00c545d323c1592cb872 | master |
| sanisizer | 32a024ae1926231ceaa02ed6ff104c6df6d524dc | master |
| kmeans | 0f427abb2a66f5e820a64338543d838a5eca4c99 | master |
| fastnoise2 | 9b75083b87d5e391d43dc0c721d8a9fa51db735a | v0.10.0-alpha |
| msdf-atlas-gen | 6148900d59423059bafde2f51a0cb303184404bd | — |
| freetype | （覆盖官方：vcpkg_from_github VER-2.13.3 tag + 5 补丁） | — |
| ktx / tinygltf | （覆盖官方：改上游 port） | — |

### 项目侧使用

```cpp
// External.h（PCH）
#include <kmeans/kmeans.hpp>
#include <FastNoise/FastNoise.h>
#include <FastNoise/Metadata.h>
#include <msdfgen.h>
#include <msdfgen-ext.h>
#include <msdf-atlas-gen/msdf-atlas-gen.h>
// Main.h 仅含 VMA/tinygltf 的 *_IMPLEMENTATION（不进 PCH，只被唯一 TU include）
```

```cmake
# CMakeLists.txt
find_package(msdfgen CONFIG REQUIRED)               # MSDF 距离场（官方 port，CONFIG）
find_package(msdf-atlas-gen CONFIG REQUIRED)        # MSDF 图集（纯自定义，CONFIG）
find_path(KMEANS_INCLUDE_DIRS "kmeans/Details.hpp")             # 目标 ForgeKmeans（INTERFACE）
find_path(FASTNOISE2_INCLUDE_DIR FastNoise/FastNoise.h)         # 目标 ForgeFastNoise（INTERFACE IMPORTED）
find_library(FASTNOISE2_LIBRARY_RELEASE NAMES FastNoise)
find_library(FASTNOISE2_LIBRARY_DEBUG NAMES FastNoiseD)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)             # 目标 GPUOpen::VulkanMemoryAllocator（VMA）
find_package(FreeALUT CONFIG REQUIRED)                          # 目标 FreeALUT::alut（freealut）
```

## 五、构建命令速查

| 操作 | 命令 |
|---|---|
| 装/更新依赖 | `vcpkg install --triplet x64-windows-static-forgeengine --overlay-ports=config\vcpkg\port --overlay-triplets=config\vcpkg\triplet --x-install-root=build\vcpkg_installed` |
| 配置（首次/改依赖后） | `cmake --preset vs` |
| 编译 Debug | `cmake --build --preset debug` |
| 编译 Release | `cmake --build --preset release` |
| 备用 Ninja 构建 | `cmake --preset ninja` |
| 调试运行 | VS 打开 `build\vs\ForgeEngine.sln` → F5（工作目录自动=项目根） |
| 命令行运行 | `cd E:\code\graphicsAPI\vulkan\ForgeEngine` + `.\build\vs\bin\output\x64\Debug\ForgeEngine.exe` |

## 六、日常维护

| 操作 | 做法 |
|---|---|
| 升级官方库 | `vcpkg x-update-baseline` → 改 baseline → reconfigure |
| 升级自定义库 | 改 portfile 的 REF（新 commit SHA）→ reconfigure |
| 新增官方库 | vcpkg.json 加一行 → reconfigure |
| 新增自定义库 | `config\vcpkg\port\新库名\` 建 2 文件 + vcpkg.json 加一行 |
| 删除库 | vcpkg.json 删一行 → reconfigure（manifest 自动卸载已装包，实测） |
| 清理依赖重装 | `Remove-Item -Recurse build\vcpkg_installed` → 重跑 install |
| 改着色器 | 直接改 GLSL → 构建自动重编（ForgeShader 依赖跟踪） |

## 七、关键注意事项

1. **port 目录名必须 = 依赖名**（vcpkg 按目录找 port）
2. **REF 必须是 commit SHA**（不能写 tag/branch 名）；FETCH_REF 用 tag/branch（浅克隆 fetch 需要）
3. **fastnoise2**：项目用 find_path/find_library 手工目标 `ForgeFastNoise`（端口亦生成 FastNoise2Config，可改 find_package）
4. **PCH=External.h**：只放系统+三方稳定头；带 *_IMPLEMENTATION 的头（VMA/tinygltf）必须在 Main.h 且只能被唯一 TU include，**勿入 PCH**（否则 LNK2005）
5. **源文件编码 UTF-8**（已全量转码），CMake 用 `/utf-8` 匹配；改源码保持 UTF-8 保存
6. **triplet 的 C/CXX_FLAGS_DEBUG 必须成对设置**（vcpkg 要求）
7. **`.gitignore`**：.vs/ build/ vcpkg_installed/ storage/chunk/ src/assets/shader/ CMakeUserPresets.json *.vcxproj.user（生成物不提交）
8. **Vulkan SDK 是系统级依赖**（不进 vcpkg）：glfw3 探测、校验层、glslangValidator 都靠它