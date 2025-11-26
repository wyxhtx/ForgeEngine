::πÿ±’√¸¡Óªÿœ‘
@echo off	
set compile=.\glslangValidator.exe
set file=../shader

%compile% -V terrain/terrain.vert		-o %file%/terrain/terrainVert.spv
%compile% -V terrain/terrain.frag		-o %file%/terrain/terrainFrag.spv

%compile% -V model/temple/temple.vert	-o %file%/model/temple/templeVert.spv
%compile% -V model/temple/temple.frag	-o %file%/model/temple/templeFrag.spv

%compile% -V skybox/skybox.vert			-o %file%/skybox/skyboxVert.spv
%compile% -V skybox/skybox.frag			-o %file%/skybox/skyboxFrag.spv

%compile% -V mine/mine.vert				-o %file%/mine/mineVert.spv
%compile% -V mine/mine.frag				-o %file%/mine/mineFrag.spv

%compile% -V uiText/uiText.vert			-o %file%/uiText/uiTextVert.spv
%compile% -V uiText/uiText.frag			-o %file%/uiText/uiTextFrag.spv

pause