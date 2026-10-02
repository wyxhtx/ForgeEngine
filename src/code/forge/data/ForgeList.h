#pragma once



//循环列表
namespace loopList {
	std::vector<glm::ivec3> cubeMapLoop;					//方块Map循环列表，用于遍历方块数组，[1,32]
	std::vector<glm::ivec3> cubeXZLoop;						//方块XZ数组循环列表，[0,33]
	std::vector<glm::dvec3> chunkMapLoop;					//区块Map循环列表，用于遍历区块数组
	std::vector<glm::dvec3> regionLoop;						//地区循环列表

	std::unordered_set<glm::dvec3> structClearLoop;			//结构管理器清理循环列表

	std::vector<glm::ivec4> treeLoop;						//树循环列表

	std::unordered_set<glm::dvec3> terrainLayerLoop;		//地形管线0层循环列表，用于判断区块坐标是否在terrainEnum::terrainLayer::Layer_0中
}



//噪音列表
namespace noiseList {
	struct procNoiseNode {
		std::vector<noiseStruct::noiseNode> node{};

		std::vector<float> xPosArray2D{};		//2d x轴坐标数组，在线程间是只读的
		std::vector<float> zPosArray2D{};		//2d z轴坐标数组，在线程间是只读的

		std::vector<float> xPosArray3D{};		//3d x轴坐标数组，在线程间是只读的
		std::vector<float> yPosArray3D{};		//3d y轴坐标数组，在线程间是只读的
		std::vector<float> zPosArray3D{};		//3d z轴坐标数组，在线程间是只读的

		int worldSeed = noiseInfo::worldSeed;	//地形种子



		procNoiseNode() : node(noiseEnum::noiseId::enumCount), xPosArray2D(plateInfo::cubeArraySquare), zPosArray2D(plateInfo::cubeArraySquare), xPosArray3D(plateInfo::cubeArrayVolume), yPosArray3D(plateInfo::cubeArrayVolume), zPosArray3D(plateInfo::cubeArrayVolume) {
			node[noiseEnum::noiseId::elevation].create("D@BCBBCS4JDQAH@BCQYAAEAcRgQDAACAPwsAAIA/DAMAAIA/BA==", worldSeed);	//elevation根据海拔划分群系，所以种子必须和ground一样
			node[noiseEnum::noiseId::ground]   .create("KQkuCSgAAQ@ABEVCScJDQAH@BC@AQEAJBg@AMhDCAM@BQmpkZvxg@CBAOamRk/CwAAgD8MAwAAgMAEKAAB@BERUJ/wIAAw@AMAEKAAB@BERUJJwkNAAc@BJBg@AEhDCAI@BQ@CBjNzEw+BAOamRk/CzMzMz8UKAAB@BERUJ/wcAAw@AEAEFQkNAAc@BI@AgQAkG@BFkQIAQ@BQDmpkZPwsAAIA/BAMAAIBABAIpCRUJJwkNAAc@BJBgAAQBxGBAMAAIA/CwAAgD8MAwAAgL8EEwAAwL8bAADAPwwCKQkVCf8PAAPNzEw+BBMAAMC/GwAAwD8MAikJFQn/DwADzczMPgQTAADAvxsAAMA/DAIpCRUJ/w8AA5qZGT8EEwAAwL8bAADAPxQTAADIwhsAAMhCBA==", worldSeed);
			node[noiseEnum::noiseId::cave]	   .create("KAAB@BESQI@BQAkuAAE@BJFQkeCQ0AB@CkIBAs@B/DAM@AC/FBYCHAklAAI@BJCw@AEhDCAE@BQzczMPRg@ADAIAM@B4Ag@BMAAIA/C83MTD8TzcxMPgwCLgAB@BCf8GAAQLzcxMPgQKFwkNCQg@BgQQgD@BBAs@B/BAMK1yM9DAIoAAE@BRBAgK16M9CwAAyEMEBggE@BG@AoEEU", randomSeed(worldSeed));
			node[noiseEnum::noiseId::tree]	   .create("AQgAACDCBA==", randomSeed(worldSeed));



			int	noiseId2D = 0;
			int	noiseId3D = 0;

			//2D
			for (int x = 0; x < plateInfo::cubeArrayLength; x++) {
				for (int z = 0; z < plateInfo::cubeArrayLength; z++) {
					xPosArray2D[noiseId2D] = static_cast<float>(x);
					zPosArray2D[noiseId2D] = static_cast<float>(z);

					noiseId2D++;
				}
			}

			//3D
			for (int x = 0; x < plateInfo::cubeArrayLength; x++) {
				for (int z = 0; z < plateInfo::cubeArrayLength; z++) {
					for (int y = 0; y < plateInfo::cubeArrayLength; y++) {
						xPosArray3D[noiseId3D] = static_cast<float>(x);
						zPosArray3D[noiseId3D] = static_cast<float>(z);
						yPosArray3D[noiseId3D] = static_cast<float>(y);

						noiseId3D++;
					}
				}
			}
		}



