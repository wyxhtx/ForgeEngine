# vs.ps1 —— DSH ↔ Visual Studio 2026 控制桥（用 Windows PowerShell 5.1 运行；pwsh 7 没有 Marshal.GetActiveObject）
#
# 用途：让 AI/脚本直接操控**正在运行的** VS 实例（构建、读错误列表、读输出窗口、跳转、起停调试、透传 VS 命令）。
# 用法：powershell.exe -NoProfile -ExecutionPolicy Bypass -File config\tool\vs-bridge\vs.ps1 -Action <动作> [参数]
#   动作：help | info | errors | output | build | rebuild | goto | debug | stop | save | command
#   例：  -Action info
#         -Action errors
#         -Action output -Pane "生成" -Tail 40
#         -Action build
#         -Action goto   -File src\code\vk\data\VkList.h -Line 168
#         -Action command -Name "Build.Cancel"
param(
	[Parameter(Mandatory = $true)][string]$Action,
	[string]$Pane = '生成',
	[int]$Tail = 40,
	[string]$File,
	[int]$Line = 1,
	[string]$Name,
	[string]$Args = '',
	[string]$Font,
	[int]$Size = 0,
	[int]$TimeoutSec = 900
)

$ErrorActionPreference = 'Stop'

function Get-Dte {
	try { return [Runtime.InteropServices.Marshal]::GetActiveObject('VisualStudio.DTE.18.0') }
	catch {
		try { return [Runtime.InteropServices.Marshal]::GetActiveObject('VisualStudio.DTE.17.0') }
		catch { throw '连不上 VS：请确认 VS 正在运行（且不是以管理员身份运行，否则普通进程无法附着）' }
	}
}

function Get-Panes($dte) {
	# 输出窗口是懒创建的：先显式打开，否则 $dte.Windows 里根本没有它
	try { $dte.Windows.Item('{34E76E81-EE4A-11D0-AE2E-00A0C90FFFC3}').Visible = $true } catch { }
	foreach ($w in $dte.Windows) {
		$panes = $null
		try { $panes = $w.Object.OutputWindowPanes } catch { continue }
		if ($panes) { foreach ($p in $panes) { $p } }
	}
}

function Read-Pane($pane) {
	$ep = $pane.TextDocument.StartPoint.CreateEditPoint()
	return $ep.GetText($pane.TextDocument.EndPoint)
}

function Show-Usage {
	@'
vs.ps1 —— VS 2026 控制桥（Windows PowerShell 5.1）

  -Action help                          本帮助
  -Action info                          解决方案 / 打开文档 / 错误列表计数 / 构建状态
  -Action errors                        打印错误列表（级别 + 文件(行,列) + 描述）
  -Action output [-Pane 生成] [-Tail N]  打印输出窗口某个面板的最后 N 行
  -Action build | rebuild               构建 / 重新构建解决方案并等待结束
  -Action goto -File <路径> [-Line N]    打开文件并跳到第 N 行
  -Action debug | stop                  启动调试 / 停止调试
  -Action save                          全部保存（File.SaveAll）
  -Action command -Name <VS命令> [-Args]  透传执行任意 VS 命令（如 Build.Cancel）
'@
}

$dte = Get-Dte
$sb = $dte.Solution.SolutionBuild

