# ForgeEngine 一条命令闸门（2026-10-01 定）
# 用法：pwsh -NoProfile -File config\tool\gate\gate.ps1 [-SkipRun]
# 退出码：0 = PASS / 1 = FAIL
$SkipRun = ($args -contains '-SkipRun')
$Root = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
Set-Location $Root
$rows = @()
$bad = @()
Write-Host '===== ForgeEngine 闸门 ====='
Write-Host ('根目录：' + $Root)

# 0) 保留件核对
$need = @('build\temp\work\gen-equiv.ps1', 'build\temp\work\run-clean-exit.ps1', 'build\temp\bak\VkStruct.h.接线前.bak', 'build\temp\bak\VkList.h.接线前.bak')
$miss = @()
foreach ($n in $need) { if (-not (Test-Path (Join-Path $Root $n))) { $miss += $n } }
if ($miss.Count -eq 0) { $rows += '保留件    PASS' } else { $rows += ('保留件    FAIL 缺 ' + ($miss -join ' , ')); $bad += '保留件' }

# 1) 编码自检
$files = @()
foreach ($line in (git status --porcelain)) { if ($line.Length -gt 3) { $f = $line.Substring(3).Trim(); if ($f.StartsWith('"')) { $f = $f.Trim('"') }; $files += $f } }
$bom = @(); $tail = @(); $mix = @(); $scan = 0
foreach ($f in $files) {
  $p = Join-Path $Root $f
  if (-not (Test-Path $p)) { continue }
  if ((Get-Item $p).PSIsContainer) { continue }
  $ext = [IO.Path]::GetExtension($f).ToLower()
  $ok = @('.h', '.hpp', '.cpp', '.md', '.ps1', '.cmake', '.txt', '.yml', '.yaml', '.json', '.inc')
  if ($ok -notcontains $ext) { continue }
  $scan = $scan + 1
  $by = [IO.File]::ReadAllBytes($p)
  $hasBom = $false
  if ($by.Length -ge 3) { if ($by[0] -eq 239) { if ($by[1] -eq 187) { if ($by[2] -eq 191) { $hasBom = $true } } } }
  if ($ext -eq '.ps1') { if (-not $hasBom) { $bom += $f } } else { if ($hasBom) { $bom += $f } }
  if ($by.Length -gt 0) { if ($by[$by.Length - 1] -eq 10) { $tail += $f } }
  $crlf = 0; $lf = 0
  for ($i = 0; $i -lt $by.Length; $i++) { if ($by[$i] -eq 10) { if ($i -gt 0) { if ($by[$i - 1] -eq 13) { $crlf = $crlf + 1 } else { $lf = $lf + 1 } } else { $lf = $lf + 1 } } }
  if ($crlf -gt 0) { if ($lf -gt 0) { $mix += $f } }
}
if (($bom.Count + $tail.Count + $mix.Count) -eq 0) { $rows += ('编码自检  PASS（扫描 ' + $scan + ' 个变更文件）') } else { $rows += ('编码自检  FAIL BOM=' + $bom.Count + ' 尾换行=' + $tail.Count + ' 混EOL=' + $mix.Count); $bad += '编码自检' }

# 2) 编译
$out = (cmake --build build\vs --config Release --target ForgeEngine -- /p:TrackFileAccess=false 2>&1 | Out-String)
$bc = $LASTEXITCODE
$errs = ([regex]::Matches($out, 'error [A-Z]+[0-9]+')).Count
$warns = ([regex]::Matches($out, 'warning [A-Z]+[0-9]+')).Count
if ($bc -eq 0) { if ($errs -eq 0) { $rows += ('编译      PASS error=0 warning=' + $warns) } else { $rows += ('编译      FAIL error=' + $errs); $bad += '编译' } } else { $rows += ('编译      FAIL exit=' + $bc + ' error=' + $errs); $bad += '编译' }

# 3) 探针
$pr = (pwsh -NoProfile -File build\temp\work\gen-equiv.ps1 2>&1 | Out-String)
$pe = $LASTEXITCODE
$bl = @()
foreach ($m in [regex]::Matches($pr, '[A-Za-z0-9_]+_BAD=[0-9]+')) { $bl += $m.Value }
foreach ($m in [regex]::Matches($pr, 'STRICT_DIFF=[0-9]+')) { $bl += $m.Value }
$bls = ($bl -join ' ')
if ($pe -eq 0) { $rows += ('探针      PASS exit=0  ' + $bls) } else { $rows += ('探针      FAIL exit=' + $pe + '  ' + $bls); $bad += '探针' }

# 4) 真跑
if ($SkipRun) { $rows += '真跑      SKIP' } else {
  $rr = (pwsh -NoProfile -File build\temp\work\run-clean-exit.ps1 2>&1 | Out-String)
  $hit = ''
  foreach ($ln in ($rr -split '\r?\n')) { if ($ln -match '闸门结论') { $hit = $ln.Trim(); break } }
  if ($hit -eq '') { $rows += '真跑      FAIL 未取到结论行'; $bad += '真跑' } else { $rows += ('真跑      ' + $hit); if ($hit -notmatch 'PASS') { $bad += '真跑' } }
}

# 5) 汇总
Write-Host ''
foreach ($r in $rows) { Write-Host $r }
Write-Host ''
$gates = @{}
foreach ($r in $rows) { $p = $r -split '\s{2,}', 2; if ($p.Count -eq 2) { $gates[$p[0].Trim()] = $p[1].Trim() } }
$json = @{ ok = ($bad.Count -eq 0); failed = $bad; gates = $gates } | ConvertTo-Json -Compress -Depth 4
Write-Output ('GATE_JSON=' + $json)
if ($bad.Count -eq 0) { Write-Host 'GATE=PASS'; exit 0 } else { Write-Host ('GATE=FAIL（' + ($bad -join ' , ') + '）'); exit 1 }
