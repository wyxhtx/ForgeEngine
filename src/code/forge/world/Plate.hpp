#pragma once



class Plate {
public:
	glm::dvec3 originPos;																						//板块原点



	Plate() {
		engineStruct::timeNode time{};

		if (debugInfo::LogSwitch) {
			if (cubeEnum::cubeId::count != cubeEnum::coverageId::count) {
				Log::printError(__FILE__, __LINE__, "方块索引与覆盖索引数量不一致");
			}
		}

		createPlate();

		Log::printTitle("板块创建信息");
		Log::print("创建用时:\t" + time.end());
		Log::print("优化面数:\t" + std::to_string((terrainVertex::vertexs[forgeEnum::terrainLayer::Layer_0].size() + terrainVertex::vertexs[forgeEnum::terrainLayer::Layer_1].size()) / 4));

		if (debugInfo::LogSwitch) {
			Log::delimiter();
			Log::printTitle("————————Plate已创建");
			Log::delimiter();
		}
	}

	~Plate() {
		//保存区块文件
		for (const glm::dvec3& chunkPos : InitStorage.waitChunkList) {
			InitStorage.saveChunk(chunkMap[chunkPos]);
		}

		if (debugInfo::LogSwitch) {
			Log::print("Plate已清除");
		}
	}

private:
	Loop InitLoop{};
	Storage InitStorage{};

	noiseList::procNoiseNode procNoise{};																		//程序化噪音

	std::unordered_map<glm::dvec3, Chunk> chunkMap;																//区块map，存储区块数据
	std::unordered_map<glm::dvec3, Chunk> updateChunkMap;														//更新区块map，临时存储区块数据，存储当前视距内未加载区块
	std::forward_list<plateStruct::terrainMeshSignNode> terrainMeshSignList[forgeEnum::terrainLayer::count];	//标识地形管线各层网格中，区块顶点数组和索引数组的大小，为裁剪区块截层网格服务

	//结构管理器，只管理小于等于32并且无空气的结构，超出32会产生很多问题，比如区块存储加载，方块更新后的网格更新
	plateStruct::structManagerNode pendingStructManager;														//未加载结构管理器[0,33]，在createChunk中赋值
	plateStruct::structManagerNode activeStructManager;															//已加载结构管理器[0,33]，将pendingStructManager压入activeStructManager时必须覆盖检测

	const int maxThreadCount = std::thread::hardware_concurrency();												//设备最大核心数量，用于板块多线程创建，在Plate赋值



