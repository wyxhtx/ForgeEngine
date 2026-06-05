#pragma once



class Storage {
public:
	std::unordered_set<glm::dvec3> waitChunkList;										//待存储区块集合，暂存需要存储的区块坐标，在区块卸载时保存
	std::unique_ptr<std::mutex> waitChunkListDataLock = std::make_unique<std::mutex>();	//waitChunkList数据锁

	//无论全存还是变存，方块数组使用范围都是[0,33]
	std::unordered_map<glm::dvec3, std::unordered_set<glm::dvec3>> fullChunkList;		//全存法区块存储集合，存储整个区块
	std::unordered_map<glm::dvec3, std::unordered_set<glm::dvec3>> deltaChunkList;		//变存法区块存储集合，存储玩家变更方块的数组坐标与id



	Storage() {
		readConfig(regionOriginPos);

		if (debugInfo::LogSwitch) {
			Log::printTitle("————————Storage已创建");

			Log::delimiter();
		}
	}

	~Storage() {
		saveConfig();

		if (debugInfo::LogSwitch) {
			Log::print("Storage已清除");
		}
	}

private:
	//地区原点坐标
	glm::dvec3 regionOriginPos = glm::floor((playerInfo::initSpawnPos / (double)plateInfo::chunkLength) / (double)storageInfo::regionLength);



	//保存地区配置
	void saveConfig() {
		for (const auto& [regionPos, chunkSet] : fullChunkList) {
			std::filesystem::path regionPath = std::filesystem::path{ fileInfo::storage::fullFile } / std::format("{}_{}_{}", regionPos.x, regionPos.y, regionPos.z) / "regionConfig.dat";
			std::filesystem::create_directories(regionPath.parent_path());    //如果当前文件父路径不存在则创建，如果存在则不操作
			std::ofstream oRegionConfig{ regionPath, std::ios::binary };
			size_t chunkSetSize = chunkSet.size();

			oRegionConfig.write(reinterpret_cast<char*>(&chunkSetSize), sizeof(size_t));

			for (const glm::dvec3& chunkPos : chunkSet) {
				oRegionConfig.write(reinterpret_cast<const char*>(&chunkPos), sizeof(glm::dvec3));
			}

			oRegionConfig.close();
		}

		for (const auto& [regionPos, chunkSet] : deltaChunkList) {
			std::filesystem::path regionPath = std::filesystem::path{ fileInfo::storage::deltaFile } / std::format("{}_{}_{}", regionPos.x, regionPos.y, regionPos.z) / "regionConfig.dat";
			std::filesystem::create_directories(regionPath.parent_path());    //如果当前文件父路径不存在则创建，如果存在则不操作
			std::ofstream oRegionConfig{ regionPath, std::ios::binary };
			size_t chunkSetSize = chunkSet.size();

			oRegionConfig.write(reinterpret_cast<char*>(&chunkSetSize), sizeof(size_t));

			for (const glm::dvec3& chunkPos : chunkSet) {
				oRegionConfig.write(reinterpret_cast<const char*>(&chunkPos), sizeof(glm::dvec3));
			}

			oRegionConfig.close();
		}
	}

	//读取地区配置
	void readConfig(glm::dvec3 regionOriginPos) {
		for (glm::dvec3 regionPos : loopList::regionLoop) {
			regionPos += regionOriginPos;
			std::filesystem::path regionPath = std::filesystem::path{ fileInfo::storage::fullFile } / std::format("{}_{}_{}", regionPos.x, regionPos.y, regionPos.z) / "regionConfig.dat";

			if (std::filesystem::exists(regionPath)) {
				if (fullChunkList.find(regionPos) == fullChunkList.end()) {
					std::ifstream iRegionConfig{ regionPath, std::ios::binary };
					size_t chunkSetSize = 0;
					glm::dvec3 chunkPos{};

					iRegionConfig.read(reinterpret_cast<char*>(&chunkSetSize), sizeof(size_t));

					for (size_t i = 0; i < chunkSetSize; i++) {
						iRegionConfig.read(reinterpret_cast<char*>(&chunkPos), sizeof(glm::dvec3));
						fullChunkList[regionPos].insert(chunkPos);
					}

					iRegionConfig.close();
				}
			}

			regionPath = std::filesystem::path{ fileInfo::storage::deltaFile } / std::format("{}_{}_{}", regionPos.x, regionPos.y, regionPos.z) / "regionConfig.dat";

			if (std::filesystem::exists(regionPath)) {
				if (deltaChunkList.find(regionPos) == deltaChunkList.end()) {
					std::ifstream iRegionConfig{ regionPath, std::ios::binary };
					size_t chunkSetSize = 0;
					glm::dvec3 chunkPos{};

					iRegionConfig.read(reinterpret_cast<char*>(&chunkSetSize), sizeof(size_t));

					for (size_t i = 0; i < chunkSetSize; i++) {
						iRegionConfig.read(reinterpret_cast<char*>(&chunkPos), sizeof(glm::dvec3));
						deltaChunkList[regionPos].insert(chunkPos);
					}

					iRegionConfig.close();
				}
			}
		}
	}



