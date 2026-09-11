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
# 七、知识库同步（Chatbox KB 手动同步辅助，集合展开+hash增量+格式适配）
# ============================================================

function fe-sync-kb {
    # 自动展开 KB 文件集合（根配置+docs+ai_docs+src\code 全量），SHA256 增量同步到 docs\chatbox-sync\
    # 格式适配（白名单制）：白名单外格式（如 .json）复制为 <原名>.txt，Chatbox KB 按 txt 解析
    # 用法：fe-sync-kb（需在仓库根运行）；只复制有变更/新增的文件并列出
    # 说明：Chatbox KB 同名拖入=新增，需先在 KB 删除同名旧文件再拖入新文件
    $isForge = Test-Path (Join-Path $PWD 'CMakePresets.json')
    if (-not $isForge) { Write-Host "请在 ForgeEngine 仓库内运行"; return }
    $repo   = $PWD.Path
    $dstDir = Join-Path $repo 'docs\chatbox-sync'
    New-Item -ItemType Directory -Path $dstDir -Force | Out-Null

    # Chatbox KB 支持的扩展名白名单（其余转 .txt）
    $whitelist = @('.md','.txt','.cpp','.h','.hpp','.rtf','.pdf','.docx','.pptx','.xlsx','.csv','.epub')

    # --- 集合展开 ---
    $files = @()
    foreach ($root in @('CMakeLists.txt','README.md','.gitignore','vcpkg.json')) {
        if (Test-Path (Join-Path $repo $root)) { $files += $root }
    }
    foreach ($pat in @('docs\*.md','docs\ai_docs\*.md')) {
        Get-ChildItem (Join-Path $repo $pat) -File | ForEach-Object { $files += $_.FullName.Substring($repo.Length + 1) }
    }
    Get-ChildItem (Join-Path $repo 'src\code') -Recurse -File -Include *.h,*.hpp,*.cpp | ForEach-Object { $files += $_.FullName.Substring($repo.Length + 1) }

    # --- 格式适配 + hash 增量 ---
    $changed = @()
    $dstNames = @()
    foreach ($rel in $files) {
        $src = Join-Path $repo $rel
        $leaf = Split-Path $rel -Leaf
        $ext  = [System.IO.Path]::GetExtension($leaf).ToLower()
        $dstName = if ($whitelist -contains $ext) { $leaf } else { $leaf + '.txt' }
        $dstNames += $dstName
        $dst = Join-Path $dstDir $dstName
        $srcHash = (Get-FileHash $src -Algorithm SHA256).Hash
        $need = $true
        if (Test-Path $dst) {
            if ($srcHash -eq (Get-FileHash $dst -Algorithm SHA256).Hash) { $need = $false }
        }
        if ($need) { Copy-Item $src $dst -Force; $changed += $dstName }
    }

    # --- 清理目录中已不在集合内的旧副本（改名/移除的残留）---
    Get-ChildItem $dstDir -File | ForEach-Object {
        if ($dstNames -notcontains $_.Name) { Remove-Item $_.FullName -Force }
    }

    # --- 报告 ---
    if ($changed.Count -eq 0) {
        Write-Host "无变更，KB 已是最新（$($files.Count) 个文件全部一致）" -ForegroundColor Green
        return
    }
    Write-Host "需同步 $($changed.Count)/$($files.Count) 个文件（KB 中显示名）：" -ForegroundColor Yellow
    $changed | ForEach-Object { Write-Host "  - $_" }
    Write-Host "步骤：先在 Chatbox KB 删除同名旧文件，再全选拖入打开的目录（.txt 结尾的即格式适配产物）"
    Start-Process explorer.exe $dstDir
}

# ============================================================
# 七、知识库同步（Chatbox KB 手动同步辅助，状态文件+增量+格式适配）
# ============================================================

