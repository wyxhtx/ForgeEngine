#pragma once



//唯一类,生成世界地形
class Plate {
public:
	glm::dvec3 originPos;																	//板块原点



	Plate() {
		timeStruct::timeNode time{};

		createPlate();

		if (debugInfo::LogSwitch) {
			Log::printTitle("板块创建信息");
			Log::print("创建用时:\t" + time.end());
			Log::print("板块面数:\t" + std::to_string(todoInfo::visibleFaceCount));
			Log::print("优化面数:\t" + std::to_string(todoInfo::visibleOptimization));
			Log::print("优化率  :\t" + std::to_string(todoInfo::visibleFaceCount / (float)todoInfo::visibleOptimization));

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
	//todo日后增加存储非自然区块(即玩家放置过方块)的map，放置方块少的区块把里面的方块放到一个列表里，通过参与地形生成来生成
	std::unordered_map<glm::dvec3, Chunk> chunkMap;											//存储当前视距内区块
	std::forward_list<plateStruct::plateMeshSignNode> plateMeshSignList;					//标识区块顶点数组和索引数组的大小，为裁剪区块截层网格服务

	Noise InitNoise{ 0 };
	Loop InitLoop{};
	Storage InitStorage{};

	//结构管理器，只管理小于等于32并且无空气的结构，超出32会产生很多问题，比如区块存储加载，方块更新后的网格更新
	std::unordered_map<glm::dvec3, std::unordered_set<glm::ivec4>> pendingStructManager;	//未加载结构管理器，[0,33]
	std::unordered_map<glm::dvec3, std::unordered_set<glm::ivec4>> activeStructManager;		//已加载结构管理器，[0,33]，将pendingStructManager压入activeStructManager时必须硬度检测



	//创建板块
	void createPlate() {
		std::vector<glm::dvec3> updateChunkLoop;
		glm::dvec3 chunkPos = glm::floor(playerInfo::initSpawnPos / (double)plateInfo::chunkLength);
		originPos = glm::floor(playerInfo::initSpawnPos / (double)plateInfo::originShiftThreshold) * (double)plateInfo::originShiftThreshold;



		//生成地形
		for (glm::dvec3 updatePos : plateList::chunkLoop) {
			updatePos += chunkPos;

			//创建区块
			chunkMap[updatePos].InitChunk(originPos, updatePos);
			createChunk(chunkMap[updatePos]);

			updateChunkLoop.push_back(updatePos);
		}

		//处理地形结构管理器
		for (auto it = pendingStructManager.begin(); it != pendingStructManager.end(); ) {
			if (chunkMap.find(it->first) != chunkMap.end()) {
				for (const glm::ivec4& tempPos : it->second) {
					short& cubeId = chunkMap[it->first].cubeArray[tempPos.x][tempPos.y][tempPos.z];

					if (cubeId == cubeEnum::cubeId::air) {
						chunkMap[it->first].isVisible++;
					}
					else if (cubeList::property[tempPos.w].hardness <= cubeList::property[cubeId].hardness) {
						continue;
					}

					cubeId = tempPos.w;
					activeStructManager[it->first].insert(tempPos);
				}

				it = pendingStructManager.erase(it);
			}
			else {
				++it;
			}
		}

		//生成并拼接区块网格
		for (const glm::dvec3& updatePos : updateChunkLoop) {
			//读取区块文件
			InitStorage.readChunk(chunkMap[updatePos]);

			if (chunkMap[updatePos].isVisible > 0 && chunkMap[updatePos].isVisible < plateInfo::filled) {
				chunkMap[updatePos].createChunkMeshData();		//生成网格

				if (chunkMap[updatePos].indexOffset) {			//拼接可见区块网格
					plateMeshSignList.push_front(plateStruct::plateMeshSignNode{ updatePos, (int)chunkMap[updatePos].chunkVertexs.size(), (int)chunkMap[updatePos].chunkIndexs.size() });
					joinMesh(originPos, updatePos);
				}
			}
			else if (debugInfo::LogSwitch) {
				if (chunkMap[updatePos].isVisible < 0 || chunkMap[updatePos].isVisible > plateInfo::filled) {
					Log::error(__FILE__, __LINE__, "区块isVisible超限");
					std::cout << chunkMap[updatePos].isVisible << "\t" << updatePos.x << "\t" << updatePos.y << "\t" << updatePos.z << std::endl;
				}
			}
		}

		//地形网格非空检测
		if (debugInfo::LogSwitch) {
			if (terrainVertex::vertexs.opaqueTerrain.size() == 0) {
				Log::error(__FILE__, __LINE__, "terrainVertex::vertexs.opaqueTerrain未存入数据");
			}

			if (terrainVertex::indexs.opaqueTerrain.size() == 0) {
				Log::error(__FILE__, __LINE__, "terrainVertex::indexs.opaqueTerrain未存入数据");
			}
		}
	}



	//创建区块，生成方块数组数据
	void createChunk(Chunk& chunk) {
		static glm::dvec3 localPos;
		static double length;
		static std::vector<glm::ivec3> structList;

		//区块数组实际范围在0~33，显示范围在1~32，所以地形生成时需要偏移到0~31
		glm::dvec3 worldPos = chunk.logicalWorldPos - glm::dvec3{ 1 };



		//生成区块地形，获取结构队列，[0,33]
		for (int x = 0; x < plateInfo::chunkMapLength; x++) {
			localPos.x = worldPos.x + x;

			for (int z = 0; z < plateInfo::chunkMapLength; z++) {
				localPos.z = worldPos.z + z;
				length = std::floor(InitNoise.terrain.GetNoise(localPos.x + 0.1, localPos.z + 0.1) * terrainInfo::undulation + terrainInfo::translation);

				for (int y = 0; y < plateInfo::chunkMapLength; y++) {
					localPos.y = worldPos.y + y;

					//高于地平线生成空气
					if (localPos.y < plateInfo::groundPlane) {
						if (localPos.y > length) {
							chunk.cubeArray[x][y][z] = cubeEnum::cubeId::air;
						}
						else if (localPos.y < length) {
							chunk.isVisible++;

							chunk.cubeArray[x][y][z] = cubeEnum::cubeId::stoneSquared;
						}
						else {
							chunk.isVisible++;

							//todo 后续做生态时外面再套一层细胞噪音
							if (std::floor(InitNoise.tree.GetNoise(localPos.x, localPos.z) * noiseList::tree::oakPut.undulation + noiseList::tree::oakPut.translation) > noiseList::tree::oakPut.density) {
								//只压入1~32内的，避免边缘重复
								if (x > 0 && x < plateInfo::lengthIncrement && y > 0 && y < plateInfo::lengthIncrement && z > 0 && z < plateInfo::lengthIncrement) {
									structList.push_back(glm::ivec3{ x,y + 1,z });
								}

								chunk.cubeArray[x][y][z] = cubeEnum::cubeId::oakLog;
							}
							else {
								chunk.cubeArray[x][y][z] = cubeEnum::cubeId::grass;
							}
						}
					}
					else {
						chunk.cubeArray[x][y][z] = cubeEnum::cubeId::air;
					}
				}
			}
		}

		//生成地形结构管理器
		if (!structList.empty()) {
			const static float lengthF = plateInfo::chunkLength;

			glm::dvec3 chunkPos;
			glm::ivec4 tempPos;

			for (const glm::ivec3& structPos : structList) {
				for (const glm::ivec4& pos : terrainList::treeLoop) {
					tempPos = glm::ivec4{ structPos,0 } + pos;

					if (tempPos.x > 0 && tempPos.x < plateInfo::lengthIncrement && tempPos.y > 0 && tempPos.y < plateInfo::lengthIncrement && tempPos.z > 0 && tempPos.z < plateInfo::lengthIncrement) {
						//[1,32]
						short& cubeId = chunk.cubeArray[tempPos.x][tempPos.y][tempPos.z];

						if (cubeId == cubeEnum::cubeId::air) {
							chunk.isVisible++;
						}
						else if (cubeList::property[tempPos.w].hardness <= cubeList::property[cubeId].hardness) {
							continue;
						}

						chunkPos = chunk.chunkPos;
						cubeId = tempPos.w;

						//[0,33]
						if (tempPos.x == plateInfo::chunkLength) {
							chunkPos.x++;
							tempPos.x = 0;
							pendingStructManager[chunkPos].insert(tempPos);
							tempPos.x = plateInfo::chunkLength;
							chunkPos.x--;
						}
						else if (tempPos.x == 1) {
							chunkPos.x--;
							tempPos.x = plateInfo::lengthIncrement;
							pendingStructManager[chunkPos].insert(tempPos);
							tempPos.x = 1;
							chunkPos.x++;
						}

						if (tempPos.y == plateInfo::chunkLength) {
							chunkPos.y++;
							tempPos.y = 0;
							pendingStructManager[chunkPos].insert(tempPos);
							tempPos.y = plateInfo::chunkLength;
							chunkPos.y--;
						}
						else if (tempPos.y == 1) {
							chunkPos.y--;
							tempPos.y = plateInfo::lengthIncrement;
							pendingStructManager[chunkPos].insert(tempPos);
							tempPos.y = 1;
							chunkPos.y++;
						}

						if (tempPos.z == plateInfo::chunkLength) {
							chunkPos.z++;
							tempPos.z = 0;
							pendingStructManager[chunkPos].insert(tempPos);
							tempPos.z = plateInfo::chunkLength;
							chunkPos.z--;
						}
						else if (tempPos.z == 1) {
							chunkPos.z--;
							tempPos.z = plateInfo::lengthIncrement;
							pendingStructManager[chunkPos].insert(tempPos);
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
						pendingStructManager[chunkPos].insert(tempPos);		//glm::ivec4一样的不用管，不一样的不会覆盖存储

						//[0,33]
						if (tempPos.x == plateInfo::chunkLength) {
							chunkPos.x++;
							tempPos.x = 0;
							pendingStructManager[chunkPos].insert(tempPos);
							tempPos.x = plateInfo::chunkLength;
							chunkPos.x--;
						}
						else if (tempPos.x == 1) {
							chunkPos.x--;
							tempPos.x = plateInfo::lengthIncrement;
							pendingStructManager[chunkPos].insert(tempPos);
							tempPos.x = 1;
							chunkPos.x++;
						}

						if (tempPos.y == plateInfo::chunkLength) {
							chunkPos.y++;
							tempPos.y = 0;
							pendingStructManager[chunkPos].insert(tempPos);
							tempPos.y = plateInfo::chunkLength;
							chunkPos.y--;
						}
						else if (tempPos.y == 1) {
							chunkPos.y--;
							tempPos.y = plateInfo::lengthIncrement;
							pendingStructManager[chunkPos].insert(tempPos);
							tempPos.y = 1;
							chunkPos.y++;
						}

						if (tempPos.z == plateInfo::chunkLength) {
							chunkPos.z++;
							tempPos.z = 0;
							pendingStructManager[chunkPos].insert(tempPos);
							tempPos.z = plateInfo::chunkLength;
							chunkPos.z--;
						}
						else if (tempPos.z == 1) {
							chunkPos.z--;
							tempPos.z = plateInfo::lengthIncrement;
							pendingStructManager[chunkPos].insert(tempPos);
							tempPos.z = 1;
							chunkPos.z++;
						}
					}
				}
			}

			structList.clear();
		}
	}



	//将区块网格拼接成板块网格
	//自动内存分配时函数耗时1.8ms，平时函数耗时0.18ms
	void joinMesh(const glm::dvec3 originOffset, const glm::dvec3 chunkPos) {
		//等同(uint32_t)(terrainVertex::indexs.opaqueTerrain.size() / 1.5);
		uint32_t offset = (uint32_t)terrainVertex::vertexs.opaqueTerrain.size();

		//配置旧区块
		if (chunkMap[chunkPos].originOffset != originOffset) {
			glm::vec3 tempPos = chunkMap[chunkPos].originOffset - originOffset;

			for (terrainVertex::vertexNode& node : chunkMap[chunkPos].chunkVertexs) {
				node.position += tempPos;
			}

			chunkMap[chunkPos].InitChunk(originOffset, chunkPos);
		}

		terrainVertex::vertexs.opaqueTerrain.insert(terrainVertex::vertexs.opaqueTerrain.end(), chunkMap[chunkPos].chunkVertexs.begin(), chunkMap[chunkPos].chunkVertexs.end());

		//拼接索引
		std::vector<uint32_t> tempIndexs = chunkMap[chunkPos].chunkIndexs;
		for (uint32_t& i : tempIndexs) {
			i += offset;
		}

		terrainVertex::indexs.opaqueTerrain.insert(terrainVertex::indexs.opaqueTerrain.end(), tempIndexs.begin(), tempIndexs.end());
	}

public:
	std::unordered_map<glm::dvec3, std::unordered_set<glm::ivec3>> cubeChangeList;		//方块变更列表，存储变更方块坐标与对应区块坐标，[0,31]
	bool internalUpdateLock = false;													//板块更新内部锁
	bool externalUpdateLock = false;													//板块更新外部锁



	//更新板块
	void updatePlate(glm::dvec3 playerPos) {
		std::unordered_map<glm::dvec3, Chunk> tempChunkMap;								//临时存储区块数据，主要存当前视距内未加载区块，次要存因加载链结构而更新的区块
		std::unordered_set<glm::dvec3> existChunkLoop;									//当前视距内已加载区块列表 
		std::vector<glm::dvec3> updateChunkLoop;										//当前视距内未加载区块列表  

		glm::dvec3 playerChunkPos = glm::floor(playerPos / (double)plateInfo::chunkLength);
		glm::dvec3 originShiftPos = glm::abs(playerInfo::logicOriginPos - playerPos);
		glm::dvec3 tempOriginPos = originPos;



		//重设逻辑原点
		if (originShiftPos.x > (double)plateInfo::originShiftThreshold || originShiftPos.y > (double)plateInfo::originShiftThreshold || originShiftPos.z > (double)plateInfo::originShiftThreshold) {
			tempOriginPos = glm::floor(playerPos / (double)plateInfo::originShiftThreshold) * (double)plateInfo::originShiftThreshold;
		}

		//获取当前视距内已加载区块列表existChunkList
		for (glm::dvec3 updatePos : plateList::chunkLoop) {
			updatePos += playerChunkPos;

			if (chunkMap.find(updatePos) != chunkMap.end()) {
				existChunkLoop.insert(updatePos);
			}
			else {
				updateChunkLoop.push_back(updatePos);
			}
		}

		//67ms(r=8),创建当前视距内未加载的区块
		for (glm::dvec3& updatePos : updateChunkLoop) {
			tempChunkMap[updatePos].InitChunk(tempOriginPos, updatePos);
			createChunk(tempChunkMap[updatePos]);
		}



		while (externalUpdateLock);
		internalUpdateLock = true;

		//变更板块原点
		if (tempOriginPos != originPos) {
			originPos = tempOriginPos;
		}

		//为避免结构加载遗漏，在tempChunkMap和activeStructManager的交集中，将activeStructManager结构数据压入对应pendingStructManager
		for (const auto& node : activeStructManager) {
			if (tempChunkMap.find(node.first) != tempChunkMap.end()) {
				//如果activeStructManager中无pendingStructManager，则直接压入
				if (pendingStructManager.find(node.first) == pendingStructManager.end()) {
					pendingStructManager[node.first] = node.second;
				}
				//如果activeStructManager中有pendingStructManager，且二者所存结构不等，则遍历压入
				else if (node.second != pendingStructManager[node.first]) {
					for (const glm::ivec4& tempPos : node.second) {
						//此处无需硬度检测，在压入结构时比较硬度
						pendingStructManager[node.first].insert(tempPos);
					}
				}
			}
		}

		//1.5ms(r=8)，加载链结构，[0,33]
		for (auto it = pendingStructManager.begin(); it != pendingStructManager.end(); ) {
			//如果pendingStructManager在updateChunkLoop中，则直接加载链结构
			if (tempChunkMap.find(it->first) != tempChunkMap.end()) {
				for (const glm::ivec4& tempPos : it->second) {
					short& cubeId = tempChunkMap[it->first].cubeArray[tempPos.x][tempPos.y][tempPos.z];

					if (cubeId == cubeEnum::cubeId::air) {
						tempChunkMap[it->first].isVisible++;
					}
					else if (cubeList::property[tempPos.w].hardness <= cubeList::property[cubeId].hardness) {
						continue;
					}

					cubeId = tempPos.w;
					//无论activeStructManager中有无pendingStructManager，为避免同一方块坐标多个方块id，所有元素都得参与硬度检测后压入
					activeStructManager[it->first].insert(tempPos);
				}

				it = pendingStructManager.erase(it);
			}
			//如果pendingStructManager在existChunkLoop中，则复制chunkMap数据到tempChunkMap后加载链结构
			else if (existChunkLoop.find(it->first) != existChunkLoop.end()) {
				tempChunkMap[it->first].InitChunk(tempOriginPos, it->first);
				tempChunkMap[it->first].isVisible = chunkMap[it->first].isVisible;
				std::memcpy(tempChunkMap[it->first].cubeArray, chunkMap[it->first].cubeArray, sizeof(chunkMap[it->first].cubeArray));

				for (const glm::ivec4& tempPos : it->second) {
					short& cubeId = tempChunkMap[it->first].cubeArray[tempPos.x][tempPos.y][tempPos.z];

					if (cubeId == cubeEnum::cubeId::air) {
						tempChunkMap[it->first].isVisible++;
					}
					else if (cubeList::property[tempPos.w].hardness <= cubeList::property[cubeId].hardness) {
						continue;
					}

					cubeId = tempPos.w;
					activeStructManager[it->first].insert(tempPos);
				}

				existChunkLoop.erase(it->first);
				updateChunkLoop.push_back(it->first);

				it = pendingStructManager.erase(it);
			}
			else {
				++it;
			}
		}

		//清理r+2外的结构
		for (auto it = activeStructManager.begin(); it != activeStructManager.end(); ) {
			if (plateList::structClearLoop.find(glm::dvec3{ it->first - playerChunkPos }) == plateList::structClearLoop.end()) {
				it = activeStructManager.erase(it);
			}
			else {
				it++;
			}
		}

		for (auto it = pendingStructManager.begin(); it != pendingStructManager.end(); ) {
			if (plateList::structClearLoop.find(glm::dvec3{ it->first - playerChunkPos }) == plateList::structClearLoop.end()) {
				it = pendingStructManager.erase(it);
			}
			else {
				it++;
			}
		}



		//todo树叶列入透明队列后或许能缓和
		//todo原点迁移破坏方块时，因为下述函数耗时太长延后更新网格
		//卸载视距外区块网格
		//原点未迁移，直接删除视距外已加载区块网格(r=8时35ms)
		if (originPos == playerInfo::logicOriginPos) {
			for (std::unordered_map<glm::dvec3, Chunk>::iterator it = chunkMap.begin(); it != chunkMap.end(); ) {
				const glm::dvec3& updatePos = it->first;

				if (existChunkLoop.find(updatePos) == existChunkLoop.end()) {
					for (std::forward_list<plateStruct::plateMeshSignNode>::iterator node = plateMeshSignList.begin(), prev = plateMeshSignList.before_begin(); node != plateMeshSignList.end(); prev = node, ++node) {
						if (node->chunkPos == updatePos) {
							std::forward_list<plateStruct::plateMeshSignNode>::iterator p = std::next(node);
							int vertexsSize = 0;

							while (p != plateMeshSignList.end()) {
								vertexsSize += p->vertexsSize;
								++p;
							}

							terrainVertex::vertexs.opaqueTerrain.erase(terrainVertex::vertexs.opaqueTerrain.begin() + vertexsSize, terrainVertex::vertexs.opaqueTerrain.begin() + vertexsSize + node->vertexsSize);
							terrainVertex::indexs.opaqueTerrain.erase(terrainVertex::indexs.opaqueTerrain.end() - node->indexsSize, terrainVertex::indexs.opaqueTerrain.end());

							plateMeshSignList.erase_after(prev);
							break;
						}
					}

					//保存区块文件
					if (InitStorage.waitChunkList.find(updatePos) != InitStorage.waitChunkList.end()) {
						InitStorage.waitChunkList.erase(updatePos);
						InitStorage.saveChunk(chunkMap[updatePos]);
					}

					it = chunkMap.erase(it);
				}
				else {
					++it;
				}
			}
		}
		//原点迁移，则全删了重拼(r=8时75ms)
		else {
			terrainVertex::vertexs.opaqueTerrain.clear();
			terrainVertex::indexs.opaqueTerrain.clear();
			plateMeshSignList.clear();

			for (std::unordered_map<glm::dvec3, Chunk>::iterator it = chunkMap.begin(); it != chunkMap.end(); ) {
				const glm::dvec3& updatePos = it->first;

				if (existChunkLoop.find(updatePos) == existChunkLoop.end()) {
					//保存区块文件
					if (InitStorage.waitChunkList.find(updatePos) != InitStorage.waitChunkList.end()) {
						InitStorage.waitChunkList.erase(updatePos);
						InitStorage.saveChunk(chunkMap[updatePos]);
					}

					it = chunkMap.erase(it);
				}
				else {
					//重拼plateMeshSignList
					if (chunkMap[updatePos].indexOffset) {
						plateMeshSignList.push_front(plateStruct::plateMeshSignNode{ updatePos, (int)chunkMap[updatePos].chunkVertexs.size(), (int)chunkMap[updatePos].chunkIndexs.size() });
						joinMesh(originPos, updatePos);
					}

					++it;
				}
			}
		}

		internalUpdateLock = false;



		InitStorage.updateConfig(playerChunkPos);

		//120ms(r=8)，生成区块网格
		for (const glm::dvec3& updatePos : updateChunkLoop) {
			//读取区块文件
			InitStorage.readChunk(tempChunkMap[updatePos]);

			if (tempChunkMap[updatePos].isVisible > 0 && tempChunkMap[updatePos].isVisible < plateInfo::filled) {
				tempChunkMap[updatePos].createChunkMeshData();		//生成网格
			}
			else if (debugInfo::LogSwitch) {
				if (tempChunkMap[updatePos].isVisible < 0 || tempChunkMap[updatePos].isVisible > plateInfo::filled) {
					Log::error(__FILE__, __LINE__, "区块isVisible超限");
					std::cout << tempChunkMap[updatePos].isVisible << "\t" << updatePos.x << "\t" << updatePos.y << "\t" << updatePos.z << std::endl;
				}
			}
		}



		while (externalUpdateLock);
		internalUpdateLock = true;

		//25ms(r=8)，将tempChunkMap装进chunkMap并拼接区块网格
		for (const glm::dvec3& updatePos : updateChunkLoop) {
			chunkMap[updatePos] = tempChunkMap[updatePos];

			if (chunkMap[updatePos].indexOffset) {
				plateMeshSignList.push_front(plateStruct::plateMeshSignNode{ updatePos, (int)chunkMap[updatePos].chunkVertexs.size(), (int)chunkMap[updatePos].chunkIndexs.size() });
				joinMesh(originPos, updatePos);
			}
		}

		internalUpdateLock = false;

		//地形网格非空检测
		if (debugInfo::LogSwitch) {
			if (terrainVertex::vertexs.opaqueTerrain.size() == 0) {
				Log::error(__FILE__, __LINE__, "terrainVertex::vertexs.opaqueTerrain未存入数据");
			}

			if (terrainVertex::indexs.opaqueTerrain.size() == 0) {
				Log::error(__FILE__, __LINE__, "terrainVertex::indexs.opaqueTerrain未存入数据");
			}
		}
	}

	//todo 选择框有误差，等写完十字标时修正
	//todo 优化算法
	//选择框算法，参考Bresenham算法
	playerStruct::selectBoxNode getSelectBox(const playerStruct::CameraNode& camera) {
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

		//如果起始坐标非空，则直接返回起始坐标
		if (chunkMap.find(chunkPos) != chunkMap.end()) {
			//将d世界坐标系偏移到方块坐标系，cubePos范围为[1,32]
			cubePos = glm::ivec3{ initialPos - chunkMap[chunkPos].logicalWorldPos } + glm::ivec3{ 1 };

			if (chunkMap[chunkPos].cubeArray[cubePos.x][cubePos.y][cubePos.z] != cubeEnum::cubeId::air) {
				if (playerInfo::mouseButtonLeft) {
					createUpdateCubeList(chunkPos, cubePos);
				}

				//返回i世界坐标系
				return playerStruct::selectBoxNode{ initialPos - playerInfo::logicOriginPos, 1 };
				//返回玩家所在区块数组真实坐标
				//return playerStruct::selectBoxNode{ cubePos, 1 };
			}
		}

		displacement = camera.front * playerInfo::interactionRange * 4.0f;
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
				cubePos = glm::ivec3{ initialPos - chunkMap[chunkPos].logicalWorldPos } + glm::ivec3{ 1 };

				if (chunkMap[chunkPos].cubeArray[cubePos.x][cubePos.y][cubePos.z] != cubeEnum::cubeId::air) {
					if (playerInfo::mouseButtonLeft) {
						createUpdateCubeList(chunkPos, cubePos);
					}

					//返回i世界坐标系
					return playerStruct::selectBoxNode{ initialPos - playerInfo::logicOriginPos, 1 };
					//返回玩家所在区块数组真实坐标
					//return playerStruct::selectBoxNode{ cubePos, 1 };
				}
			}

			remainingSteps--;
		}

		return playerStruct::selectBoxNode{ glm::vec3{0}, 0 };
	}

	//放置或破坏方块后，更新板块被改变部分的网格
	void updatePlatePartMesh() {
		//因为forward_list的性质，处理近期处理过的区块更快，平均首次处理0.22，平均近期处理0.06
		for (const auto& updateCubeNode : cubeChangeList) {
			glm::dvec3 chunkPos = updateCubeNode.first;

			if (chunkMap.find(chunkPos) != chunkMap.end()) {
				//清空所选区块网格
				for (std::forward_list<plateStruct::plateMeshSignNode>::iterator node = plateMeshSignList.begin(), prev = plateMeshSignList.before_begin(); node != plateMeshSignList.end(); prev = node, ++node) {
					if (node->chunkPos == chunkPos) {
						std::forward_list<plateStruct::plateMeshSignNode>::iterator p = std::next(node);
						int vertexsSize = 0;

						while (p != plateMeshSignList.end()) {
							vertexsSize += p->vertexsSize;
							++p;
						}

						terrainVertex::vertexs.opaqueTerrain.erase(terrainVertex::vertexs.opaqueTerrain.begin() + vertexsSize, terrainVertex::vertexs.opaqueTerrain.begin() + vertexsSize + node->vertexsSize);
						terrainVertex::indexs.opaqueTerrain.erase(terrainVertex::indexs.opaqueTerrain.end() - node->indexsSize, terrainVertex::indexs.opaqueTerrain.end());

						plateMeshSignList.erase_after(prev);
						break;
					}
				}

				chunkMap[chunkPos].updateChunkMeshData(updateCubeNode.second, cubeAbutChangeList[chunkPos]);
				plateMeshSignList.push_front(plateStruct::plateMeshSignNode{ chunkPos, (int)chunkMap[chunkPos].chunkVertexs.size(), (int)chunkMap[chunkPos].chunkIndexs.size() });
				joinMesh(originPos, chunkPos);
			}
			/*
			//已证实存在下述情况
			else if (debugInfo::LogSwitch) {
				Log::error(__FILE__, __LINE__, "变更方块所处区块为空");
			}*/
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
							Log::error(__FILE__, __LINE__, "x33为空");
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
							Log::error(__FILE__, __LINE__, "x0为空");
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
							Log::error(__FILE__, __LINE__, "y33为空");
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
							Log::error(__FILE__, __LINE__, "y0为空");
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
							Log::error(__FILE__, __LINE__, "z33为空");
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
							Log::error(__FILE__, __LINE__, "z0为空");
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