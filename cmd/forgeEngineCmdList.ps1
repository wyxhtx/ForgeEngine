# ============================================================
# ForgeEngine 项目调试指令（forgeEngineCmdList.ps1）
#
#  加载方式：由 $PROFILE 的 cd 钩子自动加载（cd 进项目→加载，离开→卸载）
#  命名约定：fe- = ForgeEngine 缩写；fe-xxx = 一条调试指令
#  分组速览：
#    构建  fe-build / fe-build-debug / fe-br
#    运行  fe-run / fe-run-debug
#    查看  fe-config / fe-config-k / fe-ports / fe-st / fe-show
#    提交  fe-done / fe-push
#    清理  fe-clean / fe-clean-ktx
#    工具  fe-code
# ============================================================



# ============================================================
# 一、构建（cmake 配置 + 编译）
# ============================================================

# Release 构建（fe-br 的前半段）
function global:fe-build {
    cmake --preset vs
    if ($LASTEXITCODE -ne 0) { return $LASTEXITCODE }   # 配置失败则停（不进入编译）
    cmake --build --preset release
}

# Debug 构建
function global:fe-build-debug {
    cmake --preset vs
    cmake --build --preset debug
}

# 构建 + 运行（最常用：一键出 Release 并启动）
function global:fe-br {
    fe-build
    if ($LASTEXITCODE -eq 0) { fe-run }                 # 构建成功才运行
}


# ============================================================
# 二、运行（直接启动编译好的 exe）
# ============================================================

# 运行 Release 版
function global:fe-run {
    .\build\vs\bin\output\x64\Release\ForgeEngine.exe
}

# 运行 Debug 版
function global:fe-run-debug {
    .\build\vs\bin\output\x64\Debug\ForgeEngine.exe
}


# ============================================================
# 三、查看（配置文件 / 端口 / git 状态）
# ============================================================

# 打印全部构建配置文件（带行号）；-Pattern 可按关键词过滤
#   用法：fe-config           全量（34 文件）
#         fe-config -Pattern ktx  只看 ktx 相关
function global:fe-config {
    param([string]$Pattern = "")                        # 过滤关键词（fe-config-k 传 "ktx"）

    [Console]::OutputEncoding = [System.Text.Encoding]::UTF8   # 中文输出不乱码

    # --- 1. 收集文件清单 ---
    # ① 根目录固定配置文件
    $root  = Get-Location
    $fixed = @("vcpkg.json","CMakePresets.json","CMakeLists.txt",".gitignore","README.md") |
        ForEach-Object { Join-Path $root $_ }
    # ② 自动扫描（cmd 脚本 / docs 文档 / config 下 cmake+json+补丁）
    $auto = @()
    $auto += Get-ChildItem "cmd"              -File -Filter *.ps1    -ErrorAction SilentlyContinue
    $auto += Get-ChildItem "docs"             -File -Filter *.md     -ErrorAction SilentlyContinue
    $auto += Get-ChildItem "config\cmake"         -File -Filter *.cmake  -ErrorAction SilentlyContinue
    $auto += Get-ChildItem "config\vcpkg\triplet" -File -Filter *.cmake  -ErrorAction SilentlyContinue
    $auto += Get-ChildItem "config\vcpkg\port" -Recurse -File -Include "portfile.cmake","vcpkg.json","*.patch","*.diff" -ErrorAction SilentlyContinue
    # ③ 合并 + 去重 + 只保留存在的文件
    $seen = @{}
    $files = @($fixed + ($auto | ForEach-Object { $_.FullName })) |
        Where-Object { $_ -and (Test-Path $_) -and -not $seen.ContainsKey($_) } |
        ForEach-Object { $seen[$_] = $true; $_ }

    # --- 2. 可选关键词过滤（fe-config-k 用）---
    if ($Pattern) { $files = $files | Where-Object { $_ -match $Pattern } }

    # --- 3. 逐个打印（带行号）---
    $total = 0
    foreach ($f in $files) {
        $lines = [System.IO.File]::ReadAllLines($f, [System.Text.Encoding]::UTF8)
        $rel   = $f.Substring($root.Path.Length + 1)                # 相对路径（显示用）
        $total += $lines.Count
        "`n===== $rel（$($lines.Count) 行）====="
        for ($i = 0; $i -lt $lines.Count; $i++) {
            "{0,4}: {1}" -f ($i + 1), $lines[$i]
        }
    }
    "`n===== 共 $($files.Count) 个文件 / $total 行 ====="
}

