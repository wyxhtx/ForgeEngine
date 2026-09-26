# align-comments-v17.ps1 —— 行尾注释三列对齐（加固版：只重排源文件里**已存在**的 Tab 边界）
# 为什么：VS 右下角"列"= 显示列（汉字 2 列），所以 Tab 能对齐；但**列边界必须由源文件给出**。
#         早期版本会把注释里的空格也当成列分隔去插 Tab（"视口 + 裁剪 …" 被切成两列、短行被补尾 Tab），
#         那不是作者想要的列。v17 起：Tab 边界才动，空格/直接相接一律原样保留；末列不补尾空白。
#   条目行：代码, →(Tab 顶到 T1)// 中文名 →(Tab，若源为 Tab)功能前半（含：）→(Tab，若源为 Tab)冒号后说明
#   标题行：//名字 →(Tab，若源为 Tab)说明
#   参与门槛：代码部分必须是"裸标识符 + 可选逗号"（枚举/表格列条目）；语句行、正文注释不参与。
#   作用域：默认只 src\code\vk\data（要改全树必须显式 -All，防误伤）。
# 内容保全：重写前断言"去掉空白后与原文相同"，不通过就拒绝。
param(
	[switch]$Apply,
	[string]$Path = '',
	[switch]$Recurse,
	[switch]$All,
	[int]$Preview = 0,
	[int]$TabSize = 4,
	[int]$MaxColumn = 64,
	[string]$FontPath = "$env:WINDIR\Fonts\MapleMono-Medium.ttf",
	[int]$Gap = 2
)
$ErrorActionPreference = 'Stop'
# 精确宽度表：从字体实测 advance（= VS 渲染用的同一套度量），半角基准 = 'M'
$script:WideChars = @{}
$useFont = $false
try {
	Add-Type -AssemblyName PresentationCore
	$gt = New-Object System.Windows.Media.GlyphTypeface($FontPath)
	$mi = 0; [void]$gt.CharacterToGlyphMap.TryGetValue([int][char]'M', [ref]$mi)
	$base = $gt.AdvanceWidths[$mi]
	foreach ($kv in $gt.CharacterToGlyphMap.GetEnumerator()) {
		if ($gt.AdvanceWidths[$kv.Value] -ge 1.5 * $base) { $script:WideChars[[char]$kv.Key] = $true }
	}
	$useFont = $true
	'宽度表：字体实测 ' + $script:WideChars.Count + ' 个宽字符（半角基准 ' + $base.ToString('N4') + 'em）'
} catch { '警告：读字体失败，退回码点规则：' + $_.Exception.Message }
$g = Split-Path $PSScriptRoot -Parent
if ($Path -eq '') { $Path = if ($All) { "$g\src\code" } else { "$g\src\code\vk\data" } }
$root = if ([IO.Path]::IsPathRooted($Path)) { $Path } else { Join-Path $g $Path }
'作用域：' + $root + $(if ($All) { '   （-All：全树）' } else { '   （默认：仅 vk\data，-All 才全树）' })

function Wd([string]$s) {
	$c = 0
	foreach ($ch in $s.ToCharArray()) {
		$k = [int][char]$ch
		if ($ch -eq "`t") { $c += $TabSize - ($c % $TabSize) }
		elseif (($useFont -and $script:WideChars.ContainsKey($ch)) -or ((-not $useFont) -and $k -ge 0x2E80)) { $c += 2 }   # 只有真正的 CJK/全角算 2 列（实测 · ≤ ≥ × → 都只有 1 个半角宽）
		else { $c += 1 }
	}
	return $c
}
function Tabs([int]$from, [int]$to) { $n = [int][Math]::Ceiling(($to - $from) / $TabSize); if ($n -lt 1) { $n = 1 }; return $n }
# -Preview：打印改动前后（Tab 画成 ·>），便于人工确认后再 -Apply
function Show([string]$o, [string]$n) {
	if ($script:pv -le 0) { return }
	$script:pv--
	'  - ' + ($o -replace "`t", '·>')
	'  + ' + ($n -replace "`t", '·>')
}
$script:pv = $Preview

$files = @()
if (Test-Path -LiteralPath $root -PathType Container) { $files = Get-ChildItem -LiteralPath $root -Recurse:$Recurse -File -Include *.h, *.hpp, *.cpp, *.inl }
else { $files = @(Get-Item -LiteralPath $root) }

