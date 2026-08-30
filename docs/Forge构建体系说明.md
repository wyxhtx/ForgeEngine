# Forge 构建体系说明（vcpkg + CMake）

> 版本：迁移完成版（2026）
> 作用：ForgeEngine 依赖管理与构建的一站式说明
> 适用：Windows + VS2022 + x64

## 一、体系概览

```
vcpkg.json（依赖清单，12 库）
   ↓ manifest 模式
vcpkg（下载 + 锁定版本 + 编译 + 安装）
   ↓ 装到 build\vcpkg_installed\
CMakeLists.txt（find_package + 构建定义）
   ↓ VS 生成器
build\vs\Forge.sln → Forge.exe
```

- **依赖分三类**：官方 port（9 个，baseline 锁版本）/ 自定义 port（6 个，REF 锁 commit）+ 覆盖官方 port（ktx、tinygltf）/ 系统级 Vulkan SDK（不走 vcpkg）
- **构建工作流**：`cmake --preset vs` → `cmake --build --preset debug` → VS 打开 `build\vs\Forge.sln` F5 调试

## 二、目录结构

```
Forge\
├─ vcpkg.json                 依赖清单（入口，必须根目录）
├─ CMakePresets.json          preset 配置（入口，必须根目录）
├─ CMakeLists.txt             构建定义（入口，必须根目录）
├─ .gitignore
├─ cmd
│  ├─ .forge.ps1
│  └─ config-view.ps1
├─ config\
│  ├─ cmake\
│  │  └─ ForgeShader.cmake    22 个着色器编译规则（替代原 bat）
│  └─ vcpkg\
│     ├─ triplet\x64-windows-static-forge.cmake   全静态 + Debug 宏定制
│     └─ port\                6 个自定义库配方（无源码，仅 vcpkg.json + portfile.cmake）
│        ├─ aarand\
│        ├─ subpar\
│        ├─ sanisizer\
│        ├─ kmeans\
│        ├─ fastnoise2\
│        ├─ forge-stb-font\
│        ├─ ktx\                 覆盖官方（7z解压，修符号链接）
│        └─ tinygltf\            覆盖官方（修正 SHA512）
├─ src\code\                  项目源码（UTF-8，已转码）
├─ src\assets\                资产（shader/图片/模型/音频，原 1_assets 改名）
├─ docs\                      说明文档（本文件）
└─ build\
   ├─ vcpkg_installed\        依赖安装目录（统一位置）
   ├─ vs\                     VS 工程 + exe
   └─ ninja\                  备用构建
```

## 三、配置文件详解

### 3.1 vcpkg.json —— 依赖清单

```json
{
  "name": "forge",
  "version": "1.0.0",
  "dependencies": [
    "glfw3", "glm", "vulkan-headers", "vulkan-memory-allocator",
    { "name": "ktx", "features": ["vulkan"] },
    "tinygltf", "stb", "openal-soft", "freealut",
    "kmeans", "fastnoise2", "forge-stb-font"
  ],
  "builtin-baseline": "61d43ee6f0cc440dc97983d36e0c85e80fe429d4"
}
```

- 前 9 个官方库：版本由 `builtin-baseline`（vcpkg 仓库 commit）锁定
- 后 3 个自定义库：版本由各自 portfile 的 `REF` 锁定
- `ktx` 必须开 `vulkan` feature（否则 ktxvulkan.h API 缺失）
- kmeans 的 3 个依赖（aarand/sanisizer/subpar）在其自身 vcpkg.json 声明，自动递归安装

### 3.2 CMakePresets.json —— 一键配置入口

