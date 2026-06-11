#pragma once



class Storage {
public:
	std::unordered_set<glm::dvec3> waitChunkList;											//待存储区块集合，暂存需要存储的区块坐标，在区块卸载时保存
	std::unique_ptr<std::mutex> waitChunkListDataLock = std::make_unique<std::mutex>();		//waitChunkList数据锁

	//无论全存还是变存，方块数组使用范围都是[0,33]
	std::unordered_map<glm::dvec3, std::unordered_set<glm::dvec3>> fullChunkList;			//全存法区块存储集合，存储整个区块
	std::unordered_map<glm::dvec3, std::unordered_set<glm::dvec3>> deltaChunkList;			//变存法区块存储集合，存储玩家变更方块的数组坐标与id



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
			storageStruct::saveFileNode saveRegionConfig{ getRegionPath(fileInfo::storage::fullFile, regionPos) / "regionConfig.dat" };

			saveRegionConfig.writeData(chunkSet.size());
			for (const glm::dvec3& chunkPos : chunkSet) {
				saveRegionConfig.writeData(chunkPos);
			}

			saveRegionConfig.close();
		}

		for (const auto& [regionPos, chunkSet] : deltaChunkList) {
			storageStruct::saveFileNode saveRegionConfig{ getRegionPath(fileInfo::storage::deltaFile, regionPos) / "regionConfig.dat" };

			saveRegionConfig.writeData(chunkSet.size());
			for (const glm::dvec3& chunkPos : chunkSet) {
				saveRegionConfig.writeData(chunkPos);
			}

			saveRegionConfig.close();
		}
	}

	//读取地区配置
	void readConfig(glm::dvec3 regionOriginPos) {
		for (glm::dvec3 regionPos : loopList::regionLoop) {
			regionPos += regionOriginPos;

			auto regionPath = getRegionPath(fileInfo::storage::fullFile, regionPos) / "regionConfig.dat";
			if (std::filesystem::exists(regionPath) && !fullChunkList.contains(regionPos)) {
				storageStruct::readFileNode readRegionConfig{ regionPath };

				size_t chunkSetSize = readRegionConfig.readValue<size_t>();
				for (size_t i = 0; i < chunkSetSize; i++) {
					fullChunkList[regionPos].insert(readRegionConfig.readValue<glm::dvec3>());
				}

				readRegionConfig.close();
			}

			regionPath = getRegionPath(fileInfo::storage::deltaFile, regionPos) / "regionConfig.dat";
			if (std::filesystem::exists(regionPath) && !deltaChunkList.contains(regionPos)) {
				storageStruct::readFileNode readRegionConfig{ regionPath };

				size_t chunkSetSize = readRegionConfig.readValue<size_t>();
				for (size_t i = 0; i < chunkSetSize; i++) {
					deltaChunkList[regionPos].insert(readRegionConfig.readValue<glm::dvec3>());
				}

				readRegionConfig.close();
			}
		}
	}



	//全存法保存区块文件
	void saveFullChunk(glm::dvec3 regionPos, Chunk& chunk) {
		storageStruct::saveFileNode saveChunkFile{ getChunkPath(fileInfo::storage::fullFile, regionPos, chunk.chunkPos) };

		chunk.cubeArrayDataLock->lock();
		chunk.linkStructCubeDeltaSetDataLock->lock();
		chunk.isVisibleDataLock->lock();

		saveChunkFile.writeData(chunk.cubeArray, storageInfo::cubeArraySize);

		saveChunkFile.writeData(chunk.linkStructCubeDeltaSet.size());
		for (const auto& cubePos : chunk.linkStructCubeDeltaSet) {
			saveChunkFile.writeData(cubePos);
		}

		saveChunkFile.writeData(chunk.isFull);
		saveChunkFile.writeData(chunk.isVisible);

		chunk.isVisibleDataLock->unlock();
		chunk.linkStructCubeDeltaSetDataLock->unlock();
		chunk.cubeArrayDataLock->unlock();

		saveChunkFile.close();
	}

	//全存法读取区块文件
	void readFullChunk(glm::dvec3 regionPos, Chunk& chunk) {
		storageStruct::readFileNode readChunkFile{ getChunkPath(fileInfo::storage::fullFile, regionPos, chunk.chunkPos) };

		chunk.cubeArrayDataLock->lock();
		chunk.linkStructCubeDeltaSetDataLock->lock();
		chunk.isVisibleDataLock->lock();

		readChunkFile.readData(chunk.cubeArray, storageInfo::cubeArraySize);

		size_t linkStructCubeDeltaSetSize = readChunkFile.readValue<size_t>();
		for (size_t i = 0; i < linkStructCubeDeltaSetSize; i++) {
			chunk.linkStructCubeDeltaSet.insert(readChunkFile.readValue<plateStruct::cubePosNode>());
		}

		readChunkFile.readData(chunk.isFull);
		readChunkFile.readData(chunk.isVisible);

		chunk.isVisibleDataLock->unlock();
		chunk.linkStructCubeDeltaSetDataLock->unlock();
		chunk.cubeArrayDataLock->unlock();

		readChunkFile.close();
	}

	//变存法保存区块文件
	void saveDeltaChunk(glm::dvec3 regionPos, Chunk& chunk) {
		storageStruct::saveFileNode saveChunkFile{ getChunkPath(fileInfo::storage::deltaFile, regionPos, chunk.chunkPos) };

		//保存cubeDeltaSet
		saveChunkFile.writeData(chunk.cubeDeltaSet.size());
		for (const auto& [cubePos, cubeId] : chunk.cubeDeltaSet) {
			saveChunkFile.writeData(cubePos);
			saveChunkFile.writeData(cubeId);
		}

		//保存linkStructCubeDeltaSet
		chunk.linkStructCubeDeltaSetDataLock->lock();
		saveChunkFile.writeData(chunk.linkStructCubeDeltaSet.size());
		for (const auto& cubePos : chunk.linkStructCubeDeltaSet) {
			saveChunkFile.writeData(cubePos);
		}
		chunk.linkStructCubeDeltaSetDataLock->unlock();

		//保存isFull
		saveChunkFile.writeData(chunk.isFull);

		saveChunkFile.close();
	}

	//变存法读取区块文件
	void readDeltaChunk(glm::dvec3 regionPos, Chunk& chunk) {
		storageStruct::readFileNode readChunkFile{ getChunkPath(fileInfo::storage::deltaFile, regionPos, chunk.chunkPos) };

		chunk.cubeArrayDataLock->lock();
		chunk.cubeDeltaSetDataLock->lock();
		chunk.linkStructCubeDeltaSetDataLock->lock();
		chunk.isVisibleDataLock->lock();

		//读取cubeDeltaSet
		size_t cubeDeltaSetSize = readChunkFile.readValue<size_t>();
		std::array<short, 3> cubePos{};
		short newCubeId = cubeEnum::cubeId::count;
		for (size_t i = 0; i < cubeDeltaSetSize; i++) {
			readChunkFile.readData(cubePos);
			readChunkFile.readData(newCubeId);

			short& oldCubeId = chunk.cubeArray[cubePos[0]][cubePos[1]][cubePos[2]];
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
			chunk.cubeDeltaSet[cubePos] = newCubeId;
		}

		//读取linkStructCubeDeltaSet
		size_t linkStructCubeDeltaSetSize = readChunkFile.readValue<size_t>();
		for (size_t i = 0; i < linkStructCubeDeltaSetSize; i++) {
			chunk.linkStructCubeDeltaSet.insert(readChunkFile.readValue<plateStruct::cubePosNode>());
		}

		//读取isFull
		readChunkFile.readData(chunk.isFull);

		chunk.isVisibleDataLock->unlock();
		chunk.linkStructCubeDeltaSetDataLock->unlock();
		chunk.cubeDeltaSetDataLock->unlock();
		chunk.cubeArrayDataLock->unlock();

		readChunkFile.close();
	}

