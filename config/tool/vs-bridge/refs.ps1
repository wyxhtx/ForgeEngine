# refs.ps1 —— Find All References（自动化）
# 背景：VS2026 里 `Edit.FindAllReferences` **命令存在**（guid={5EFC7975-14BC-11CF-9B2B-00AA00573819} id=1915 ✓ 由 DTE 命令表枚举得到）
#       但 DTE 的 ExecuteCommand(名字) 解析不了它 ✗ ⇒ 必须用 Commands.Raise(guid, id) 直调 ✓
# 用法：powershell.exe -NoProfile -ExecutionPolicy Bypass -File config\tool\vs-bridge\refs.ps1 [-File <相对路径>] [-Line <N>]
# 说明：-File/-Line 给了就先经 vs.ps1 -Action goto 定位光标，再发出查找全部引用命令 ✓
param(
  [string]$File = "",
  [int]$Line = 1,
  [int]$WaitMs = 2000
)
$ErrorActionPreference = "Stop"
$dte = $null
try { $dte = [Runtime.InteropServices.Marshal]::GetActiveObject("VisualStudio.DTE.18.0") }
catch { try { $dte = [Runtime.InteropServices.Marshal]::GetActiveObject("VisualStudio.DTE.17.0") } catch { throw "连不上 VS：请确认 VS 正在运行且非管理员" } }
if ($File -ne "") {
  $here = Split-Path -Parent $MyInvocation.MyCommand.Path
  & powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $here "vs.ps1") -Action goto -File $File -Line $Line | Out-Null
  Start-Sleep -Milliseconds 400
}
$GUID = "{5EFC7975-14BC-11CF-9B2B-00AA00573819}"
$ID = 1915
$dte.Commands.Raise($GUID, $ID, $null, $null)
Write-Output ("已发出 Find All References（guid=" + $GUID + " id=" + $ID + "）")
Start-Sleep -Milliseconds $WaitMs
# 结果窗口是工具窗口「<元素> references」⇒ 尽力读取其文本（不同 VS 版本暴露方式不同 ✓ 失败不报错 ✓）
$got = $false
foreach ($w in $dte.Windows) {
  $cap = ""
  try { $cap = [string]$w.Caption } catch { continue }
  if ($cap -match "references") {
    try {
      $doc = $w.Object.TextDocument
      if ($doc) {
        $ep = $doc.StartPoint.CreateEditPoint()
        $txt = $ep.GetText($doc.EndPoint)
        Write-Output ("--- 引用结果窗口「" + $cap + "」---")
        Write-Output $txt
        $got = $true
      }
    } catch { }
  }
}
if (-not $got) { Write-Output "（未能读取引用窗口文本 ⇒ 请在 VS 里查看「<元素> references」窗口 ✓）" }