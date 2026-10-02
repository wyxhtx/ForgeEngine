#pragma once



class Debug {
public:
	Debug() {
		if (debugInfo::LogSwitch) {
			Log::printTitle("————————Debug已创建");

			Log::delimiter();
		}
	}

	~Debug() {
		if (debugInfo::LogSwitch) {
			Log::print("Debug已清除");
		}
	}

public:
	//校验chunkMap长度
	void validateChunkMapSize(size_t chunkMapSize) {
		if (chunkMapSize != loopList::chunkMapLoop.size()) {
			Log::printError(__FILE__, __LINE__, "chunkMap长度错误：\t" + std::to_string(chunkMapSize));
		}
	}

	//校验isVisible越界
	void validateIsVisibleRange(const Chunk& chunk) {
		chunk.isVisibleDataLock->lock();
		if (chunk.isVisible < 0 || chunk.isVisible > plateInfo::cubeArrayVolume) {
			Log::printError(__FILE__, __LINE__, "isVisible超限：\t" + std::to_string(chunk.isVisible));
		}
		chunk.isVisibleDataLock->unlock();
	}

	//校验indexOffset越界
	void validateIndexOffsetRange(const Chunk& chunk) {
		if (chunk.indexOffset < 0) {
			Log::printError(__FILE__, __LINE__, "indexOffset超限：\t" + std::to_string(chunk.indexOffset));
		}
	}

	//校验地形层非空
	void validateTerrainLayerEmpty() {
		for (int layer = 0; layer < plateEnum::terrainLayer::enumCount; layer++) {
			if (vertexStruct::terrainVertex::mesh[layer].vertexs.size() == 0) {
				Log::printWarning(__FILE__, __LINE__, std::format("Layer_{}对应顶点数组为空", layer));
			}

			if (vertexStruct::terrainVertex::mesh[layer].indexs.size() == 0) {
				Log::printWarning(__FILE__, __LINE__, std::format("Layer_{}对应索引数组为空", layer));
			}
		}
	}

	//校验isVisible一致性，15ms
	void validateIsVisibleConsistency(const std::unordered_map<glm::dvec3, Chunk>& chunkMap) {
		while (lockInfo::chunkMapDataLock.exchange(true)) { std::this_thread::yield(); }

		std::mutex logMutex;		//确保输出不冲突

		std::for_each(std::execution::par, chunkMap.begin(), chunkMap.end(),
			[&](auto& node) {
				int tempId = 0;

				for (int x = 0; x < plateInfo::cubeArrayLength; x++) {
					for (int z = 0; z < plateInfo::cubeArrayLength; z++) {
						for (int y = 0; y < plateInfo::cubeArrayLength; y++) {
							if (node.second.cubeArray[x][y][z] != cubeEnum::cubeId::air) {
								tempId++;
							}
						}
					}
				}

				if (tempId != node.second.isVisible) {
					std::lock_guard<std::mutex> lock(logMutex);

					Log::printPos(node.first);
					std::cout << "\t" << "实际：" << "\t" << tempId << std::endl;
					std::cout << "\t" << "记录：" << "\t" << node.second.isVisible << std::endl;
					std::cout << "\t" << "变更：" << "\t" << node.second.cubeDeltaSet.size() << std::endl;
					Log::print("---");
				}
			}
		);

		lockInfo::chunkMapDataLock.store(false);
	}

