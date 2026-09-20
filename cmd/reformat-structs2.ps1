param([switch]$Apply, [string]$Path = '')
$ErrorActionPreference = 'Stop'
$g = Split-Path $PSScriptRoot -Parent    # 仓库根 = 本脚本所在目录(cmd) 的上级
if ($Path -ne '') { $p = $Path } else { $p = "$g\src\code\vk\data\VkStruct.h" }
$enc = New-Object System.Text.UTF8Encoding($false)

$raw = [System.IO.File]::ReadAllText($p)
if ([regex]::Matches($raw, "(?<!`r)`n").Count -gt 0) { throw "存在纯 LF 行: $p" }
$L = New-Object System.Collections.Generic.List[string]
foreach ($ln in [System.IO.File]::ReadAllLines($p, $enc)) { $L.Add($ln) }

function IndentOf([string]$x) { return ([regex]::Match($x, '^[ \t]*')).Value }
function StripComment([string]$x) {
    $sb = New-Object System.Text.StringBuilder
    $inStr = $false
    for ($i = 0; $i -lt $x.Length; $i++) {
        $c = $x[$i]
        if ($inStr) {
            if ($c -eq '\') { [void]$sb.Append($c); if ($i + 1 -lt $x.Length) { [void]$sb.Append($x[$i + 1]); $i++ }; continue }
            if ($c -eq '"') { $inStr = $false }
            [void]$sb.Append($c); continue
        }
        if ($c -eq '"') { $inStr = $true; [void]$sb.Append($c); continue }
        if ($c -eq '/' -and $i + 1 -lt $x.Length -and $x[$i + 1] -eq '/') { break }
        [void]$sb.Append($c)
    }
    return $sb.ToString()
}
function Bal([string]$s, [string]$open, [string]$close) {
    return ([regex]::Matches($s, [regex]::Escape($open))).Count - ([regex]::Matches($s, [regex]::Escape($close))).Count
}
function ParenInTemplate([string]$t) {
    $depth = 0
    for ($i = 0; $i -lt $t.Length; $i++) {
        $c = $t[$i]
        if ($c -eq '(') { return ($depth -gt 0) }
        if ($c -eq '<') { if ($i -gt 0 -and $t[$i - 1] -eq '-') { continue }; $depth++ }
        elseif ($c -eq '>') { if ($depth -gt 0) { $depth-- } }
    }
    return $false
}
function IsFnStart([string]$x) {
    $t2 = (StripComment $x).Trim()
    if ($t2 -eq '') { return $false }
    if ($t2 -match '^(if|for|while|switch|return|else|case|break|continue|do|try|catch)\b') { return $false }
    if ($t2 -match '^(private|public|protected):') { return $false }
    if ($t2 -match '^template\s*<') { return $false }
    if ($t2 -notmatch '\(') { return $false }
    if (ParenInTemplate $t2) { return $false }
    if ($t2 -match '^[\w:~<>,&\*\s]+=[^=]') { return $false }
    if ($t2 -match '^\w+\(\)\s*=\s*delete;') { return $true }
    if ($t2.EndsWith('{')) { return $true }
    if ($t2.EndsWith('}')) { return $true }
    if ($t2.EndsWith(';')) { return $true }
    if ($t2.EndsWith(')') -or $t2.EndsWith('const')) { return $true }
    if ((Bal $t2 '(' ')') -gt 0) { return $true }
    return $false
}
function FnName([string]$x) {
    if ($x.Trim() -match '^(\w+)\(\)\s*=\s*delete;') { return $Matches[1] + '()' }
    $m = [regex]::Match((StripComment $x).Trim(), '([\w~]+)\s*\(')
    if ($m.Success) { return $m.Groups[1].Value }
    return (StripComment $x).Trim()
}
function FnEnd([int]$k, [int]$limit) {
    $par = 0; $depth = 0; $inBody = $false
    for ($j = $k; $j -le $limit; $j++) {
        $code = StripComment $L[$j]
        $par += (Bal $code '(' ')')
        if ($par -gt 0) { continue }
        $ob = (Bal $code '{' '}')
        if (-not $inBody) {
            if ($code -match '\{') { $inBody = $true; $depth += $ob }
            else { if ($code.TrimEnd().EndsWith(';')) { return $j } }
        }
        else { $depth += $ob }
        if ($inBody -and $depth -le 0) { return $j }
    }
    return $k
}
function IsMemberDecl([string]$x) {
    # 成员声明（数据成员/含初始化）：第一个 { 或 = 出现在第一个 ( 之前 → 不是函数项
    $t2 = (StripComment $x).Trim()
    if ($t2 -eq '') { return $false }
    if ($t2 -like '#*') { return $false }
    $pi = $t2.IndexOf('('); $bi = $t2.IndexOf('{'); $ei = $t2.IndexOf('=')
    $c = @(); if ($pi -ge 0) { $c += $pi }; if ($bi -ge 0) { $c += $bi }; if ($ei -ge 0) { $c += $ei }
    if ($c.Count -eq 0) { return $true }
    return ((($c | Sort-Object)[0]) -ne $pi)
}
function NonBlank([string[]]$arr) { return @($arr | Where-Object { $_.Trim() -ne '' }) }

# ---- 结构体范围 ----
$structs = New-Object System.Collections.ArrayList
for ($k = 0; $k -lt $L.Count; $k++) {
    # 支持 class 与 struct（class 默认访问为 private，标签处理同 struct）
    if ($L[$k] -match '^(\s*)(?:struct|class)\s+(\w+)\s*(?:final)?\s*(?::[^{]*)?\{$') {
        $ind = $Matches[1]; $nm = $Matches[2]
        $e = -1
        for ($j = $k + 1; $j -lt $L.Count; $j++) { if ($L[$j] -match ('^' + [regex]::Escape($ind) + '\};')) { $e = $j; break } }
        if ($e -gt 0) { [void]$structs.Add([pscustomobject]@{ Name = $nm; Start = $k; End = $e }) }
    }
}

$plan = New-Object System.Collections.ArrayList
$skip = New-Object System.Collections.ArrayList
$nReorder = 0; $nGap = 0; $nBad = 0

foreach ($s in ($structs | Sort-Object Start -Descending)) {
    $structInd = IndentOf $L[$s.Start]
    $fnInd = $structInd + '    '

    # 分段（按标签）
    $segs = New-Object System.Collections.ArrayList
    $cur = $s.Start + 1
    $curLabel = $null
    for ($k = $s.Start + 1; $k -lt $s.End; $k++) {
        if ($L[$k] -match '^\s*(public|private|protected):') {
            [void]$segs.Add([pscustomobject]@{ Label = $curLabel; From = $cur; To = $k - 1 })
            $curLabel = $L[$k].Trim()
            $cur = $k + 1
        }
    }
    [void]$segs.Add([pscustomobject]@{ Label = $curLabel; From = $cur; To = $s.End - 1 })

    $orig = $L.GetRange($s.Start, $s.End - $s.Start + 1)
    $newBody = New-Object System.Collections.Generic.List[string]
    $unsupportedWhy = ''
    $reordered = $false

    foreach ($sg in $segs) {
        $items = New-Object System.Collections.ArrayList
        $members = New-Object System.Collections.Generic.List[string]
        $pending = New-Object System.Collections.Generic.List[string]
        for ($k = $sg.From; $k -le $sg.To; $k++) {
            $t = $L[$k].Trim()
            if ($t -eq '') {
                if ($items.Count -eq 0 -and $members.Count -gt 0) { $members.Add('') }
                continue
            }
            if ($t -like '//*' -or $t -like 'template<*') { $pending.Add($L[$k]); continue }
            if (((IndentOf $L[$k]) -eq $fnInd -or (IndentOf $L[$k]) -eq ($structInd + "`t")) -and (IsFnStart $L[$k])) {
                $e = FnEnd $k $sg.To
                $cs = $k - $pending.Count
                [void]$items.Add([pscustomobject]@{ Name = (FnName $L[$k]); From = $cs; To = $e })
                $pending.Clear()
                $k = $e
                continue
            }
            if ($t -match '^(?:struct|class)\s+\w+') {
                # 嵌套 struct/class：整块作为函数项参与分组（Grp 2），与紧随其后的运算符重载同组
                $nd = 0; $ne = $k
                for ($j = $k; $j -le $sg.To; $j++) { $nd += (Bal (StripComment $L[$j]) '{' '}'); if ($nd -le 0) { $ne = $j; break } }
                $cs = $k - $pending.Count
                [void]$items.Add([pscustomobject]@{ Name = (FnName $L[$k]); From = $cs; To = $ne })
                $pending.Clear()
                $k = $ne
                continue
            }
            if ($items.Count -eq 0) {
                foreach ($c in $pending) { $members.Add($c) }
                $pending.Clear()
                $members.Add($L[$k])
            }
            else { if (-not $unsupportedWhy) { $unsupportedWhy = "L$($k+1): $t" } }
        }
        if (-not $unsupportedWhy) { foreach ($c in $pending) { $members.Add($c) } }
        while ($members.Count -gt 0 -and $members[$members.Count - 1] -eq '') { $members.RemoveAt($members.Count - 1) }
        if ($unsupportedWhy) { break }

        foreach ($it in $items) {
            $nm = $it.Name
            # 只有真正的构造函数算构造：create<结构体名>() 归"创建·重建·清理"组（与 clear 系列同组）
            $isCtor = ($nm -eq $s.Name) -or ($nm -eq ($s.Name + '()')) -or ($nm -eq ('~' + $s.Name))
            $isCrud = (-not $isCtor) -and ($nm -match '^(create|recreate|clear)')
            $it | Add-Member -NotePropertyName Grp -NotePropertyValue $(if ($isCtor) { 0 } elseif ($isCrud) { 1 } else { 2 }) -Force
            $it | Add-Member -NotePropertyName Fam -NotePropertyValue $(([regex]::Match($nm, '^[a-z]+')).Value) -Force
            $sub = 0
            if ($isCrud) { if ($nm -match '^recreate') { $sub = 1 } elseif ($nm -match '^clear') { $sub = 2 } }
            $it | Add-Member -NotePropertyName Sub -NotePropertyValue $sub -Force
            $base = ''; $kind = 9
            if ($nm -match '^(get|set|begin|end)(.*)$') { $base = $Matches[2]; $kind = $(if ($Matches[1] -eq 'get' -or $Matches[1] -eq 'begin') { 0 } else { 1 }) }
            $it | Add-Member -NotePropertyName Base -NotePropertyValue $base -Force
            $it | Add-Member -NotePropertyName Kind -NotePropertyValue $kind -Force
        }
        $pairMin = @{}
        foreach ($it in $items) {
            if ($it.Kind -gt 1) { continue }
            if (-not $pairMin.ContainsKey($it.Base)) { $pairMin[$it.Base] = @{ get = -1; set = -1 } }
            $slot = if ($it.Kind -eq 0) { 'get' } else { 'set' }
            if ($pairMin[$it.Base][$slot] -lt 0 -or $it.From -lt $pairMin[$it.Base][$slot]) { $pairMin[$it.Base][$slot] = $it.From }
        }
        foreach ($it in $items) {
            $pk = $it.From
            if ($it.Kind -le 1) {
                $pp = $pairMin[$it.Base]
                if ($pp.get -ge 0 -and $pp.set -ge 0) { $pk = [Math]::Min($pp.get, $pp.set) }
            }
            $it | Add-Member -NotePropertyName PairKey -NotePropertyValue $pk -Force
        }
        $famOrder = [ordered]@{}
        foreach ($it in $items) { if (-not $famOrder.Contains($it.Fam)) { $famOrder[$it.Fam] = $famOrder.Count } }
        $sorted = @($items | Sort-Object -Property @{Expression = { $_.Grp } }, @{Expression = { $_.Sub } }, @{Expression = { $_.PairKey } }, @{Expression = { $_.Kind } }, @{Expression = { $_.From } })
        # 成对族登记处①：get/set、begin/end（含无后缀）——同对必须前后相邻、开者在前；新增同类前缀族只需扩展上面 161 行正则并保持 PairKey/Kind 语义
        # create<基名> / clear<基名>（含空基名）成对：create 必须在前；违规时交换二者位置（中间的 recreate 等保持原位）
        # 成对族登记处②：create/clear 族；日后新增同类族（如 enable/disable）照此再写一段，或并入登记处①的括号嵌套逻辑
        $crudIdx = @{}
        for ($ci = 0; $ci -lt $sorted.Count; $ci++) {
            $cn = $sorted[$ci].Name
            if ($cn -match '^(create|clear)(.*)$') {
                $cb = $Matches[2]
                if (-not $crudIdx.ContainsKey($cb)) { $crudIdx[$cb] = @{ c = -1; d = -1 } }
                if ($Matches[1] -eq 'create') { $crudIdx[$cb].c = $ci } else { $crudIdx[$cb].d = $ci }
            }
        }
        foreach ($cb in @($crudIdx.Keys)) {
            $cv = $crudIdx[$cb]
            if ($cv.c -ge 0 -and $cv.d -ge 0 -and $cv.c -gt $cv.d) {
                $tmpIt = $sorted[$cv.c]; $sorted[$cv.c] = $sorted[$cv.d]; $sorted[$cv.d] = $tmpIt
                $reordered = $true
            }
        }
        if ((@($items | ForEach-Object { $_.From }) -join ',') -ne (@($sorted | ForEach-Object { $_.From }) -join ',')) { $reordered = $true }

        if ($null -ne $sg.Label) {
            if ($newBody.Count -gt 0) { $newBody.Add('') }
            $newBody.Add($structInd + $sg.Label)
        }
        if ($members.Count -gt 0) {
            foreach ($m in $members) { $newBody.Add($m) }
            if ($sorted.Count -gt 0) { $newBody.Add(''); $newBody.Add(''); $newBody.Add('') }
        }
        $prevGrp = -1; $prevFam = ''
        foreach ($it in $sorted) {
            if ($newBody.Count -gt 0) {
                $gap = if ($prevGrp -lt 0) { 0 } elseif ($it.Grp -ne $prevGrp) { 3 } else { 1 }
                if ($gap -eq 3) { $newBody.Add(''); $newBody.Add(''); $newBody.Add('') }
                elseif ($gap -eq 1) { $newBody.Add('') }
            }
            for ($k = $it.From; $k -le $it.To; $k++) { $newBody.Add($L[$k]) }
            $prevGrp = $it.Grp; $prevFam = $it.Fam
        }
    }

    if ($unsupportedWhy) { [void]$skip.Add([pscustomobject]@{ Name = $s.Name; Why = $unsupportedWhy }); continue }

    $trail = 0
    for ($k = $s.End - 1; $k -gt $s.Start; $k--) { if ($L[$k].Trim() -eq '') { $trail++ } else { break } }
    for ($i = 0; $i -lt $trail; $i++) { $newBody.Add('') }

    $new = New-Object System.Collections.Generic.List[string]
    $new.Add($L[$s.Start]); foreach ($x in $newBody) { $new.Add($x) }; $new.Add($L[$s.End])

    $a = NonBlank ([string[]]$orig); $b = NonBlank ([string[]]$new)
    if ((($a | Sort-Object -CaseSensitive) -join "`n") -ne (($b | Sort-Object -CaseSensitive) -join "`n")) {
        $nBad++
        Write-Output ("  [!! 校验失败] {0}  原 {1} 行 / 新 {2} 行" -f $s.Name, $a.Count, $b.Count)
        $d = Compare-Object $a $b
        ($d | Where-Object { $_.SideIndicator -eq '<=' } | Select-Object -First 3) | ForEach-Object { "       仅旧: $($_.InputObject.Trim())" }
        ($d | Where-Object { $_.SideIndicator -eq '=>' } | Select-Object -First 3) | ForEach-Object { "       仅新: $($_.InputObject.Trim())" }
        continue
    }
    if (-not $reordered) {
        # 多重集相同还不够：逐行（含空行）比对，捕捉"纯空行位置"变化（组内 1 空行 / 组间 3 空行）
        if ($new.Count -ne $orig.Count) { $reordered = $true }
        else { for ($qi = 0; $qi -lt $new.Count; $qi++) { if ($new[$qi] -cne $orig[$qi]) { $reordered = $true; break } } }
    }
    if ($reordered) { $nReorder++ }
    $delta = $new.Count - $orig.Count
    $nGap += [Math]::Abs($delta)
    if ($reordered -or $delta -ne 0) { [void]$plan.Add([pscustomobject]@{ Name = $s.Name; Reorder = $reordered; Delta = $delta }) }

    if ($Apply) {
        for ($k = $s.End; $k -ge $s.Start; $k--) { $L.RemoveAt($k) }
        for ($k = $new.Count - 1; $k -ge 0; $k--) { $L.Insert($s.Start, $new[$k]) }
    }
}

Write-Output ("模式: {0}" -f $(if ($Apply) { '落盘' } else { 'dry-run' }))
Write-Output ("结构/类 {0}；跳过 {1}；需重排 {2}；行数变化合计 {3}；校验失败 {4}" -f $structs.Count, $skip.Count, $nReorder, $nGap, $nBad)
Write-Output ''
Write-Output '=== 变更清单 ==='
foreach ($r in $plan) { "  {0,-22} 重排={1,-6} 行数变化={2}" -f $r.Name, $r.Reorder, $r.Delta }
if ($skip.Count -gt 0) {
    Write-Output ''
    Write-Output '=== 跳过（需人工看） ==='
    foreach ($x in $skip) { "  {0,-22} {1}" -f $x.Name, $x.Why }
}
if ($Apply) {
    $out = ($L -join "`r`n"); if ($raw.EndsWith("`r`n")) { $out += "`r`n" }
    [System.IO.File]::WriteAllText($p, $out, $enc)
    $bytes = [System.IO.File]::ReadAllBytes($p); $txt = [System.IO.File]::ReadAllText($p)
    Write-Output ''
    Write-Output ("编码: 字节={0} 行数={1} bom={2} crlf={3} 纯LF={4} 尾字节={5}" -f $bytes.Length, $L.Count, (($bytes[0] -eq 0xEF) -and ($bytes[1] -eq 0xBB) -and ($bytes[2] -eq 0xBF)), ([regex]::Matches($txt, "`r`n")).Count, ([regex]::Matches($txt, "(?<!`r)`n")).Count, $bytes[$bytes.Length - 1])
}
Write-Output 'DONE'