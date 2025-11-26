@echo off	
set compile=.\glslangValidator.exe
set file=../shader

%compile% -V terrain/terrain.vert -o %file%/terrain/terrainVert.spv
%compile% -V terrain/terrain.frag -o %file%/terrain/terrainFrag.spv