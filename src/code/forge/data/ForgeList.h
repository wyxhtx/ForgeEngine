#pragma once



//todo 后期循环直接读取文件，不用每次开局写一遍
namespace plateList {
	std::vector<glm::dvec3> palteLoop;						//板块循环列表，用于遍历区块数组
	std::unordered_set<glm::dvec3> structClearLoop;			//结构管理器清理循环列表
	std::vector<glm::ivec3> chunkLoop;						//区块循环列表,用于遍历方块数组
}

namespace terrainList {
	std::vector<glm::ivec4> treeLoop;						//树循环列表

	terrainStruct::treeNode oak{ 5,10 };					//橡树生成参数
}

namespace cubeList {
	//方块列表
	const std::vector<cubeStruct::propertyNode> property = {
		cubeStruct::propertyNode{{{ 0,  0}, { 0,  2}, { 0,  1}, { 0,  1}, { 0,  1}, { 0,  1}}, cubeEnum::coverageId::grass,			false},
		cubeStruct::propertyNode{{{ 0,  2}, { 0,  2}, { 0,  2}, { 0,  2}, { 0,  2}, { 0,  2}}, cubeEnum::coverageId::dirt,			false},
		cubeStruct::propertyNode{{{32,  0}, {32,  0}, {32,  1}, {32,  1}, {32,  1}, {32,  1}}, cubeEnum::coverageId::oakLog,		false},
		cubeStruct::propertyNode{{{32,  2}, {32,  2}, {32,  2}, {32,  2}, {32,  2}, {32,  2}}, cubeEnum::coverageId::oakLeaf,		false},
		cubeStruct::propertyNode{{{ 0, 16}, { 0, 16}, { 0, 16}, { 0, 16}, { 0, 16}, { 0, 16}}, cubeEnum::coverageId::stoneSquared,	false},
	};
}

namespace faceList {
	//面剔除索引,在此可控制面剔除朝向
	const std::vector<glm::ivec3> faceCullId{
		{ 0, 1, 0},
		{ 0,-1, 0},
		{ 1, 0, 0},
		{-1, 0, 0},
		{ 0, 0, 1},
		{ 0, 0,-1}
	};
}

namespace noiseList {
	namespace tree {
		const noiseStruct::treeNode oakPut{ 16,32,44 };		//橡树噪音参数
	}
}

namespace storageList {
	std::vector<glm::dvec3> regionLoop;					//地区循环列表
}