$total = 0; $rejected = 0; $out = @(); $CHK = 0; $Bad = 0
foreach ($f in $files) {
	$raw = [System.IO.File]::ReadAllText($f.FullName)
	$eol = if ($raw -match "`r`n") { "`r`n" } else { "`n" }
	$lines = [System.IO.File]::ReadAllLines($f.FullName)

	$items = @(); $titles = @()
	$maxCode = 0; $sec = 0
	$secCn = @{}; $secHead = @{}; $secNmEnd = @{}
	for ($i = 0; $i -lt $lines.Count; $i++) {
		$l = $lines[$i]
		if ($l -match '^\s*(namespace|struct|enum|class)\b' -or $l -match '^\s*\}\s*;?\s*$') { $sec++ }
		if ($l -match '^[\t ]*//') {
			$m = [regex]::Match($l, '^(?<ind>\t*)//(?<nm>\S+)(?<g>[ \t]+)(?<rest>.*\S)\s*$')
			if ($m.Success) {
				$nm = $m.Groups['nm'].Value
				$gt2 = $m.Groups['g'].Value
				$rest = $m.Groups['rest'].Value
				# 规则 A：只有"名字像标识符"（ASCII 标识符 或 纯汉字≤6）且"说明不以符号开头"的标题行才参与列对齐
				$idOk = ($nm -match '^[A-Za-z_][A-Za-z0-9_]*$') -or ($nm -match '^[\u4e00-\u9fa5]{1,6}$')
				$symOk = -not ($rest -match '^[^\p{L}\p{Nd}\u4e00-\u9fa5]')
				if ($idOk -and $symOk) {
					$t = $gt2.Contains("`t")
					$titles += [pscustomobject]@{ i = $i; sec = $sec; ind = $m.Groups['ind'].Value; nm = $nm; g = $gt2; rest = $rest; t = $t }
					if ($t) { $we = Wd ($m.Groups['ind'].Value + '//' + $nm); if (-not $secNmEnd.ContainsKey($sec) -or $we -gt $secNmEnd[$sec]) { $secNmEnd[$sec] = $we } }
				}
			}
			continue
		}
		$m = [regex]::Match($l, '^(?<code>.*?\S)(?<g0>[ \t]*)//(?<gq>[ \t]*)(?<cmt>.*)$')
		if (-not $m.Success) { continue }
		$code = $m.Groups['code'].Value
		$g0 = $m.Groups['g0'].Value
		$gq = $m.Groups['gq'].Value   # 加固：`//` 之后的原始空白（如 `// + 线宽` 里那个空格）必须原样保留
		if ($code -match '"') { continue }
		if ($code -notmatch '^\t+\w[\w]*,?$') { continue }   # 加固：只处理枚举/表格列条目，语句行与正文注释不动
		$cmt = $m.Groups['cmt'].Value
		if ($cmt.Trim() -eq '') { continue }
		$fm = [regex]::Match($cmt, '^(?<cn>[^\s\t]+)(?<g1>[ \t]*)(?<fn>.*)$')
		if (-not $fm.Success) { continue }
		$cn = $fm.Groups['cn'].Value
		$g1 = $fm.Groups['g1'].Value
		$fn = $fm.Groups['fn'].Value   # 加固：注释文本保持原样，只动分隔空白
		$k = $fn.IndexOf([char]0xFF1A)
		$after = if ($k -ge 0) { $fn.Substring($k + 1) } else { '' }
		$g2 = if ($k -ge 0) { $after.Substring(0, $after.Length - $after.TrimStart().Length) } else { '' }
		$head = if ($k -ge 0) { $fn.Substring(0, $k + 1) } else { '' }
		$tail = if ($k -ge 0) { $after.TrimStart() } else { $fn }
		$col = Wd $code
		if ($col -gt $MaxColumn) { continue }
		# 加固：分隔符"种类"由源文件决定 —— 原本是 Tab 的边界才参与对齐，空格/直接相接一律原样保留
		$t0 = $g0.Contains("`t"); $t1 = $g1.Contains("`t"); $t2 = $g2.Contains("`t")
		$items += [pscustomobject]@{ i = $i; sec = $sec; code = $code; col = $col; g0 = $g0; gq = $gq; cn = $cn; g1 = $g1; head = $head; g2 = $g2; tail = $tail; has = ($k -ge 0); t0 = $t0; t1 = $t1; t2 = $t2 }
		if ($t0 -and $col -gt $maxCode) { $maxCode = $col }
		if ($t1) { $w = Wd $cn; if (-not $secCn.ContainsKey($sec) -or $w -gt $secCn[$sec]) { $secCn[$sec] = $w } }
		if ($k -ge 0 -and $t2) { $w2 = Wd $head; if (-not $secHead.ContainsKey($sec) -or $w2 -gt $secHead[$sec]) { $secHead[$sec] = $w2 } }
	}
	$step = $TabSize * 2
	$T1 = ([int][Math]::Floor($maxCode / $step) + 1) * $step
	# 跨枚举统一：三列目标都取全文件最大值（宽度表已来自字体实测）
	$cm = 0; foreach ($k in $secCn.Keys) { if ($secCn[$k] -gt $cm) { $cm = $secCn[$k] } }
	$hm = 0; foreach ($k in $secHead.Keys) { if ($secHead[$k] -gt $hm) { $hm = $secHead[$k] } }
	$nm2 = 0; foreach ($k in $secNmEnd.Keys) { if ($secNmEnd[$k] -gt $nm2) { $nm2 = $secNmEnd[$k] } }
	$T2 = ([int][Math]::Floor(($T1 + 2 + $cm) / $TabSize) + 1) * $TabSize
	$T3 = ([int][Math]::Floor(($T2 + $hm) / $TabSize) + 1) * $TabSize
	$T4 = ([int][Math]::Floor($nm2 / $TabSize) + 1) * $TabSize

	$changed = 0
	foreach ($c in $items) {
		$old = $lines[$c.i]
		$new = $c.code + $(if ($c.t0) { "`t" * (Tabs $c.col $T1) } else { $c.g0 }) + '//' + $c.gq + $c.cn
		if ($c.t1) {
			$new += ("`t" * (Tabs ($T1 + 2 + (Wd $c.cn)) $T2)) + $c.head
			$new += $(if ($c.has -and $c.t2) { "`t" * (Tabs ($T2 + (Wd $c.head)) $T3) } else { $c.g2 }) + $c.tail
		} else { $new += $c.g1 + $c.head + $c.g2 + $c.tail }
		if (($new -replace '\s', '') -ne ($old -replace '\s', '')) { $rejected++; continue }
		if ($new -ne $old) { Show $old $new; $lines[$c.i] = $new; $changed++ }
	}
	foreach ($t in $titles) {
		$old = $lines[$t.i]
		$new = $t.ind + '//' + $t.nm + $(if ($t.t) { "`t" * (Tabs (Wd ($t.ind + '//' + $t.nm)) $T4) } else { $t.g }) + $t.rest
		if (($new -replace '\s', '') -ne ($old -replace '\s', '')) { $rejected++; continue }
		if ($new -ne $old) { Show $old $new; $lines[$t.i] = $new; $changed++ }
	}
	# ---- 自校验：用与写入完全相同的解析读回结果行，只核对"参与对齐的 Tab 边界" ----
	$short = $f.Name
	$secFn = @{}; $secTail = @{}; $secTl = @{}
	foreach ($c in $items) {
		$ln = $lines[$c.i]
		if ($c.t1) {
			$q = $ln.IndexOf('//') + 2 + $c.cn.Length
			while ($q -lt $ln.Length -and ($ln[$q] -eq ' ' -or $ln[$q] -eq "`t")) { $q++ }
			if (-not $secFn.ContainsKey($c.sec)) { $secFn[$c.sec] = @() }
			$secFn[$c.sec] += [pscustomobject]@{ col = (Wd $ln.Substring(0, $q)); line = $c.i + 1 }
		}
		if ($c.has -and $c.t1 -and $c.t2) {   # 自检只核对"链路上每一处都是 Tab"的行（空格边界的行本就不参与对齐）
			$z = $ln.IndexOf([char]0xFF1A) + 1
			while ($z -lt $ln.Length -and ($ln[$z] -eq ' ' -or $ln[$z] -eq "`t")) { $z++ }
			if (-not $secTail.ContainsKey($c.sec)) { $secTail[$c.sec] = @() }
			$secTail[$c.sec] += [pscustomobject]@{ col = (Wd $ln.Substring(0, $z)); line = $c.i + 1 }
		}
	}
	foreach ($tt in $titles) {
		if (-not $tt.t) { continue }
		$ln = $lines[$tt.i]
		$q = $ln.IndexOf('//') + 2 + $tt.nm.Length
		while ($q -lt $ln.Length -and ($ln[$q] -eq ' ' -or $ln[$q] -eq "`t")) { $q++ }
		if (-not $secTl.ContainsKey($tt.sec)) { $secTl[$tt.sec] = @() }
		$secTl[$tt.sec] += [pscustomobject]@{ col = (Wd $ln.Substring(0, $q)); line = $tt.i + 1 }
	}
	foreach ($pair in @(@{ n = '功能列'; m = $secFn }, @{ n = '第四列'; m = $secTail }, @{ n = '标题说明列'; m = $secTl })) {
		foreach ($k in $pair.m.Keys) {
			$vs = $pair.m[$k]
			$u = ($vs | ForEach-Object { $_.col } | Sort-Object -Unique)
			$CHK++
			if ($u.Count -gt 1) {
				$Bad++
				$ex = ($vs | Where-Object { $_.col -ne $u[0] } | Select-Object -First 3 | ForEach-Object { 'L' + $_.line + '=' + $_.col }) -join ' '
				'  ✗ ' + $short + ' 段s' + $k + ' ' + $pair.n + ' 不唯一(' + ($u -join ',') + ')  ' + $ex
			}
		}
	}
	if ($changed -gt 0) {
		$total += $changed
		$out += ('{0}  T1={1} T2={2} T3={3} 段={4}  条目={5} 标题={6}  改动={7}' -f $f.FullName.Replace("$g\", ''), $T1, $T2, $T3, $sec, $items.Count, $titles.Count, $changed)
		if ($Apply) { [System.IO.File]::WriteAllText($f.FullName, ($lines -join $eol), (New-Object System.Text.UTF8Encoding($false))) }
	}
}
if ($out) { $out | ForEach-Object { '  ' + $_ } } else { '  无需改动' }
'模式: ' + $(if ($Apply) { 'apply' } else { 'dry-run' }) + '；自校验列组=' + $CHK + ' 不一致=' + $Bad + "；改动=$total 内容保全拒绝=$rejected TabSize=$TabSize"