switch ($Action.ToLower()) {
	'help' { Show-Usage }
	'font' {
		# DTE 正统接口：Properties("FontsAndColors", <页面>) → FontFamily / FontSize，实时生效、免重启
		$pages = @('TextEditor', 'OutputWindow', 'DialogsAndToolWindows', 'CommandWindow', 'Environment', 'Printer')
		foreach ($pg in $pages) {
			$props = $null
			try { $props = $dte.Properties('FontsAndColors', $pg) } catch { continue }
			if (-not $props) { continue }
			if ($Font) {
				$changed = @()
				try { $props.Item('FontFamily').Value = $Font; $changed += 'FontFamily' } catch { $changed += ('FontFamily失败:' + $_.Exception.Message) }
				if ($Size -gt 0) { try { $props.Item('FontSize').Value = $Size; $changed += 'FontSize' } catch { $changed += 'FontSize失败' } }
				$now = ''
				try { $now = $props.Item('FontFamily').Value + ' / ' + $props.Item('FontSize').Value } catch { }
				'  ' + $pg.PadRight(22) + ' → ' + $now + '   (' + ($changed -join ',') + ')'
			}
			else {
				$fam = ''; $sz = ''
				try { $fam = $props.Item('FontFamily').Value } catch { $fam = '(读不到)' }
				try { $sz = $props.Item('FontSize').Value } catch { }
				'  ' + $pg.PadRight(22) + $fam + ' / ' + $sz
			}
		}
	}
	'info' {
		'VS 版本        : ' + $dte.Version
		'解决方案       : ' + $dte.Solution.FullName
		'全部项目       : ' + $dte.Solution.Projects.Count
		foreach ($p in $dte.Solution.Projects) { '   - ' + $p.Name }
		'打开文档 (' + $dte.Documents.Count + ') :'
		foreach ($d in $dte.Documents) { '   - ' + $d.FullName }
		'错误列表项数   : ' + $dte.ToolWindows.ErrorList.ErrorItems.Count
		'构建状态       : ' + $sb.BuildState + '  (1=未开始 2=进行中 3=完成)'
		'最后构建信息   : ' + $sb.LastBuildInfo + '  (0=成功 否则失败项数)'
		'输出面板       : ' + ((Get-Panes $dte | ForEach-Object { $_.Name }) -join ' / ')
	}
	'errors' {
		$items = $dte.ToolWindows.ErrorList.ErrorItems
		'错误列表共 ' + $items.Count + ' 项：'
		for ($i = 1; $i -le $items.Count; $i++) {
			$e = $items.Item($i)
			$lvl = switch ($e.ErrorLevel) { 3 { '错误' } 2 { '警告' } 1 { '信息' } default { '?' } }
			'  [' + $lvl + '] ' + $e.FileName + '(' + $e.Line + ',' + $e.Column + ') ' + $e.Description
		}
	}
	'panes' { (Get-Panes $dte | ForEach-Object { '[' + $_.Name + ']' }) -join ' ' }
	'output' {
		$hit = $null
		foreach ($p in Get-Panes $dte) { if ($p.Name -eq $Pane) { $hit = $p; break } }
		if (-not $hit) {
			$all = @(Get-Panes $dte)
			$hit = $all | Where-Object { $_.Name -ne '' } | Select-Object -First 1
			if (-not $hit -and $all.Count -gt 0) { $hit = $all[0] }
			if (-not $hit) { throw ('输出窗口里没有任何面板（先在 VS 里打开：视图 → 输出）') }
			Write-Host ('（未找到「' + $Pane + '」，改用面板「' + $hit.Name + '」；候选：' + (($all | ForEach-Object { $_.Name }) -join ' / ') + '）')
		}
		$lines = (Read-Pane $hit) -split "`r?`n"
		$from = [Math]::Max(0, $lines.Count - $Tail)
		for ($i = $from; $i -lt $lines.Count; $i++) { $lines[$i] }
	}
	'build' { $sb.Build($true) | Out-Null; '构建结束，LastBuildInfo = ' + $sb.LastBuildInfo }
	'rebuild' { $sb.Rebuild($true) | Out-Null; '重新构建结束，LastBuildInfo = ' + $sb.LastBuildInfo }
	'goto' {
		if (-not $File) { throw '缺 -File' }
		$full = [System.IO.Path]::GetFullPath((Join-Path (Get-Location) $File))
		$dte.ItemOperations.OpenFile($full) | Out-Null
		$dte.ActiveDocument.Selection.GotoLine($Line, $false)
		'已跳到 ' + $full + ' : ' + $Line
	}
	'debug' { $dte.ExecuteCommand('Debug.Start'); '已启动调试' }
	'stop' { $dte.ExecuteCommand('Debug.StopDebugging'); '已停止调试' }
	'save' { $dte.ExecuteCommand('File.SaveAll'); '已全部保存' }
	'command' {
		if (-not $Name) { throw '缺 -Name' }
		if ($Args) { $dte.ExecuteCommand($Name, $Args) } else { $dte.ExecuteCommand($Name) }
		'已执行 VS 命令 ' + $Name
	}
	default { throw ('未知动作：' + $Action + '（用 -Action help 看用法）') }
}
