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
$PidFile = Join-Path $Root 'build\temp\work\dsh.pid'	# 记录最后一次启动的子进程，供 fe-dsh-end 清「还在启动、尚未绑端口」的情况

function W($m) { try { ('[{0}] {1}' -f (Get-Date -Format 'HH:mm:ss'), $m) | Out-File -FilePath $Log -Append -Encoding utf8 } catch { } ; Write-Host $m }

$env:DSH_PERMISSION_MODE = 'danger-full-access'
if ($env:PATH -notlike ('*' + $NpxBin + '*')) { $env:PATH = $NpxBin + ';' + $env:PATH }

W ('DSH_PERMISSION_MODE = ' + $env:DSH_PERMISSION_MODE)
W ('PATH 已含 npx bin：' + ($env:PATH -like ('*' + $NpxBin + '*')))
W ('工作目录 = ' + $Root)
W ('启动命令 = ' + $Node + ' ' + $Bin + ' web')
foreach ($p in @($Node, $Bin)) { if (-not (Test-Path $p)) { W ('✗ 缺少文件：' + $p); exit 1 } }

if ($DryRun) { W '（-DryRun）未启动'; exit 0 }

if (-not ($args -contains '-Force')) { $busy = Get-NetTCPConnection -LocalPort 3080 -State Listen -ErrorAction SilentlyContinue | Select-Object -First 1 }
if ($busy) { W ('✓ 已有实例在运行：http://127.0.0.1:3080 （PID ' + $busy.OwningProcess + '）⇒ 无需重启，直接刷新浏览器；要重启先 fe-dsh-end'); exit 0 }

# 就绪探测：轮询而非固定睡眠（冷启动可能远超 8 秒：node/npx 引导 + 插件管理器 + Web 构建都吃时间）
$TimeoutSec = 120	# 实测正常启动 ≈28~35 秒（node 运行时剥离类型 + 插件加载 + Web 构建），冷启动更久；留足余量
$ti = [array]::IndexOf($args, '-TimeoutSec')
if ($ti -ge 0 -and ($ti + 1) -lt $args.Count) { $TimeoutSec = [int]$args[$ti + 1] }

# 子进程输出落盘：-WindowStyle Hidden 没有控制台可看，失败时要能真的打开日志
$outLog = Join-Path $Root 'build\temp\log\dsh-server.out.log'
$errLog = Join-Path $Root 'build\temp\log\dsh-server.err.log'
$proc = Start-Process -FilePath $Node -ArgumentList @($Bin, 'web') -WorkingDirectory $Root -WindowStyle Hidden -PassThru -RedirectStandardOutput $outLog -RedirectStandardError $errLog
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $PidFile) | Out-Null
Set-Content -Path $PidFile -Value $proc.Id -Encoding ascii
W ('子进程 PID = ' + $proc.Id + '；等待 3080 就绪（最多 ' + $TimeoutSec + ' 秒）')

$deadline = (Get-Date).AddSeconds($TimeoutSec)
$ok = $null
while ((Get-Date) -lt $deadline) {
    $ok = Get-NetTCPConnection -LocalPort 3080 -State Listen -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($ok) { break }
    if ($proc.HasExited) { break }
    Start-Sleep -Milliseconds 500
}

if ($ok) {
    W ('✓ 已启动：http://127.0.0.1:3080 （PID ' + $ok.OwningProcess + '）')
} else {
    # 超时不杀：实测正常启动就要 ≈28~35 秒，贸然杀掉会打断刚要做完的引导（而且下一次照样要重来）
    # 仍在启动 ⇒ 稍后直接刷新浏览器即可；要放弃时用 fe-dsh-end（它按命令行匹配清理，见 stop-dsh.ps1）
    W ('⚠ ' + $TimeoutSec + ' 秒内 3080 仍未监听，但子进程 PID ' + $proc.Id + ' 仍在运行（已退出=' + $proc.HasExited + '）')
    W ('  服务端输出：' + $outLog + ' / ' + $errLog)
    W '  稍等片刻后直接刷新浏览器；若要放弃，运行 fe-dsh-end'
    exit 3
}