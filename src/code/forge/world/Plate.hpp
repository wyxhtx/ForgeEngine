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
	Loop	InitLoop{};
	Debug	InitDebug{};
	Storage InitStorage{};

	noiseList::procNoiseNode procNoise{};																		//程序化噪音

	std::unordered_map<glm::dvec3, Chunk> chunkMap;																//区块map，存储区块数据
	std::unordered_map<glm::dvec3, Chunk> updateChunkMap;														//更新区块map，临时存储区块数据，存储当前视距内未加载区块
	std::forward_list<plateStruct::terrainMeshSignNode> terrainMeshSignList[forgeEnum::terrainLayer::count];	//标识地形管线各层网格中，区块顶点数组和索引数组的大小，为裁剪区块截层网格服务

	const int maxThreadCount = std::thread::hardware_concurrency();												//设备最大核心数量，用于板块多线程创建，在Plate赋值
	plateStruct::linkStructManagerNode linkStructManager;															//链结构管理器[0,33]，只管理小于等于32并且无空气的结构，超出32会产生很多问题，比如区块存储加载，方块更新后的网格更新



	//创建板块，RM_0.4.6（8_32：1300）
	void createPlate() {
		std::vector<glm::dvec3> updateChunkLoop;								//更新区块循环
		plateStruct::kMeansNode partChunkLoop;									//分区区块循环，用于并行生成地形
		std::vector<plateStruct::linkStructManagerNode> linkStructPartManager;		//链结构分区管理器

		const glm::dvec3 playerChunkPos = glm::floor(playerInfo::initSpawnPos / (double)plateInfo::chunkLength);
		originPos = glm::floor(playerInfo::initSpawnPos / (double)plateInfo::originShiftThreshold) * (double)plateInfo::originShiftThreshold;



		//初始化，RM_0.4.6（8_32：60）
		for (glm::dvec3 chunkPos : loopList::chunkMapLoop) {
			chunkPos += playerChunkPos;

			updateChunkLoop.push_back(chunkPos);
			chunkMap[chunkPos].InitChunk(originPos, chunkPos);	//提前创建对应区块，避免并行访问chunkMap时冲突
		}

		//避免创建区块数大于线程数，RM_0.4.6（8_32：6）
		if (updateChunkLoop.size() <= 7 || maxThreadCount == 1) {
			//适用于区块数[1,7]，线程数1
			partChunkLoop.kMeans(updateChunkLoop, 1);
		}
		else {
			//适用于区块数[8,+∞)，线程数[2,+∞)，确保一个线程至少处理四个区块
			for (int i = maxThreadCount; i >= 2; i--) {
				if (std::floor(updateChunkLoop.size() / i) >= 4) {
					partChunkLoop.kMeans(updateChunkLoop, i);
					break;
				}
			}
		}
		linkStructPartManager.resize(partChunkLoop.node.size());

		//创建区块，RM_0.4.7（8_32：900）
		std::transform(std::execution::par, partChunkLoop.node.begin(), partChunkLoop.node.end(), linkStructPartManager.begin(),
			[this](const std::vector<glm::dvec3>& chunkLoop) {
				plateStruct::linkStructManagerNode linkStructSet;										//链结构集合[0,33]，由createChunk赋值
				std::unordered_set<glm::dvec3> chunkPosSet(chunkLoop.begin(), chunkLoop.end());		//区块坐标集合，用于判断区块坐标是否在当前分区内

				//创建分区内区块
				for (const glm::dvec3& chunkPos : chunkLoop) {
					this->createChunk(chunkMap[chunkPos], linkStructSet.node);
				}

				//压入分区内链结构，RM_0.4.7（8_32：1~4）
				for (auto& [structPos, chunkSet] : linkStructSet.node) {
					for (auto& [chunkPos, cubeNode] : chunkSet) {
						if (chunkPosSet.find(chunkPos) != chunkPosSet.end()) {
							if (debugInfo::LogSwitch) {
								if (chunkMap.find(chunkPos) == chunkMap.end()) {
									Log::printError(__FILE__, __LINE__, "区块未在当前分区生成");
								}
							}

							Chunk& chunkNode = chunkMap[chunkPos];

							chunkNode.isVisibleDataLock->lock();
							for (const glm::ivec4& cubeData : cubeNode.cubeSet) {
								short& cubeId = chunkNode.cubeArray[cubeData.x][cubeData.y][cubeData.z];

								if (cubeId == cubeEnum::cubeId::air) {
									chunkNode.isVisible++;
								}
								else if (cubeList::procCube.node[cubeData.w].coverage <= cubeList::procCube.node[cubeId].coverage) {
									continue;
								}

								cubeId = cubeData.w;
							}
							chunkNode.isVisibleDataLock->unlock();

							//标识active状态
							cubeNode.isComplete = true;
						}
					}
				}

				return linkStructSet;
			}
		);

		//压入分区间链结构，处理链结构分区管理器linkStructPartManager，RM_0.4.7（8_32：10）
		for (auto& linkStructSet : linkStructPartManager) {
			for (auto& [structPos, chunkSet] : linkStructSet.node) {
				for (auto& [chunkPos, cubeNode] : chunkSet) {
					//处理分区间pending状态链结构
					if (!cubeNode.isComplete && chunkMap.find(chunkPos) != chunkMap.end()) {
						Chunk& chunkNode = chunkMap[chunkPos];

						chunkNode.isVisibleDataLock->lock();
						for (const glm::ivec4& cubeData : cubeNode.cubeSet) {
							short& cubeId = chunkNode.cubeArray[cubeData.x][cubeData.y][cubeData.z];

							if (cubeId == cubeEnum::cubeId::air) {
								chunkNode.isVisible++;
							}
							else if (cubeList::procCube.node[cubeData.w].coverage <= cubeList::procCube.node[cubeId].coverage) {
								continue;
							}

							cubeId = cubeData.w;
						}
						chunkNode.isVisibleDataLock->unlock();

						//标识active状态
						cubeNode.isComplete = true;
					}

					linkStructManager.node[structPos][chunkPos] = std::move(cubeNode);
				}
			}
		}



		//生成区块网格，直接遍历updateChunkLoop因为少了遍历分区所以性能更高，RM_0.4.7（8_32：170）
		std::for_each(std::execution::par, updateChunkLoop.begin(), updateChunkLoop.end(),
			[this](const glm::dvec3& chunkPos) {
				Chunk& chunkNode = chunkMap[chunkPos];
				InitStorage.readChunk(chunkNode);		//读取区块文件

				chunkNode.isVisibleDataLock->lock();
				if (chunkNode.isVisible > 0 && chunkNode.isVisible < plateInfo::cubeArrayVolume) {
					chunkNode.createChunkMeshData();	//生成网格
				}
				chunkNode.isVisibleDataLock->unlock();
			}
		);

		//拼接区块网格，RM_0.4.7（8_32：100）
		for (const glm::dvec3& chunkPos : updateChunkLoop) {
			Chunk& chunkNode = chunkMap[chunkPos];
			chunkNode.layerIdDataLock->lock();

			int layer = forgeEnum::terrainLayer::Layer_1;
			if (loopList::terrainLayerLoop.find(chunkPos - playerChunkPos) != loopList::terrainLayerLoop.end()) { layer = forgeEnum::terrainLayer::Layer_0; }

			chunkNode.layerId = layer;			//初始化layerID
			joinTerrainMesh(chunkNode, originPos, chunkPos, layer);
			chunkNode.layerIdDataLock->unlock();

			if (debugInfo::LogSwitch) {
				InitDebug.validateIsVisibleRange(chunkNode);
				InitDebug.validateIndexOffsetRange(chunkNode);
			}
		}



		//RM_0.4.7（8_32：20）
		//校验chunkMap长度
		if (debugInfo::LogSwitch) {
			InitDebug.validateChunkMapSize(chunkMap.size());
		}

		//地形网格非空检测
		if (debugInfo::LogSwitch) {
			InitDebug.validateTerrainLayerEmpty();
		}

		//校验isVisible一致性
		if (debugInfo::LogSwitch && !isHandleChangeCubeList()) {
			InitDebug.validateIsVisibleConsistency(chunkMap);
		}

		//校验本区块与邻区块，0或33处的方块是否对应
		if (debugInfo::LogSwitch) {
			InitDebug.validateExtremeCube(chunkMap);
		}
	}


	
	//创建区块，生成方块数组数据
	void createChunk(Chunk& chunk, std::unordered_map<glm::dvec3, std::unordered_map<glm::dvec3, plateStruct::linkStructChunkNode>>& linkStructSet) {
		std::vector<glm::ivec3> structPosList{};								//结构源点列表，存储结构源点，方块坐标系
		glm::dvec3 cubeWorldPos = chunk.logicalWorldPos - glm::dvec3{ 1 };		//方块世界坐标，将方块坐标[1,32]减一偏移到世界坐标[0,31]
		glm::ivec3 tempCubePos{};												//临时存储cubePos变体

		//噪音数据，因为并行化所以写在createChunk内部
		noiseStruct::noiseDataNode elevation{};
		noiseStruct::noiseDataNode ground{};
		noiseStruct::noiseDataNode cave{};

		bool isExtreme = !isNoiseExtreme(chunk.logicalWorldPos);
		bool isCave    = false;
		int	noiseId2D = 0;
		int	noiseId3D = 0;



		elevation.data.resize(plateInfo::cubeArraySquare);
		ground.data.resize(plateInfo::cubeArraySquare);
		if (isExtreme) {
			procNoise.getNoiseArray2D(noiseEnum::noiseId::elevation, elevation.data, cubeWorldPos);
			procNoise.getNoiseArray2D(noiseEnum::noiseId::ground,	 ground.data,	 cubeWorldPos);
		}
		else {
			procNoise.getNoiseSingleArray2D(noiseEnum::noiseId::elevation, elevation.data, cubeWorldPos);
			procNoise.getNoiseSingleArray2D(noiseEnum::noiseId::ground,    ground.data,    cubeWorldPos);
		}

		//获取ground噪音最大值，如果区块高度最小值<=ground最大值，则非全空，即需要调用cave噪音
		isCave = (cubeWorldPos.y <= std::reduce(std::execution::par, ground.data.begin(), ground.data.end(), ground.data[0], [](float a, float b) { return std::max(a, b); }));
		if (isCave) {
			cave.data.resize(plateInfo::cubeArrayVolume);

			if (isExtreme) {
				procNoise.getNoiseArray3D(noiseEnum::noiseId::cave, cave.data, cubeWorldPos);
			}
			else {
				procNoise.getNoiseSingleArray3D(noiseEnum::noiseId::cave, cave.data, cubeWorldPos);
			}
		}

		for (const glm::ivec3& cubePos : loopList::cubeXZLoop) {
			elevation.value = elevation.data[noiseId2D];
			ground.value	= std::floor(ground.data[noiseId2D]);
			noiseId2D++;

			tempCubePos = cubePos;

			chunk.isVisibleDataLock->lock();
			for (int y = 0; y < plateInfo::cubeArrayLength; y++) {
				if (isCave) {
					cave.value = cave.data[noiseId3D];
					noiseId3D++;
				}

				tempCubePos.y = y;
				generateCube(chunk.cubeArray[cubePos.x][y][cubePos.z], chunk.isVisible, glm::dvec3{ tempCubePos } + cubeWorldPos, tempCubePos, elevation.value, ground.value, cave.value, isCave, structPosList);
			}
			chunk.isVisibleDataLock->unlock();
		}



		//生成地形结构管理器
		if (!structPosList.empty()) {
			const float lengthF = plateInfo::chunkLength;
			glm::dvec3 chunkPos;			//区块坐标
			glm::dvec3 strucWorldPos;		//结构源点世界坐标
			glm::ivec4 structCubeData;		//结构方块数据

			for (const glm::ivec3& structPos : structPosList) {
				strucWorldPos = glm::dvec3{ structPos } + cubeWorldPos;

				chunk.isVisibleDataLock->lock();
				for (const glm::ivec4& nodePos : loopList::treeLoop) {
					structCubeData = glm::ivec4{ structPos,0 } + nodePos;

					if (structCubeData.x > 0 && structCubeData.x < plateInfo::lengthIncrement && structCubeData.y > 0 && structCubeData.y < plateInfo::lengthIncrement && structCubeData.z > 0 && structCubeData.z < plateInfo::lengthIncrement) {
						//[1,32]
						short& cubeId = chunk.cubeArray[structCubeData.x][structCubeData.y][structCubeData.z];

						if (cubeId == cubeEnum::cubeId::air) {
							chunk.isVisible++;
						}
						else if (cubeList::procCube.node[structCubeData.w].coverage <= cubeList::procCube.node[cubeId].coverage) {
							continue;
						}

						chunkPos = chunk.chunkPos;
						cubeId = structCubeData.w;

						//[0,33]
						if (structCubeData.x == plateInfo::chunkLength) {
							linkStructSet[strucWorldPos][glm::dvec3{ chunkPos.x + 1, chunkPos.y, chunkPos.z }].cubeSet.insert(glm::ivec4{ 0, structCubeData.y, structCubeData.z, structCubeData.w });
						}
						else if (structCubeData.x == 1) {
							linkStructSet[strucWorldPos][glm::dvec3{ chunkPos.x - 1, chunkPos.y, chunkPos.z }].cubeSet.insert(glm::ivec4{ plateInfo::lengthIncrement, structCubeData.y, structCubeData.z, structCubeData.w });
						}

						if (structCubeData.y == plateInfo::chunkLength) {
							linkStructSet[strucWorldPos][glm::dvec3{ chunkPos.x, chunkPos.y + 1, chunkPos.z }].cubeSet.insert(glm::ivec4{ structCubeData.x, 0, structCubeData.z, structCubeData.w });
						}
						else if (structCubeData.y == 1) {
							linkStructSet[strucWorldPos][glm::dvec3{ chunkPos.x, chunkPos.y - 1, chunkPos.z }].cubeSet.insert(glm::ivec4{ structCubeData.x, plateInfo::lengthIncrement, structCubeData.z, structCubeData.w });
						}

						if (structCubeData.z == plateInfo::chunkLength) {
							linkStructSet[strucWorldPos][glm::dvec3{ chunkPos.x, chunkPos.y, chunkPos.z + 1 }].cubeSet.insert(glm::ivec4{ structCubeData.x, structCubeData.y, 0, structCubeData.w });
						}
						else if (structCubeData.z == 1) {
							linkStructSet[strucWorldPos][glm::dvec3{ chunkPos.x, chunkPos.y, chunkPos.z - 1 }].cubeSet.insert(glm::ivec4{ structCubeData.x, structCubeData.y, plateInfo::lengthIncrement, structCubeData.w });
						}
					}
					else {
						//小于等于0和大于等于33
						chunkPos.x = chunk.chunkPos.x + glm::floor((structCubeData.x - 1) / lengthF);
						chunkPos.y = chunk.chunkPos.y + glm::floor((structCubeData.y - 1) / lengthF);
						chunkPos.z = chunk.chunkPos.z + glm::floor((structCubeData.z - 1) / lengthF);

						//从(-∞,0]和[33,+∞)偏移到[1,32]
						structCubeData.x = (structCubeData.x + plateInfo::lengthDecrement) % plateInfo::chunkLength + 1;
						structCubeData.y = (structCubeData.y + plateInfo::lengthDecrement) % plateInfo::chunkLength + 1;
						structCubeData.z = (structCubeData.z + plateInfo::lengthDecrement) % plateInfo::chunkLength + 1;

						//[1,32]
						linkStructSet[strucWorldPos][chunkPos].cubeSet.insert(structCubeData);		//glm::ivec4一样的不用管，不一样的不会覆盖存储

						//[0,33]
						if (structCubeData.x == plateInfo::chunkLength) {
							linkStructSet[strucWorldPos][glm::dvec3{ chunkPos.x + 1, chunkPos.y, chunkPos.z }].cubeSet.insert(glm::ivec4{ 0, structCubeData.y, structCubeData.z, structCubeData.w });
						}
						else if (structCubeData.x == 1) {
							linkStructSet[strucWorldPos][glm::dvec3{ chunkPos.x - 1, chunkPos.y, chunkPos.z }].cubeSet.insert(glm::ivec4{ plateInfo::lengthIncrement, structCubeData.y, structCubeData.z, structCubeData.w });
						}

						if (structCubeData.y == plateInfo::chunkLength) {
							linkStructSet[strucWorldPos][glm::dvec3{ chunkPos.x, chunkPos.y + 1, chunkPos.z }].cubeSet.insert(glm::ivec4{ structCubeData.x, 0, structCubeData.z, structCubeData.w });
						}
						else if (structCubeData.y == 1) {
							linkStructSet[strucWorldPos][glm::dvec3{ chunkPos.x, chunkPos.y - 1, chunkPos.z }].cubeSet.insert(glm::ivec4{ structCubeData.x, plateInfo::lengthIncrement, structCubeData.z, structCubeData.w });
						}

						if (structCubeData.z == plateInfo::chunkLength) {
							linkStructSet[strucWorldPos][glm::dvec3{ chunkPos.x, chunkPos.y, chunkPos.z + 1 }].cubeSet.insert(glm::ivec4{ structCubeData.x, structCubeData.y, 0, structCubeData.w });
						}
						else if (structCubeData.z == 1) {
							linkStructSet[strucWorldPos][glm::dvec3{ chunkPos.x, chunkPos.y, chunkPos.z - 1 }].cubeSet.insert(glm::ivec4{ structCubeData.x, structCubeData.y, plateInfo::lengthIncrement, structCubeData.w });
						}
					}
				}
				chunk.isVisibleDataLock->unlock();
			}
		}
	}

	//判断是否切换噪音算法
	bool isNoiseExtreme(const glm::dvec3& logicalWorldPos) {
		return std::abs(logicalWorldPos.x) > noiseInfo::noiseExtreme || std::abs(logicalWorldPos.y) > noiseInfo::noiseExtreme || std::abs(logicalWorldPos.z) > noiseInfo::noiseExtreme;
	}

	//生成方块
	__forceinline void generateCube(short& cubeId, int& isVisible, const glm::dvec3& cubeWorldPos, const glm::ivec3& cubePos, const double& elevationValue, const double& groundValue, const double& caveValue, const bool& isCave, std::vector<glm::ivec3>& structPosList) {
		if (cubeWorldPos.y < plateInfo::groundPlane) {
			if (cubeWorldPos.y > groundValue || (isCave && caveValue > 0)) {
				cubeId = cubeEnum::cubeId::air;
			}
			else if (cubeWorldPos.y < groundValue) {
				isVisible++;

				cubeId = cubeEnum::cubeId::stone;
			}
			else {
				isVisible++;

				if (elevationValue > 0.5) {
					cubeId = cubeEnum::cubeId::stone;
				}
				else if (elevationValue > -0.2) {
					if (procNoise.getNoiseSingle2D(noiseEnum::noiseId::tree, cubeWorldPos) > 0) {
						if (cubePos.x > 0 && cubePos.x < plateInfo::lengthIncrement && cubePos.y > 0 && cubePos.y < plateInfo::lengthIncrement && cubePos.z > 0 && cubePos.z < plateInfo::lengthIncrement) {
							structPosList.push_back(glm::ivec3{ cubePos.x, cubePos.y + 1, cubePos.z });
						}

						cubeId = cubeEnum::cubeId::oakTrunk;
					}
					else {
						cubeId = cubeEnum::cubeId::grass;
					}
				}
				else if (elevationValue > -0.5) {
					cubeId = cubeEnum::cubeId::sand;
				}
				else {
					cubeId = cubeEnum::cubeId::gravel;
				}
			}
		}
		else {
			cubeId = cubeEnum::cubeId::air;
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
	std::atomic<bool> isEntireRebuild = false;												//用于判断InitBuffer.recreateBuffer是完全重建还是部分重建



	//更新板块，RM_0.4.6（8_32：200~300）
	void updatePlate(const glm::dvec3 logicOriginPos, const glm::dvec3 playerPos) {
		std::unordered_set<glm::dvec3> existChunkLoop;										//存储当前视距内已加载区块列表 
		std::vector<glm::dvec3> updateChunkLoop;											//存储当前视距内未加载区块列表，和需重载网格的区块（updateStructLoop）
		plateStruct::kMeansNode partChunkLoop;												//分区区块循环，用于并行生成地形
		std::vector<plateStruct::linkStructManagerNode> linkStructPartManager;				//链结构分区管理器
		std::unordered_map<glm::dvec3, std::unordered_set<glm::dvec3>> updateStructSet;		//更新结构集合，存储板块新向旧压入结构时所涉及的structPos、chunkSet
		std::unordered_set<glm::dvec3> updateStructLoop;									//更新结构循环，存储updateStructSet所涉及的区块坐标

		glm::dvec3 playerChunkPos = glm::floor(playerPos / (double)plateInfo::chunkLength);
		glm::dvec3 originShiftPos = glm::abs(logicOriginPos - playerPos);
		glm::dvec3 tempOriginPos  = originPos;												//临时原点坐标，确保originPos在第三阶段结束后再变更



		//---1、划分循环---
		//重设地区原点
		InitStorage.updateConfig(playerChunkPos);

		//重设逻辑原点
		if (originShiftPos.x > (double)plateInfo::originShiftThreshold || originShiftPos.y > (double)plateInfo::originShiftThreshold || originShiftPos.z > (double)plateInfo::originShiftThreshold) {
			tempOriginPos = glm::floor(playerPos / (double)plateInfo::originShiftThreshold) * (double)plateInfo::originShiftThreshold;
		}

		//获取existChunkLoop、updateChunkLoop，RM_0.4.6（8_32：5~10）
		for (glm::dvec3 chunkPos : loopList::chunkMapLoop) {
			chunkPos += playerChunkPos;

			if (chunkMap.find(chunkPos) != chunkMap.end()) {
				existChunkLoop.insert(chunkPos);
			}
			else {
				updateChunkLoop.push_back(chunkPos);
				updateChunkMap[chunkPos].InitChunk(tempOriginPos, chunkPos);
			}
		}

		//避免更新区块数大于线程数
		if (updateChunkLoop.size() <= 7 || maxThreadCount == 1) {
			//适用于区块数[1,7]，线程数1
			partChunkLoop.kMeans(updateChunkLoop, 1);
		}
		else {
			//适用于区块数[8,+∞)，线程数[2,+∞)，确保一个线程至少处理四个区块
			for (int i = maxThreadCount; i >= 2; i--) {
				if (std::floor(updateChunkLoop.size() / i) >= 4) {
					partChunkLoop.kMeans(updateChunkLoop, i);
					break;
				}
			}
		}
		linkStructPartManager.resize(partChunkLoop.node.size());



		//---2、生成区块数据---
		//创建当前视距内未加载的区块，RM_0.4.7（8_32：100~120）
		std::transform(std::execution::par, partChunkLoop.node.begin(), partChunkLoop.node.end(), linkStructPartManager.begin(),
			[this](const std::vector<glm::dvec3>& chunkLoop) {
				plateStruct::linkStructManagerNode linkStructSet;										//链结构集合[0,33]，由createChunk赋值
				std::unordered_set<glm::dvec3> chunkPosSet(chunkLoop.begin(), chunkLoop.end());		//区块坐标集合，用于判断区块坐标是否在当前分区内

				//创建分区内区块
				for (const glm::dvec3& chunkPos : chunkLoop) {
					this->createChunk(updateChunkMap[chunkPos], linkStructSet.node);
				}

				//压入分区内链结构，RM_0.4.7（8_32：1~4）
				for (auto& [structPos, chunkSet] : linkStructSet.node) {
					for (auto& [chunkPos, cubeNode] : chunkSet) {
						if (chunkPosSet.find(chunkPos) != chunkPosSet.end()) {
							if (debugInfo::LogSwitch) {
								if (updateChunkMap.find(chunkPos) == updateChunkMap.end()) {
									Log::printError(__FILE__, __LINE__, "区块未在当前分区生成");
								}
							}

							Chunk& chunkNode = updateChunkMap[chunkPos];

							chunkNode.isVisibleDataLock->lock();
							for (const glm::ivec4& cubeData : cubeNode.cubeSet) {
								short& cubeId = chunkNode.cubeArray[cubeData.x][cubeData.y][cubeData.z];

								if (cubeId == cubeEnum::cubeId::air) {
									chunkNode.isVisible++;
								}
								else if (cubeList::procCube.node[cubeData.w].coverage <= cubeList::procCube.node[cubeId].coverage) {
									continue;
								}

								cubeId = cubeData.w;
							}
							chunkNode.isVisibleDataLock->unlock();

							//标识active状态
							cubeNode.isComplete = true;
						}
					}
				}

				return linkStructSet;
			}
		);

		//已载过旧向新压入，RM_0.4.7（8_32：2）
		for (auto& [structPos, chunkSet] : linkStructManager.node) {
			for (auto& [chunkPos, cubeNode] : chunkSet) {
				//将active转为pending
				if (cubeNode.isComplete && updateChunkMap.find(chunkPos) != updateChunkMap.end()) {
					cubeNode.isComplete = false;
				}
			}
		}

		//压入分区间链结构，处理链结构分区管理器linkStructPartManager，RM_0.4.7（8_32：1~4）
		for (auto& linkStructSet : linkStructPartManager) {
			for (auto& [structPos, chunkSet] : linkStructSet.node) {
				for (auto& [chunkPos, cubeNode] : chunkSet) {
					if (debugInfo::LogSwitch) {
						if (linkStructManager.node.find(structPos) != linkStructManager.node.end() && linkStructManager.node[structPos].find(chunkPos) != linkStructManager.node[structPos].end()) {
							if (linkStructManager.node[structPos][chunkPos].cubeSet != cubeNode.cubeSet) {
								Log::printError(__FILE__, __LINE__, "跳过链结构的cubeSet不相等");
							}
						}
					}

					//跳过linkStructManager与linkStructPartManager的交集
					if (linkStructManager.node.find(structPos) == linkStructManager.node.end() || linkStructManager.node[structPos].find(chunkPos) == linkStructManager.node[structPos].end()) {
						//处理分区间pending状态链结构
						if (!cubeNode.isComplete && updateChunkMap.find(chunkPos) != updateChunkMap.end()) {
							Chunk& chunkNode = updateChunkMap[chunkPos];

							chunkNode.isVisibleDataLock->lock();
							for (const glm::ivec4& cubeData : cubeNode.cubeSet) {
								short& cubeId = chunkNode.cubeArray[cubeData.x][cubeData.y][cubeData.z];

								if (cubeId == cubeEnum::cubeId::air) {
									chunkNode.isVisible++;
								}
								else if (cubeList::procCube.node[cubeData.w].coverage <= cubeList::procCube.node[cubeId].coverage) {
									continue;
								}

								cubeId = cubeData.w;
							}
							chunkNode.isVisibleDataLock->unlock();

							//标识active状态
							cubeNode.isComplete = true;
						}

						linkStructManager.node[structPos][chunkPos] = std::move(cubeNode);
					}
				}
			}
		}

		//新旧板块间压入，处理链结构管理器linkStructManager，RM_0.4.7（8_32：2）
		for (auto& [structPos, chunkSet] : linkStructManager.node) {
			for (auto& [chunkPos, cubeNode] : chunkSet) {
				if (!cubeNode.isComplete) {
					if (updateChunkMap.find(chunkPos) != updateChunkMap.end()) {
						Chunk& chunkNode = updateChunkMap[chunkPos];

						chunkNode.isVisibleDataLock->lock();
						for (const glm::ivec4& cubeData : cubeNode.cubeSet) {
							short& cubeId = chunkNode.cubeArray[cubeData.x][cubeData.y][cubeData.z];

							if (cubeId == cubeEnum::cubeId::air) {
								chunkNode.isVisible++;
							}
							else if (cubeList::procCube.node[cubeData.w].coverage <= cubeList::procCube.node[cubeId].coverage) {
								continue;
							}

							cubeId = cubeData.w;
						}
						chunkNode.isVisibleDataLock->unlock();

						//标识active状态
						cubeNode.isComplete = true;
					}
					else if (existChunkLoop.find(chunkPos) != existChunkLoop.end()) {
						updateStructSet[structPos].insert(chunkPos);
						updateStructLoop.insert(chunkPos);
					}
				}
			}
		}



		//---3、处理链结构、删除视距外区块网格---
		while (lockInfo::plateMeshDataLock.exchange(true)) { std::this_thread::yield(); }
		while (lockInfo::chunkMapDataLock.exchange(true)) { std::this_thread::yield(); }

		//处理updateStructSet，复制chunkMap数据到updateChunkMap后加载链结构，RM_0.4.7（8_32：1）
		for (const auto& [structPos, chunkSet] : updateStructSet) {
			auto& linkStructNode = linkStructManager.node[structPos];

			for (const auto& chunkPos : chunkSet) {
				bool   isExist   = updateChunkMap.find(chunkPos) == updateChunkMap.end();
				Chunk& chunkNode = updateChunkMap[chunkPos];

				if (isExist) {
					chunkNode.copyCoreData(chunkMap[chunkPos]);
					chunkMap.erase(chunkPos);

					updateChunkLoop.push_back(chunkPos);
					existChunkLoop.erase(chunkPos);
				}

				bool isNotEmpty = !chunkNode.cubeDeltaSet.empty();

				chunkNode.isVisibleDataLock->lock();
				for (const glm::ivec4& cubeData : linkStructNode[chunkPos].cubeSet) {
					//如果cubeDeltaSet非空，且当前方块坐标进行过方块变更，则直接跳过压入链结构方块
					if (isNotEmpty && std::find_if(chunkNode.cubeDeltaSet.begin(), chunkNode.cubeDeltaSet.end(), [&](const storageStruct::cubeDeltaNode& n) {return n.data[0] == (short)cubeData.x && n.data[1] == (short)cubeData.y && n.data[2] == (short)cubeData.z; }) != chunkNode.cubeDeltaSet.end()) {
						continue;
					}

					short& cubeId = chunkNode.cubeArray[cubeData.x][cubeData.y][cubeData.z];

					if (cubeId == cubeEnum::cubeId::air) {
						chunkNode.isVisible++;
					}
					else if (cubeList::procCube.node[cubeData.w].coverage <= cubeList::procCube.node[cubeId].coverage) {
						continue;
					}

					cubeId = cubeData.w;
				}
				chunkNode.isVisibleDataLock->unlock();

				//标识active状态
				linkStructNode[chunkPos].isComplete = true;
			}
		}

		lockInfo::chunkMapDataLock.store(false);

		//清理r+2外的链结构，RM_0.4.7（8_32：1~10）
		for (auto structIt = linkStructManager.node.begin(); structIt != linkStructManager.node.end();) {
			bool isKeep = false;

			for (auto& [chunkPos, cubeNode] : structIt->second) {
				if (loopList::structClearLoop.find(chunkPos - playerChunkPos) != loopList::structClearLoop.end()) {
					isKeep = true;
					break;
				}
			}

			if (isKeep) {
				structIt++;
			}
			else {
				structIt = linkStructManager.node.erase(structIt);
			}
		}



		//卸载视距外区块网格，因为原点未迁移处理耗时太高，所以全采用原点迁移处理，RM_0.4.6（8_32：20）
		for (int layer = 0; layer < forgeEnum::terrainLayer::count; layer++) {
			//清空全部区块网格
			terrainVertex::vertexs[layer].clear();
			terrainVertex::indexs[layer].clear();
			terrainMeshSignList[layer].clear();
		}

		for (auto chunkIt = chunkMap.begin(); chunkIt != chunkMap.end(); ) {
			const glm::dvec3& chunkPos = chunkIt->first;

			//删除existChunkLoop外区块
			if (existChunkLoop.find(chunkPos) == existChunkLoop.end()) {
				//保存区块文件
				if (InitStorage.waitChunkList.find(chunkPos) != InitStorage.waitChunkList.end()) {
					InitStorage.waitChunkList.erase(chunkPos);
					InitStorage.saveChunk(chunkIt->second);
				}

				//删除区块
				chunkIt = chunkMap.erase(chunkIt);
			}
			//拼接existChunkLoop内区块网格
			else {
				chunkIt->second.layerIdDataLock->lock();

				int layer = forgeEnum::terrainLayer::Layer_1;
				if (loopList::terrainLayerLoop.find(chunkPos - playerChunkPos) != loopList::terrainLayerLoop.end()) { layer = forgeEnum::terrainLayer::Layer_0; }
				
				chunkIt->second.layerId = layer;										//重设layerID
				joinTerrainMesh(chunkIt->second, tempOriginPos, chunkPos, layer);		//重拼区块网格到对应地形网格层
				chunkIt->second.layerIdDataLock->unlock();

				chunkIt++;
			}
		}

		//变更板块原点
		if (tempOriginPos != originPos) {
			originPos = tempOriginPos;
		}

		isEntireRebuild.store(true);
		lockInfo::plateMeshDataLock.store(false);



		//---4、生成区块网格--- RM_0.4.6（8_32：30）
		std::for_each(std::execution::par, updateChunkLoop.begin(), updateChunkLoop.end(),
			[this, updateStructLoop](const glm::dvec3& chunkPos) {
				Chunk& chunkNode = updateChunkMap[chunkPos];

				if (updateStructLoop.find(chunkPos) == updateStructLoop.end()) {	//updateStructLoop由视距内已加载区块得到，故不用文件加载
					InitStorage.readChunk(chunkNode);								//读取区块文件，在加载链结构后执行，避免压入链结构后被覆盖
				}

				chunkNode.isVisibleDataLock->lock();
				if (chunkNode.isVisible > 0 && chunkNode.isVisible < plateInfo::cubeArrayVolume) {
					chunkNode.createChunkMeshData();	//生成网格
				}
				chunkNode.isVisibleDataLock->unlock();
			}
		);



		//---5、拼接区块网格---
		while (lockInfo::plateMeshDataLock.exchange(true)) { std::this_thread::yield(); }
		while (lockInfo::chunkMapDataLock.exchange(true)) { std::this_thread::yield(); }

		//拼接区块网格到对应地形网格层，RM_0.4.6（8_32：60）
		for (const glm::dvec3& chunkPos : updateChunkLoop) {
			Chunk& chunkNode = chunkMap[chunkPos];
			chunkNode = std::move(updateChunkMap[chunkPos]);		//此处使用move后，updateChunkMap就不能再使用了
			chunkNode.layerIdDataLock->lock();

			int layer = forgeEnum::terrainLayer::Layer_1;
			if (loopList::terrainLayerLoop.find(chunkPos - playerChunkPos) != loopList::terrainLayerLoop.end()) { layer = forgeEnum::terrainLayer::Layer_0; }
			
			//删除旧区块网格后重拼
			if (updateStructLoop.find(chunkPos) != updateStructLoop.end()) {
				delTerrainMesh(chunkPos, chunkNode.layerId);
			}
			chunkNode.layerId = layer;			//初始化layerID
			joinTerrainMesh(chunkNode, originPos, chunkPos, layer);
			chunkNode.layerIdDataLock->unlock();

			if (debugInfo::LogSwitch) {
				InitDebug.validateIsVisibleRange(chunkNode);
				InitDebug.validateIndexOffsetRange(chunkNode);
			}
		}

		updateChunkMap.clear();

		isEntireRebuild.store(true);
		lockInfo::chunkMapDataLock.store(false);
		lockInfo::plateMeshDataLock.store(false);



		//校验chunkMap长度
		if (debugInfo::LogSwitch && true) {
			InitDebug.validateChunkMapSize(chunkMap.size());
		}

		//地形网格非空检测
		if (debugInfo::LogSwitch && false) {
			InitDebug.validateTerrainLayerEmpty();
		}

		//校验isVisible一致性
		if (debugInfo::LogSwitch && true && !isHandleChangeCubeList()) {
			InitDebug.validateIsVisibleConsistency(chunkMap);
		}

		//校验本区块与邻区块，0或33处的方块是否对应
		if (debugInfo::LogSwitch && true) {
			InitDebug.validateExtremeCube(chunkMap);
		}
	}

	//todo 选择框有误差，等写完十字标时修正
	//选择框算法，参考Bresenham算法
	playerStruct::selectBoxNode getSelectBox(const glm::dvec3 logicOriginPos, const playerStruct::cameraNode& camera) {
		static glm::dvec3 displacement;						//坐标变化量
		static int remainingSteps;							//剩余步骤数量
		glm::dvec3 offsetPos{ 0 };							//坐标偏移量
		static glm::dvec3 stepDirection;					//步进方向
		static glm::dvec3 stepTime;							//每步用时
		static glm::dvec3 currentTime;						//当前用时

		static double chunkLength = (double)plateInfo::chunkLength;
		std::vector<playerStruct::selectBoxTraceNode> cubeList{};	//方块列表
		static playerStruct::selectBoxTraceNode cube{};				//方块结点



		displacement	= camera.front * playerInfo::cubeInteractionRange;
		remainingSteps	= (int)(glm::floor(glm::abs(displacement.x) + glm::abs(displacement.y) + glm::abs(displacement.z)));
		stepDirection.x = (displacement.x >= 0) ? 1 : -1;
		stepDirection.y = (displacement.y >= 0) ? 1 : -1;
		stepDirection.z = (displacement.z >= 0) ? 1 : -1;
		stepTime		= glm::dvec3(1.0) / glm::abs(displacement);
		currentTime		= stepTime;

		cube.initPos  = glm::floor(camera.pos);		//以玩家坐标为起始坐标
		cube.chunkPos = glm::floor(cube.initPos / chunkLength);

		while (remainingSteps != 0) {
			if (chunkMap.find(cube.chunkPos) != chunkMap.end()) {
				//将d世界坐标系偏移到方块坐标系，cubePos范围为[1,32]
				//此处chunkPos * chunkLength原为chunkMap[chunkPos].logicalWorldPos，因updatePlate执行chunkMap[chunkPos] = updateChunkMap[chunkPos]后会导致该值错误故更换
				cube.cubePos = glm::ivec3{ cube.initPos - cube.chunkPos * chunkLength } + glm::ivec3{ 1 };

				if (debugInfo::LogSwitch) {
					if (cube.cubePos.x < 1 || cube.cubePos.x > chunkLength || cube.cubePos.y < 1 || cube.cubePos.y > chunkLength || cube.cubePos.z < 1 || cube.cubePos.z > chunkLength) {
						Log::printError(__FILE__, __LINE__, "getSelectBox函数cubePos超限");
					}
				}

				if (chunkMap[cube.chunkPos].cubeArray[cube.cubePos.x][cube.cubePos.y][cube.cubePos.z] != cubeEnum::cubeId::air) {
					if (playerInfo::mouseButtonLeft.load()) {
						getChangeCubeList(cube.chunkPos, cube.cubePos, cubeEnum::cubeId::air);
					}
					else if (!cubeList.empty() && playerInfo::mouseButtonRightJust.load()) {
						playerInfo::mouseButtonRightJust.store(false);
						getChangeCubeList(cubeList.back().chunkPos, cubeList.back().cubePos, cubeEnum::cubeId::stoneSquared);
					}

					return playerStruct::selectBoxNode{ cube.initPos - logicOriginPos, 1 };			//返回i世界坐标系，如果起始坐标非空，则直接返回起始坐标
					//return playerStruct::selectBoxNode{ cubeBreakPos, 1 };						//返回玩家所在区块数组真实坐标
				}

				cubeList.push_back(cube);
			}

			if (currentTime.x < currentTime.y) {
				if (currentTime.x < currentTime.z) {
					currentTime.x += stepTime.x;
					offsetPos.x   += stepDirection.x;
				}
				else {
					currentTime.z += stepTime.z;
					offsetPos.z   += stepDirection.z;
				}
			}
			else {
				if (currentTime.y < currentTime.z) {
					currentTime.y += stepTime.y;
					offsetPos.y   += stepDirection.y;
				}
				else {
					currentTime.z += stepTime.z;
					offsetPos.z   += stepDirection.z;
				}
			}

			cube.initPos  = glm::floor(camera.pos + offsetPos);
			cube.chunkPos = glm::floor(cube.initPos / chunkLength);

			remainingSteps--;
		}

		return playerStruct::selectBoxNode{ glm::vec3{0}, 0 };
	}

	//判断是否调用handleChangeCubeList
	bool isHandleChangeCubeList() {
		return !changeCubeList.empty();
	}

	//处理变更方块列表，放置或破坏方块后更新地形网格
	void handleChangeCubeList() {
		//因为forward_list的性质，处理近期处理过的区块更快，平均首次处理0.22，平均近期处理0.06
		for (const auto& updateCubeNode : changeCubeList) {
			glm::dvec3 chunkPos = updateCubeNode.first;

			if (chunkMap.find(chunkPos) != chunkMap.end()) {
				Chunk& chunkNode = chunkMap[chunkPos];
				chunkNode.layerIdDataLock->lock();

				int layer = chunkNode.layerId;
				if (layer != forgeEnum::terrainLayer::Layer_0) { isEntireRebuild.store(true); }

				delTerrainMesh(chunkPos, layer);													//从地形网格中删除旧区块网格
				chunkNode.updateChunkMeshData(updateCubeNode.second, changeAbutCubeList[chunkPos]);	//生成方块变更后的新区块网格
				joinTerrainMesh(chunkNode, originPos, chunkPos, layer);								//重拼新区块网格到地形网格中
				chunkNode.layerIdDataLock->unlock();
			}
			//已证实存在下述情况，变更板块边缘区块导致的，不影响运行，所以不处理
			/*else if (debugInfo::LogSwitch) {
				Log::printError(__FILE__, __LINE__, "变更方块所处区块为空");
			}*/
		}

		changeCubeList.clear();
		changeAbutCubeList.clear();
	}

private:
	std::unordered_map<glm::dvec3, std::unordered_set<glm::ivec3>> changeCubeList;		//变更方块列表，存储变更方块坐标与对应区块坐标，[0,31]
	std::unordered_map<glm::dvec3, std::unordered_set<glm::ivec2>> changeAbutCubeList;	//变更相邻方块列表，位于边缘的方块变更后，存储相邻区块需要变更的层，glm::ivec2存朝向和朝向对应的层，[0,31]



	//获取变更方块列表，用于updateDeltaCube修改网格，plateMeshDataLock也能屏蔽chunkMap[chunkPos]资源竞争，chunkPos：非空方块所处区块坐标、区块坐标系，cubePos：非空方块坐标、方块坐标系（内部范围[1,32]）
	void getChangeCubeList(const glm::dvec3& chunkPos, const glm::ivec3& cubePos, const short& newCubeId) {
		if (debugInfo::LogSwitch) {
			if (chunkMap.find(chunkPos) == chunkMap.end()) {
				Log::printError(__FILE__, __LINE__, "变更方块所处区块为空");
			}
		}

		//边界方块相邻的区块，有一个不存在则退出
		if ((cubePos.x == 1						 && chunkMap.find(chunkPos - glm::dvec3{ 1, 0, 0 }) == chunkMap.end()) ||
			(cubePos.x == plateInfo::chunkLength && chunkMap.find(chunkPos + glm::dvec3{ 1, 0, 0 }) == chunkMap.end()) ||
			(cubePos.y == 1						 && chunkMap.find(chunkPos - glm::dvec3{ 0, 1, 0 }) == chunkMap.end()) ||
			(cubePos.y == plateInfo::chunkLength && chunkMap.find(chunkPos + glm::dvec3{ 0, 1, 0 }) == chunkMap.end()) ||
			(cubePos.z == 1						 && chunkMap.find(chunkPos - glm::dvec3{ 0, 0, 1 }) == chunkMap.end()) ||
			(cubePos.z == plateInfo::chunkLength && chunkMap.find(chunkPos + glm::dvec3{ 0, 0, 1 }) == chunkMap.end())) {
			return;
		}



		Chunk& chunkNode = chunkMap[chunkPos];
		short& oldCubeId = chunkNode.cubeArray[cubePos.x][cubePos.y][cubePos.z];		//[1,32]

		//---1~32---
		chunkNode.isVisibleDataLock->lock();
		if (oldCubeId == cubeEnum::cubeId::air) {
			if (newCubeId != cubeEnum::cubeId::air) {
				chunkNode.isVisible++;
			}
		}
		else {
			if (newCubeId == cubeEnum::cubeId::air) {
				chunkNode.isVisible--;
			}
		}
		oldCubeId = newCubeId;
		chunkNode.isVisibleDataLock->unlock();

		changeCubeList[chunkPos].insert(cubePos - glm::ivec3{ 1 });					//从[1,32]偏移到[0,31]，方便遍历32层

		//删除cubeDeltaSet中首位相同方块坐标的数据再插入，每次插入都删除首位相同方块坐标数据就能维持不重复
		auto& deltaSet = chunkNode.cubeDeltaSet;
		auto it = std::find_if(deltaSet.begin(), deltaSet.end(), [&](const storageStruct::cubeDeltaNode& n) {return n.data[0] == (short)cubePos.x && n.data[1] == (short)cubePos.y && n.data[2] == (short)cubePos.z; });
		if (it != deltaSet.end()) { deltaSet.erase(it); }

		chunkNode.cubeDeltaSet.emplace((short)cubePos.x, (short)cubePos.y, (short)cubePos.z, newCubeId);
		InitStorage.waitChunkList.insert(chunkPos);



		//---0、33---
		if (cubePos.x == 1) {
			getExtremeChangeCubeList(chunkPos - glm::dvec3{ 1, 0, 0 }, glm::dvec3{ plateInfo::lengthIncrement, cubePos.y, cubePos.z }, glm::ivec2{ 2,plateInfo::lengthDecrement }, newCubeId);
		}
		else if (cubePos.x == plateInfo::chunkLength) {
			getExtremeChangeCubeList(chunkPos + glm::dvec3{ 1, 0, 0 }, glm::dvec3{ 0, cubePos.y, cubePos.z },						   glm::ivec2{ 3,0 },						   newCubeId);
		}

		if (cubePos.y == 1) {
			getExtremeChangeCubeList(chunkPos - glm::dvec3{ 0, 1, 0 }, glm::dvec3{ cubePos.x, plateInfo::lengthIncrement, cubePos.z }, glm::ivec2{ 0,plateInfo::lengthDecrement }, newCubeId);
		}
		else if (cubePos.y == plateInfo::chunkLength) {
			getExtremeChangeCubeList(chunkPos + glm::dvec3{ 0, 1, 0 }, glm::dvec3{ cubePos.x, 0, cubePos.z },						   glm::ivec2{ 1,0 },						   newCubeId);
		}

		if (cubePos.z == 1) {
			getExtremeChangeCubeList(chunkPos - glm::dvec3{ 0, 0, 1 }, glm::dvec3{ cubePos.x, cubePos.y, plateInfo::lengthIncrement }, glm::ivec2{ 4,plateInfo::lengthDecrement }, newCubeId);
		}
		else if (cubePos.z == plateInfo::chunkLength) {
			getExtremeChangeCubeList(chunkPos + glm::dvec3{ 0, 0, 1 }, glm::dvec3{ cubePos.x, cubePos.y, 0 },						   glm::ivec2{ 5,0 },						   newCubeId);
		}

		if (debugInfo::LogSwitch) {
			if (chunkMap.find(chunkPos) == chunkMap.end()) {
				Log::printError(__FILE__, __LINE__, "变更方块所处区块为空");
			}
		}
	}

	//获取边界变更方块列表
	void getExtremeChangeCubeList(const glm::dvec3& extremeChunkPos, const glm::ivec3& extremeCubePos, const glm::ivec2& abutCubePos, const short& newCubeId) {
		if (debugInfo::LogSwitch) {
			if (chunkMap.find(extremeChunkPos) == chunkMap.end()) {
				Log::printError(__FILE__, __LINE__, "变更方块所处临界区块为空");
			}
		}

		Chunk& chunkNode = chunkMap[extremeChunkPos];
		short& oldCubeId = chunkNode.cubeArray[extremeCubePos.x][extremeCubePos.y][extremeCubePos.z];		//[0,33]

		chunkNode.isVisibleDataLock->lock();
		if (oldCubeId == cubeEnum::cubeId::air) {
			if (newCubeId != cubeEnum::cubeId::air) {
				chunkNode.isVisible++;
			}
		}
		else {
			if (newCubeId == cubeEnum::cubeId::air) {
				chunkNode.isVisible--;
			}
		}
		oldCubeId = newCubeId;
		chunkNode.isVisibleDataLock->unlock();

		changeCubeList[extremeChunkPos];		//压入一个空元素，让updateDeltaCube遍历时处理cubeAbutChangeList所在区块，在Chunk.updateChunkMeshData中遍历updateCubeNode.second时如果为空则直接跳过
		changeAbutCubeList[extremeChunkPos].insert(abutCubePos);

		//删除cubeDeltaSet中首位相同方块坐标的数据再插入，每次插入都删除首位相同方块坐标数据就能维持不重复
		auto& deltaSet = chunkNode.cubeDeltaSet;
		auto it = std::find_if(deltaSet.begin(), deltaSet.end(), [&](const storageStruct::cubeDeltaNode& n) {return n.data[0] == (short)extremeCubePos.x && n.data[1] == (short)extremeCubePos.y && n.data[2] == (short)extremeCubePos.z; });
		if (it != deltaSet.end()) { deltaSet.erase(it); }

		chunkNode.cubeDeltaSet.emplace((short)extremeCubePos.x, (short)extremeCubePos.y, (short)extremeCubePos.z, newCubeId);
		InitStorage.waitChunkList.insert(extremeChunkPos);

		if (debugInfo::LogSwitch) {
			if (chunkMap.find(extremeChunkPos) == chunkMap.end()) {
				Log::printError(__FILE__, __LINE__, "变更方块所处临界区块为空");
			}
		}
	}
};