	//创建板块，R(8)N(34)M(350)
	void createPlate() {
		std::vector<glm::dvec3> updateChunkLoop;											//更新区块循环
		plateStruct::kMeansNode partChunkLoop;												//分区区块循环，用于并行生成地形
		std::vector<plateStruct::structPartManagerNode> linkStructManager(maxThreadCount);	//链结构管理器

		const glm::dvec3 playerChunkPos = glm::floor(playerInfo::initSpawnPos / (double)plateInfo::chunkLength);
		originPos = glm::floor(playerInfo::initSpawnPos / (double)plateInfo::originShiftThreshold) * (double)plateInfo::originShiftThreshold;



		//初始化，R(8)N(34)M(60)
		for (glm::dvec3 updatePos : loopList::chunkMapLoop) {
			updatePos += playerChunkPos;

			updateChunkLoop.push_back(updatePos);
			chunkMap[updatePos].InitChunk(originPos, updatePos);	//提前创建对应区块，避免并行访问chunkMap时冲突
		}

		partChunkLoop.kMeans(updateChunkLoop, maxThreadCount);

		//创建区块，R(8)N(34)M(130)
		std::transform(std::execution::par, partChunkLoop.node.begin(), partChunkLoop.node.end(), linkStructManager.begin(),
			[this](const std::vector<glm::dvec3>& chunkLoop) {
				plateStruct::structPartManagerNode linkStructSet;									//链结构集合[0,33]，由createChunk赋值，用于pendingStructManager适配多线程
				std::unordered_set<glm::dvec3> chunkPosSet(chunkLoop.begin(), chunkLoop.end());		//区块坐标集合，用于判断区块坐标是否在当前分区内

				//创建区块
				for (const glm::dvec3& updatePos : chunkLoop) {
					this->createChunk(chunkMap[updatePos], linkStructSet.pending);
				}

				//压入分区内链结构
				for (auto it = linkStructSet.pending.begin(); it != linkStructSet.pending.end(); ) {
					const glm::dvec3& updatePos = it->first;

					//如果区块在当前分区中存在，则chunkMap中必存在
					if (chunkPosSet.find(updatePos) != chunkPosSet.end()) {
						if (debugInfo::LogSwitch) {
							if (chunkMap.find(updatePos) == chunkMap.end()) {
								Log::printError(__FILE__, __LINE__, "区块未在当前分区生成");
							}
						}

						for (const glm::ivec4& cubePos : it->second) {
							short& cubeId = chunkMap[updatePos].cubeArray[cubePos.x][cubePos.y][cubePos.z];

							if (cubeId == cubeEnum::cubeId::air) {
								chunkMap[updatePos].isVisible++;
							}
							else if (cubeList::procCube.node[cubePos.w].coverage <= cubeList::procCube.node[cubeId].coverage) {
								continue;
							}

							cubeId = cubePos.w;
							linkStructSet.active[updatePos].insert(cubePos);
						}

						it = linkStructSet.pending.erase(it);
					}
					else {
						++it;
					}
				}

				//返回剩余的分区外pending结构和已压入的分区内active结构
				return linkStructSet;
			}
		);

		//压入分区外链结构，R(8)N(34)M(50)
		for (plateStruct::structPartManagerNode& node : linkStructManager) {
			for (auto it = node.pending.begin(); it != node.pending.end(); ) {
				const glm::dvec3& updatePos = it->first;

				if (chunkMap.find(updatePos) != chunkMap.end()) {
					for (const glm::ivec4& cubePos : it->second) {
						short& cubeId = chunkMap[updatePos].cubeArray[cubePos.x][cubePos.y][cubePos.z];

						if (cubeId == cubeEnum::cubeId::air) {
							chunkMap[updatePos].isVisible++;
						}
						else if (cubeList::procCube.node[cubePos.w].coverage <= cubeList::procCube.node[cubeId].coverage) {
							continue;
						}

						cubeId = cubePos.w;
						activeStructManager.node[updatePos].insert(cubePos);
					}

					it = node.pending.erase(it);
				}
				else {
					++it;
				}
			}

			//将剩余未加载链结构压入pendingStructManager
			for (const auto& [chunkPos, cubeSet] : node.pending) {
				//直接赋值
				if (pendingStructManager.node.find(chunkPos) == pendingStructManager.node.end()) {
					pendingStructManager.node[chunkPos] = cubeSet;
				}
				//遍历插入
				else {
					for (const glm::ivec4& cubePos : cubeSet) {
						pendingStructManager.node[chunkPos].insert(cubePos);
					}
				}
			}

			//将已加载链结构压入activeStructManager
			for (const auto& [chunkPos, cubeSet] : node.active) {
				//直接赋值
				if (activeStructManager.node.find(chunkPos) == activeStructManager.node.end()) {
					activeStructManager.node[chunkPos] = cubeSet;
				}
				//遍历插入
				else {
					for (const glm::ivec4& cubePos : cubeSet) {
						activeStructManager.node[chunkPos].insert(cubePos);
					}
				}
			}
		}



		//生成区块网格，直接遍历updateChunkLoop因为少了遍历分区所以性能更高，R(8)N(34)M(60)
		std::for_each(std::execution::par, updateChunkLoop.begin(), updateChunkLoop.end(),
			[this](const glm::dvec3& updatePos) {
				InitStorage.readChunk(chunkMap[updatePos]);		//读取区块文件

				if (chunkMap[updatePos].isVisible > 0 && chunkMap[updatePos].isVisible < plateInfo::cubeArrayVolume) {
					chunkMap[updatePos].createChunkMeshData();	//生成网格
				}
			}
		);

		//拼接区块网格，R(8)N(34)M(40)
		for (const glm::dvec3& updatePos : updateChunkLoop) {
			int layer = forgeEnum::terrainLayer::Layer_1;
			if (loopList::terrainLayerLoop.find(updatePos - playerChunkPos) != loopList::terrainLayerLoop.end()) {
				layer = forgeEnum::terrainLayer::Layer_0;
			}
			chunkMap[updatePos].layerID = layer;			//初始化layerID

			joinTerrainMesh(chunkMap[updatePos], originPos, updatePos, layer);

			if (debugInfo::LogSwitch) {
				if (chunkMap[updatePos].isVisible < 0 || chunkMap[updatePos].isVisible > plateInfo::cubeArrayVolume) {
					Log::printError(__FILE__, __LINE__, "isVisible超限：\t" + std::to_string(chunkMap[updatePos].isVisible));
				}

				if (chunkMap[updatePos].indexOffset < 0) {
					Log::printError(__FILE__, __LINE__, "indexOffset超限：\t" + std::to_string(chunkMap[updatePos].indexOffset));
				}
			}
		}

		//地形网格非空检测
		if (debugInfo::LogSwitch) {
			for (int layer = 0; layer < forgeEnum::terrainLayer::count; layer++) {
				if (terrainVertex::vertexs[layer].size() == 0) {
					Log::printError(__FILE__, __LINE__, std::format("Layer_{}对应顶点数组为空", layer));
				}

				if (terrainVertex::indexs[layer].size() == 0) {
					Log::printError(__FILE__, __LINE__, std::format("Layer_{}对应索引数组为空", layer));
				}
			}
		}
	}


	
	//创建区块，生成方块数组数据
	void createChunk(Chunk& chunk, std::unordered_map<glm::dvec3, std::unordered_set<glm::ivec4>>& linkStructSet) {
		std::vector<glm::ivec3> structList{};								//结构列表
		glm::dvec3 worldPos = chunk.logicalWorldPos - glm::dvec3{ 1 };		//区块数组实际范围在0~33，显示范围在1~32，所以地形生成时需要偏移到0~31
		glm::dvec3 tempWorldPos{};

		//噪音数据，因为并行化所以写在createChunk内部
		noiseStruct::noiseDataNode elevation{};
		noiseStruct::noiseDataNode ground{};
		noiseStruct::noiseDataNode cave{};



		if (!isNoiseExtreme(chunk.logicalWorldPos)) {
			int	noiseId2D = 0;

			elevation.data.resize(plateInfo::cubeArraySquare);
			ground.data.resize(plateInfo::cubeArraySquare);

			procNoise.getNoiseArray2D(noiseEnum::noiseId::elevation, elevation.data, worldPos);
			procNoise.getNoiseArray2D(noiseEnum::noiseId::ground,	 ground.data,	 worldPos);

			//地表生成
			for (int x = 0; x < plateInfo::cubeArrayLength; x++) {			tempWorldPos.x = worldPos.x + x;
				for (int z = 0; z < plateInfo::cubeArrayLength; z++) {		tempWorldPos.z = worldPos.z + z;
					elevation.value = elevation.data[noiseId2D];
					ground.value	= std::floor(ground.data[noiseId2D]);
					noiseId2D++;

					for (int y = 0; y < plateInfo::cubeArrayLength; y++) {	tempWorldPos.y = worldPos.y + y;
						//高于地平线生成空气
						if (tempWorldPos.y < plateInfo::groundPlane) {
							setCube(chunk.cubeArray[x][y][z], chunk.isVisible, tempWorldPos, x, y, z, elevation.value, ground.value, structList);
						}
						else {
							chunk.cubeArray[x][y][z] = cubeEnum::cubeId::air;
						}
					}
				}
			}

			//洞穴生成，只处理非空区块
			if (chunk.isVisible != 0) {
				int	noiseId3D = 0;

				cave.data.resize(plateInfo::cubeArrayVolume);
				procNoise.getNoiseArray3D(noiseEnum::noiseId::cave, cave.data, worldPos);

				for (int x = 0; x < plateInfo::cubeArrayLength; x++) {			tempWorldPos.x = worldPos.x + x;
					for (int z = 0; z < plateInfo::cubeArrayLength; z++) {		tempWorldPos.z = worldPos.z + z;
						for (int y = 0; y < plateInfo::cubeArrayLength; y++) {	tempWorldPos.y = worldPos.y + y;
							short& cubeId = chunk.cubeArray[x][y][z];

							//只处理非空方块
							if ((cubeId != cubeEnum::cubeId::air) && (cave.data[noiseId3D] > 0)) {
								chunk.isVisible--;

								cubeId = cubeEnum::cubeId::air;
							}

							noiseId3D++;
						}
					}
				}
			}
		}
		else {
			//地表生成
			for (int x = 0; x < plateInfo::cubeArrayLength; x++) {			tempWorldPos.x = worldPos.x + x;
				for (int z = 0; z < plateInfo::cubeArrayLength; z++) {		tempWorldPos.z = worldPos.z + z;
					elevation.value = procNoise.getNoiseSingle2D(noiseEnum::noiseId::elevation, tempWorldPos);
					ground.value    = std::floor(procNoise.getNoiseSingle2D(noiseEnum::noiseId::ground, tempWorldPos));

					for (int y = 0; y < plateInfo::cubeArrayLength; y++) {	tempWorldPos.y = worldPos.y + y;
						//高于地平线生成空气
						if (tempWorldPos.y < plateInfo::groundPlane) {
							setCube(chunk.cubeArray[x][y][z], chunk.isVisible, tempWorldPos, x, y, z, elevation.value, ground.value, structList);
						}
						else {
							chunk.cubeArray[x][y][z] = cubeEnum::cubeId::air;
						}
					}
				}
			}

			//洞穴生成，只处理非空区块
			if (chunk.isVisible != 0) {
				for (int x = 0; x < plateInfo::cubeArrayLength; x++) {			tempWorldPos.x = worldPos.x + x;
					for (int z = 0; z < plateInfo::cubeArrayLength; z++) {		tempWorldPos.z = worldPos.z + z;
						for (int y = 0; y < plateInfo::cubeArrayLength; y++) {	tempWorldPos.y = worldPos.y + y;
							short& cubeId = chunk.cubeArray[x][y][z];

							//只处理非空方块
							if ((cubeId != cubeEnum::cubeId::air) && (procNoise.getNoiseSingle3D(noiseEnum::noiseId::cave, tempWorldPos) > 0)) {
								chunk.isVisible--;

								cubeId = cubeEnum::cubeId::air;
							}
						}
					}
				}
			}
		}



		//生成地形结构管理器
		if (!structList.empty()) {
			const float lengthF = plateInfo::chunkLength;

			glm::dvec3 chunkPos;
			glm::ivec4 tempPos;

			for (const glm::ivec3& structPos : structList) {
				for (const glm::ivec4& pos : loopList::treeLoop) {
					tempPos = glm::ivec4{ structPos,0 } + pos;

					if (tempPos.x > 0 && tempPos.x < plateInfo::lengthIncrement && tempPos.y > 0 && tempPos.y < plateInfo::lengthIncrement && tempPos.z > 0 && tempPos.z < plateInfo::lengthIncrement) {
						//[1,32]
						short& cubeId = chunk.cubeArray[tempPos.x][tempPos.y][tempPos.z];

						if (cubeId == cubeEnum::cubeId::air) {
							chunk.isVisible++;
						}
						else if (cubeList::procCube.node[tempPos.w].coverage <= cubeList::procCube.node[cubeId].coverage) {
							continue;
						}

						chunkPos = chunk.chunkPos;
						cubeId = tempPos.w;

						//[0,33]
						if (tempPos.x == plateInfo::chunkLength) {
							chunkPos.x++;
							tempPos.x = 0;
							linkStructSet[chunkPos].insert(tempPos);
							tempPos.x = plateInfo::chunkLength;
							chunkPos.x--;
						}
						else if (tempPos.x == 1) {
							chunkPos.x--;
							tempPos.x = plateInfo::lengthIncrement;
							linkStructSet[chunkPos].insert(tempPos);
							tempPos.x = 1;
							chunkPos.x++;
						}

						if (tempPos.y == plateInfo::chunkLength) {
							chunkPos.y++;
							tempPos.y = 0;
							linkStructSet[chunkPos].insert(tempPos);
							tempPos.y = plateInfo::chunkLength;
							chunkPos.y--;
						}
						else if (tempPos.y == 1) {
							chunkPos.y--;
							tempPos.y = plateInfo::lengthIncrement;
							linkStructSet[chunkPos].insert(tempPos);
							tempPos.y = 1;
							chunkPos.y++;
						}

						if (tempPos.z == plateInfo::chunkLength) {
							chunkPos.z++;
							tempPos.z = 0;
							linkStructSet[chunkPos].insert(tempPos);
							tempPos.z = plateInfo::chunkLength;
							chunkPos.z--;
						}
						else if (tempPos.z == 1) {
							chunkPos.z--;
							tempPos.z = plateInfo::lengthIncrement;
							linkStructSet[chunkPos].insert(tempPos);
							tempPos.z = 1;
							chunkPos.z++;
						}
					}
					else {
						//小于等于0和大于等于33
						chunkPos.x = chunk.chunkPos.x + glm::floor((tempPos.x - 1) / lengthF);
						chunkPos.y = chunk.chunkPos.y + glm::floor((tempPos.y - 1) / lengthF);
						chunkPos.z = chunk.chunkPos.z + glm::floor((tempPos.z - 1) / lengthF);

						//从(-∞,0]和[33,+∞)偏移到[1,32]
						tempPos.x = (tempPos.x + plateInfo::lengthDecrement) % plateInfo::chunkLength + 1;
						tempPos.y = (tempPos.y + plateInfo::lengthDecrement) % plateInfo::chunkLength + 1;
						tempPos.z = (tempPos.z + plateInfo::lengthDecrement) % plateInfo::chunkLength + 1;

						//[1,32]
						linkStructSet[chunkPos].insert(tempPos);		//glm::ivec4一样的不用管，不一样的不会覆盖存储

						//[0,33]
						if (tempPos.x == plateInfo::chunkLength) {
							chunkPos.x++;
							tempPos.x = 0;
							linkStructSet[chunkPos].insert(tempPos);
							tempPos.x = plateInfo::chunkLength;
							chunkPos.x--;
						}
						else if (tempPos.x == 1) {
							chunkPos.x--;
							tempPos.x = plateInfo::lengthIncrement;
							linkStructSet[chunkPos].insert(tempPos);
							tempPos.x = 1;
							chunkPos.x++;
						}

						if (tempPos.y == plateInfo::chunkLength) {
							chunkPos.y++;
							tempPos.y = 0;
							linkStructSet[chunkPos].insert(tempPos);
							tempPos.y = plateInfo::chunkLength;
							chunkPos.y--;
						}
						else if (tempPos.y == 1) {
							chunkPos.y--;
							tempPos.y = plateInfo::lengthIncrement;
							linkStructSet[chunkPos].insert(tempPos);
							tempPos.y = 1;
							chunkPos.y++;
						}

						if (tempPos.z == plateInfo::chunkLength) {
							chunkPos.z++;
							tempPos.z = 0;
							linkStructSet[chunkPos].insert(tempPos);
							tempPos.z = plateInfo::chunkLength;
							chunkPos.z--;
						}
						else if (tempPos.z == 1) {
							chunkPos.z--;
							tempPos.z = plateInfo::lengthIncrement;
							linkStructSet[chunkPos].insert(tempPos);
							tempPos.z = 1;
							chunkPos.z++;
						}
					}
				}
			}
		}
	}

