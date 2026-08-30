# ============================================================
# Forge 项目调试指令（由 $PROFILE 的 cd 钩子自动加载）
# 命名约定：f- 前缀 + 小写连字符，仅在本目录内有效
# ============================================================

function f-build {           # Release 构建
    cmake --preset vs
    if ($LASTEXITCODE -ne 0) { return $LASTEXITCODE }
    cmake --build --preset release
}
function f-build-debug {     # Debug 构建
    cmake --preset vs
    cmake --build --preset debug
}
function f-run {             # 运行 Release
    .\build\vs\bin\output\x64\Release\Forge.exe
}
function f-run-debug {       # 运行 Debug
    .\build\vs\bin\output\x64\Debug\Forge.exe
}
function f-br {              # 构建 + 运行（最常用）
    f-build
    if ($LASTEXITCODE -eq 0) { f-run }
}
function f-config {          # 打印全部构建配置文件（带行号）
    .\config-view.ps1
}
function f-config-k {        # 只看 ktx 端口相关
    $Pattern = "ktx"; .\config-view.ps1; Remove-Variable Pattern
}
function f-ports {           # 已安装端口列表
    & "$env:VCPKG_ROOT\vcpkg.exe" list --x-install-root=.\build\vcpkg_installed
}
function f-st {              # git 状态 + 最近 5 条
    git status -sb
    git log --oneline -5
}
function f-show {            # 本次改动 diff
    git diff --stat
    git diff --color
}
function f-done {            # 一键提交：f-done "commit message"
    git add -A
    git commit -m "$args"
}
function f-push { git push }
function f-clean {           # 清 VS 工程（保留 vcpkg_installed）
    Remove-Item -Recurse -Force build\vs -ErrorAction SilentlyContinue
    Write-Host "build\vs 已清理（vcpkg_installed 保留）" -ForegroundColor Green
}
function f-clean-ktx {       # 清 ktx 构建残留
    Remove-Item -Recurse -Force "$env:VCPKG_ROOT\buildtrees\ktx" -ErrorAction SilentlyContinue
    Write-Host "buildtrees\ktx 已清理" -ForegroundColor Green
}
function f-code {            # VS Code 打开工程
    if (Get-Command code -ErrorAction SilentlyContinue) { code . } else { start . }
}