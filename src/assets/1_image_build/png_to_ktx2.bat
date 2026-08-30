@echo off
setlocal enabledelayedexpansion

set "COMPILER=.\toktx.exe"
set "SOURCE_DIR=."
set "TARGET_DIR=..\1_image"

echo 开始转换地形纹理...
echo ---

::处理地形纹理
for /r "%SOURCE_DIR%\terrain" %%f in (*.png *.jpeg *.jpg) do (
    set "source_file=%%f"
    set "source_path=%%~dpf"
    
    ::计算相对路径
    set "relative_path=!source_path:%CD%\=!"
    set "relative_path=!relative_path:~0,-1!"
    
    ::获取文件名
    set "filename=%%~nf"
    
    ::判断是否为高光贴图
    echo !filename! | findstr /i "specular" >nul
    if !errorlevel! equ 0 (
        set "toktx_params=--t2 --zcmp 22 --assign_primaries bt709 --assign_oetf linear"
    ) else (
        set "toktx_params=--t2 --zcmp 22 --assign_primaries bt709 --assign_oetf srgb"
    )
    
    ::构建目标路径 - 使用延迟扩展避免变量污染
    set "target_path=!TARGET_DIR!\!relative_path!\!filename!.ktx2"
    set "target_dir_temp=!TARGET_DIR!\!relative_path!"
    
    ::创建目录并转换
    if not exist "!target_dir_temp!" (
        echo 创建目录: !target_dir_temp!
        mkdir "!target_dir_temp!"
    )
    
    echo 转换: !relative_path!\!filename!%%~xf → !target_path!
    "%COMPILER%" !toktx_params! "!target_path!" "%%f" >nul 2>&1
    if not !errorlevel! equ 0 (
        echo 失败
    )
    echo ---
)

echo.
echo 开始转换模型纹理...
echo ---

::处理模型纹理
for /r "%SOURCE_DIR%\model" %%f in (*.png *.jpeg *.jpg) do (
    set "source_file=%%f"
    set "source_path=%%~dpf"
    
    ::计算相对路径
    set "relative_path=!source_path:%CD%\=!"
    set "relative_path=!relative_path:~0,-1!"
    
    ::获取文件名
    set "filename=%%~nf"
    
    ::模型纹理使用固定参数
    set "toktx_params=--t2 --encode uastc --zcmp 22 --genmipmap --assign_primaries bt709 --assign_oetf srgb"
    
    ::构建目标路径
    set "target_path=%TARGET_DIR%\!relative_path!\!filename!.ktx2"
    set "target_dir=%TARGET_DIR%\!relative_path!"
    
    ::创建目录并转换
    if not exist "!target_dir!" (
        echo 创建目录: !target_dir!
        mkdir "!target_dir!"
    )
    
    echo 转换: !relative_path!\!filename!%%~xf → !target_path!
    "%COMPILER%" !toktx_params! "!target_path!" "%%f" >nul 2>&1
    if not !errorlevel! equ 0 (
        echo 失败
    )
    echo ---
)

pause