@echo off	
set compile=.\glslangValidator.exe
set file=../shader

%compile% -V model/pbr.vert -o %file%/model/pbrVert.spv
%compile% -V model/pbr.frag -o %file%/model/pbrFrag.spv