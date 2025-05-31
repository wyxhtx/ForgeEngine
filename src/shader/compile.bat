.\glslangValidator.exe -V terrain/terrain.vert -o terrain/terrainVert.spv
.\glslangValidator.exe -V terrain/terrain.frag -o terrain/terrainFrag.spv

.\glslangValidator.exe -V model/model.vert -o model/modelVert.spv
.\glslangValidator.exe -V model/model.frag -o model/modelFrag.spv

.\glslangValidator.exe -V skybox/skybox.vert -o skybox/skyboxVert.spv
.\glslangValidator.exe -V skybox/skybox.frag -o skybox/skyboxFrag.spv

.\glslangValidator.exe -V mine/mine.vert -o mine/mineVert.spv
.\glslangValidator.exe -V mine/mine.frag -o mine/mineFrag.spv

.\glslangValidator.exe -V uiText/uiText.vert -o uiText/uiTextVert.spv
.\glslangValidator.exe -V uiText/uiText.frag -o uiText/uiTextFrag.spv

pause