	bool isNoiseExtreme(const glm::dvec3& logicalWorldPos) {
		return std::abs(logicalWorldPos.x) > noiseInfo::noiseExtreme || std::abs(logicalWorldPos.y) > noiseInfo::noiseExtreme || std::abs(logicalWorldPos.z) > noiseInfo::noiseExtreme;
	}

	__forceinline void setCube(short& cubeId, int& isVisible, const glm::dvec3& tempWorldPos, const int& x, const int& y, const int& z, const double& elevationValue, const double& groundValue, std::vector<glm::ivec3>& structList) {
		if (tempWorldPos.y > groundValue) {
			cubeId = cubeEnum::cubeId::air;
		}
		else if (tempWorldPos.y < groundValue) {
			isVisible++;

			cubeId = cubeEnum::cubeId::stone;
		}
		else {
			isVisible++;

			if (elevationValue > 0.5) {
				cubeId = cubeEnum::cubeId::stone;
			}
			else if (elevationValue > -0.2) {
				cubeId = cubeEnum::cubeId::grass;
			}
			else if (elevationValue > -0.5) {
				cubeId = cubeEnum::cubeId::sand;
			}
			else {
				cubeId = cubeEnum::cubeId::gravel;
			}
		}
	}

	//拼接地形网格，将区块网格拼成地形网格，自动内存分配时函数耗时1.8ms，平时函数耗时0.18ms
	void joinTerrainMesh(Chunk& chunk, const glm::dvec3 originOffset, const glm::dvec3 chunkPos, const int layer) {
		if (chunk.indexOffset) {
			//等同(uint32_t)(terrainVertex::indexs.terrain[layer].size() / 1.5);
			uint32_t offset = (uint32_t)terrainVertex::vertexs[layer].size();

			if (debugInfo::LogSwitch) {
				if (offset != (uint32_t)(terrainVertex::indexs[layer].size() / 1.5)) {
					Log::printError(__FILE__, __LINE__, "索引偏移错误");
				}
			}



			//维护terrainMeshSignList
			terrainMeshSignList[layer].push_front(plateStruct::terrainMeshSignNode{ chunkPos, (int)chunk.chunkVertexs.size(), (int)chunk.chunkIndexs.size() });

			//配置旧区块
			if (chunk.originOffset != originOffset) {
				glm::vec3 tempPos = chunk.originOffset - originOffset;

				for (terrainVertex::vertexNode& node : chunk.chunkVertexs) {
					node.position += tempPos;
				}

				chunk.InitChunk(originOffset, chunkPos);
			}

			//拼入地形顶点数组
			terrainVertex::vertexs[layer].insert(terrainVertex::vertexs[layer].end(), chunk.chunkVertexs.begin(), chunk.chunkVertexs.end());

			//处理索引偏移
			std::vector<uint32_t> tempIndexs = chunk.chunkIndexs;
			for (uint32_t& i : tempIndexs) {
				i += offset;
			}

			//拼入地形索引数组
			terrainVertex::indexs[layer].insert(terrainVertex::indexs[layer].end(), tempIndexs.begin(), tempIndexs.end());
		}
	}

