@echo off
setlocal enabledelayedexpansion

set "COMPILER=.\toktx.exe"
set "SOURCE_DIR=."
set "TARGET_DIR=..\image"

echo ��ʼת����������...
echo ---

::������������
for /r "%SOURCE_DIR%\terrain" %%f in (*.png *.jpeg *.jpg) do (
    set "source_file=%%f"
    set "source_path=%%~dpf"
    
    ::�������·��
    set "relative_path=!source_path:%CD%\=!"
    set "relative_path=!relative_path:~0,-1!"
    
    ::��ȡ�ļ���
    set "filename=%%~nf"
    
    ::�ж��Ƿ�Ϊ�߹���ͼ
    echo !filename! | findstr /i "specular" >nul
    if !errorlevel! equ 0 (
        set "toktx_params=--t2 --zcmp 22 --assign_primaries bt709 --assign_oetf linear"
    ) else (
        set "toktx_params=--t2 --zcmp 22 --assign_primaries bt709 --assign_oetf srgb"
    )
    
    ::����Ŀ��·�� - ʹ���ӳ���չ���������Ⱦ
    set "target_path=!TARGET_DIR!\!relative_path!\!filename!.ktx2"
    set "target_dir_temp=!TARGET_DIR!\!relative_path!"
    
    ::����Ŀ¼��ת��
    if not exist "!target_dir_temp!" (
        echo ����Ŀ¼: !target_dir_temp!
        mkdir "!target_dir_temp!"
    )
    
    echo ת��: !relative_path!\!filename!%%~xf �� !target_path!
    "%COMPILER%" !toktx_params! "!target_path!" "%%f" >nul 2>&1
    if not !errorlevel! equ 0 (
        echo ʧ��
    )
    echo ---
)

echo.
echo ��ʼת��ģ������...
echo ---

::����ģ������
for /r "%SOURCE_DIR%\model" %%f in (*.png *.jpeg *.jpg) do (
    set "source_file=%%f"
    set "source_path=%%~dpf"
    
    ::�������·��
    set "relative_path=!source_path:%CD%\=!"
    set "relative_path=!relative_path:~0,-1!"
    
    ::��ȡ�ļ���
    set "filename=%%~nf"
    
    ::ģ������ʹ�ù̶�����
    set "toktx_params=--t2 --encode uastc --zcmp 22 --genmipmap --assign_primaries bt709 --assign_oetf srgb"
    
    ::����Ŀ��·��
    set "target_path=%TARGET_DIR%\!relative_path!\!filename!.ktx2"
    set "target_dir=%TARGET_DIR%\!relative_path!"
    
    ::����Ŀ¼��ת��
    if not exist "!target_dir!" (
        echo ����Ŀ¼: !target_dir!
        mkdir "!target_dir!"
    )
    
    echo ת��: !relative_path!\!filename!%%~xf �� !target_path!
    "%COMPILER%" !toktx_params! "!target_path!" "%%f" >nul 2>&1
    if not !errorlevel! equ 0 (
        echo ʧ��
    )
    echo ---
)

pause