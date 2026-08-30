[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$root  = Get-Location
$fixed = @("vcpkg.json","CMakePresets.json","CMakeLists.txt",".gitignore") |
    ForEach-Object { Join-Path $root $_ }
$auto = @()
$auto += Get-ChildItem "config\cmake"         -File -Filter *.cmake -ErrorAction SilentlyContinue
$auto += Get-ChildItem "config\vcpkg\triplet" -File -Filter *.cmake -ErrorAction SilentlyContinue
$auto += Get-ChildItem "config\vcpkg\port" -Recurse -File -Include "portfile.cmake","vcpkg.json","*.patch","*.diff" -ErrorAction SilentlyContinue
$seen = @{}
$files = @($fixed + ($auto | ForEach-Object { $_.FullName })) |
    Where-Object { $_ -and (Test-Path $_) -and -not $seen.ContainsKey($_) } |
    ForEach-Object { $seen[$_] = $true; $_ }
if ($Pattern) { $files = $files | Where-Object { $_ -match $Pattern } }
$total = 0
foreach ($f in $files) {
    $lines = [System.IO.File]::ReadAllLines($f, [System.Text.Encoding]::UTF8)
    $rel   = $f.Substring($root.Path.Length + 1)
    $total += $lines.Count
    "`n===== $rel（$($lines.Count) 行）====="
    for ($i = 0; $i -lt $lines.Count; $i++) {
        "{0,4}: {1}" -f ($i + 1), $lines[$i]
    }
}
"`n===== 共 $($files.Count) 个文件 / $total 行 ====="