	//于地形网格中删除区块网格
	void delTerrainMesh(const glm::dvec3 chunkPos, const int layer) {
		for (std::forward_list<plateStruct::terrainMeshSignNode>::iterator node = terrainMeshSignList[layer].begin(), prev = terrainMeshSignList[layer].before_begin(); node != terrainMeshSignList[layer].end(); prev = node, ++node) {
			if (node->chunkPos == chunkPos) {
				std::forward_list<plateStruct::terrainMeshSignNode>::iterator p = std::next(node);
				int vertexsSize = 0;

				while (p != terrainMeshSignList[layer].end()) {
					vertexsSize += p->vertexsSize;
					++p;
				}

				terrainVertex::vertexs[layer].erase(terrainVertex::vertexs[layer].begin() + vertexsSize, terrainVertex::vertexs[layer].begin() + vertexsSize + node->vertexsSize);
				terrainVertex::indexs[layer].erase(terrainVertex::indexs[layer].end() - node->indexsSize, terrainVertex::indexs[layer].end());

				terrainMeshSignList[layer].erase_after(prev);
				break;
			}
		}
	}

public:
	std::unordered_map<glm::dvec3, std::unordered_set<glm::ivec3>> cubeChangeList;		//方块变更列表，存储变更方块坐标与对应区块坐标，[0,31]
	std::atomic<bool> isEntireRebuild = false;											//用于判断InitBuffer.recreateBuffer是完全重建还是部分重建