		//2D噪音数组
		void getNoiseArray2D(noiseEnum::noiseId::type terrainType, std::vector<float>& noiseData, const glm::dvec3& worldPos) {
			node[terrainType].noise->GenPositionArray2D(noiseData.data(), static_cast<int>(xPosArray2D.size()), xPosArray2D.data(), zPosArray2D.data(), static_cast<float>(worldPos.x), static_cast<float>(worldPos.z), node[terrainType].seed);
		}

		//2D噪音点
		double getNoiseSingle2D(noiseEnum::noiseId::type terrainType, const glm::dvec3& worldPos) {
			return node[terrainType].noise->GenSingle2D(static_cast<float>(worldPos.x), static_cast<float>(worldPos.z), node[terrainType].seed);
		}

		//2D噪音点数组
		void getNoiseSingleArray2D(noiseEnum::noiseId::type terrainType, std::vector<float>& noiseData, const glm::dvec3& worldPos) {
			int	noiseId2D = 0;

			for (const glm::ivec3& arrayPos : loopList::cubeXZLoop) {
				noiseData[noiseId2D] = node[terrainType].noise->GenSingle2D(static_cast<float>(worldPos.x + arrayPos.x), static_cast<float>(worldPos.z + arrayPos.z), node[terrainType].seed);
				noiseId2D++;
			}
		}

		//3D噪音数组
		void getNoiseArray3D(noiseEnum::noiseId::type terrainType, std::vector<float>& noiseData, const glm::dvec3& worldPos) {
			node[terrainType].noise->GenPositionArray3D(noiseData.data(), static_cast<int>(xPosArray3D.size()), xPosArray3D.data(), yPosArray3D.data(), zPosArray3D.data(), static_cast<float>(worldPos.x), static_cast<float>(worldPos.y), static_cast<float>(worldPos.z), node[terrainType].seed);
		}

		//3D噪音点
		double getNoiseSingle3D(noiseEnum::noiseId::type terrainType, const glm::dvec3& worldPos) {
			return node[terrainType].noise->GenSingle3D(static_cast<float>(worldPos.x), static_cast<float>(worldPos.y), static_cast<float>(worldPos.z), node[terrainType].seed);
		}

		//3D噪音点数组
		void getNoiseSingleArray3D(noiseEnum::noiseId::type terrainType, std::vector<float>& noiseData, const glm::dvec3& worldPos) {
			int	noiseId3D = 0;

			for (const glm::ivec3& arrayPos : loopList::cubeXZLoop) {
				for (int y = 0; y < plateInfo::cubeArrayLength; y++) {
					noiseData[noiseId3D] = node[terrainType].noise->GenSingle3D(static_cast<float>(worldPos.x + arrayPos.x), static_cast<float>(worldPos.y + y), static_cast<float>(worldPos.z + arrayPos.z), node[terrainType].seed);
					noiseId3D++;
				}
			}
		}

	private:
		int randomSeed(int tempSeed) {
			static unsigned int salt = 0;

			//转为unsigned进行位运算，避免溢出UB
			unsigned int x = static_cast<unsigned int>(tempSeed) + salt * 0x9E3779B9U;
			x = (x ^ (x >> 16)) * 0x85ebca6bU;
			salt++;

			//转回int
			return static_cast<int>(x);
		}
	};

	struct procStructureNode {
		std::vector<noiseStruct::treeStructureNode> treeNode{};



		procStructureNode() : treeNode(noiseEnum::treeId::enumCount) {
			treeNode[noiseEnum::treeId::oak].create(5, 10, cubeEnum::cubeId::oakLeaf, cubeEnum::cubeId::oakTrunk);
		}
	};
}

//方块列表
namespace cubeList {
	struct procCubeNode {
		std::vector<cubeStruct::cubeNode> node{};



		procCubeNode() : node(cubeEnum::cubeId::enumCount) {
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