- `vs` 预设：VS2022 生成器，输出 `build\vs\`，F5 调试全功能
- `ninja` 预设：备用命令行快速构建，输出 `build\ninja\`
- 关键变量：
  - `CMAKE_TOOLCHAIN_FILE`：vcpkg 入口（manifest 模式自动装依赖）
  - `VCPKG_TARGET_TRIPLET`：`x64-windows-static-forge`
  - `VCPKG_OVERLAY_TRIPLETS` / `VCPKG_OVERLAY_PORTS`：自定义目录
  - `VCPKG_INSTALLED_DIR`：`build\vcpkg_installed`（依赖统一位置，不进项目根）

### 3.3 CMakeLists.txt —— 构建定义（7 个区块）

| 区块 | 内容 |
|---|---|
| 1 MSVC 全局 | `_ITERATOR_DEBUG_LEVEL=1`（与 triplet 一致防 LNK2038）+ `/utf-8`（源码已转码） |
| 2 Vulkan SDK | `find_package(Vulkan)` + glslangValidator 查找 |
| 3 vcpkg 官方包 | find_package（VMA 用 VulkanMemoryAllocator） |
| 4 自定义库 | `unofficial-kmeans` / `FastNoise2` / `unofficial-forge-stb-font` |
| 5 主程序 | `add_executable(Forge src/code/vk/Main.cpp)` + 链接全部依赖 |
| 6 调试工作目录 | `VS_DEBUGGER_WORKING_DIRECTORY` = 项目根（资源相对路径可解析） |
| 7 着色器 | include ForgeShader.cmake + `add_dependencies(Forge ForgeShader)` |

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

## 四、自定义库说明书（config\vcpkg\port\）

### 总览

| port | 上游 | 是什么 | 装到 include 下 | 被谁用 |
|---|---|---|---|---|
| aarand | LTLA/aarand | 随机数生成 | `aarand\` | kmeans 依赖 |
| subpar | LTLA/subpar | 并行化工具 | `subpar\` | kmeans 依赖 |
| sanisizer | LTLA/sanisizer | 数值安全检查 | `sanisizer\` | kmeans 依赖 |
| kmeans | libscran/kmeans | k-means 聚类 | `kmeans\` | **项目直接使用**（区块分区） |
| fastnoise2 | Auburn/FastNoise2 | 模块化噪音（需编译） | `FastNoise\` | **项目直接使用**（地形生成） |
| forge-stb-font | KTX-Software | 字体数据 | `stb\` | **项目直接使用**（UI 文字） |

### 三种配方模式

① 纯头文件库（aarand/subpar/sanisizer/kmeans）：`vcpkg_from_git` + `file(COPY include)` + 手写 config
② 需编译库（fastnoise2）：`vcpkg_cmake_configure`（含 FastSIMD 编译单元，非纯 header）+ `vcpkg_cmake_install`
③ 单文件下载（forge-stb-font）：`vcpkg_download_distfile`（raw URL + SHA512，不 clone 大仓库）

### 各库版本锁定（REF = commit SHA / FETCH_REF = tag 或分支）

| port | REF | FETCH_REF |
|---|---|---|
| aarand | e8ba3436d6a713deab943be81b7be0fce609e565 | master |
| subpar | cbcc024b9bb1ba691fae00c545d323c1592cb872 | master |
| sanisizer | 32a024ae1926231ceaa02ed6ff104c6df6d524dc | master |
| kmeans | 0f427abb2a66f5e820a64338543d838a5eca4c99 | master |
| fastnoise2 | 9b75083b87d5e391d43dc0c721d8a9fa51db735a | v0.10.0-alpha |
| forge-stb-font | （distfile，SHA512 校验） | — |

### 项目侧使用

```cpp
// Main.h
#include <kmeans/kmeans.hpp>
#include <FastNoise/FastNoise.h>
#include <FastNoise/Metadata.h>
#include <stb/stb_font_consolas_24_latin1.inl>
```

```cmake
# CMakeLists.txt
find_package(unofficial-kmeans CONFIG REQUIRED)   # 目标 kmeans
find_package(FastNoise2 CONFIG REQUIRED)          # 目标 FastNoise2::FastNoise（注意不是 unofficial-）
find_package(unofficial-forge-stb-font CONFIG REQUIRED)  # 目标 forge-stb-font
find_package(VulkanMemoryAllocator CONFIG REQUIRED)      # 目标 GPUOpen::VulkanMemoryAllocator（VMA）
find_package(FreeALUT CONFIG REQUIRED)                   # 目标 FreeALUT::alut（freealut）
```

## 五、构建命令速查

| 操作 | 命令 |
|---|---|
| 装/更新依赖 | `vcpkg install --triplet x64-windows-static-forge --overlay-ports=config\vcpkg\port --overlay-triplets=config\vcpkg\triplet --x-install-root=build\vcpkg_installed` |
| 配置（首次/改依赖后） | `cmake --preset vs` |
| 编译 Debug | `cmake --build --preset debug` |
| 编译 Release | `cmake --build --preset release` |
| 备用 Ninja 构建 | `cmake --preset ninja` |
| 调试运行 | VS 打开 `build\vs\Forge.sln` → F5（工作目录自动=项目根） |
| 命令行运行 | `cd E:\code\graphicsAPI\vulkan\Forge` + `.\build\vs\bin\output\x64\Debug\Forge.exe` |

## 六、日常维护

| 操作 | 做法 |
|---|---|
| 升级官方库 | `vcpkg x-update-baseline` → 改 baseline → reconfigure |
| 升级自定义库 | 改 portfile 的 REF（新 commit SHA）→ reconfigure |
| 新增官方库 | vcpkg.json 加一行 → reconfigure |
| 新增自定义库 | `config\vcpkg\port\新库名\` 建 2 文件 + vcpkg.json 加一行 |
| 删除库 | vcpkg.json 删一行 → reconfigure |
| 清理依赖重装 | `Remove-Item -Recurse build\vcpkg_installed` → 重跑 install |
| 改着色器 | 直接改 GLSL → 构建自动重编（ForgeShader 依赖跟踪） |

## 七、关键注意事项

1. **port 目录名必须 = 依赖名**（vcpkg 按目录找 port）
2. **REF 必须是 commit SHA**（不能写 tag/branch 名）；FETCH_REF 用 tag/branch（浅克隆 fetch 需要）
3. **fastnoise2 目标名是 `FastNoise2::FastNoise`**，find_package 用 `FastNoise2`
4. **源文件编码 UTF-8**（已全量转码），CMake 用 `/utf-8` 匹配；改源码保持 UTF-8 保存
5. **triplet 的 C/CXX_FLAGS_DEBUG 必须成对设置**（vcpkg 要求）
6. **`.gitignore`**：.vs/ build/ vcpkg_installed/ storage/chunk/ src/assets/shader/ CMakeUserPresets.json *.vcxproj.user（生成物不提交）
7. **Vulkan SDK 是系统级依赖**（不进 vcpkg）：glfw3 探测、校验层、glslangValidator 都靠它
