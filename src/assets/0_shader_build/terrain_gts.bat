@echo off	
set compile=.\glslangValidator.exe
set file=../0_shader

%compile% -V terrain/terrainDeferred.vert -o %file%/terrain/terrainDeferredVert.spv
%compile% -V terrain/terrainDeferred.frag -o %file%/terrain/terrainDeferredFrag.spv