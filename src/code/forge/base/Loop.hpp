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
	void createLoop() {
		/*plateList::chunkMapLoop*/
		int range = plateInfo::sightRange;
		int rangeSquare = range * range;

		for (int x = -range; x < range; x++) {
			for (int y = -range; y < range; y++) {
				for (int z = -range; z < range; z++) {
					if (x * x + y * y + z * z < rangeSquare) {
						plateList::chunkMapLoop.push_back(glm::dvec3{ x,y,z });
					}
				}
			}
		}



		/*terrainList::layerLoop*/
		range = terrainList::layerSightRange[terrainEnum::terrainLayer::Layer_0];
		rangeSquare = range * range;

		for (int x = -range; x < range; x++) {
			for (int y = -range; y < range; y++) {
				for (int z = -range; z < range; z++) {
					if (x * x + y * y + z * z < rangeSquare) {
						terrainList::layerLoop.insert(glm::dvec3{ x,y,z });
					}
				}
			}
		}



		/*plateList::structClearLoop*/
		range = plateInfo::sightRange + 2;
		rangeSquare = range * range;
		
		for (int x = -range; x < range; x++) {
			for (int y = -range; y < range; y++) {
				for (int z = -range; z < range; z++) {
					if (x * x + y * y + z * z < rangeSquare) {
						plateList::structClearLoop.insert(glm::dvec3{ x,y,z });
					}
				}
			}
		}



		/*plateList::cubekMapLoop*/
		range = plateInfo::chunkLength;

		for (int x = 1; x <= range; x++) {
			for (int y = 1; y <= range; y++) {
				for (int z = 1; z <= range; z++) {
					plateList::cubekMapLoop.push_back(glm::ivec3{ x,y,z });
				}
			}
		}



		/*terrainList::treeLoop*/
		std::unordered_map<glm::ivec3, short> tempMap;
		range = terrainStructureList::terrainStructureProperty[terrainStructureEnum::treeId::oak].r;
		rangeSquare = range * range;
		
		for (int x = -range; x <= range; x++) {
			for (int y = -range; y <= range; y++) {
				for (int z = -range; z <= range; z++) {
					if (x * x + y * y + z * z < rangeSquare) {
						tempMap[glm::ivec3{ x,y + terrainStructureList::terrainStructureProperty[terrainStructureEnum::treeId::oak].length,z }] = terrainStructureList::terrainStructureProperty[terrainStructureEnum::treeId::oak].leaf;
					}
				}
			}
		}

		for (int y = 0; y <= terrainStructureList::terrainStructureProperty[terrainStructureEnum::treeId::oak].length; y++) {
			tempMap[glm::ivec3{ 0,y,0 }] = terrainStructureList::terrainStructureProperty[terrainStructureEnum::treeId::oak].trunk;
		}

		for (auto& node : tempMap) {
			terrainStructureList::treeLoop.push_back(glm::ivec4{ node.first,node.second });
		}



		/*storageList::regionLoop*/
		range = storageInfo::regionLoopLength;
		rangeSquare = range * range;

		for (int x = -range; x < range; x++) {
			for (int y = -range; y < range; y++) {
				for (int z = -range; z < range; z++) {
					if (x * x + y * y + z * z < rangeSquare) {
						storageList::regionLoop.push_back(glm::dvec3{ x,y,z });
					}
				}
			}
		}
	}
};