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
		/*plateInfo::chunkLoop*/
		int rangeSquare = plateInfo::sightRange * plateInfo::sightRange;

		for (int x = -plateInfo::sightRange; x < plateInfo::sightRange; x++) {
			for (int y = -plateInfo::sightRange; y < plateInfo::sightRange; y++) {
				for (int z = -plateInfo::sightRange; z < plateInfo::sightRange; z++) {
					if (x * x + y * y + z * z < rangeSquare) {
						plateList::chunkLoop.push_back(glm::dvec3{ x,y,z });
					}
				}
			}
		}

		/*plateInfo::structClearLoop*/
		int structSightRange = plateInfo::sightRange + 2;
		rangeSquare = structSightRange * structSightRange;

		for (int x = -structSightRange; x < structSightRange; x++) {
			for (int y = -structSightRange; y < structSightRange; y++) {
				for (int z = -structSightRange; z < structSightRange; z++) {
					if (x * x + y * y + z * z < rangeSquare) {
						plateList::structClearLoop.insert(glm::dvec3{ x,y,z });
					}
				}
			}
		}

		/*plateInfo::cubeLoop*/
		for (int x = 1; x <= plateInfo::chunkLength; x++) {
			for (int y = 1; y <= plateInfo::chunkLength; y++) {
				for (int z = 1; z <= plateInfo::chunkLength; z++) {
					plateList::cubeLoop.push_back(glm::ivec3{ x,y,z });
				}
			}
		}

		/*terrainList::treeLoop*/
		std::unordered_map<glm::ivec3, short> tempMap;
		
		for (int x = -terrainList::oak.r; x <= terrainList::oak.r; x++) {
			for (int y = -terrainList::oak.r; y <= terrainList::oak.r; y++) {
				for (int z = -terrainList::oak.r; z <= terrainList::oak.r; z++) {
					if (x * x + y * y + z * z < terrainList::oak.r * terrainList::oak.r) {
						//tempMap[glm::ivec3{ x,y + terrainList::oak.length,z }] = cubeList::cubeId::oakLeaf;
						tempMap[glm::ivec3{ x,y + terrainList::oak.length,z }] = cubeEnum::cubeId::stoneSquared;
					}
				}
			}
		}

		for (int y = 0; y <= terrainList::oak.length; y++) {
			tempMap[glm::ivec3{ 0,y,0 }] = cubeEnum::cubeId::oakLog;
		}

		for (auto& node : tempMap) {
			terrainList::treeLoop.push_back(glm::ivec4{ node.first,node.second });
		}

		/*storageList::regionLoop*/
		rangeSquare = storageInfo::regionLoopLength * storageInfo::regionLoopLength;

		for (int x = -storageInfo::regionLoopLength; x < storageInfo::regionLoopLength; x++) {
			for (int y = -storageInfo::regionLoopLength; y < storageInfo::regionLoopLength; y++) {
				for (int z = -storageInfo::regionLoopLength; z < storageInfo::regionLoopLength; z++) {
					if (x * x + y * y + z * z < rangeSquare) {
						storageList::regionLoop.push_back(glm::dvec3{ x,y,z });
					}
				}
			}
		}
	}
};