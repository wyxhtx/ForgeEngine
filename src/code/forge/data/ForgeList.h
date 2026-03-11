#pragma once



//方块列表
namespace cubeList {
	struct procCubeNode {
		std::vector<cubeStruct::cubeNode> node{};



		procCubeNode() : node(cubeEnum::cubeId::count) {
			node[cubeEnum::cubeId::dirt]		.create({ { 0,  2}, { 0,  2}, { 0,  2}, { 0,  2}, { 0,  2}, { 0,  2} }, cubeEnum::coverageId::dirt);
			node[cubeEnum::cubeId::grass]		.create({ { 0,  0}, { 0,  2}, { 0,  1}, { 0,  1}, { 0,  1}, { 0,  1} }, cubeEnum::coverageId::grass);
			node[cubeEnum::cubeId::stone]		.create({ {16,  0}, {16,  0}, {16,  0}, {16,  0}, {16,  0}, {16,  0} }, cubeEnum::coverageId::stone);
			node[cubeEnum::cubeId::gravel]		.create({ {16,  1}, {16,  1}, {16,  1}, {16,  1}, {16,  1}, {16,  1} }, cubeEnum::coverageId::gravel);
			node[cubeEnum::cubeId::sand]		.create({ {16,  2}, {16,  2}, {16,  2}, {16,  2}, {16,  2}, {16,  2} }, cubeEnum::coverageId::sand);

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
		std::vector<noiseStruct::noiseNode> node{};

		std::vector<float> xPosArray;			//2d x轴坐标数组，在线程间是只读的
		std::vector<float> zPosArray;			//2d z轴坐标数组，在线程间是只读的

		int worldSeed = noiseInfo::worldSeed;	//地形种子
		


		procNoiseNode() : node(noiseEnum::noiseId::count), xPosArray(plateInfo::cubeArraySquare), zPosArray(plateInfo::cubeArraySquare) {
			node[noiseEnum::noiseId::elevation].create("D@BCBBCS4JDQAH@BCQYAAEAcRgQDAACAPwsAAIA/DAMAAIA/BA==", worldSeed);	//elevation根据海拔划分群系，所以种子必须和ground一样
			node[noiseEnum::noiseId::ground]   .create("KQkuCSgAAQ@ABEVCScJDQAH@BC@AQEAJBg@AMhDCAM@BQmpkZvxg@CBAOamRk/CwAAgD8MAg@BCAwAwoAAE@BRFQn/AgAC@DDADCgAAQ@ABEVCScJDQAH@BCQY@ABIQwgC@BE@DYzcxMPgQDmpkZPwszMzM/DAI@FMKAAB@BERUJ/wkAAg@DQAwVCQ0ABw@BgAACBACQY@BWRAgB@BBAOamRk/CwAAgD8EAg@BCAQAwCKQkVCScJDQAH@BCQYAAEAcRgQDAACAPwsAAIA/DAI@CgL8MEwAAwL8bAADAPwwCKQkVCf8UAAIAAM3MTD4MEwAAwL8bAADAPwwCKQkVCf8UAAIAAM3MzD4MEwAAwL8bAADAPwwCKQkVCf8UAAIAAJqZGT8MEwAAwL8bAADAPxQTAADIwhsAAMhCBA==", worldSeed);
			


			int	valueId = 0;

			for (int x = 0; x < plateInfo::cubeArrayLength; x++) {
				for (int z = 0; z < plateInfo::cubeArrayLength; z++) {
					valueId = x + z * plateInfo::cubeArrayLength;

					xPosArray[valueId] = (float)x;
					zPosArray[valueId] = (float)z;
				}
			}
		}

		void getNoiseArrayData(noiseEnum::noiseId::type terrainType, std::vector<float>& noiseData, const glm::dvec3& worldPos) {
			node[terrainType].noise->GenPositionArray2D(noiseData.data(), (int)xPosArray.size(), xPosArray.data(), zPosArray.data(), (float)worldPos.x, (float)worldPos.z, node[terrainType].seed);
		}

		double getNoiseSingleData(noiseEnum::noiseId::type terrainType, const glm::dvec3& cubePos) {
			return node[terrainType].noise->GenSingle2D((float)cubePos.x, (float)cubePos.z, node[terrainType].seed);
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