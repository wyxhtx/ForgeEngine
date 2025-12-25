@echo off	
set compile=.\glslangValidator.exe
set file=../shader

%compile% -V terrain/terrainDeferred.vert -o %file%/terrain/terrainDeferredVert.spv
%compile% -V terrain/terrainDeferred.frag -o %file%/terrain/terrainDeferredFrag.spv