	//更新板块，R(8)N(34)M(100~160)(原点迁移时删除网格更快)
	void updatePlate(const glm::dvec3 logicOriginPos, const glm::dvec3 playerPos) {
		std::unordered_set<glm::dvec3> existChunkLoop;										//当前视距内已加载区块列表 
		std::vector<glm::dvec3> updateChunkLoop;											//当前视距内未加载区块列表
		plateStruct::kMeansNode partChunkLoop;												//分区区块循环，用于并行生成地形
		std::vector<plateStruct::structPartManagerNode> linkStructManager(maxThreadCount);	//链结构管理器
		std::unordered_set<glm::dvec3> updateStructLoop;									//更新结构循环，已加载区块被压入新链结构时，为避免线程冲突将对应区块延到锁域处理

		glm::dvec3 playerChunkPos = glm::floor(playerPos / (double)plateInfo::chunkLength);
		glm::dvec3 originShiftPos = glm::abs(logicOriginPos - playerPos);
		glm::dvec3 tempOriginPos  = originPos;												//临时原点坐标



		//---1、划分循环---
		//重设地区原点
		InitStorage.updateConfig(playerChunkPos);

		//重设逻辑原点
		if (originShiftPos.x > (double)plateInfo::originShiftThreshold || originShiftPos.y > (double)plateInfo::originShiftThreshold || originShiftPos.z > (double)plateInfo::originShiftThreshold) {
			tempOriginPos = glm::floor(playerPos / (double)plateInfo::originShiftThreshold) * (double)plateInfo::originShiftThreshold;
		}

		//获取existChunkLoop、updateChunkLoop，R(8)N(34)M(7)
		for (glm::dvec3 updatePos : loopList::chunkMapLoop) {
			updatePos += playerChunkPos;

			if (chunkMap.find(updatePos) != chunkMap.end()) {
				existChunkLoop.insert(updatePos);
			}
			else {
				updateChunkLoop.push_back(updatePos);
				updateChunkMap[updatePos].InitChunk(tempOriginPos, updatePos);
			}
		}

		//避免更新区块数大于线程数
		if (updateChunkLoop.size() <= 7 || maxThreadCount == 1) {
			//适用于区块数[1,7]，线程数1
			partChunkLoop.kMeans(updateChunkLoop, 1);
		}
		else {
			//适用于区块数[8,+∞)，线程数[2,+∞)
			for (int i = maxThreadCount; i >= 2; i--) {
				if (std::floor(updateChunkLoop.size() / i) >= 4) {
					partChunkLoop.kMeans(updateChunkLoop, i);
					break;
				}
			}
		}



		//---2、生成板块数据---
		//创建当前视距内未加载的区块，R(8)N(34)M(14)
		std::transform(std::execution::par, partChunkLoop.node.begin(), partChunkLoop.node.end(), linkStructManager.begin(),
			[this](const std::vector<glm::dvec3>& chunkLoop) {
				plateStruct::structPartManagerNode linkStructSet;									//链结构集合[0,33]
				std::unordered_set<glm::dvec3> chunkPosSet(chunkLoop.begin(), chunkLoop.end());		//区块坐标集合

				//创建区块
				for (const glm::dvec3& updatePos : chunkLoop) {
					this->createChunk(updateChunkMap[updatePos], linkStructSet.pending);
				}

				//压入分区内链结构
				for (auto it = linkStructSet.pending.begin(); it != linkStructSet.pending.end(); ) {
					const glm::dvec3& updatePos = it->first;

					if (chunkPosSet.find(updatePos) != chunkPosSet.end()) {
						if (debugInfo::LogSwitch) {
							if (updateChunkMap.find(updatePos) == updateChunkMap.end()) {
								Log::printError(__FILE__, __LINE__, "区块未在当前分区生成");
							}
						}

						for (const glm::ivec4& cubePos : it->second) {
							short& cubeId = updateChunkMap[updatePos].cubeArray[cubePos.x][cubePos.y][cubePos.z];

							if (cubeId == cubeEnum::cubeId::air) {
								updateChunkMap[updatePos].isVisible++;
							}
							else if (cubeList::procCube.node[cubePos.w].coverage <= cubeList::procCube.node[cubeId].coverage) {
								continue;
							}

							cubeId = cubePos.w;
							linkStructSet.active[updatePos].insert(cubePos);
						}

						it = linkStructSet.pending.erase(it);
					}
					else {
						++it;
					}
				}

				//返回剩余的分区外pending结构和已压入的分区内active结构
				return linkStructSet;
			}
		);

		//为避免卸载后又加载的链结构被遗漏，在updateChunkMap和activeStructManager的交集中，将activeStructManager中链结构压入对应pendingStructManager，R(8)N(34)M(0.05)
		for (const auto& [chunkPos, cubeSet] : activeStructManager.node) {
			if (updateChunkMap.find(chunkPos) != updateChunkMap.end()) {
				//直接压入
				if (pendingStructManager.node.find(chunkPos) == pendingStructManager.node.end()) {
					pendingStructManager.node[chunkPos] = cubeSet;
				}
				//遍历压入
				else {
					for (const glm::ivec4& cubePos : cubeSet) {
						pendingStructManager.node[chunkPos].insert(cubePos);	//此处无需覆盖检测，在压入结构时进行覆盖检测
					}
				}
			}
		}
		
		//压入分区外链结构，R(8)N(34)M(5)
		for (plateStruct::structPartManagerNode& node : linkStructManager) {
			for (auto it = node.pending.begin(); it != node.pending.end(); ) {
				const glm::dvec3& updatePos = it->first;

				if (updateChunkMap.find(updatePos) != updateChunkMap.end()) {
					for (const glm::ivec4& cubePos : it->second) {
						short& cubeId = updateChunkMap[updatePos].cubeArray[cubePos.x][cubePos.y][cubePos.z];

						if (cubeId == cubeEnum::cubeId::air) {
							updateChunkMap[updatePos].isVisible++;
						}
						else if (cubeList::procCube.node[cubePos.w].coverage <= cubeList::procCube.node[cubeId].coverage) {
							continue;
						}

						cubeId = cubePos.w;
						activeStructManager.node[updatePos].insert(cubePos);
					}

					it = node.pending.erase(it);
				}
				else {
					++it;
				}
			}

			//将剩余未加载链结构压入pendingStructManager
			for (const auto& [chunkPos, cubeSet] : node.pending) {
				//直接赋值
				if (pendingStructManager.node.find(chunkPos) == pendingStructManager.node.end()) {
					pendingStructManager.node[chunkPos] = cubeSet;
				}
				//遍历插入
				else {
					for (const glm::ivec4& cubePos : cubeSet) {
						pendingStructManager.node[chunkPos].insert(cubePos);
					}
				}
			}

			//将已加载链结构压入activeStructManager
			for (const auto& [chunkPos, cubeSet] : node.active) {
				//直接赋值
				if (activeStructManager.node.find(chunkPos) == activeStructManager.node.end()) {
					activeStructManager.node[chunkPos] = cubeSet;
				}
				//遍历插入
				else {
					for (const glm::ivec4& cubePos : cubeSet) {
						activeStructManager.node[chunkPos].insert(cubePos);
					}
				}
			}
		}

		//加载链结构，[0,33]，R(8)N(34)M(2)
		for (auto it = pendingStructManager.node.begin(); it != pendingStructManager.node.end(); ) {
			//如果pendingStructManager在updateChunkLoop中，则直接加载链结构
			if (updateChunkMap.find(it->first) != updateChunkMap.end()) {
				for (const glm::ivec4& cubePos : it->second) {
					short& cubeId = updateChunkMap[it->first].cubeArray[cubePos.x][cubePos.y][cubePos.z];

					if (cubeId == cubeEnum::cubeId::air) {
						updateChunkMap[it->first].isVisible++;
					}
					else if (cubeList::procCube.node[cubePos.w].coverage <= cubeList::procCube.node[cubeId].coverage) {
						continue;
					}

					cubeId = cubePos.w;
					activeStructManager.node[it->first].insert(cubePos);	//为避免同一方块坐标多个方块id，所有元素都得参与覆盖检测后压入
				}

				it = pendingStructManager.node.erase(it);
			}
			//如果pendingStructManager在existChunkLoop中，为避免拷贝chunkMap导致的多线程冲突，故延后到锁域处理
			else if (existChunkLoop.find(it->first) != existChunkLoop.end()) {
				updateStructLoop.insert(it->first);

				++it;
			}
			else {
				++it;
			}
		}



		//---3、处理结构、删除视距外板块网格---
		while (lockInfo::plateMeshDataLock.exchange(true)) { std::this_thread::yield(); }

		//处理updateStructLoop，复制chunkMap数据到updateChunkMap后加载链结构，R(8)N(34)M(4)
		for (const glm::dvec3& updatePos : updateStructLoop) {
			updateChunkMap[updatePos].InitChunk(tempOriginPos, updatePos);
			updateChunkMap[updatePos].isVisible = chunkMap[updatePos].isVisible;
			updateChunkMap[updatePos].layerID	= chunkMap[updatePos].layerID;
			std::memcpy(updateChunkMap[updatePos].cubeArray, chunkMap[updatePos].cubeArray, sizeof(chunkMap[updatePos].cubeArray));

			for (const glm::ivec4& cubePos : pendingStructManager.node[updatePos]) {
				short& cubeId = updateChunkMap[updatePos].cubeArray[cubePos.x][cubePos.y][cubePos.z];

				if (cubeId == cubeEnum::cubeId::air) {
					updateChunkMap[updatePos].isVisible++;
				}
				else if (cubeList::procCube.node[cubePos.w].coverage <= cubeList::procCube.node[cubeId].coverage) {
					continue;
				}

				cubeId = cubePos.w;
				activeStructManager.node[updatePos].insert(cubePos);
			}

			updateChunkLoop.push_back(updatePos);
			pendingStructManager.node.erase(updatePos);
		}

		//清理r+2外的结构，R(8)N(34)M(1)
		for (auto it = activeStructManager.node.begin(); it != activeStructManager.node.end(); ) {
			if (loopList::structClearLoop.find(glm::dvec3{ it->first - playerChunkPos }) == loopList::structClearLoop.end()) {
				it = activeStructManager.node.erase(it);
			}
			else {
				it++;
			}
		}

		for (auto it = pendingStructManager.node.begin(); it != pendingStructManager.node.end(); ) {
			if (loopList::structClearLoop.find(glm::dvec3{ it->first - playerChunkPos }) == loopList::structClearLoop.end()) {
				it = pendingStructManager.node.erase(it);
			}
			else {
				it++;
			}
		}

		//卸载视距外地形网格，因为原点未迁移处理耗时太高，所以全采用原点迁移处理，if (tempOriginPos == playerInfo::logicOriginPos)
		if (false) {
			//原点未迁移，R(8)N(34)M(90)
			for (std::unordered_map<glm::dvec3, Chunk>::iterator it = chunkMap.begin(); it != chunkMap.end(); ) {
				const glm::dvec3& updatePos = it->first;
				int oldLayer = chunkMap[updatePos].layerID;

				//删除existChunkLoop外区块及对应地形层区块网格
				if (existChunkLoop.find(updatePos) == existChunkLoop.end()) {
					delTerrainMesh(updatePos, oldLayer);											//删除oldLayer中区块网格

					//保存区块文件
					if (InitStorage.waitChunkList.find(updatePos) != InitStorage.waitChunkList.end()) {
						InitStorage.waitChunkList.erase(updatePos);
						InitStorage.saveChunk(chunkMap[updatePos]);
					}

					//删除区块
					it = chunkMap.erase(it);
				}
				//迁移existChunkLoop内区块网格到对应地形网格层
				else {
					int newLayer = forgeEnum::terrainLayer::Layer_1;

					if (loopList::terrainLayerLoop.find(updatePos - playerChunkPos) != loopList::terrainLayerLoop.end()) {
						newLayer = forgeEnum::terrainLayer::Layer_0;
					}

					if (oldLayer != newLayer) {
						delTerrainMesh(updatePos, oldLayer);										//删除oldLayer中区块网格
						chunkMap[updatePos].layerID = newLayer;										//重设layerID
						joinTerrainMesh(chunkMap[updatePos], tempOriginPos, updatePos, newLayer);	//重拼区块网格到newLayer
					}

					++it;
				}
			}
		}
		else {
			//原点已迁移，R(8)N(34)M(12)
			//清空全部地形网格
			for (int layer = 0; layer < forgeEnum::terrainLayer::count; layer++) {
				terrainVertex::vertexs[layer].clear();
				terrainVertex::indexs[layer].clear();
				terrainMeshSignList[layer].clear();
			}

			for (std::unordered_map<glm::dvec3, Chunk>::iterator it = chunkMap.begin(); it != chunkMap.end(); ) {
				const glm::dvec3& updatePos = it->first;

				//删除existChunkLoop外区块
				if (existChunkLoop.find(updatePos) == existChunkLoop.end()) {
					//保存区块文件
					if (InitStorage.waitChunkList.find(updatePos) != InitStorage.waitChunkList.end()) {
						InitStorage.waitChunkList.erase(updatePos);
						InitStorage.saveChunk(chunkMap[updatePos]);
					}

					//删除区块
					it = chunkMap.erase(it);
				}
				else {
					int layer = forgeEnum::terrainLayer::Layer_1;
					if (loopList::terrainLayerLoop.find(updatePos - playerChunkPos) != loopList::terrainLayerLoop.end()) {
						layer = forgeEnum::terrainLayer::Layer_0;
					}

					chunkMap[updatePos].layerID = layer;									//重设layerID
					joinTerrainMesh(chunkMap[updatePos], tempOriginPos, updatePos, layer);	//重拼区块网格到对应地形网格层

					++it;
				}
			}
		}

		//变更板块原点
		if (tempOriginPos != originPos) {
			originPos = tempOriginPos;
		}

		isEntireRebuild.store(true);
		lockInfo::plateMeshDataLock.store(false);



		//---4、生成板块网格---
		//生成区块网格，R(8)N(34)M(10)
		std::for_each(std::execution::par, updateChunkLoop.begin(), updateChunkLoop.end(),
			[this, updateStructLoop](const glm::dvec3& updatePos) {
				if (updateStructLoop.find(updatePos) == updateStructLoop.end()) {		//updateStructLoop由视距内已加载区块得到，故不用文件加载
					InitStorage.readChunk(updateChunkMap[updatePos]);					//读取区块文件，在加载链结构后执行，避免压入链结构后被覆盖
				}

				if (updateChunkMap[updatePos].isVisible > 0 && updateChunkMap[updatePos].isVisible < plateInfo::cubeArrayVolume) {
					updateChunkMap[updatePos].createChunkMeshData();	//生成网格
				}
			}
		);



		//---5、拼接板块网格---
		while (lockInfo::plateMeshDataLock.exchange(true)) { std::this_thread::yield(); }

		//拼接区块网格到对应地形网格层，R(8)N(34)M(45)
		for (const glm::dvec3& updatePos : updateChunkLoop) {
			chunkMap[updatePos] = updateChunkMap[updatePos];

			int layer = forgeEnum::terrainLayer::Layer_1;
			if (loopList::terrainLayerLoop.find(updatePos - playerChunkPos) != loopList::terrainLayerLoop.end()) {
				layer = forgeEnum::terrainLayer::Layer_0;
			}

			if (updateStructLoop.find(updatePos) != updateStructLoop.end()) {
				delTerrainMesh(updatePos, chunkMap[updatePos].layerID);
			}

			chunkMap[updatePos].layerID = layer;			//初始化layerID
			joinTerrainMesh(chunkMap[updatePos], originPos, updatePos, layer);

			if (debugInfo::LogSwitch) {
				if (chunkMap[updatePos].isVisible < 0 || chunkMap[updatePos].isVisible > plateInfo::cubeArrayVolume) {
					Log::printError(__FILE__, __LINE__, "isVisible超限：\t" + std::to_string(chunkMap[updatePos].isVisible));
				}

				if (chunkMap[updatePos].indexOffset < 0) {
					Log::printError(__FILE__, __LINE__, "indexOffset超限：\t" + std::to_string(chunkMap[updatePos].indexOffset));
				}
			}
		}

		updateChunkMap.clear();

		isEntireRebuild.store(true);
		lockInfo::plateMeshDataLock.store(false);



		//地形网格非空检测
		if (debugInfo::LogSwitch) {
			for (int layer = 0; layer < forgeEnum::terrainLayer::count; layer++) {
				if (terrainVertex::vertexs[layer].size() == 0) {
					Log::printWarning(__FILE__, __LINE__, std::format("Layer_{}对应顶点数组为空", layer));
				}

				if (terrainVertex::indexs[layer].size() == 0) {
					Log::printWarning(__FILE__, __LINE__, std::format("Layer_{}对应索引数组为空", layer));
				}
			}
		}
	}