function fe-sync-kb {
    # 集合展开（根配置+docs+chatbox-docs+src\code）+ SHA256 增量 + 白名单格式适配
    # hash 基准存 docs\chatbox-sync-state.json；执行后 chatbox-sync 只留本次需同步文件
    # 用法：fe-sync-kb（需在仓库根运行）
    $isForge = Test-Path (Join-Path $PWD 'CMakePresets.json')
    if (-not $isForge) { Write-Host "请在 ForgeEngine 仓库内运行"; return }
    $repo      = $PWD.Path
    $dstDir    = Join-Path $repo 'docs\chatbox-sync'
    $stateFile = Join-Path $repo 'docs\chatbox-sync-state.json'
    New-Item -ItemType Directory -Path $dstDir -Force | Out-Null

    $whitelist = @('.md','.txt','.cpp','.h','.hpp','.rtf','.pdf','.docx','.pptx','.xlsx','.csv','.epub')

    # --- 集合展开 ---
    $files = @()
    foreach ($root in @('CMakeLists.txt','README.md','.gitignore','vcpkg.json')) {
        if (Test-Path (Join-Path $repo $root)) { $files += $root }
    }
    foreach ($pat in @('docs\*.md','docs\chatbox-docs\*.md')) {
        Get-ChildItem (Join-Path $repo $pat) -File | ForEach-Object { $files += $_.FullName.Substring($repo.Length + 1) }
    }
    Get-ChildItem (Join-Path $repo 'src\code') -Recurse -File -Include *.h,*.hpp,*.cpp | ForEach-Object { $files += $_.FullName.Substring($repo.Length + 1) }

    # --- 读状态（文件名 -> SHA256）；无状态时用 chatbox-sync 现有副本初始化（平滑首轮）---
    $state = @{}
    if (Test-Path $stateFile) {
        $state = Get-Content $stateFile -Raw | ConvertFrom-Json -AsHashtable
    } else {
        Get-ChildItem $dstDir -File | ForEach-Object {
            $state[$_.Name] = (Get-FileHash $_.FullName -Algorithm SHA256).Hash
        }
    }

    # --- 逐个对比 + 复制变更 ---
    $changed = @()
    $newState = @{}
    foreach ($rel in $files) {
        $src = Join-Path $repo $rel
        $leaf = Split-Path $rel -Leaf
        $ext  = [System.IO.Path]::GetExtension($leaf).ToLower()
        $dstName = if ($whitelist -contains $ext) { $leaf } else { $leaf + '.txt' }
        $srcHash = (Get-FileHash $src -Algorithm SHA256).Hash
        $need = $true
        if ($state.ContainsKey($dstName)) {
            if ($state[$dstName] -eq $srcHash) { $need = $false }
        }
        if ($need) {
            Copy-Item $src (Join-Path $dstDir $dstName) -Force
            $changed += $dstName
        }
        $newState[$dstName] = $srcHash
    }

    # --- 清理：目录只留本次需同步的 ---
    Get-ChildItem $dstDir -File | ForEach-Object {
        if ($changed -notcontains $_.Name) { Remove-Item $_.FullName -Force }
    }

    # --- 写回状态 ---
    ($newState | ConvertTo-Json) | Set-Content $stateFile -Encoding utf8

    # --- 报告（不自动打开目录）---
    if ($changed.Count -eq 0) {
        Write-Host "无变更，KB 已是最新（$($files.Count) 个文件全部一致）" -ForegroundColor Green
        return
    }
    Write-Host "需同步 $($changed.Count) 个文件（已放入 docs\chatbox-sync\）：" -ForegroundColor Yellow
    $changed | ForEach-Object { Write-Host "  - $_" }
    Write-Host "步骤：先在 Chatbox KB 删除同名旧文件，再从 docs\chatbox-sync\ 全选拖入新文件（.txt 结尾为格式适配产物）"
}

# 启动 dsh（DeepSeek Harness）Web 界面 → http://127.0.0.1:3080
# 依赖：Node（PATH 或 E:\tool\app\node）、DSH_HOME=E:\tool\app\dsHarness；工作区=仓库根
function fe-dsh {
    $npx = (Get-Command npx -ErrorAction SilentlyContinue).Source
    if (-not $npx) {
        $npx = Join-Path 'E:\tool\app\node' 'npx.cmd'
    }
    if (-not (Test-Path $npx)) {
        Write-Host "未找到 npx：$npx（检查 Node 安装，或重启终端补 PATH）" -ForegroundColor Red
        return
    }

    if ([string]::IsNullOrWhiteSpace($env:DSH_HOME)) {
        $env:DSH_HOME = 'E:\tool\app\dsHarness'
    }

    $busy = Get-NetTCPConnection -LocalPort 3080 -State Listen -ErrorAction SilentlyContinue
    if ($busy) {
        Write-Host "端口 3080 已被 PID $($busy.OwningProcess) 占用——dsh 可能已在运行" -ForegroundColor Yellow
    }

    Write-Host "启动 dsh  DSH_HOME=$env:DSH_HOME  工作区=$PWD" -ForegroundColor Cyan
    Write-Host "浏览器打开：http://127.0.0.1:3080  （Ctrl+C 停止）" -ForegroundColor Cyan
    & $npx '@deepseek-ai/dsh' web
}