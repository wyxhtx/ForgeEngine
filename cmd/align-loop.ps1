# align-loop.ps1 —— 对齐自循环：对齐 → 自检 → 若仍有改动/不一致就再来一轮，直到收敛
# 用法：pwsh -File cmd\align-loop.ps1 [-Path src\code\vk\data] [-Recurse] [-MaxIter 8] [-TabSize 4] [-Tool cmd\align-comments-v17.ps1]
param(
	[string]$Path = 'src\code\vk\data',
	[switch]$Recurse,
	[int]$MaxIter = 8,
	[int]$TabSize = 4,
	[string]$Tool = 'cmd\align-comments-v17.ps1'
)
$ErrorActionPreference = 'Stop'
$g = Split-Path $PSScriptRoot -Parent
$toolPath = Join-Path $g $Tool
if (-not (Test-Path -LiteralPath $toolPath)) { throw ('找不到工具：' + $toolPath) }

$converged = $false
for ($i = 1; $i -le $MaxIter; $i++) {
	$args = @('-NoProfile', '-File', $toolPath, '-Path', $Path, '-Apply', '-TabSize', "$TabSize")
	if ($Recurse) { $args += '-Recurse' }
	$out = & pwsh @args 2>&1
	$text = ($out | Out-String)
	$changed = if ($text -match '改动=(\d+)') { [int]$Matches[1] } else { -1 }
	$bad = if ($text -match '不一致=(\d+)') { [int]$Matches[1] } else { -1 }
	$rej = if ($text -match '内容保全拒绝=(\d+)') { [int]$Matches[1] } else { -1 }
	'第 {0} 轮：改动={1}  不一致={2}  内容保全拒绝={3}' -f $i, $changed, $bad, $rej
	if ($bad -gt 0) { ($out | Where-Object { $_ -match '✗' }) | Select-Object -First 8 | ForEach-Object { '    ' + $_ } }
	if ($changed -eq 0 -and $bad -eq 0) { $converged = $true; '收敛：第 {0} 轮达到“改动 0 且不一致 0” ✓' -f $i; break }
}
if (-not $converged) { '未在 ' + $MaxIter + ' 轮内收敛 ✗ —— 需要人工看上面列出的不一致项' } else { '对齐自循环结束 ✓' }