	//todo 选择框有误差，等写完十字标时修正
	//todo 优化算法
	//选择框算法，参考Bresenham算法
	playerStruct::selectBoxNode getSelectBox(const glm::dvec3 logicOriginPos, const playerStruct::cameraNode& camera) {
		static glm::dvec3 displacement;			//坐标变化量
		static int remainingSteps;				//剩余步骤数量
		glm::dvec3 offsetPos{ 0 };				//坐标偏移量
		static glm::dvec3 stepDirection;		//步进方向
		static glm::dvec3 stepTime;				//每步用时
		static glm::dvec3 currentTime;			//当前用时

		static glm::dvec3 initialPos;			//起始坐标（d世界坐标系）
		static glm::dvec3 chunkPos;				//区块坐标（区块坐标系）
		static glm::ivec3 cubePos;				//方块坐标（方块坐标系）

		static double chunkLength = (double)plateInfo::chunkLength;



		initialPos = glm::floor(camera.pos);	//以玩家坐标为起始坐标
		chunkPos = glm::floor(initialPos / chunkLength);

		if (chunkMap.find(chunkPos) != chunkMap.end()) {
			//将d世界坐标系偏移到方块坐标系，cubePos范围为[1,32]
			//此处chunkPos * (double)plateInfo::chunkLength原为chunkMap[chunkPos].logicalWorldPos，因updatePlate执行chunkMap[updatePos] = updateChunkMap[updatePos]后会导致该值错误故更换
			cubePos = glm::ivec3{ initialPos - chunkPos * (double)plateInfo::chunkLength } + glm::ivec3{ 1 };

			if (debugInfo::LogSwitch) {
				if (cubePos.x < 1 || cubePos.x > plateInfo::chunkLength || cubePos.y < 1 || cubePos.y > plateInfo::chunkLength || cubePos.z < 1 || cubePos.z > plateInfo::chunkLength) {
					Log::printError(__FILE__, __LINE__, "getSelectBox函数cubePos超限");
				}
			}

			if (chunkMap[chunkPos].cubeArray[cubePos.x][cubePos.y][cubePos.z] != cubeEnum::cubeId::air) {
				if (playerInfo::mouseButtonLeft) {
					createUpdateCubeList(chunkPos, cubePos);
				}

				return playerStruct::selectBoxNode{ initialPos - logicOriginPos, 1 };	//返回i世界坐标系，如果起始坐标非空，则直接返回起始坐标
				//return playerStruct::selectBoxNode{ cubePos, 1 };						//返回玩家所在区块数组真实坐标
			}
		}

		displacement = camera.front * playerInfo::cubeInteractionRange;
		remainingSteps = (int)(glm::floor(glm::abs(displacement.x) + glm::abs(displacement.y) + glm::abs(displacement.z)));
		stepDirection.x = (displacement.x >= 0) ? 1 : -1;
		stepDirection.y = (displacement.y >= 0) ? 1 : -1;
		stepDirection.z = (displacement.z >= 0) ? 1 : -1;
		stepTime = glm::dvec3(1.0) / glm::abs(displacement);
		currentTime = stepTime;

		while (remainingSteps != 0) {
			if (currentTime.x < currentTime.y) {
				if (currentTime.x < currentTime.z) {
					currentTime.x += stepTime.x;
					offsetPos.x += stepDirection.x;
				}
				else {
					currentTime.z += stepTime.z;
					offsetPos.z += stepDirection.z;
				}
			}
			else {
				if (currentTime.y < currentTime.z) {
					currentTime.y += stepTime.y;
					offsetPos.y += stepDirection.y;
				}
				else {
					currentTime.z += stepTime.z;
					offsetPos.z += stepDirection.z;
				}
			}

			initialPos = glm::floor(camera.pos + offsetPos);
			chunkPos = glm::floor(initialPos / chunkLength);

			if (chunkMap.find(chunkPos) != chunkMap.end()) {
				//将d世界坐标系偏移到方块坐标系，cubePos范围为[1,32]
				cubePos = glm::ivec3{ initialPos - chunkPos * (double)plateInfo::chunkLength } + glm::ivec3{ 1 };

				if (debugInfo::LogSwitch) {
					if (cubePos.x < 1 || cubePos.x > plateInfo::chunkLength || cubePos.y < 1 || cubePos.y > plateInfo::chunkLength || cubePos.z < 1 || cubePos.z > plateInfo::chunkLength) {
						Log::printError(__FILE__, __LINE__, "getSelectBox函数cubePos超限");
					}
				}

				if (chunkMap[chunkPos].cubeArray[cubePos.x][cubePos.y][cubePos.z] != cubeEnum::cubeId::air) {
					if (playerInfo::mouseButtonLeft) {
						createUpdateCubeList(chunkPos, cubePos);
					}

					return playerStruct::selectBoxNode{ initialPos - logicOriginPos, 1 };	//返回i世界坐标系
					//return playerStruct::selectBoxNode{ cubePos, 1 };						//返回玩家所在区块数组真实坐标
				}
			}

			remainingSteps--;
		}

		return playerStruct::selectBoxNode{ glm::vec3{0}, 0 };
	}

