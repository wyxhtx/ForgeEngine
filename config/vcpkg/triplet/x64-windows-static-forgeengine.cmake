# ============================================================
# ForgeEngine 专属 vcpkg triplet（x64-windows-static-forgeengine）
#
#  作用：vcpkg 编译 22 个依赖库时遵循的规格
#  指名：CMakePresets.json 的 VCPKG_TARGET_TRIPLET
#  定位：官方 x64-windows-static + Debug 宏定制（配 Main.h 防 LNK2038）
# ============================================================

set(VCPKG_TARGET_ARCHITECTURE x64)      # 目标架构：x64（只编 x64 版库）

set(VCPKG_CRT_LINKAGE static)           # C 运行库(CRT)：静态（/MT）→ 库不依赖 msvcp*.dll
set(VCPKG_LIBRARY_LINKAGE static)       # 库本身：静态库 .lib（不产 DLL）→ 全静态三件套之二

# Debug 宏定制：vcpkg 编库（Debug）时强制 _ITERATOR_DEBUG_LEVEL=1
#   与 Main.h 的 #define _ITERATOR_DEBUG_LEVEL 1 保持一致 → exe 与库同级别 → 不报 LNK2038
#   只加在 DEBUG：Release 无 _DEBUG，STL 默认 0，无需改
#   C/CXX 必须成对设置（vcpkg 要求）
set(VCPKG_C_FLAGS_DEBUG   "${VCPKG_C_FLAGS_DEBUG}   /D_ITERATOR_DEBUG_LEVEL=1")   # C 编译器（Debug）
set(VCPKG_CXX_FLAGS_DEBUG "${VCPKG_CXX_FLAGS_DEBUG} /D_ITERATOR_DEBUG_LEVEL=1")   # C++ 编译器（Debug）
