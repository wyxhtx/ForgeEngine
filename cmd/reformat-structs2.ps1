param([switch]$Apply, [switch]$Layers, [string]$Path = '')
$ErrorActionPreference = 'Stop'
$g = Split-Path $PSScriptRoot -Parent    # 仓库根 = 本脚本所在目录(cmd) 的上级
if ($Path -ne '') { $p = $Path } else { $p = "$g\src\code\vk\data\VkStruct.h" }
$enc = New-Object System.Text.UTF8Encoding($false)

$raw = [System.IO.File]::ReadAllText($p)
if ([regex]::Matches($raw, "(?<!`r)`n").Count -gt 0) { throw "存在纯 LF 行: $p" }
$L = New-Object System.Collections.Generic.List[string]
foreach ($ln in [System.IO.File]::ReadAllLines($p, $enc)) { $L.Add($ln) }
$NF = $L.Count

# ---- 纯函数记忆化缓存（键 = 行文本；Ordinal 比较器区分大小写，保证与直接调用逐字节等价） ----
# 这些函数在同一行上会被反复调用（StripComment 每行曾被调 4~6 次），缓存后同一行只解析一次
$stripCache   = [hashtable]::new([System.StringComparer]::Ordinal)   # StripComment
$balCache     = [hashtable]::new([System.StringComparer]::Ordinal)   # Bal（文本 → 各括号对的差值）
$indCache     = [hashtable]::new([System.StringComparer]::Ordinal)   # IndentOf
$fnStartCache = [hashtable]::new([System.StringComparer]::Ordinal)   # IsFnStart
$dataMemCache = [hashtable]::new([System.StringComparer]::Ordinal)   # IsDataMember

