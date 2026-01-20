@echo off	
set compile=.\glslangValidator.exe
set file=../shader

%compile% -V model/pbrDeferred.vert -o %file%/model/pbrDeferredVert.spv
%compile% -V model/pbrDeferred.frag -o %file%/model/pbrDeferredFrag.spv