	//更新变更方块，放置或破坏方块后更新地形网格
	void updateDeltaCube() {
		//因为forward_list的性质，处理近期处理过的区块更快，平均首次处理0.22，平均近期处理0.06
		for (const auto& updateCubeNode : cubeChangeList) {
			glm::dvec3 chunkPos = updateCubeNode.first;

			if (chunkMap.find(chunkPos) != chunkMap.end()) {
				int layer = chunkMap[chunkPos].layerID;

				if (layer != forgeEnum::terrainLayer::Layer_0) {
					isEntireRebuild.store(true);
				}

				delTerrainMesh(chunkPos, layer);																//从地形网格中删除旧区块网格
				chunkMap[chunkPos].updateChunkMeshData(updateCubeNode.second, cubeAbutChangeList[chunkPos]);	//生成方块变更后的新区块网格
				joinTerrainMesh(chunkMap[chunkPos], originPos, chunkPos, layer);								//重拼新区块网格到地形网格中
			}
			//已证实存在下述情况，变更板块边缘区块导致的
			//else if (debugInfo::LogSwitch) {
			//	Log::printError(__FILE__, __LINE__, "变更方块所处区块为空");
			//}
		}

		cubeChangeList.clear();
		cubeAbutChangeList.clear();
	}

private:
	std::unordered_map<glm::dvec3, std::unordered_set<glm::ivec2>> cubeAbutChangeList;	//方块相邻变更列表，边缘方块变更后，存储相邻区块需要变更的层，glm::ivec2存朝向和朝向对应的层，[0,31]