public:
	//更新配置，只动态读取配置加载集合元素，不动态保存配置卸载集合元素，在程序结束时统一保存配置
	void updateConfig(glm::dvec3 playerChunkPos) {
		glm::dvec3 tempPos = glm::floor(playerChunkPos / (double)storageInfo::regionLength);

		if (regionOriginPos != tempPos) {
			regionOriginPos = tempPos;
			readConfig(regionOriginPos);
		}
	}

	//保存区块文件，1ms
	void saveChunk(Chunk& chunk) {
		glm::dvec3 regionPos = glm::floor(chunk.chunkPos / (double)storageInfo::regionLength);

		chunk.cubeDeltaSetDataLock->lock();
		if (chunk.getIsFull()) {
			fullChunkList[regionPos].insert(chunk.chunkPos);
			saveFullChunk(regionPos, chunk);

			if (deltaChunkList.contains(regionPos) && deltaChunkList[regionPos].contains(chunk.chunkPos)) {
				deltaChunkList[regionPos].erase(chunk.chunkPos);
				std::filesystem::remove(getChunkPath(fileInfo::storage::deltaFile, regionPos, chunk.chunkPos));

				//删除空路径
				if (deltaChunkList[regionPos].empty()) {
					deltaChunkList.erase(regionPos);
					std::filesystem::remove_all(getRegionPath(fileInfo::storage::deltaFile, regionPos));
				}
			}
		}
		else {
			deltaChunkList[regionPos].insert(chunk.chunkPos);
			saveDeltaChunk(regionPos, chunk);

			if (fullChunkList.contains(regionPos) && fullChunkList[regionPos].contains(chunk.chunkPos)) {
				fullChunkList[regionPos].erase(chunk.chunkPos);
				std::filesystem::remove(getChunkPath(fileInfo::storage::fullFile, regionPos, chunk.chunkPos));

				//删除空路径
				if (fullChunkList[regionPos].empty()) {
					fullChunkList.erase(regionPos);
					std::filesystem::remove_all(getRegionPath(fileInfo::storage::fullFile, regionPos));
				}
			}
		}
		chunk.cubeDeltaSetDataLock->unlock();
	}

	//读取区块文件，0.5ms
	void readChunk(Chunk& chunk) {
		glm::dvec3 regionPos = glm::floor(chunk.chunkPos / (double)storageInfo::regionLength);

		if (deltaChunkList.contains(regionPos) && deltaChunkList[regionPos].contains(chunk.chunkPos)) {
			readDeltaChunk(regionPos, chunk);
		}
		else if (fullChunkList.contains(regionPos) && fullChunkList[regionPos].contains(chunk.chunkPos)) {
			readFullChunk(regionPos, chunk);
		}
	}

private:
	//获取地区路径
	std::filesystem::path getRegionPath(const std::filesystem::path& basePath, const glm::dvec3& regionPos) const {
		return basePath / std::format("{}_{}_{}", regionPos.x, regionPos.y, regionPos.z);
	}

	//获取区块路径
	std::filesystem::path getChunkPath(const std::filesystem::path& basePath, const glm::dvec3& regionPos, const glm::dvec3& chunkPos) const {
		return getRegionPath(basePath, regionPos) / std::format("{}_{}_{}.dat", chunkPos.x, chunkPos.y, chunkPos.z);
	}
};