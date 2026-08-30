# ============================================================
# ForgeEngine 项目调试指令（由 $PROFILE 的 cd 钩子自动加载）
# 命名约定：fe- 前缀 + 小写连字符，仅在本目录内有效
# ============================================================

function global:fe-build {           # Release 构建
    cmake --preset vs
    if ($LASTEXITCODE -ne 0) { return $LASTEXITCODE }
    cmake --build --preset release
}
function global:fe-build-debug {     # Debug 构建
    cmake --preset vs
    cmake --build --preset debug
}
function global:fe-run {             # 运行 Release
    .\build\vs\bin\output\x64\Release\ForgeEngine.exe
}
function global:fe-run-debug {       # 运行 Debug
    .\build\vs\bin\output\x64\Debug\ForgeEngine.exe
}
function global:fe-br {              # 构建 + 运行（最常用）
    fe-build
    if ($LASTEXITCODE -eq 0) { fe-run }
}
function global:fe-config {          # 打印全部构建配置文件（带行号）
    .\cmd\config-view.ps1
}
function global:fe-config-k {        # 只看 ktx 端口相关
    $Pattern = "ktx"; .\cmd\config-view.ps1; Remove-Variable Pattern
}
function global:fe-ports {           # 已安装端口列表
    & "$env:VCPKG_ROOT\vcpkg.exe" list --x-install-root=.\build\vcpkg_installed
}
function global:fe-st {              # git 状态 + 最近 5 条
    git status -sb
    git log --oneline -5
}
function global:fe-show {            # 本次改动 diff
    git diff --stat
    git diff --color
}
function global:fe-done {            # 一键提交：fe-done "commit message"
    git add -A
    git commit -m "$args"
}
function global:fe-push { git push }
function global:fe-clean {           # 清 VS 工程（保留 vcpkg_installed）
    Remove-Item -Recurse -Force build\vs -ErrorAction SilentlyContinue
    Write-Host "build\vs 已清理（vcpkg_installed 保留）" -ForegroundColor Green
}
function global:fe-clean-ktx {       # 清 ktx 构建残留
    Remove-Item -Recurse -Force "$env:VCPKG_ROOT\buildtrees\ktx" -ErrorAction SilentlyContinue
    Write-Host "buildtrees\ktx 已清理" -ForegroundColor Green
}
function global:fe-code {            # VS Code 打开工程
    if (Get-Command code -ErrorAction SilentlyContinue) { code . } else { start . }
}