function IndentOf([string]$x) {
    if ($indCache.ContainsKey($x)) { return $indCache[$x] }
    $v = ([regex]::Match($x, '^[ \t]*')).Value
    $indCache[$x] = $v
    return $v
}
function StripComment([string]$x) {
    if ($stripCache.ContainsKey($x)) { return $stripCache[$x] }
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
    $r = $sb.ToString()
    $stripCache[$x] = $r
    return $r
}
function Bal([string]$s, [string]$open, [string]$close) {
    $ck = $open + $close
    $m = $balCache[$s]
    if ($null -eq $m) { $m = @{}; $balCache[$s] = $m }
    if ($m.ContainsKey($ck)) { return $m[$ck] }
    $v = ([regex]::Matches($s, [regex]::Escape($open))).Count - ([regex]::Matches($s, [regex]::Escape($close))).Count
    $m[$ck] = $v
    return $v
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
    if ($fnStartCache.ContainsKey($x)) { return $fnStartCache[$x] }
    $r = TestFnStartRaw $x
    $fnStartCache[$x] = $r
    return $r
}
function TestFnStartRaw([string]$x) {
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
        # 参数表未闭合且尚未进入函数体 → 本行还不算函数体起点（原有行为）；
        # 已进入函数体（含 lambda / 多行调用实参）时，括号内的花括号必须照常计数，
        # 否则 `xxx.submit([this] {` 这类 lambda 的 { 会被漏计，函数末尾的 } 会变成孤儿行
        if ($par -gt 0 -and -not $inBody) { continue }
        $ob = (Bal $code '{' '}')
        if (-not $inBody) {
            if ($code -match '\{') { $inBody = $true; $depth += $ob }
            else { if ($code.TrimEnd().EndsWith(';')) { return $j } }
        }
        else { $depth += $ob }
        if ($inBody -and $par -le 0 -and $depth -le 0) { return $j }
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
function MemberEnd([int]$k, [int]$limit) {
    # 数据成员声明块的结束行：花括号配对归零 且 该行以 ';' 结尾（支持多行 Type name{ … }; / Type name = …;）
    $depth = 0
    for ($j = $k; $j -le $limit; $j++) {
        $code = StripComment $L[$j]
        $depth += (Bal $code '{' '}')
        if ($depth -le 0 -and $code.TrimEnd().EndsWith(';')) { return $j }
    }
    return -1
}
function IsDataMember([string]$x) {
    if ($dataMemCache.ContainsKey($x)) { return $dataMemCache[$x] }
    $r = TestDataMemberRaw $x
    $dataMemCache[$x] = $r
    return $r
}
function TestDataMemberRaw([string]$x) {
    # 真正的数据成员：Type name; / Type name{}; / Type name{ … }; / Type name = …; / Type name = { … };
    # 复用 IsMemberDecl（第一个 { 或 = 在第一个 ( 之前），并要求"声明头 = 类型 + 名字"至少两个记号，
    # 以排除 }; / #if 宏 / 孤立注释 / 调用与表达式等"函数之后的非函数行"
    $t2 = (StripComment $x).Trim()
    if ($t2 -eq '' -or $t2 -like '#*') { return $false }
    # 运算符重载是函数声明，绝非数据成员（其名字里带 '==' / '=' 会骗过下面的初始化式判定）
    if ($t2 -match '\boperator\b') { return $false }
    if (-not (IsMemberDecl $t2)) { return $false }
    $cut = $t2.Length
    $bi = $t2.IndexOf('{'); if ($bi -ge 0) { $cut = $bi }
    # 只有真正的初始化 '=' 才算声明头结束：==/!=/<=/>=/+= 等运算符里的 '=' 不算
    $ei = -1
    for ($i = 1; $i -lt $t2.Length; $i++) {
        if ($t2[$i] -ne '=') { continue }
        if ($t2[$i - 1] -match '[=!<>+\-*/%&|^]' -or ($i + 1 -lt $t2.Length -and $t2[$i + 1] -eq '=')) { continue }
        $ei = $i; break
    }
    if ($ei -ge 0 -and $ei -lt $cut) { $cut = $ei }
    $si = $t2.IndexOf(';'); if ($si -ge 0 -and $si -lt $cut) { $cut = $si }
    if ($cut -le 0) { return $false }
    $head = $t2.Substring(0, $cut).Trim()
    if ($head -eq '') { return $false }
    if ($head -match '[(){}\[\]=;,\.]') { return $false }
    $toks = @($head -split '\s+' | Where-Object { $_ -ne '' })
    if ($toks.Count -lt 2) { return $false }
    if ($toks[$toks.Count - 1] -notmatch '^[\w~]+(\[[^\]]*\])?$') { return $false }
    return $true
}
function NonBlank([string[]]$arr) { return @($arr | Where-Object { $_.Trim() -ne '' }) }
function InitKeyOf([string]$t2) {
    # 初始化 '=' 的下标（==/!=/<=/>=/+= 等运算符里的 '=' 不算），无则 -1
    for ($i = 1; $i -lt $t2.Length; $i++) {
        if ($t2[$i] -ne '=') { continue }
        if ($t2[$i - 1] -match '[=!<>+\-*/%&|^]' -or ($i + 1 -lt $t2.Length -and $t2[$i + 1] -eq '=')) { continue }
        return $i
    }
    return -1
}
function DeclName([string]$line) {
    # 数据成员声明的"名字" = 声明头最后一个记号（去数组、去 * / & 前缀）
    $t2 = (StripComment $line).Trim()
    if ($t2 -eq '' -or $t2 -like '#*') { return '' }
    $cut = $t2.Length
    $si = $t2.IndexOf(';'); if ($si -ge 0 -and $si -lt $cut) { $cut = $si }
    $bi = $t2.IndexOf('{'); if ($bi -ge 0 -and $bi -lt $cut) { $cut = $bi }
    $ei = InitKeyOf $t2;    if ($ei -ge 0 -and $ei -lt $cut) { $cut = $ei }
    if ($cut -le 0) { return '' }
    $head = $t2.Substring(0, $cut).Trim()
    $toks = @($head -split '\s+' | Where-Object { $_ -ne '' })
    if ($toks.Count -lt 2) { return '' }
    $nm = $toks[$toks.Count - 1]
    $nm = ($nm -replace '\[.*$', '')
    $nm = ($nm -replace '^[*&]+', '')
    return $nm
}
function CallName([string]$nm) {
    # 可作为"被调用名"解析的项名；否则返回 ''（嵌套类型 / 运算符重载 等 → 保守，保持原位）
    $x = $nm
    if ($x.EndsWith('()')) { $x = $x.Substring(0, $x.Length - 2) }
    if ($x -match '^~?[A-Za-z_]\w*$') { return $x }
    return ''
}
function HeadText([int]$from, [int]$to) {
    # 函数"签名区"文本：From 起，到首个含 '{' 或行尾 ';' 的行为止（含该行）
    $sb = New-Object System.Text.StringBuilder
    for ($j = $from; $j -le $to; $j++) {
        if ($j -lt 0 -or $j -ge $NF) { continue }
        $code = StripComment $L[$j]
        [void]$sb.Append(' ').Append($code)
        if ($code -match '\{' -or $code.TrimEnd().EndsWith(';')) { break }
    }
    return $sb.ToString()
}
function OwnCalls([int]$from, [int]$to, [string]$structName, [hashtable]$nameMap) {
    # **函数体**里出现的"本类成员函数名("调用，按出现先后去重返回：
    #   this->foo(  / 类名::foo( / 裸 foo( 均算；obj.foo( / obj->foo( / 其它 X::foo( 不算（非本类调用）
    # 签名区（From 起至首个 '{' 之前）不扫：否则函数自己的名字会被当成本类的调用（自环），形参里的
    # "名字后紧跟 (" 也会被误认；纯声明行（无 '{'）整行跳过，不产生任何边。
    $res = New-Object System.Collections.Generic.List[string]
    $seen = @{}
    $started = $false
    for ($j = $from; $j -le $to; $j++) {
        if ($j -lt 0 -or $j -ge $NF) { continue }
        $code = StripComment $L[$j]
        if ($code -eq '') { continue }
        if (-not $started) {
            $bi = $code.IndexOf('{')
            if ($bi -lt 0) { continue }
            $code = $code.Substring($bi + 1)
            $started = $true
        }
        foreach ($m in [regex]::Matches($code, '([A-Za-z_]\w*)\s*(?:<[^<>]*>\s*)?\(')) {
            $nm = $m.Groups[1].Value
            if (-not $nameMap.ContainsKey($nm)) { continue }
            $p0 = $m.Groups[1].Index
            $ok = $false
            if ($p0 -eq 0) { $ok = $true }
            else {
                $pre = $code.Substring(0, $p0)
                if ($pre -match 'this\s*->\s*$') { $ok = $true }
                elseif ($pre -match ([regex]::Escape($structName) + '\s*::\s*$')) { $ok = $true }
                elseif ('.:>'.IndexOf([string]$code[$p0 - 1]) -lt 0) { $ok = $true }
            }
            if ($ok -and -not $seen.ContainsKey($nm)) { $seen[$nm] = $true; [void]$res.Add($nm) }
        }
    }
    return $res
}

# ---- 结构体范围 ----
$structs = New-Object System.Collections.ArrayList
for ($k = 0; $k -lt $L.Count; $k++) {
    # 支持 class 与 struct（class 默认访问为 private，标签处理同 struct）
    if ($L[$k] -match '^(\s*)(?:struct|class)\s+(\w+)\s*(?:final)?\s*(?::[^{]*)?\{$') {
        $ind = $Matches[1]; $nm = $Matches[2]
        $e = -1
        for ($j = $k + 1; $j -lt $L.Count; $j++) { if ($L[$j] -match ('^' + [regex]::Escape($ind) + '\};')) { $e = $j; break } }
        if ($e -gt 0) { [void]$structs.Add([pscustomobject]@{ Name = $nm; Start = $k; End = $e; IsClass = ($L[$k] -match '^\s*class\s') }) }
    }
}

$plan = New-Object System.Collections.ArrayList
$skip = New-Object System.Collections.ArrayList
$shield = New-Object System.Collections.ArrayList      # 同名遮蔽：数据成员不上移
$inplace = New-Object System.Collections.ArrayList     # 调用关系不可解析：函数项保持原位
$nReorder = 0; $nGap = 0; $nBad = 0
$nFn = 0; $nUnreach = 0
$famDump = New-Object System.Collections.ArrayList      # 同后缀家族（-Layers 自证）
$layerDump = New-Object System.Collections.ArrayList
$edgeDump = New-Object System.Collections.ArrayList
$grpDump = New-Object System.Collections.ArrayList      # struct 的最终发射顺序（Grp/Sub/组内空行），配合 -Layers 自证分组
$BIG = 1000000000.0

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
    $unsupportedWhy = ''

    # ---------- 第一遍：逐段解析成 成员区 / 函数项 / 尾部数据成员块 ----------
    $segParsed = New-Object System.Collections.ArrayList
    foreach ($sg in $segs) {
        $items = New-Object System.Collections.ArrayList
        $members = New-Object System.Collections.Generic.List[string]
        $tails = New-Object System.Collections.ArrayList
        $pending = New-Object System.Collections.Generic.List[string]
        for ($k = $sg.From; $k -le $sg.To; $k++) {
            $t = $L[$k].Trim()
            if ($t -eq '') {
                if ($items.Count -eq 0 -and $members.Count -gt 0) { $members.Add('') }
                continue
            }
            if ($t -like '//*' -or $t -like 'template<*') { $pending.Add($L[$k]); continue }
            if (((IndentOf $L[$k]) -eq $fnInd -or (IndentOf $L[$k]) -eq ($structInd + "`t")) -and (IsFnStart $L[$k]) -and (-not (IsDataMember $L[$k]))) {
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
            elseif (IsDataMember $L[$k]) {
                # 尾部数据成员块：默认整块上移到本访问段成员区（成员置顶），不做任何改名
                $me = MemberEnd $k $sg.To
                if ($me -lt 0) { if (-not $unsupportedWhy) { $unsupportedWhy = "L$($k+1): $t" } }
                else {
                    $ls = New-Object System.Collections.Generic.List[string]
                    foreach ($c in $pending) { $ls.Add($c) }
                    $pending.Clear()
                    for ($j = $k; $j -le $me; $j++) { $ls.Add($L[$j]) }
                    $nmT = ''
                    foreach ($x in $ls) { if ((StripComment $x).Trim() -ne '') { $nmT = DeclName $x; break } }
                    [void]$tails.Add([pscustomobject]@{ Lines = $ls; First = $k; Last = $me; Anchor = ($items.Count - 1); Name = $nmT })
                    $k = $me
                }
            }
            else { if (-not $unsupportedWhy) { $unsupportedWhy = "L$($k+1): $t" } }
        }
        if (-not $unsupportedWhy) { foreach ($c in $pending) { $members.Add($c) } }
        while ($members.Count -gt 0 -and $members[$members.Count - 1] -eq '') { $members.RemoveAt($members.Count - 1) }
        [void]$segParsed.Add([pscustomobject]@{ Label = $sg.Label; From = $sg.From; To = $sg.To; Members = $members; Items = $items; Tails = $tails })
        if ($unsupportedWhy) { break }
    }
    if ($unsupportedWhy) { [void]$skip.Add([pscustomobject]@{ Name = $s.Name; Why = $unsupportedWhy }); continue }

    # ---------- 本类"签名区出现的类型名"集合（同名遮蔽判定用） ----------
    $headIds = @{}
    foreach ($sd in $segParsed) {
        foreach ($it in $sd.Items) {
            foreach ($m in [regex]::Matches((HeadText $it.From $it.To), '[A-Za-z_]\w*')) { $headIds[$m.Value] = $true }
        }
    }

    # ---------- 尾部数据成员块去向：上移 / 同名遮蔽整块原位 ----------
    # 最小让步：若某数据成员的名字与"本类某个签名里出现的类型名"相同，上移就会造成同名遮蔽（C2327 一类），
    # 该成员不上移；并且它所在的**同一连续数据成员块**整块不上移——否则同块其它成员会被上移到它前面，
    # 改变类内成员声明顺序（= 初始化顺序），使依赖该成员的初始化式读到未绑定引用（未定义行为）。
    for ($si = 0; $si -lt $segParsed.Count; $si++) {
        $sd = $segParsed[$si]
        $mode = @{}
        $runs = New-Object System.Collections.ArrayList
        $r0 = 0
        while ($r0 -lt $sd.Tails.Count) {
            $r1 = $r0
            while ($r1 + 1 -lt $sd.Tails.Count -and $sd.Tails[$r1 + 1].First -eq ($sd.Tails[$r1].Last + 1)) { $r1++ }
            $who = ''; $isShield = $false
            for ($ti = $r0; $ti -le $r1; $ti++) {
                $nmT = $sd.Tails[$ti].Name
                if ($nmT -ne '' -and $headIds.ContainsKey($nmT)) { $isShield = $true; $who = $nmT; break }
            }
            $rlines = New-Object System.Collections.Generic.List[string]
            for ($ti = $r0; $ti -le $r1; $ti++) { foreach ($x in $sd.Tails[$ti].Lines) { $rlines.Add($x) } }
            [void]$runs.Add([pscustomobject]@{ Pin = $isShield; Name = $sd.Tails[$r0].Name; Lines = $rlines; First = $sd.Tails[$r0].First; Anchor = $sd.Tails[$r0].Anchor })
            for ($ti = $r0; $ti -le $r1; $ti++) { $mode[$ti] = $(if ($isShield) { 'pin' } else { 'hoist' }) }
            if ($isShield) {
                [void]$shield.Add([pscustomobject]@{ Struct = $s.Name; Line = ($sd.Tails[$r0].First + 1); Member = $who; Run = ($r1 - $r0 + 1) })
            }
            $r0 = $r1 + 1
        }
        $sd | Add-Member -NotePropertyName Runs -NotePropertyValue $runs -Force
        $sd | Add-Member -NotePropertyName TailMode -NotePropertyValue $mode -Force
    }

    # ---------- 组别 / 成对族（沿用既有判定） ----------
    $allFn = New-Object System.Collections.ArrayList
    $sidx = 0
    foreach ($sd in $segParsed) {
        foreach ($it in $sd.Items) {
            $it | Add-Member -NotePropertyName SIdx -NotePropertyValue $sidx -Force
            $sidx++
            [void]$allFn.Add($it)
        }
    }
    foreach ($it in $allFn) {
        $nm = $it.Name
        # 只有真正的构造函数算构造：create<结构体名>() 归"创建·重建·清理"组（与 clear 系列同组）
        $isCtor = ($nm -eq $s.Name) -or ($nm -eq ($s.Name + '()')) -or ($nm -eq ('~' + $s.Name))
            # 创建族条件①：本结构体自己的生命周期入口（create/recreate/clear，含裸名与 <结构名>/<去Node基名>）
            # 条件②（调用层）与"无构造函数"兜底见下方"创建族扩展"段；两者满足任一即入创建组（Grp 1）
            # 其余 createXxx 辅助函数（如 createChunk，其调用者不全是构造/① 入口）不归创建组，改由调用层分组决定
            $sBase = $s.Name -replace 'Node$', '' 
            $isCrud = (-not $isCtor) -and ($nm -eq 'create' -or $nm -eq 'recreate' -or $nm -eq 'clear' -or $nm -eq ('create' + $s.Name) -or $nm -eq ('recreate' + $s.Name) -or $nm -eq ('clear' + $s.Name) -or $nm -eq ('create' + $sBase) -or $nm -eq ('recreate' + $sBase) -or $nm -eq ('clear' + $sBase))
        $it | Add-Member -NotePropertyName Grp -NotePropertyValue $(if ($isCtor) { 0 } elseif ($isCrud) { 1 } else { 2 }) -Force
        $it | Add-Member -NotePropertyName Fam -NotePropertyValue $(([regex]::Match($nm, '^[a-z]+')).Value) -Force
        $sub = 0
        if ($isCrud) { if ($nm -match '^recreate') { $sub = 1 } elseif ($nm -match '^clear') { $sub = 2 } }
        $it | Add-Member -NotePropertyName Sub -NotePropertyValue $sub -Force
        $base = ''; $kind = 9
        if ($nm -match '^(get|set|begin|end)(.*)$') { $base = $Matches[2]; $kind = $(if ($Matches[1] -eq 'get' -or $Matches[1] -eq 'begin') { 0 } else { 1 }) }
        $it | Add-Member -NotePropertyName Base -NotePropertyValue $base -Force
        $it | Add-Member -NotePropertyName Kind -NotePropertyValue $kind -Force
        $it | Add-Member -NotePropertyName CallN -NotePropertyValue (CallName $nm) -Force
    }

    # ---------- 创建族扩展：② 调用层条件 + 兜底（无构造函数） ----------
    # ① = 上面 $isCrud（本结构体自己的生命周期入口：裸 create/recreate/clear 或 create<结构名>/<去Node基名>）
    # ② = 名字以 create/recreate/clear 开头的同族辅助，且其**全部**直接调用者都是构造函数或 ① 类入口（至少一个调用者）
    #      → "只服务于本结构体创建流程"的辅助（如 imageBufferListNode::createSwapImageList/clearView）。
    #      反例：Plate::createChunk 虽由 createPlate 调用，但还被 updatePlate 调用 → 不满足"全部" → 不入创建族，
    #      仍由调用层/成对规则分组（用户要求它与 joinTerrainMesh 同组）。
    # 兜底：本结构体没有构造函数（如 imageBufferListNode 这种纯数据结点）→ 名字族的 create*/recreate*/clear* 一律算创建族
    $hasCtor = $false
    foreach ($it in $allFn) { if ($it.Grp -eq 0) { $hasCtor = $true; break } }
    $famCand = New-Object System.Collections.ArrayList
    foreach ($it in $allFn) {
        if ($it.Grp -eq 1) { continue }
        if ($it.Name -match '^(create|recreate|clear)') { [void]$famCand.Add($it) }
    }
    $famAdd = @{}
    if ($famCand.Count -gt 0) {
        if (-not $hasCtor) { foreach ($it in $famCand) { $famAdd[$it.Name] = $true } }
        else {
            $nmAll = @{}
            foreach ($it in $allFn) { if ($it.CallN -ne '') { $nmAll[$it.CallN] = $true } }
            $callers = @{}
            foreach ($it in $allFn) {
                if ($it.CallN -eq '') { continue }
                foreach ($cn in (OwnCalls $it.From $it.To $s.Name $nmAll)) {
                    if (-not $callers.ContainsKey($cn)) { $callers[$cn] = New-Object System.Collections.ArrayList }
                    [void]$callers[$cn].Add($it)
                }
            }
            foreach ($it in $famCand) {
                $cs = $callers[$it.Name]
                if ($null -eq $cs -or $cs.Count -eq 0) { continue }
                $ok = $true
                foreach ($c in $cs) { if ($c.Grp -eq 2) { $ok = $false; break } }
                if ($ok) { $famAdd[$it.Name] = $true }
            }
        }
    }
    foreach ($it in $allFn) {
        if (-not $famAdd.ContainsKey($it.Name)) { continue }
        $it.Grp = 1
        $it.Sub = $(if ($it.Name -match '^recreate') { 1 } elseif ($it.Name -match '^clear') { 2 } else { 0 })
    }

    # ---------- 类内 BFS 分层：入口 = 构造函数族（Grp 0，含析构），层号取最浅 ----------
    foreach ($it in $allFn) {
        $it | Add-Member -NotePropertyName Layer -NotePropertyValue $BIG -Force
        $it | Add-Member -NotePropertyName Ord -NotePropertyValue ($BIG + $it.SIdx) -Force
    }
    $nFn += $allFn.Count
    # struct 走"成对优先"排序：分层分量（ELayer/EOrd）稍后会被整体清零，分层结果对 struct 的排序毫无影响
    # → 整段调用图分析（建 nameMap / BFS 边扫描 / 不可达统计 / 分层明细）对 struct 全部跳过（性能优化，输出排序不变）
    if ($s.IsClass) {
        $nameMap = @{}
        foreach ($it in $allFn) {
            if ($it.CallN -eq '') { continue }
            if (-not $nameMap.ContainsKey($it.CallN)) { $nameMap[$it.CallN] = New-Object System.Collections.ArrayList }
            [void]$nameMap[$it.CallN].Add($it)
        }
        $queue = New-Object System.Collections.ArrayList
        $seq = 0.0
        foreach ($it in $allFn) {
            if ($it.Grp -eq 0) { $it.Layer = 0.0; $it.Ord = $seq; $seq++; [void]$queue.Add($it) }
        }
        $qi = 0
        while ($qi -lt $queue.Count) {
            $cur = $queue[$qi]; $qi++
            if ($cur.CallN -eq '') { continue }
            foreach ($cn in (OwnCalls $cur.From $cur.To $s.Name $nameMap)) {
                if ($Layers) { [void]$edgeDump.Add(("{0}: {1}[L{2}] -> {3}" -f $s.Name, $cur.Name, $cur.Layer, $cn)) }
                foreach ($cal in $nameMap[$cn]) {
                    if ($cal.Layer -ge $BIG) {
                        $cal.Layer = $cur.Layer + 1.0
                        $cal.Ord = $seq; $seq++
                        [void]$queue.Add($cal)
                    }
                }
            }
        }
        foreach ($it in $allFn) { if ($it.Layer -ge $BIG) { $nUnreach++ } }
        if ($Layers) { foreach ($it in $allFn) { [void]$layerDump.Add([pscustomobject]@{ S = $s.Name; G = $it.Grp; L = $it.Layer; O = $it.Ord; N = $it.Name }) } }
    }

    # ---------- 逐段排序 + 发射 ----------
    $newBody = New-Object System.Collections.Generic.List[string]
    $reordered = $false
    for ($si = 0; $si -lt $segParsed.Count; $si++) {
        $sd = $segParsed[$si]
        # ---------- 同后缀家族：同一访问段内"末位驼峰词"相同、彼此不互相调用的兄弟函数
        #   例：Camera 的 mouseHandle / keyHandle / scrollHandle（三个同族处理器）→ 视为一个子块：
        #   整体落在族内"已知调用层"（不可达成员并入该层），块内按原顺序压缩 Ord（保证紧挨、块内 1 空行）；
        #   族内被调用的辅助函数（如 updateView，层更深）不受影响，仍在族块之后、跨层空 3 行。
        #   排除① 派生名：一个成员名以另一成员名结尾、忽略大小写（isHandleChangeCubeList 之于 handleChangeCubeList）→ 不算兄弟族
        #   排除② 已知层不唯一：族内既有 L1 又有 L2（initUniformBuffer 与 createVertexBuffer/IndexBuffer）→ 不并层
        #        只有"同一已知层 + 不可达成员"（或全不可达）才成族，避免把深层函数上拉到浅层
        #   排除③ 族内互相调用（createVertexBuffer → createBuffer 之类，本来就不是并列兄弟）
        #   排除④ 构造/创建族（Grp≠2）、get/set 成对族（Kind≤1）、调用名不可解析（CallN 空）
        if ($s.IsClass) {
            $famOf = @{}
            foreach ($it in $sd.Items) {
                if ($null -eq $it) { continue }
                if ($it.Grp -ne 2) { continue }
                if ($it.Kind -le 1) { continue }
                if ($it.CallN -eq '') { continue }
                $sfx = ([regex]::Match($it.Name, '([A-Z][a-z0-9]*)$')).Value
                if ($sfx -eq '') { $sfx = $it.Name }
                if (-not $famOf.ContainsKey($sfx)) { $famOf[$sfx] = New-Object System.Collections.ArrayList }
                [void]$famOf[$sfx].Add($it)
            }
            foreach ($sfx in @($famOf.Keys)) {
                $famItems = $famOf[$sfx]
                if ($famItems.Count -lt 2) { continue }
                $famDerived = $false
                foreach ($fa in $famItems) {
                    foreach ($fb in $famItems) {
                        if ($fa.Name.Length -gt $fb.Name.Length -and $fa.Name.ToLowerInvariant().EndsWith($fb.Name.ToLowerInvariant())) { $famDerived = $true; break }
                    }
                    if ($famDerived) { break }
                }
                if ($famDerived) { continue }
                $famLayer = -1.0; $famMulti = $false
                foreach ($it in $famItems) {
                    if ($it.Layer -ge $BIG) { continue }
                    if ($famLayer -lt 0) { $famLayer = $it.Layer }
                    elseif ($it.Layer -ne $famLayer) { $famMulti = $true; break }
                }
                if ($famMulti) { continue }
                $famNames = @{}
                foreach ($it in $famItems) { $famNames[$it.CallN] = $true }
                $famBad = $false
                foreach ($it in $famItems) {
                    foreach ($cn in (OwnCalls $it.From $it.To $s.Name $nameMap)) { if ($famNames.ContainsKey($cn)) { $famBad = $true; break } }
                    if ($famBad) { break }
                }
                if ($famBad) { continue }
                if ($famLayer -lt 0) { $famLayer = [double]$BIG }
                $famOrd = @($famItems | Sort-Object -Property SIdx)
                $famMin = [double]::MaxValue
                foreach ($it in $famOrd) { if ($it.Ord -lt $famMin) { $famMin = $it.Ord } }
                $fk = 0
                foreach ($it in $famOrd) {
                    $it.Layer = $famLayer
                    $it.Ord = $famMin + $fk * 0.0001
                    $fk++
                }
                if ($Layers) { [void]$famDump.Add([pscustomobject]@{ S = $s.Name; Fam = $sfx; L = $famLayer; N = (($famOrd | ForEach-Object { $_.Name }) -join ',') }) }
            }
        }
        $members = New-Object System.Collections.Generic.List[string]
        foreach ($m in $sd.Members) { $members.Add($m) }
        $emit = New-Object System.Collections.ArrayList
        $byIdx = @{}
        for ($ii = 0; $ii -lt $sd.Items.Count; $ii++) {
            $it = $sd.Items[$ii]
            $lines = New-Object System.Collections.Generic.List[string]
            for ($j = $it.From; $j -le $it.To; $j++) { $lines.Add($L[$j]) }
            $e = [pscustomobject]@{ IsPin = $false; It = $it; Name = $it.Name; From = $it.From; Lines = $lines; EGrp = $it.Grp; ESub = $it.Sub; ELayer = $it.Layer; EOrd = $it.Ord; EPairKey = $it.From; EKind = $it.Kind; EGrpK = 0.0; EFrom = [double]$it.From }
            if ($it.CallN -eq '') {
                # 无法解析调用关系（嵌套类型 / 运算符重载 / 无法命名的声明）→ 保守：保持原位（锚定其前一函数项）
                $e.IsPin = $true
                if ($ii -eq 0) { $e.ESub = -1.0; $e.ELayer = -1.0; $e.EOrd = -1.0 }
                else {
                    $prev = $byIdx[$ii - 1]
                    $e.EGrp = $prev.EGrp; $e.ESub = $prev.ESub; $e.ELayer = $prev.ELayer; $e.EOrd = $prev.EOrd + 0.25
                }
                [void]$inplace.Add([pscustomobject]@{ Struct = $s.Name; Line = ($it.From + 1); Name = $it.Name })
            }
            $byIdx[$ii] = $e
            [void]$emit.Add($e)
        }
        for ($ri = 0; $ri -lt $sd.Runs.Count; $ri++) {
            $run = $sd.Runs[$ri]
            if (-not $run.Pin) {
                foreach ($x in $run.Lines) { $members.Add($x) }
                $reordered = $true
            }
            else {
                # 同名遮蔽：整块保持原位（锚定其上方的函数项，块内不加空行）
                $e = [pscustomobject]@{ IsPin = $true; It = $null; Name = $run.Name; From = $run.First; Lines = $run.Lines; EGrp = 2; ESub = -1.0; ELayer = -1.0; EOrd = -1.0; EPairKey = $run.First; EKind = 9; EGrpK = 0.0; EFrom = [double]$run.First }
                if ($run.Anchor -ge 0) {
                    $anc = $byIdx[$run.Anchor]
                    $e.EGrp = $anc.EGrp; $e.ESub = $anc.ESub; $e.ELayer = $anc.ELayer; $e.EOrd = $anc.EOrd + 0.25
                }
                [void]$emit.Add($e)
            }
        }
        $pairMin = @{}
        foreach ($it in $sd.Items) {
            if ($it.Kind -gt 1) { continue }
            if (-not $pairMin.ContainsKey($it.Base)) { $pairMin[$it.Base] = @{ get = -1; set = -1 } }
            $slot = if ($it.Kind -eq 0) { 'get' } else { 'set' }
            if ($pairMin[$it.Base][$slot] -lt 0 -or $it.From -lt $pairMin[$it.Base][$slot]) { $pairMin[$it.Base][$slot] = $it.From }
        }
        foreach ($e in $emit) {
            if ($null -eq $e.It) { continue }
            $pk = $e.It.From
            if ($e.It.Kind -le 1) {
                $pp = $pairMin[$e.It.Base]
                if ($pp.get -ge 0 -and $pp.set -ge 0) { $pk = [Math]::Min($pp.get, $pp.set) }
            }
            $e.EPairKey = $pk
        }
        # 排序键（**层优先**）：Layer（调用层升序，不可达者 = +∞ 置最末）
        #         → EGrpK（**组规则只在 L1 生效**：Layer==1 取 Grp（组内 Sub：create=0 / recreate=1 / clear=2，
        #           恢复"先建后清"），Layer>=2 常量化为 0 → 层内一律按调用先后 EOrd / 原位排，不再被"创建组"提前）
        #         → Sub → Ord（层内被上层调用的先后）→ EPairKey/EKind/EFrom（既有 get/set 成对与位置兜底）
        # struct 不参与层优先：其 ELayer 已清零（< 2）→ EGrpK 恒等于 EGrp，
        #         排序键退化为 Grp → Sub → PairKey → Kind → 原位置（与旧键逐项等价，struct 排序不变）
        if (-not $s.IsClass) { foreach ($e2 in $emit) { $e2.ELayer = 0.0; $e2.EOrd = 0.0 } }
        foreach ($e2 in $emit) { $e2.EGrpK = $(if ($e2.ELayer -ge 2.0) { 0.0 } else { [double]$e2.EGrp }) }
        # 排序键已是发射时算好的普通属性（ELayer/EGrpK/ESub/EOrd/EPairKey/EKind/EFrom），直接按属性名排序；
        # 不再用 @{Expression = {...}} 脚本块键（脚本块键每比较一次就调用一次，开销远大于属性取值）
        $sorted = @($emit | Sort-Object -Property ELayer, EGrpK, ESub, EOrd, EPairKey, EKind, EFrom)
        # 成对族登记处②：create/clear 族（create 必须在前；违规时交换二者位置，中间的 recreate 等保持原位）
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
        $origSeq = (@($emit | Sort-Object -Property From | ForEach-Object { "F$($_.From)" }) -join ',')
        $newSeq = (@($sorted | ForEach-Object { "F$($_.From)" }) -join ',')
        if ($origSeq -ne $newSeq) { $reordered = $true }

        if ($null -ne $sd.Label) {
            if ($newBody.Count -gt 0) { $newBody.Add('') }
            $newBody.Add($structInd + $sd.Label)
        }
        if ($members.Count -gt 0) {
            foreach ($m in $members) { $newBody.Add($m) }
            if ($sorted.Count -gt 0) { $newBody.Add(''); $newBody.Add(''); $newBody.Add('') }
        }
        # 组内空 1 行；跨组或跨层空 3 行。**组规则只在 L1 生效**：分组边界比较改用 EGrpK
        # （Layer>=2 时 EGrpK 已常量化为 0）→ L2 及更深不再有"组间 3 空行"，同层一律 1 空行；
        # Layer<=1 的 EGrpK = 真实 Grp → L1 内"创建族 ↔ 其余"之间仍是 3 空行（struct 亦等价，其 EGrpK = EGrp）
        $prevGrpK = -1; $prevLayer = -1.0
        foreach ($e in $sorted) {
            $gap = 0
            if ($newBody.Count -gt 0) {
                $gap = if ($prevGrpK -lt 0) { 0 } elseif (($e.EGrpK -ne $prevGrpK) -or ($e.ELayer -ne $prevLayer)) { 3 } else { 1 }
                if ($gap -eq 3) { $newBody.Add(''); $newBody.Add(''); $newBody.Add('') }
                elseif ($gap -eq 1) { $newBody.Add('') }
            }
            if ($Layers -and (-not $s.IsClass) -and $null -ne $e.It) { [void]$grpDump.Add([pscustomobject]@{ S = $s.Name; G = [int]$e.EGrp; Sb = [int]$e.ESub; Gap = $gap; N = $e.Name }) }
            foreach ($x in $e.Lines) { $newBody.Add($x) }
            $prevGrpK = $e.EGrpK; $prevLayer = $e.ELayer
        }
    }

    # 容器结束前不留空行（作者要求）：正文末尾与闭合 }; 之间 **0 空行**——原"保留原尾部空行"逻辑已取消；
    # 同样适用于"容器最后是访问标签"的情形（标签与 }; 之间也无空行）。空行不计入非空行多重集校验，故校验不受影响。

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

    # 只跑必要次数：本结构体逐行判定为"未变化"（$reordered 保持 false）时，重写内容与原文完全一致 → 跳过拼接
    if ($Apply -and $reordered) {
        for ($k = $s.End; $k -ge $s.Start; $k--) { $L.RemoveAt($k) }
        for ($k = $new.Count - 1; $k -ge 0; $k--) { $L.Insert($s.Start, $new[$k]) }
    }
}

Write-Output ("模式: {0}" -f $(if ($Apply) { '落盘' } else { 'dry-run' }))
Write-Output ("结构/类 {0}；跳过 {1}；需重排 {2}；行数变化合计 {3}；校验失败 {4}" -f $structs.Count, $skip.Count, $nReorder, $nGap, $nBad)
Write-Output ("分层: 函数项 {0}；不可达置本组末 {1}；同名遮蔽保持原位 {2}；调用不可解析保持原位 {3}" -f $nFn, $nUnreach, $shield.Count, $inplace.Count)
Write-Output ''
Write-Output '=== 变更清单 ==='
foreach ($r in $plan) { "  {0,-22} 重排={1,-6} 行数变化={2}" -f $r.Name, $r.Reorder, $r.Delta }
if ($shield.Count -gt 0) {
    Write-Output ''
    Write-Output '=== 同名遮蔽，保持原位（不上移；同一连续数据成员块一并原位） ==='
    foreach ($x in $shield) { "  {0,-22} L{1}  {2}  整块 {3} 个数据成员" -f $x.Struct, $x.Line, $x.Member, $x.Run }
}
if ($inplace.Count -gt 0) {
    Write-Output ''
    Write-Output ("=== 调用关系不可解析，保持原位（保守；共 {0} 条，最多列 30 条） ===" -f $inplace.Count)
    foreach ($x in ($inplace | Select-Object -First 30)) { "  {0,-22} L{1}  {2}" -f $x.Struct, $x.Line, $x.Name }
}
if ($skip.Count -gt 0) {
    Write-Output ''
    Write-Output '=== 跳过（需人工看） ==='
    foreach ($x in $skip) { "  {0,-22} {1}" -f $x.Name, $x.Why }
}
if ($Layers) {
    Write-Output ''
    Write-Output '=== 同后缀家族（族内强制相邻同层；L = 族所在层） ==='
    foreach ($x in $famDump) { "  {0,-20} {1,-12} L{2}  {3}" -f $x.S, $x.Fam, $x.L, $x.N }
    Write-Output ''
    Write-Output '=== 分层明细（Grp / Layer / Ord / 名称；Layer=1000000000 表示不可达，置于本组末） ==='
    foreach ($x in $layerDump) { "  {0,-20} G{1} L{2} O{3}  {4}" -f $x.S, $x.G, $x.L, $x.O, $x.N }
    Write-Output ''
    Write-Output '=== BFS 边集（本类成员函数名 调用 → 被调用） ==='
    foreach ($x in $edgeDump) { "  $x" }
    Write-Output ''
    Write-Output '=== 结构体发射顺序（struct：Grp / Sub / 组内空行 gap / 名称；gap=1 同组，gap=3 跨组） ==='
    foreach ($x in $grpDump) { "  {0,-28} G{1} S{2} gap{3}  {4}" -f $x.S, $x.G, $x.Sb, $x.Gap, $x.N }
}
# 只跑必要次数：干跑判定"需重排 0"（$nReorder = 0）→ 内容必然与原文一致，不落盘
if ($Apply -and $nReorder -gt 0) {
    $out = ($L -join "`r`n"); if ($raw.EndsWith("`r`n")) { $out += "`r`n" }
    [System.IO.File]::WriteAllText($p, $out, $enc)
    $bytes = [System.IO.File]::ReadAllBytes($p); $txt = [System.IO.File]::ReadAllText($p)
    Write-Output ''
    Write-Output ("编码: 字节={0} 行数={1} bom={2} crlf={3} 纯LF={4} 尾字节={5}" -f $bytes.Length, $L.Count, (($bytes[0] -eq 0xEF) -and ($bytes[1] -eq 0xBB) -and ($bytes[2] -eq 0xBF)), ([regex]::Matches($txt, "`r`n")).Count, ([regex]::Matches($txt, "(?<!`r)`n")).Count, $bytes[$bytes.Length - 1])
}
elseif ($Apply) {
    Write-Output ''
    Write-Output '未落盘：需重排 0（内容与原文一致，跳过写盘）'
}
Write-Output 'DONE'