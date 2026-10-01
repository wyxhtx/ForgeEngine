# start-dsh.ps1 —— 用「正确权限档 + 可用 PATH」启动 DSH web（2026-10-01 定）
# 用法：pwsh -NoProfile -File config\tool\gate\start-dsh.ps1 [-DryRun]
#   -DryRun  只打印将设置的变量与将执行的命令，不启动（用于自检）
# 背景：sandbox-policy.mode = env DSH_PERMISSION_MODE ?? 'workspace-write'，而本机 workspace-write 的 ACL 建立会失败
#       （SetNamedSecurityInfoW Win32 5）⇒ 必须显式给 danger-full-access，否则新会话写盘会被拒。
$DryRun = ($args -contains '-DryRun')
$Root   = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
$NpxBin = 'E:\tool\app\npm-cache\_npx\1e7f6d9597241db0\node_modules\.bin'
$Node   = 'E:\tool\app\node\node.exe'
$Bin    = 'E:\tool\app\npm-cache\_npx\1e7f6d9597241db0\node_modules\@deepseek-ai\dsh\lib\bin.js'
$Log    = Join-Path $Root 'build\temp\log\dsh-start.log'

function W($m) { try { ('[{0}] {1}' -f (Get-Date -Format 'HH:mm:ss'), $m) | Out-File -FilePath $Log -Append -Encoding utf8 } catch { } ; Write-Host $m }

$env:DSH_PERMISSION_MODE = 'danger-full-access'
if ($env:PATH -notlike ('*' + $NpxBin + '*')) { $env:PATH = $NpxBin + ';' + $env:PATH }

W ('DSH_PERMISSION_MODE = ' + $env:DSH_PERMISSION_MODE)
W ('PATH 已含 npx bin：' + ($env:PATH -like ('*' + $NpxBin + '*')))
W ('工作目录 = ' + $Root)
W ('启动命令 = ' + $Node + ' ' + $Bin + ' web')
foreach ($p in @($Node, $Bin)) { if (-not (Test-Path $p)) { W ('✗ 缺少文件：' + $p); exit 1 } }

if ($DryRun) { W '（-DryRun）未启动'; exit 0 }

$busy = Get-NetTCPConnection -LocalPort 3080 -State Listen -ErrorAction SilentlyContinue | Select-Object -First 1
if ($busy) { W ('⚠ 3080 已被 PID ' + $busy.OwningProcess + ' 占用 ⇒ 不启动（先 fe-dsh-end 关闭，或用 -Force 强制）'); if (-not ($args -contains '-Force')) { exit 2 } }

Start-Process -FilePath $Node -ArgumentList @($Bin, 'web') -WorkingDirectory $Root -WindowStyle Hidden
Start-Sleep -Seconds 8
$ok = Get-NetTCPConnection -LocalPort 3080 -State Listen -ErrorAction SilentlyContinue | Select-Object -First 1
if ($ok) { W ('✓ 已启动：http://127.0.0.1:3080 （PID ' + $ok.OwningProcess + '）') } else { W '✗ 8 秒内 3080 未监听，请看上方输出与日志' }