	//todo createUpdateCubeList使用chunkMap[chunkPos]与updatePlate竞争资源，当前区块被卸载就会出问题
	//todo 破坏方块也能用下述思想优化
	//todo 等到写放置方块时，会涉及边缘0与33的放置，写一个全局变量存储，此时对应区块未加载则保留对应元素，已加载则删除对应元素
	//todo 增加按压时长检测
	//todo chunkMap[chunkPos].cubeArray[cubePos.x][cubePos.y][cubePos.z].id = cubeList::air;只适用于挖掘
	//todo 速度过快时（128），相邻区块没加载出来时边缘数据不能正常更新
	//创建方块变更列表，供updatePlatePartMesh修改网格
	//chunkPos区块坐标系，cubePos方块坐标系（内部范围[1,32]）
	void createUpdateCubeList(const glm::dvec3& chunkPos, const glm::ivec3& cubePos) {
		static glm::dvec3 tempChunkPos;
		static const short cubeId = cubeEnum::cubeId::air;



		if (debugInfo::LogSwitch) {
			if (chunkMap.find(chunkPos) == chunkMap.end()) {
				Log::printError(__FILE__, __LINE__, "变更方块所处区块为空");
			}
		}

		chunkMap[chunkPos].cubeArray[cubePos.x][cubePos.y][cubePos.z] = cubeId;		//[1,32]
		chunkMap[chunkPos].isVisible--;
		cubeChangeList[chunkPos].insert(cubePos - glm::ivec3{ 1 });					//从[1,32]偏移到[0,31]，方便遍历32层

		InitStorage.waitChunkList.insert(chunkPos);
		chunkMap[chunkPos].cubeDeltaSet.emplace((short)cubePos.x, (short)cubePos.y, (short)cubePos.z, cubeId);

		if (cubePos.x == 1) {
			//[0,33]
			if (chunkMap[chunkPos].cubeArray[0][cubePos.y][cubePos.z] != cubeId) {
				//因为本区块x1非空，所以邻区块x33非空，因为本区块x0非空，所以邻区块x32非空，所以省略一层检测
				tempChunkPos = chunkPos - glm::dvec3(1, 0, 0);

				if (chunkMap.find(tempChunkPos) != chunkMap.end()) {
					if (debugInfo::LogSwitch) {
						if (chunkMap[tempChunkPos].cubeArray[plateInfo::lengthIncrement][cubePos.y][cubePos.z] == cubeId) {
							Log::printError(__FILE__, __LINE__, "x33为空");
						}
					}

					chunkMap[tempChunkPos].cubeArray[plateInfo::lengthIncrement][cubePos.y][cubePos.z] = cubeId;
					chunkMap[tempChunkPos].isVisible--;

					cubeChangeList[tempChunkPos];		//压入一个空元素，让updatePlatePartMesh遍历时处理cubeAbutChangeList所在区块，在Chunk.updateChunkMeshData中遍历updateCubeNode.second时如果为空则直接跳过
					cubeAbutChangeList[tempChunkPos].insert(glm::ivec2{ 2,plateInfo::lengthDecrement });

					InitStorage.waitChunkList.insert(tempChunkPos);
					chunkMap[tempChunkPos].cubeDeltaSet.emplace((short)plateInfo::lengthIncrement, (short)cubePos.y, (short)cubePos.z, cubeId);
				}
			}
		}
		else if (cubePos.x == plateInfo::chunkLength) {
			if (chunkMap[chunkPos].cubeArray[plateInfo::lengthIncrement][cubePos.y][cubePos.z] != cubeId) {
				tempChunkPos = chunkPos + glm::dvec3(1, 0, 0);

				if (chunkMap.find(tempChunkPos) != chunkMap.end()) {
					if (debugInfo::LogSwitch) {
						if (chunkMap[tempChunkPos].cubeArray[0][cubePos.y][cubePos.z] == cubeId) {
							Log::printError(__FILE__, __LINE__, "x0为空");
						}
					}

					chunkMap[tempChunkPos].cubeArray[0][cubePos.y][cubePos.z] = cubeId;
					chunkMap[tempChunkPos].isVisible--;

					cubeChangeList[tempChunkPos];
					cubeAbutChangeList[tempChunkPos].insert(glm::ivec2{ 3,0 });

					InitStorage.waitChunkList.insert(tempChunkPos);
					chunkMap[tempChunkPos].cubeDeltaSet.emplace((short)0, (short)cubePos.y, (short)cubePos.z, cubeId);
				}
			}
		}

		if (cubePos.y == 1) {
			if (chunkMap[chunkPos].cubeArray[cubePos.x][0][cubePos.z] != cubeId) {
				tempChunkPos = chunkPos - glm::dvec3(0, 1, 0);

				if (chunkMap.find(tempChunkPos) != chunkMap.end()) {
					if (debugInfo::LogSwitch) {
						if (chunkMap[tempChunkPos].cubeArray[cubePos.x][plateInfo::lengthIncrement][cubePos.z] == cubeId) {
							Log::printError(__FILE__, __LINE__, "y33为空");
						}
					}

					chunkMap[tempChunkPos].cubeArray[cubePos.x][plateInfo::lengthIncrement][cubePos.z] = cubeId;
					chunkMap[tempChunkPos].isVisible--;

					cubeChangeList[tempChunkPos];
					cubeAbutChangeList[tempChunkPos].insert(glm::ivec2{ 0,plateInfo::lengthDecrement });

					InitStorage.waitChunkList.insert(tempChunkPos);
					chunkMap[tempChunkPos].cubeDeltaSet.emplace((short)cubePos.x, (short)plateInfo::lengthIncrement, (short)cubePos.z, cubeId);
				}
			}
		}
		else if (cubePos.y == plateInfo::chunkLength) {
			if (chunkMap[chunkPos].cubeArray[cubePos.x][plateInfo::lengthIncrement][cubePos.z] != cubeId) {
				tempChunkPos = chunkPos + glm::dvec3(0, 1, 0);

				if (chunkMap.find(tempChunkPos) != chunkMap.end()) {
					if (debugInfo::LogSwitch) {
						if (chunkMap[tempChunkPos].cubeArray[cubePos.x][0][cubePos.z] == cubeId) {
							Log::printError(__FILE__, __LINE__, "y0为空");
						}
					}

					chunkMap[tempChunkPos].cubeArray[cubePos.x][0][cubePos.z] = cubeId;
					chunkMap[tempChunkPos].isVisible--;

					cubeChangeList[tempChunkPos];
					cubeAbutChangeList[tempChunkPos].insert(glm::ivec2{ 1,0 });

					InitStorage.waitChunkList.insert(tempChunkPos);
					chunkMap[tempChunkPos].cubeDeltaSet.emplace((short)cubePos.x, (short)0, (short)cubePos.z, cubeId);
				}
			}
		}

		if (cubePos.z == 1) {
			if (chunkMap[chunkPos].cubeArray[cubePos.x][cubePos.y][0] != cubeId) {
				tempChunkPos = chunkPos - glm::dvec3(0, 0, 1);

				if (chunkMap.find(tempChunkPos) != chunkMap.end()) {
					if (debugInfo::LogSwitch) {
						if (chunkMap[tempChunkPos].cubeArray[cubePos.x][cubePos.y][plateInfo::lengthIncrement] == cubeId) {
							Log::printError(__FILE__, __LINE__, "z33为空");
						}
					}

					chunkMap[tempChunkPos].cubeArray[cubePos.x][cubePos.y][plateInfo::lengthIncrement] = cubeId;
					chunkMap[tempChunkPos].isVisible--;

					cubeChangeList[tempChunkPos];
					cubeAbutChangeList[tempChunkPos].insert(glm::ivec2{ 4,plateInfo::lengthDecrement });

					InitStorage.waitChunkList.insert(tempChunkPos);
					chunkMap[tempChunkPos].cubeDeltaSet.emplace((short)cubePos.x, (short)cubePos.y, (short)plateInfo::lengthIncrement, cubeId);
				}
			}
		}
		else if (cubePos.z == plateInfo::chunkLength) {
			if (chunkMap[chunkPos].cubeArray[cubePos.x][cubePos.y][plateInfo::lengthIncrement] != cubeId) {
				tempChunkPos = chunkPos + glm::dvec3(0, 0, 1);

				if (chunkMap.find(tempChunkPos) != chunkMap.end()) {
					if (debugInfo::LogSwitch) {
						if (chunkMap[tempChunkPos].cubeArray[cubePos.x][cubePos.y][0] == cubeId) {
							Log::printError(__FILE__, __LINE__, "z0为空");
						}
					}

					chunkMap[tempChunkPos].cubeArray[cubePos.x][cubePos.y][0] = cubeId;
					chunkMap[tempChunkPos].isVisible--;

					cubeChangeList[tempChunkPos];
					cubeAbutChangeList[tempChunkPos].insert(glm::ivec2{ 5,0 });

					InitStorage.waitChunkList.insert(tempChunkPos);
					chunkMap[tempChunkPos].cubeDeltaSet.emplace((short)cubePos.x, (short)cubePos.y, (short)0, cubeId);
				}
			}
		}
	}
};