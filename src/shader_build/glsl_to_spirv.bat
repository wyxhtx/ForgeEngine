@echo off
setlocal enabledelayedexpansion

set "COMPILER=.\glslangValidator.exe"
set "SOURCE_DIR=."
set "TARGET_DIR=..\shader"

for /r "%SOURCE_DIR%" %%f in (*.vert *.frag) do (
    set "source_file=%%f"
    set "source_path=%%~dpf"
    
    REM 获取相对于shader_build目录的路径
    set "relative_path=!source_path:%CD%\=!"
    set "relative_path=!relative_path:~0,-1!"
    
    REM 获取文件名（不含扩展名）
    set "filename=%%~nf"
    
    REM 确定目标文件名
    if "%%~xf"==".vert" (
        set "target_filename=!filename!Vert.spv"
    ) else if "%%~xf"==".frag" (
        set "target_filename=!filename!Frag.spv"
    )
    
    REM 构建目标路径
    set "target_path=%TARGET_DIR%\!relative_path!\!target_filename!"
    
    REM 创建目标目录（如果不存在）
    set "target_dir=!target_path!\!.."
    if not exist "!target_dir!" (
        echo 创建目录: !target_dir!
        mkdir "!target_dir!"
    )
    
    echo 编译: !relative_path!\!filename!%%~xf → !target_path!
    "%COMPILER%" -V "%%f" -o "!target_path!" >nul 2>&1
)

pause