	//校验本区块与邻区块，0或33处的方块是否对应，10ms
	void validateExtremeCube(const std::unordered_map<glm::dvec3, Chunk>& chunkMap) {
		std::mutex posMutex;
		std::unordered_map<glm::dvec3, std::unordered_set<gameDebugStruct::extremeCubeNode, gameDebugStruct::extremeCubeNode::Hash>> posList{};

		std::for_each(std::execution::par, chunkMap.begin(), chunkMap.end(),
			[&](const auto& node) {
				const glm::dvec3& chunkPos = node.second.chunkPos;
				const auto& cubeArray	   = node.second.cubeArray;

				const glm::dvec3 xPosA = chunkPos + glm::dvec3{ 1, 0, 0 };		//左
				const glm::dvec3 xPosB = chunkPos - glm::dvec3{ 1, 0, 0 };		//右
				const glm::dvec3 yPosA = chunkPos + glm::dvec3{ 0, 1, 0 };		//上
				const glm::dvec3 yPosB = chunkPos - glm::dvec3{ 0, 1, 0 };		//下
				const glm::dvec3 zPosA = chunkPos + glm::dvec3{ 0, 0, 1 };		//前
				const glm::dvec3 zPosB = chunkPos - glm::dvec3{ 0, 0, 1 };		//后

				auto itXA = chunkMap.find(xPosA);			//左
				auto itXB = chunkMap.find(xPosB);			//右
				auto itYA = chunkMap.find(yPosA);			//上
				auto itYB = chunkMap.find(yPosB);			//下
				auto itZA = chunkMap.find(zPosA);			//前
				auto itZB = chunkMap.find(zPosB);			//后

				const bool xA = itXA != chunkMap.end();		//左
				const bool xB = itXB != chunkMap.end();		//右
				const bool yA = itYA != chunkMap.end();		//上
				const bool yB = itYB != chunkMap.end();		//下
				const bool zA = itZA != chunkMap.end();		//前
				const bool zB = itZB != chunkMap.end();		//后

				//33存临界1，0存临界32
				node.second.cubeArrayDataLock->lock();
				for (int a = 1; a <= plateInfo::chunkLength; a++) {
					for (int b = 1; b <= plateInfo::chunkLength; b++) {
						//左
						if (xA && cubeArray[plateInfo::lengthIncrement][a][b] != itXA->second.cubeArray[1][a][b]) {
							std::lock_guard<std::mutex> lock(posMutex);
							posList[chunkPos].insert(gameDebugStruct::extremeCubeNode{ glm::dvec3{plateInfo::lengthIncrement, a, b} + node.second.logicalWorldPos - glm::dvec3{1},	cubeArray[plateInfo::lengthIncrement][a][b], itXA->second.cubeArray[1][a][b] });
						}

						//右
						if (xB && cubeArray[0][a][b] != itXB->second.cubeArray[plateInfo::chunkLength][a][b]) {
							std::lock_guard<std::mutex> lock(posMutex);
							posList[chunkPos].insert(gameDebugStruct::extremeCubeNode{glm::dvec3{0, a, b} + node.second.logicalWorldPos - glm::dvec3{1},							cubeArray[0][a][b],						  itXB->second.cubeArray[plateInfo::chunkLength][a][b] });
						}

						//上
						if (yA && cubeArray[a][plateInfo::lengthIncrement][b] != itYA->second.cubeArray[a][1][b]) {
							std::lock_guard<std::mutex> lock(posMutex);
							posList[chunkPos].insert(gameDebugStruct::extremeCubeNode{glm::dvec3{a, plateInfo::lengthIncrement, b} + node.second.logicalWorldPos - glm::dvec3{1},	cubeArray[a][plateInfo::lengthIncrement][b], itYA->second.cubeArray[a][1][b] });
						}

						//下
						if (yB && cubeArray[a][0][b] != itYB->second.cubeArray[a][plateInfo::chunkLength][b]) {
							std::lock_guard<std::mutex> lock(posMutex);
							posList[chunkPos].insert(gameDebugStruct::extremeCubeNode{glm::dvec3{a, 0, b} + node.second.logicalWorldPos - glm::dvec3{1},							cubeArray[a][0][b],						  itYB->second.cubeArray[a][plateInfo::chunkLength][b] });
						}

						//前
						if (zA && cubeArray[a][b][plateInfo::lengthIncrement] != itZA->second.cubeArray[a][b][1]) {
							std::lock_guard<std::mutex> lock(posMutex);
							posList[chunkPos].insert(gameDebugStruct::extremeCubeNode{glm::dvec3{a, b, plateInfo::lengthIncrement} + node.second.logicalWorldPos - glm::dvec3{1},	cubeArray[a][b][plateInfo::lengthIncrement], itZA->second.cubeArray[a][b][1] });
						}

						//后
						if (zB && cubeArray[a][b][0] != itZB->second.cubeArray[a][b][plateInfo::chunkLength]) {
							std::lock_guard<std::mutex> lock(posMutex);
							posList[chunkPos].insert(gameDebugStruct::extremeCubeNode{glm::dvec3{a, b, 0} + node.second.logicalWorldPos - glm::dvec3{1},							cubeArray[a][b][0],						  itZB->second.cubeArray[a][b][plateInfo::chunkLength] });
						}
					}
				}
				node.second.cubeArrayDataLock->unlock();
			}
		);

		if (posList.size() != 0) {
			Log::print(posList.size());

			for (auto& node : posList) {
				if (node.second.size() != 0) {
					Log::print("");

					std::cout << "\t" << "chunk:" << "\t" << node.first.x << "\t" << node.first.y << "\t" << node.first.z << "\t" << "count:" << "\t" << node.second.size() << std::endl;

					for (auto& set : node.second) {
						std::cout << "\t" << "cube:" << "\t" << set.pos.x << "\t" << set.pos.y << "\t" << set.pos.z << "\t" << "oldId:" << "\t" << set.oldId << "\t" << "newId:" << "\t" << set.newId << std::endl;
					}
				}
			}

			Log::print("------");
		}
	}
};