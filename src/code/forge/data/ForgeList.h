#pragma once



//todo 后期循环直接读取文件，不用每次开局写一遍
namespace plateList {
	std::vector<glm::dvec3> chunkLoop;						//区块循环列表，用于遍历区块数组
	std::unordered_set<glm::dvec3> structClearLoop;			//结构管理器清理循环列表
	std::vector<glm::ivec3> cubeLoop;						//方块循环列表，用于遍历方块数组，[1,32]
}

namespace terrainList {
	std::vector<glm::ivec4> treeLoop;						//树循环列表

	terrainStruct::treeNode oak{ 5,10 };					//橡树生成参数
}

namespace cubeList {
	//方块列表
	const std::array<cubeStruct::propertyNode, cubeEnum::cubeId::count> property = []() {
		std::array<cubeStruct::propertyNode, cubeEnum::cubeId::count> array{};

		array[cubeEnum::cubeId::grass]			= cubeStruct::propertyNode{ {{ 0,  0}, { 0,  2}, { 0,  1}, { 0,  1}, { 0,  1}, { 0,  1}}, cubeEnum::coverageId::grass };
		array[cubeEnum::cubeId::dirt]			= cubeStruct::propertyNode{ {{ 0,  2}, { 0,  2}, { 0,  2}, { 0,  2}, { 0,  2}, { 0,  2}}, cubeEnum::coverageId::dirt };
		array[cubeEnum::cubeId::oakLog]			= cubeStruct::propertyNode{ {{32,  0}, {32,  0}, {32,  1}, {32,  1}, {32,  1}, {32,  1}}, cubeEnum::coverageId::oakLog };
		array[cubeEnum::cubeId::oakLeaf]		= cubeStruct::propertyNode{ {{32,  2}, {32,  2}, {32,  2}, {32,  2}, {32,  2}, {32,  2}}, cubeEnum::coverageId::oakLeaf };
		array[cubeEnum::cubeId::stoneSquared]	= cubeStruct::propertyNode{ {{ 0, 16}, { 0, 16}, { 0, 16}, { 0, 16}, { 0, 16}, { 0, 16}}, cubeEnum::coverageId::stoneSquared };

		return array;
	}();
}

namespace noiseList {
	namespace tree {
		//const noiseStruct::treeNode oakPut{ 16,32,44 };		//橡树噪音参数
		
		//todo 性能测试
		const noiseStruct::treeNode oakPut{ 16,40,44 };
	}
}

namespace storageList {
	std::vector<glm::dvec3> regionLoop;					//地区循环列表
}