	//全存法保存区块文件
	void saveFullChunk(glm::dvec3 regionPos, Chunk& chunk) {
		std::filesystem::path chunkPath = std::filesystem::path{ fileInfo::storage::fullFile } / std::format("{}_{}_{}", regionPos.x, regionPos.y, regionPos.z) / std::format("{}_{}_{}.dat", chunk.chunkPos.x, chunk.chunkPos.y, chunk.chunkPos.z);
		std::filesystem::create_directories(chunkPath.parent_path());	//如果当前文件父路径不存在则创建，如果存在则不操作
		std::ofstream oChunkFile{ chunkPath, std::ios::binary };

		chunk.cubeArrayDataLock->lock();
		oChunkFile.write(reinterpret_cast<char*>(chunk.cubeArray), storageInfo::cubeArraySize);
		chunk.cubeArrayDataLock->unlock();

		//size_t size = chunk.cubeDeltaSet.size();
		//oChunkFile.write(reinterpret_cast<char*>(&size), sizeof(size_t));

		chunk.isVisibleDataLock->lock();
		oChunkFile.write(reinterpret_cast<char*>(&chunk.isVisible), sizeof(int));
		chunk.isVisibleDataLock->unlock();
		
		oChunkFile.close();
	}

	//全存法读取区块文件
	void readFullChunk(glm::dvec3 regionPos, Chunk& chunk) {
		std::filesystem::path chunkPath = std::filesystem::path{ fileInfo::storage::fullFile } / std::format("{}_{}_{}", regionPos.x, regionPos.y, regionPos.z) / std::format("{}_{}_{}.dat", chunk.chunkPos.x, chunk.chunkPos.y, chunk.chunkPos.z);
		std::ifstream iChunkFile{ chunkPath, std::ios::binary };

		chunk.cubeArrayDataLock->lock();
		iChunkFile.read(reinterpret_cast<char*>(chunk.cubeArray), storageInfo::cubeArraySize);
		chunk.cubeArrayDataLock->unlock();

		//size_t size = 0;
		//iChunkFile.read(reinterpret_cast<char*>(&size), sizeof(size_t));

		chunk.isVisibleDataLock->lock();
		iChunkFile.read(reinterpret_cast<char*>(&chunk.isVisible), sizeof(int));
		chunk.isVisibleDataLock->unlock();

		iChunkFile.close();
	}

	//变存法保存区块文件
	void saveDeltaChunk(glm::dvec3 regionPos, Chunk& chunk) {
		std::filesystem::path chunkPath = std::filesystem::path{ fileInfo::storage::deltaFile } / std::format("{}_{}_{}", regionPos.x, regionPos.y, regionPos.z) / std::format("{}_{}_{}.dat", chunk.chunkPos.x, chunk.chunkPos.y, chunk.chunkPos.z);
		std::filesystem::create_directories(chunkPath.parent_path());	//如果当前文件父路径不存在则创建，如果存在则不操作
		std::ofstream oChunkFile{ chunkPath, std::ios::binary };

		//保存cubeDeltaSet.size
		size_t size = chunk.cubeDeltaSet.size();
		oChunkFile.write(reinterpret_cast<char*>(&size), sizeof(size_t));

		//保存cubeDeltaSet
		for (const auto& [key, id] : chunk.cubeDeltaSet) {
			oChunkFile.write(reinterpret_cast<const char*>(key.data()), sizeof(short) * 3);
			oChunkFile.write(reinterpret_cast<const char*>(&id), sizeof(short));
		}

		oChunkFile.close();
	}

