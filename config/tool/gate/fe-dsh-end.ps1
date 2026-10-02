# fe-dsh-end.ps1 —— 关闭 DSH web（按 3080 端口定位进程与其 dsh 祖先链）
# 用法：pwsh -NoProfile -File config\tool\gate\fe-dsh-end.ps1 [-DryRun]
#   -DryRun  只列出将关闭的 PID，不实际关闭
$DryRun = ($args -contains '-DryRun')
$Root   = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
$Log    = Join-Path $Root 'build\temp\log\dsh-end.log'
$PidFile = Join-Path $Root 'build\temp\work\dsh.pid'
function W($m) { try { ('[{0}] {1}' -f (Get-Date -Format 'HH:mm:ss'), $m) | Out-File -FilePath $Log -Append -Encoding utf8 } catch { } ; Write-Host $m }
# 双来源：3080 端口占用者 ∪ PID 文件记录的存活进程（后者覆盖「还在启动、尚未绑端口」的情况）
$c = Get-NetTCPConnection -LocalPort 3080 -State Listen -ErrorAction SilentlyContinue | Select-Object -First 1
$roots = @()
if ($c) { $roots += [int]$c.OwningProcess }
if (Test-Path $PidFile) {
  $pf = (Get-Content $PidFile -Raw -ErrorAction SilentlyContinue)
  if ($pf) { $pf = $pf.Trim() }
  if ($pf -match '^\d+$' -and (Get-Process -Id ([int]$pf) -ErrorAction SilentlyContinue)) { $roots += [int]$pf }
}
if (-not $roots) { W '没有实例在监听 3080，也没有 PID 文件记录的存活进程（无需关闭）'; Remove-Item $PidFile -ErrorAction SilentlyContinue; exit 0 }
$chain = @()
foreach ($rootPid in ($roots | Select-Object -Unique)) {
  $chain += $rootPid
  $p = Get-CimInstance Win32_Process -Filter ('ProcessId=' + $rootPid) -ErrorAction SilentlyContinue
  while ($p -and $p.ParentProcessId) {
    $pp = Get-CimInstance Win32_Process -Filter ('ProcessId=' + $p.ParentProcessId) -ErrorAction SilentlyContinue
    if (-not $pp) { break }
    # 只沿命令行含 dsh 的祖先进上溯，避免误伤外层交互 shell
    if ($pp.CommandLine -match 'dsh') { $chain += $pp.ProcessId; $p = $pp } else { break }
  }
}
$ids = $chain | Select-Object -Unique
W ('将关闭 PID：' + ($ids -join ', '))
if ($DryRun) { W '（-DryRun）未关闭'; exit 0 }
foreach ($procId in $ids) {
  try {
    # /T：连子进程树一起结束（服务会带若干 node 子进程）；/F 强制
    & taskkill.exe /PID $procId /T /F 2>$null | Out-Null
    if ($LASTEXITCODE -eq 0) { W ('已结束 PID ' + $procId + '（含子进程树）') } else { W ('taskkill 返回 ' + $LASTEXITCODE + '：PID ' + $procId + '（可能已退出）') }
  } catch { W ('结束失败 PID ' + $procId + '：' + $_.Exception.Message) }
}
Start-Sleep -Seconds 2
Remove-Item $PidFile -ErrorAction SilentlyContinue	# 清 PID 文件，避免下次误判
if (Get-NetTCPConnection -LocalPort 3080 -State Listen -ErrorAction SilentlyContinue) { W '⚠ 3080 仍在监听（可能还有其它实例）' } else { W '✓ DSH web 已关闭（重启：config\tool\gate\start-dsh.ps1）' }