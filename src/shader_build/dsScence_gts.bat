@echo off	
set compile=.\glslangValidator.exe
set file=../shader

%compile% -V dsScence/dsScence.vert -o %file%/dsScence/dsScenceVert.spv
%compile% -V dsScence/dsScence.frag -o %file%/dsScence/dsScenceFrag.spv