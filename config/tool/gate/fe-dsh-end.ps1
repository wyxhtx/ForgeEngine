# fe-dsh-end.ps1 —— 关闭 DSH web（按 3080 端口定位进程与其 dsh 祖先链）
# 用法：pwsh -NoProfile -File config\tool\gate\fe-dsh-end.ps1 [-DryRun]
#   -DryRun  只列出将关闭的 PID，不实际关闭
$DryRun = ($args -contains '-DryRun')
$Root   = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
$Log    = Join-Path $Root 'build\temp\log\dsh-end.log'
function W($m) { try { ('[{0}] {1}' -f (Get-Date -Format 'HH:mm:ss'), $m) | Out-File -FilePath $Log -Append -Encoding utf8 } catch { } ; Write-Host $m }
$c = Get-NetTCPConnection -LocalPort 3080 -State Listen -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $c) { W '没有实例在监听 3080（无需关闭）'; exit 0 }
$chain = @($c.OwningProcess)
$p = Get-CimInstance Win32_Process -Filter ('ProcessId=' + $c.OwningProcess) -ErrorAction SilentlyContinue
while ($p -and $p.ParentProcessId) {
  $pp = Get-CimInstance Win32_Process -Filter ('ProcessId=' + $p.ParentProcessId) -ErrorAction SilentlyContinue
  if (-not $pp) { break }
  if ($pp.CommandLine -match 'dsh') { $chain += $pp.ProcessId; $p = $pp } else { break }
}
$ids = $chain | Select-Object -Unique
W ('将关闭 PID：' + ($ids -join ', '))
if ($DryRun) { W '（-DryRun）未关闭'; exit 0 }
foreach ($procId in $ids) { try { Stop-Process -Id $procId -Force -ErrorAction Stop; W ('已结束 PID ' + $procId) } catch { W ('结束失败 PID ' + $procId + '：' + $_.Exception.Message) } }
Start-Sleep -Seconds 2
if (Get-NetTCPConnection -LocalPort 3080 -State Listen -ErrorAction SilentlyContinue) { W '⚠ 3080 仍在监听（可能还有其它实例）' } else { W '✓ DSH web 已关闭（重启：config\tool\gate\start-dsh.ps1）' }