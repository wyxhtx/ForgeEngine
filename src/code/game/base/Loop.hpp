#pragma once



class Loop {
public:
	Loop() {
		createLoop();

		if (debugInfo::LogSwitch) {
			Log::printTitle("————————Loop已创建");

			Log::delimiter();
		}
	}

	~Loop() {
		if (debugInfo::LogSwitch) {
			Log::print("Loop已清除");
		}
	}

private:
	noiseList::procStructureNode procStructure{};



	void createLoop() {
		//cubeMapLoop
		int range = plateInfo::chunkLength;

		for (int x = 1; x <= range; x++) {
			for (int y = 1; y <= range; y++) {
				for (int z = 1; z <= range; z++) {
					loopList::cubeMapLoop.push_back(glm::ivec3{ x,y,z });
				}
			}
		}

		//cubeXZLoop
		range = plateInfo::cubeArrayLength;

		for (int x = 0; x < range; x++) {
			for (int z = 0; z < range; z++) {
				loopList::cubeXZLoop.push_back(glm::ivec3{ x,0,z });
			}
		}

		//chunkMapLoop
		range = plateInfo::sightRange;
		int rangeSquare = range * range;

		for (int x = -range; x < range; x++) {
			for (int y = -range; y < range; y++) {
				for (int z = -range; z < range; z++) {
					if (x * x + y * y + z * z < rangeSquare) {
						loopList::chunkMapLoop.push_back(glm::dvec3{ x,y,z });
					}
				}
			}
		}

		//regionLoop
		range = storageInfo::regionLoopLength;
		rangeSquare = range * range;

		for (int x = -range; x < range; x++) {
			for (int y = -range; y < range; y++) {
				for (int z = -range; z < range; z++) {
					if (x * x + y * y + z * z < rangeSquare) {
						loopList::regionLoop.push_back(glm::dvec3{ x,y,z });
					}
				}
			}
		}

		//structClearLoop
		range = plateInfo::sightRange + 2;
		rangeSquare = range * range;

		for (int x = -range; x < range; x++) {
			for (int y = -range; y < range; y++) {
				for (int z = -range; z < range; z++) {
					if (x * x + y * y + z * z < rangeSquare) {
						loopList::structClearLoop.insert(glm::dvec3{ x,y,z });
					}
				}
			}
		}

		//treeLoop
		std::unordered_map<glm::ivec3, short> tempMap;
		range = procStructure.treeNode[noiseEnum::treeId::oak].r;
		rangeSquare = range * range;
		
		for (int x = -range; x <= range; x++) {
			for (int y = -range; y <= range; y++) {
				for (int z = -range; z <= range; z++) {
					if (x * x + y * y + z * z < rangeSquare) {
						tempMap[glm::ivec3{ x,y + procStructure.treeNode[noiseEnum::treeId::oak].length,z }] = procStructure.treeNode[noiseEnum::treeId::oak].leaf;
					}
				}
			}
		}

		for (int y = 0; y <= procStructure.treeNode[noiseEnum::treeId::oak].length; y++) {
			tempMap[glm::ivec3{ 0,y,0 }] = procStructure.treeNode[noiseEnum::treeId::oak].trunk;
		}

		for (auto& node : tempMap) {
			loopList::treeLoop.push_back(glm::ivec4{ node.first,node.second });
		}

		//terrainLayerLoop
		range = plateInfo::terrainLayerSight[plateEnum::terrainLayer::Layer_0];
		rangeSquare = range * range;

		for (int x = -range; x < range; x++) {
			for (int y = -range; y < range; y++) {
				for (int z = -range; z < range; z++) {
					if (x * x + y * y + z * z < rangeSquare) {
						loopList::terrainLayerLoop.insert(glm::dvec3{ x,y,z });
					}
				}
			}
		}
	}
};