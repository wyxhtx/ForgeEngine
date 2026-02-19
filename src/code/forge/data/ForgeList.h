#pragma once



//方块列表
namespace cubeList {
	struct procCubeNode {
		std::vector<cubeStruct::cubeNode> node{};



		procCubeNode() : node(cubeEnum::cubeId::count) {
			node[cubeEnum::cubeId::grass]		.create({ { 0,  0}, { 0,  2}, { 0,  1}, { 0,  1}, { 0,  1}, { 0,  1} }, cubeEnum::coverageId::grass);
			node[cubeEnum::cubeId::dirt]		.create({ { 0,  2}, { 0,  2}, { 0,  2}, { 0,  2}, { 0,  2}, { 0,  2} }, cubeEnum::coverageId::dirt);
			node[cubeEnum::cubeId::oakTrunk]	.create({ {32,  0}, {32,  0}, {32,  1}, {32,  1}, {32,  1}, {32,  1} }, cubeEnum::coverageId::oakTrunk);
			node[cubeEnum::cubeId::oakLeaf]		.create({ {32,  2}, {32,  2}, {32,  2}, {32,  2}, {32,  2}, {32,  2} }, cubeEnum::coverageId::oakLeaf);
			node[cubeEnum::cubeId::stoneSquared].create({ { 0, 16}, { 0, 16}, { 0, 16}, { 0, 16}, { 0, 16}, { 0, 16} }, cubeEnum::coverageId::stoneSquared);
		}
	};

	procCubeNode procCube{};
}

//噪音列表
namespace noiseList {
	struct procNoiseNode {
		std::vector<noiseStruct::noiseNode> terrainNode{};
		std::vector<noiseStruct::noiseNode> treeNode{};

		int seed = noiseInfo::worldSeed;



		procNoiseNode() : terrainNode(noiseEnum::terrainId::count), treeNode(noiseEnum::treeId::count) {
			terrainNode[noiseEnum::terrainId::plain].create(getSeed(seed), FastNoiseLite::NoiseType_OpenSimplex2, 0.01f, FastNoiseLite::FractalType_FBm, 2, 3);

			treeNode[noiseEnum::treeId::oak].create(getSeed(seed), FastNoiseLite::NoiseType_OpenSimplex2S, 0.5f, FastNoiseLite::FractalType_FBm, 2, 3);
		}

		int getSeed(int seed) {
			static unsigned int salt = 0;

			//转为unsigned进行位运算，避免溢出UB
			unsigned int x = static_cast<unsigned int>(seed) + static_cast<unsigned int>(salt) * 0x9E3779B9U;
			x = (x ^ (x >> 16)) * 0x85ebca6bU;
			salt++;

			//转回int
			return static_cast<int>(x);
		}
	};

	struct procNoiseParamNode {
		std::vector<noiseStruct::terrainParamNode> terrainParam{};
		std::vector<noiseStruct::treeParamNode>    treeParam{};



		procNoiseParamNode() : terrainParam(noiseEnum::terrainId::count), treeParam(noiseEnum::treeId::count) {
			terrainParam[noiseEnum::terrainId::plain].create(8, 16);

			treeParam[noiseEnum::treeId::oak].create(16, 34, 44);
		}
	};

	struct procStructureNode {
		std::vector<noiseStruct::treeStructureNode> treeNode{};



		procStructureNode() : treeNode(noiseEnum::treeId::count) {
			treeNode[noiseEnum::treeId::oak].create(5, 10, cubeEnum::cubeId::oakLeaf, cubeEnum::cubeId::oakTrunk);
		}
	};
}

//循环列表
namespace loopList {
	std::vector<glm::ivec3> cubeMapLoop;					//方块Map循环列表，用于遍历方块数组，[1,32]
	std::vector<glm::dvec3> chunkMapLoop;					//区块Map循环列表，用于遍历区块数组
	std::vector<glm::dvec3> regionLoop;						//地区循环列表

	std::unordered_set<glm::dvec3> structClearLoop;			//结构管理器清理循环列表

	std::vector<glm::ivec4> treeLoop;						//树循环列表

	std::unordered_set<glm::dvec3> terrainLayerLoop;		//地形管线0层循环列表，用于判断区块坐标是否在terrainEnum::terrainLayer::Layer_0中
}