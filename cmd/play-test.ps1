# play-test.ps1 —— 自动化"试玩"：注入输入逼出一次存档写入，验证 storage/game/chunk 生成
#
# 依据（源码实测，非猜测）：
#   src\code\vk\base\Window.hpp:161-180  W/S/A/D 移动、SPACE 跳、LEFT_SHIFT 下蹲（glfwGetKey 长按）
#   src\code\vk\base\Window.hpp:188-193  左键 → playerInfo::mouseButtonLeft（长按 → 破坏方块）
#   src\code\vk\base\Window.hpp:196-205  右键 → mouseButtonRightJust（边沿 → 放置方块）
#   src\code\vk\base\Window.hpp:37-39 / 151-153  鼠标=光标回调 Δ → InitCamera.mouseHandle（视角）
#   src\code\vk\base\Window.hpp:32       GLFW_CURSOR_DISABLED（未用 RAW_MOUSE_MOTION ⇒ SendInput 相对移动有效）
#   src\code\vk\base\Window.hpp:128-129  END 键 → glfwSetWindowShouldClose（正常退出）
#
# 工作目录选择理由：引擎用相对路径找 `src/assets/...`（构建时已镜像到 exe 同级）与 `storage/...`（运行期自建），
#   而这些都相对 CWD ⇒ 取 **exe 所在目录**（实测该 CWD 下能正常加载资产并自建 storage/）✓
#
# 用法示例：
#   pwsh -File cmd\play-test.ps1
#   pwsh -File cmd\play-test.ps1 -LookDownY 220 -BreakHoldMs 1200 -MoveForwardMs 800
param(
	[string]$Exe      = 'build\vs\bin\output\x64\Release\ForgeEngine.exe',
	[string]$WorkDir  = '',                      # 空 = exe 所在目录
	[int]$LookDownY   = 180,                     # 鼠标下移量（Δy，正=向下看）；0 = 不动视角
	[int]$LookRightX  = 0,                       # 鼠标右移量（Δx）；0 = 不转
	[int]$MoveForwardMs = 0,                     # W 长按毫秒（0 = 不前进）
	[int]$BreakHoldMs   = 1500,                  # 左键长按毫秒（破坏方块）
	[int]$Cycles        = 3,                     # 循环次数：前进 → 下压视角 → 破坏（单次常打不准，需重复）
	[int]$ForwardMs     = 900,                   # 每次前进毫秒
	[int]$LookStepY     = 120,                   # 每次循环额外下压视角量（Δy）
	[string]$BreakMode  = 'hold',                # hold=左键按住 BreakHoldMs；tap=连续点按 TapMs
	[int]$TapMs         = 120,                   # tap 模式的单次点按毫秒
	[int]$SaveWaitMs    = 2500,                  # 收手后等待存档刷盘
	[int]$TimeoutSec    = 60,                    # 全程超时；到点强杀（Log::printError 有 while(true) 会挂起）
	[string]$LogFile  = 'build\temp\log\play-test.log',
	[switch]$KeepAlive                           # 只注入不退出（调试用）
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent (Split-Path -Parent $PSCommandPath)     # cmd\ 的上级 = 仓库根
if (-not [IO.Path]::IsPathRooted($Exe))     { $Exe     = Join-Path $root $Exe }
if (-not [IO.Path]::IsPathRooted($LogFile)) { $LogFile = Join-Path $root $LogFile }
if ([string]::IsNullOrWhiteSpace($WorkDir)) { $WorkDir = Split-Path -Parent $Exe }

Add-Type -Namespace PT -Name In -MemberDefinition @'
[StructLayout(LayoutKind.Sequential)] public struct MOUSEINPUT { public int dx; public int dy; public uint mouseData; public uint dwFlags; public uint time; public IntPtr dwExtraInfo; }
[StructLayout(LayoutKind.Sequential)] public struct KEYBDINPUT { public ushort wVk; public ushort wScan; public uint dwFlags; public uint time; public IntPtr dwExtraInfo; }
[StructLayout(LayoutKind.Explicit)]  public struct INPUTUNION { [FieldOffset(0)] public MOUSEINPUT mi; [FieldOffset(0)] public KEYBDINPUT ki; }
[StructLayout(LayoutKind.Sequential)] public struct INPUT { public uint type; public INPUTUNION u; }
[DllImport("user32.dll", SetLastError=true)] public static extern uint SendInput(uint n, INPUT[] p, int cb);
[DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
[DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
[DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, IntPtr p);
[DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
[DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
public delegate bool EnumProc(IntPtr h, IntPtr p);
public static void Key(ushort vk, bool up) {
    INPUT[] a = new INPUT[1];
    a[0].type = 1; a[0].u.ki.wVk = vk; a[0].u.ki.dwFlags = up ? 2u : 0u;
    SendInput(1, a, Marshal.SizeOf(typeof(INPUT)));
}
public static void MouseMove(int dx, int dy) {
    INPUT[] a = new INPUT[1];
    a[0].type = 0; a[0].u.mi.dx = dx; a[0].u.mi.dy = dy; a[0].u.mi.dwFlags = 0x0001;   // MOUSEEVENTF_MOVE
    SendInput(1, a, Marshal.SizeOf(typeof(INPUT)));
}
public static void MouseBtn(bool right, bool up) {
    INPUT[] a = new INPUT[1];
    a[0].type = 0; a[0].u.mi.dwFlags = (right ? 0x0008u : 0x0002u) | (up ? 0x0010u : 0u);   // DOWN/RIGHTDOWN + UP
    SendInput(1, a, Marshal.SizeOf(typeof(INPUT)));
}
'@ -ReferencedAssemblies System.Runtime.InteropServices

$VK = @{ W = 0x57; A = 0x41; S = 0x53; D = 0x44; SPACE = 0x20; LSHIFT = 0xA0; END = 0x23 }

function Log([string]$m) { $line = "[{0}] {1}" -f (Get-Date -Format 'HH:mm:ss'), $m; Write-Host $line; Add-Content -LiteralPath $LogFile -Value $line -Encoding utf8 }

New-Item -ItemType Directory -Path (Split-Path -Parent $LogFile) -Force | Out-Null
"=== play-test 开始 $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss') ===" | Set-Content -LiteralPath $LogFile -Encoding utf8
Log "exe      = $Exe"
Log "workDir  = $WorkDir"
Log "按键序列 = 前景窗口确认 → 鼠标移动(Δx=$LookRightX, Δy=$LookDownY) → [W 前进 ${ForwardMs}ms ×$Cycles] → 左键长按 ${BreakHoldMs}ms（破坏方块）→ 等 ${SaveWaitMs}ms 刷盘 → END 退出"
Log "超时     = ${TimeoutSec}s（到点强杀；printError 的 while(true) 会挂起进程）"

# 只清理我们自己会启动的同名进程（避免单实例互斥：Main.cpp:9 CreateMutex）
Get-Process ForgeEngine -ErrorAction SilentlyContinue | ForEach-Object { Log "清理残留 ForgeEngine PID=$($_.Id)"; Stop-Process -Id $_.Id -Force }
Start-Sleep -Milliseconds 500

$proc     = $null
$window   = [IntPtr]::Zero
$deadline = (Get-Date).AddSeconds($TimeoutSec)
$keyDown  = New-Object Collections.ArrayList
$runStart = Get-Date
function Release-All {
	foreach ($vk in $keyDown) { [PT.In]::Key($vk, $true) }
	[PT.In]::MouseBtn($false, $true); [PT.In]::MouseBtn($true, $true)
	$script:keyDown.Clear()
}
try {
	$proc = Start-Process -FilePath $Exe -WorkingDirectory $WorkDir -PassThru -RedirectStandardOutput (Join-Path $root 'build\temp\log\play-test-stdout.log') -RedirectStandardError (Join-Path $root 'build\temp\log\play-test-stderr.log')
	Log "已启动 ForgeEngine PID=$($proc.Id)（stdout → build\temp\log\play-test-stdout.log）"

	# 等窗口出现（按 PID 精确匹配 ⇒ 只对我们启动的进程发输入）
	for ($i = 0; $i -lt 120 -and $window -eq [IntPtr]::Zero; $i++) {
		Start-Sleep -Milliseconds 500
		$script:found = [IntPtr]::Zero
		$cb = [PT.In+EnumProc]{ param($h, $p) $pid2 = 0; [void][PT.In]::GetWindowThreadProcessId($h, [ref]$pid2); if ($pid2 -eq $proc.Id -and [PT.In]::IsWindowVisible($h)) { $script:found = $h; return $false }; return $true }
		[void][PT.In]::EnumWindows($cb, [IntPtr]::Zero)
		$window = $script:found
	}
	if ($window -eq [IntPtr]::Zero) { throw "未找到游戏窗口（PID $($proc.Id)）" }
	Log "窗口句柄 = $window"
	[void][PT.In]::SetForegroundWindow($window)
	Start-Sleep -Milliseconds 800
	if ([PT.In]::GetForegroundWindow() -ne $window) { Log "⚠ 前景窗口不是游戏窗口（SendInput 可能发到别处）——仍继续，但请注意" }
	else { Log "前景窗口已确认 = 游戏窗口" }
	Start-Sleep -Milliseconds 1500			# 等首帧与审计打印

		# 循环：前进 → 下压视角 → 破坏（用户实测：站着点一下不会破坏，必须移动配合）
	for ($c = 1; $c -le $Cycles; $c++) {
		if ($ForwardMs -gt 0) {
			[PT.In]::Key($VK.W, $false); [void]$keyDown.Add($VK.W)
			Start-Sleep -Milliseconds $MoveForwardMs
			[PT.In]::Key($VK.W, $true); $keyDown.Remove($VK.W) | Out-Null
			Log "第 $c 轮：W 前进 ${ForwardMs}ms（已补 key-up）"
		}
		if ($LookDownY -ne 0 -or $LookRightX -ne 0) {
			[PT.In]::MouseMove($LookRightX, ($LookDownY + ($c - 1) * $LookStepY))
			Start-Sleep -Milliseconds 250
			Log "第 $c 轮：视角下压 Δy=$($LookDownY + ($c - 1) * $LookStepY)"
		}
		if ($BreakMode -eq 'tap') {
			for ($k = 1; $k -le 3; $k++) {
				[PT.In]::MouseBtn($false, $false); Start-Sleep -Milliseconds $TapMs; [PT.In]::MouseBtn($false, $true)
				Start-Sleep -Milliseconds 150
			}
			Log "第 $c 轮：左键点按 3 次（各 ${TapMs}ms，已补 mouse-up）"
		}
		else {
			[PT.In]::MouseBtn($false, $false); Start-Sleep -Milliseconds $BreakHoldMs; [PT.In]::MouseBtn($false, $true)
			Log "第 $c 轮：左键长按 ${BreakHoldMs}ms 破坏方块（已补 mouse-up）"
		}
		Start-Sleep -Milliseconds 400
	}
	Start-Sleep -Milliseconds $SaveWaitMs		# 等存档刷盘

	if (-not $KeepAlive) {
		[PT.In]::Key($VK.END, $false); Start-Sleep -Milliseconds 120; [PT.In]::Key($VK.END, $true)
		Log "已按 END 请求正常退出（key-up 已补）"
	}

	# 等进程退出；超时强杀
	while (-not $proc.HasExited -and (Get-Date) -lt $deadline) { Start-Sleep -Milliseconds 500 }
	if (-not $proc.HasExited) { Log "⚠ 超时 ${TimeoutSec}s 未退出 ⇒ 强杀"; Stop-Process -Id $proc.Id -Force } else { Log "进程正常退出，ExitCode=$($proc.ExitCode)" }
}
catch { Log "异常：$_" }
finally {
	Release-All
	if ($proc -and -not $proc.HasExited) { Stop-Process -Id $proc.Id -Force; Log "finally：强杀 PID=$($proc.Id)" }
}

# ---------------- 证据 ----------------
$chunk = Join-Path $WorkDir 'storage\game\chunk'
Log "证据① storage/game/chunk 存在 = $(Test-Path $chunk)"
if (Test-Path $chunk) {
	$files = Get-ChildItem $chunk -Recurse -File | Where-Object { $_.LastWriteTime -ge $runStart }
	Log ("证据①a 本次运行期间新增/修改的存档文件 = {0} 个，字节合计 = {1}（判定破坏成功的硬证据）" -f $files.Count, ($files | Measure-Object Length -Sum).Sum)
	foreach ($d in @('full', 'delta')) {
		$p = Join-Path $chunk $d
		$n = 0; $b = 0
		if (Test-Path $p) { $fs = Get-ChildItem $p -Recurse -File | Where-Object { $_.LastWriteTime -ge $runStart }; $n = $fs.Count; $b = ($fs | Measure-Object Length -Sum).Sum }
		Log ("证据①b {0,-5} 存在 = {1}  本次新增 {2} 个 / {3} 字节" -f $d, (Test-Path $p), $n, $b)
	}
	$files | Select-Object -First 10 | ForEach-Object { Log ("    " + ($_.FullName -replace [regex]::Escape($WorkDir), '') + "  " + $_.Length + " 字节") }
}
$cache = Join-Path $WorkDir 'storage\local\cache\pipeline.bin'
$ci = Get-Item $cache -ErrorAction SilentlyContinue
Log ("证据② storage/local/cache/pipeline.bin 存在 = {0}  字节 = {1}" -f ($null -ne $ci), $(if ($ci) { $ci.Length } else { 0 }))
$out = Join-Path $root 'build\temp\log\play-test-stdout.log'
if (Test-Path $out) {
	$leak = (Select-String -Path $out -Pattern 'has not been destroyed' | Measure-Object).Count
	$vuid = (Select-String -Path $out -Pattern 'VUID|Validation Error' | Measure-Object).Count
	$audit = (Select-String -Path $out -Pattern '自洽 OK' | Measure-Object).Count
	$mism = (Select-String -Path $out -Pattern '自洽 不符' | Measure-Object).Count
	Log "证据③ 审计行 = $audit（应 9 或 18），不符 = $mism；校验层 VUID/Error = $vuid；未销毁句柄 = $leak"
}
"=== play-test 结束 $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss') ===" | Add-Content -LiteralPath $LogFile -Encoding utf8
Write-Host "`n日志：$LogFile"
