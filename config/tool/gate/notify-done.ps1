# 任务列表全完成时播一次完成提示音（中途轮次静音；由 AI 在 todo 全 completed 后调用）
# 用法：pwsh -NoProfile -File config\tool\gate\notify-done.ps1
$wav = 'C:\Windows\Media\Windows Notify System Generic.wav'
if (Test-Path $wav) { (New-Object Media.SoundPlayer $wav).PlaySync(); Write-Host '完成提示音已播放' }
else { [console]::beep(880, 150); [console]::beep(1320, 200); Write-Host '完成提示音（Beep）' }