	//变存法读取区块文件
	void readDeltaChunk(glm::dvec3 regionPos, Chunk& chunk) {
		std::filesystem::path chunkPath = std::filesystem::path{ fileInfo::storage::deltaFile } / std::format("{}_{}_{}", regionPos.x, regionPos.y, regionPos.z) / std::format("{}_{}_{}.dat", chunk.chunkPos.x, chunk.chunkPos.y, chunk.chunkPos.z);
		std::ifstream iChunkFile{ chunkPath, std::ios::binary };

		chunk.cubeArrayDataLock->lock();
		chunk.cubeDeltaSetDataLock->lock();
		chunk.isVisibleDataLock->lock();
		//读取cubeDeltaSet.size
		size_t size = 0;
		iChunkFile.read(reinterpret_cast<char*>(&size), sizeof(size_t));

		//读取cubeDeltaSet
		std::array<short, 3> key{};
		short newCubeId = cubeEnum::cubeId::count;

		for (size_t i = 0; i < size; i++) {
			iChunkFile.read(reinterpret_cast<char*>(key.data()), sizeof(short) * 3);
			iChunkFile.read(reinterpret_cast<char*>(&newCubeId), sizeof(short));

			short& oldCubeId = chunk.cubeArray[key[0]][key[1]][key[2]];
			if (oldCubeId == cubeEnum::cubeId::air) {
				if (newCubeId != cubeEnum::cubeId::air) {
					chunk.isVisible++;
				}
			}
			else {
				if (newCubeId == cubeEnum::cubeId::air) {
					chunk.isVisible--;
				}
			}

			oldCubeId = newCubeId;
			chunk.cubeDeltaSet[key] = newCubeId;
		}
		chunk.isVisibleDataLock->unlock();
		chunk.cubeDeltaSetDataLock->unlock();
		chunk.cubeArrayDataLock->unlock();

		iChunkFile.close();
	}

public:
	//更新配置
	//只动态读取配置加载集合元素，不动态保存配置卸载集合元素，在程序结束时统一保存配置
	void updateConfig(glm::dvec3 playerChunkPos) {
		glm::dvec3 tempPos = glm::floor(playerChunkPos / (double)storageInfo::regionLength);

		if (regionOriginPos != tempPos) {
			regionOriginPos = tempPos;
			readConfig(regionOriginPos);
		}
	}

	//保存区块文件
	void saveChunk(Chunk& chunk) {
		glm::dvec3 regionPos = glm::floor(chunk.chunkPos / (double)storageInfo::regionLength);

		chunk.cubeDeltaSetDataLock->lock();
		if (chunk.cubeDeltaSet.size() < storageInfo::thresholdSize) {
			deltaChunkList[regionPos].insert(chunk.chunkPos);
			saveDeltaChunk(regionPos, chunk);

			if (fullChunkList.find(regionPos) != fullChunkList.end() && fullChunkList[regionPos].find(chunk.chunkPos) != fullChunkList[regionPos].end()) {
				fullChunkList[regionPos].erase(chunk.chunkPos);
				std::filesystem::remove(std::filesystem::path{ fileInfo::storage::fullFile } / std::format("{}_{}_{}", regionPos.x, regionPos.y, regionPos.z) / std::format("{}_{}_{}.dat", chunk.chunkPos.x, chunk.chunkPos.y, chunk.chunkPos.z));
			}
		}
		else {
			fullChunkList[regionPos].insert(chunk.chunkPos);
			saveFullChunk(regionPos, chunk);

			if (deltaChunkList.find(regionPos) != deltaChunkList.end() && deltaChunkList[regionPos].find(chunk.chunkPos) != deltaChunkList[regionPos].end()) {
				deltaChunkList[regionPos].erase(chunk.chunkPos);
				std::filesystem::remove(std::filesystem::path{ fileInfo::storage::deltaFile } / std::format("{}_{}_{}", regionPos.x, regionPos.y, regionPos.z) / std::format("{}_{}_{}.dat", chunk.chunkPos.x, chunk.chunkPos.y, chunk.chunkPos.z));
			}
		}
		chunk.cubeDeltaSetDataLock->unlock();
	}

	//读取区块文件
	void readChunk(Chunk& chunk) {
		glm::dvec3 regionPos = glm::floor(chunk.chunkPos / (double)storageInfo::regionLength);

		if (deltaChunkList.find(regionPos) != deltaChunkList.end()) {
			if (deltaChunkList[regionPos].find(chunk.chunkPos) != deltaChunkList[regionPos].end()) {
				readDeltaChunk(regionPos, chunk);
			}
		}
		else if (fullChunkList.find(regionPos) != fullChunkList.end()) {
			if (fullChunkList[regionPos].find(chunk.chunkPos) != fullChunkList[regionPos].end()) {
				readFullChunk(regionPos, chunk);
			}
		}
	}
};