# 只看 ktx 端口相关（复用 fe-config 的过滤参数）
function global:fe-config-k {
    fe-config -Pattern "ktx"
}

# 已安装端口列表（vcpkg）
function global:fe-ports {
    & "$env:VCPKG_ROOT\vcpkg.exe" list --x-install-root=.\build\vcpkg_installed
}

# git 状态 + 最近 5 条提交
function global:fe-st {
    git status -sb
    git log --oneline -5
}

# 本次改动 diff（先统计后详情）
function global:fe-show {
    git diff --stat
    git diff --color
}


# ============================================================
# 四、提交 / 推送（git 日常）
# ============================================================

# 一键提交：fe-done "commit message"
function global:fe-done {
    git add -A
    git commit -m "$args"
}

# 推送到远端
function global:fe-push { git push }


# ============================================================
# 五、清理（构建产物）
# ============================================================

# 清 VS 工程（保留 vcpkg_installed，下次构建更快）
function global:fe-clean {
    Remove-Item -Recurse -Force build\vs -ErrorAction SilentlyContinue
    Write-Host "build\vs 已清理（vcpkg_installed 保留）" -ForegroundColor Green
}

# 清 ktx 构建残留（vcpkg 编译缓存）
function global:fe-clean-ktx {
    Remove-Item -Recurse -Force "$env:VCPKG_ROOT\buildtrees\ktx" -ErrorAction SilentlyContinue
    Write-Host "buildtrees\ktx 已清理" -ForegroundColor Green
}


# ============================================================
# 六、工具
# ============================================================

# VS Code 打开工程（没有 code 命令则用资源管理器）
function global:fe-code {
    if (Get-Command code -ErrorAction SilentlyContinue) { code . } else { start . }
}

# ============================================================
# 七、dsh（DeepSeek Harness）启动
# ============================================================

# 启动 dsh（DeepSeek Harness）Web 界面 → http://127.0.0.1:3080
# 依赖：Node（PATH 或 E:\tool\app\node）、DSH_HOME=E:\tool\app\dsHarness；工作区=仓库根
# 启动 dsh（DeepSeek Harness）Web 界面 → http://127.0.0.1:3080
# 单一入口：内部转调 config\tool\gate\start-dsh.ps1（设 DSH_PERMISSION_MODE=danger-full-access、补 PATH、日志、8 秒复查）
# 关闭用：fe-dsh-end　·　自检用：fe-dsh -DryRun
function fe-dsh {
    $script = Join-Path (Split-Path -Parent $PSScriptRoot) 'config\tool\gate\start-dsh.ps1'
    if (-not (Test-Path $script)) {
        Write-Host "未找到启动脚本：$script" -ForegroundColor Red
        return
    }
    if ([string]::IsNullOrWhiteSpace($env:DSH_HOME)) { $env:DSH_HOME = 'E:\tool\app\dsHarness' }
    Write-Host '启动 dsh（权限档 danger-full-access；关闭用 fe-dsh-end）' -ForegroundColor Cyan
    & pwsh -NoProfile -File $script @args
}

# fe-dsh-end：关闭 DSH web（重启用：config\tool\gate\start-dsh.ps1）
$feCmdRoot = Split-Path -Parent $PSScriptRoot
function fe-dsh-end { & pwsh -NoProfile -File (Join-Path $feCmdRoot 'config\tool\gate\fe-dsh-end.ps1') @args }