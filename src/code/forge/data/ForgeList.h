#pragma once



namespace cubeList {
	//方块属性列表
	const std::array<cubeStruct::propertyNode, cubeEnum::cubeId::count> cubeProperty = []() {
		std::array<cubeStruct::propertyNode, cubeEnum::cubeId::count> array{};

		array[cubeEnum::cubeId::grass]			= cubeStruct::propertyNode{ {{ 0,  0}, { 0,  2}, { 0,  1}, { 0,  1}, { 0,  1}, { 0,  1}}, cubeEnum::coverageId::grass,		  glm::vec3(0.05f, 0.08f, 0.02f), glm::vec3(0.2f, 0.5f, 0.1f), glm::vec3(0.1f, 0.2f, 0.05f),    8.0f};
		array[cubeEnum::cubeId::dirt]			= cubeStruct::propertyNode{ {{ 0,  2}, { 0,  2}, { 0,  2}, { 0,  2}, { 0,  2}, { 0,  2}}, cubeEnum::coverageId::dirt,		  glm::vec3(0.08f, 0.05f, 0.03f), glm::vec3(0.4f, 0.3f, 0.2f), glm::vec3(0.05f, 0.04f, 0.02f),  4.0f};
		array[cubeEnum::cubeId::oakTrunk]		= cubeStruct::propertyNode{ {{32,  0}, {32,  0}, {32,  1}, {32,  1}, {32,  1}, {32,  1}}, cubeEnum::coverageId::oakTrunk,	  glm::vec3(0.06f, 0.04f, 0.02f), glm::vec3(0.3f, 0.2f, 0.1f), glm::vec3(0.08f, 0.06f, 0.04f), 12.0f};
		array[cubeEnum::cubeId::oakLeaf]		= cubeStruct::propertyNode{ {{32,  2}, {32,  2}, {32,  2}, {32,  2}, {32,  2}, {32,  2}}, cubeEnum::coverageId::oakLeaf,	  glm::vec3(0.02f, 0.04f, 0.01f), glm::vec3(0.1f, 0.3f, 0.05f), glm::vec3(0.05f, 0.1f, 0.03f), 16.0f};
		array[cubeEnum::cubeId::stoneSquared]	= cubeStruct::propertyNode{ {{ 0, 16}, { 0, 16}, { 0, 16}, { 0, 16}, { 0, 16}, { 0, 16}}, cubeEnum::coverageId::stoneSquared, glm::vec3(0.1f, 0.1f, 0.1f),	  glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(0.3f, 0.3f, 0.3f),	   32.0f};

		return array;
		}();
}

namespace terrainList {
	std::unordered_set<glm::dvec3> layerLoop;													//地形管线0层循环列表，用于判断区块坐标是否在terrainEnum::terrainLayer::Layer_0中
	const int layerSightRange[terrainEnum::terrainLayer::count] = { 4,plateInfo::sightRange };	//地形管线各层视距列表
}

namespace plateList {
	std::vector<glm::dvec3> chunkMapLoop;					//区块Map循环列表，用于遍历区块数组

	std::unordered_set<glm::dvec3> structClearLoop;			//结构管理器清理循环列表
	std::vector<glm::ivec3> cubekMapLoop;					//方块Map循环列表，用于遍历方块数组，[1,32]
}

namespace terrainStructureList {
	std::vector<glm::ivec4> treeLoop;								//树循环列表

	//地形结构列表
	const std::array<terrainStructureStruct::treeStructureNode, terrainStructureEnum::treeId::count> terrainStructureProperty = []() {
		std::array<terrainStructureStruct::treeStructureNode, terrainStructureEnum::treeId::count> array{};

		//array[terrainStructureEnum::treeId::oak] = terrainStructureStruct::treeStructureNode{ 5,10,cubeEnum::cubeId::oakLeaf,cubeEnum::cubeId::oakTrunk };	//橡树
		array[terrainStructureEnum::treeId::oak] = terrainStructureStruct::treeStructureNode{ 5,10,cubeEnum::cubeId::stoneSquared,cubeEnum::cubeId::oakTrunk };	//橡树

		return array;
		}();
}

namespace noiseList {
	//地形噪音参数
	const std::array<noiseStruct::terrainParamNode, terrainEnum::terrainId::count> terrainProperty = []() {
		std::array<noiseStruct::terrainParamNode, terrainEnum::terrainId::count> array{};

		array[terrainEnum::terrainId::plain] = noiseStruct::terrainParamNode{ 8,16 };		//平原

		return array;
		}();

	//树噪音参数
	const std::array<noiseStruct::treeParamNode, terrainStructureEnum::treeId::count> treeProperty = []() {
		std::array<noiseStruct::treeParamNode, terrainStructureEnum::treeId::count> array{};

		array[terrainStructureEnum::treeId::oak] = noiseStruct::treeParamNode{ 16,34,44 };	//橡树

		return array;
		}();
}

namespace storageList {
	std::vector<glm::dvec3> regionLoop;					//地区循环列表
}