#pragma once



//Plate.updatePlate执行的chunkMap[updatePos] = tempChunkMap[updatePos]相当于给区块所有变量赋值，容易导致资源冲突
class Chunk {
public:
	//Storage（saveFullChunk、readFullChunk、readDeltaChunk），Plate（createPlate、updatePlate、getSelectBox、createUpdateCubeList），Chunk（createChunkMeshData、updateChunkMeshData）
	short cubeArray[plateInfo::cubeArrayLength][plateInfo::cubeArrayLength][plateInfo::cubeArrayLength]{};	//方块数组，最外一圈存储相邻区块边缘方块数据，方便区块间面检测，使用范围[1,32]，边缘方块数据0和33
	std::unordered_set<storageStruct::cubeDeltaNode, storageStruct::cubeDeltaNodeHash> cubeDeltaSet;		//方块变更集合，为变存法服务					//Storage（saveDeltaChunk、readDeltaChunk、saveChunk），Plate（createUpdateCubeList）

	int isVisible = 0;												//标识区块是否可见，如果全空或者全满为不可见，但是不能排除1~32全空但边缘非空的情况			//Storage（saveFullChunk、readFullChunk、saveDeltaChunk、readDeltaChunk），Plate（createPlate、createChunk、updatePlate、createUpdateCubeList）
	int layerID = terrainEnum::terrainLayer::count;					//区块所在地形管线层																	//Plate（createPlate、updatePlate、updatePlatePartMesh）

	//由InitChunk初始化
	glm::dvec3 originOffset{};										//区块原点偏移（d世界坐标系），用于plate类里joinChunkMesh方法处理世界原点迁移时的情况		//Plate（joinTerrainMesh），Chunk（InitChunk）
	glm::dvec3 chunkPos{};											//区块坐标（区块坐标系），区块在Plate.chunkMap中的坐标									//Storage（saveFullChunk、readFullChunk、saveDeltaChunk、readDeltaChunk、saveChunk、readChunk），Plate（createChunk、），Chunk（InitChunk）
	glm::dvec3 logicalWorldPos{};									//区块逻辑世界坐标（d世界坐标系）（对应cubeArray[0][0][0]处），用于地形生成				//Plate（createChunk），Chunk（InitChunk）
	glm::ivec3 actualWorldPos{};									//区块实际世界坐标（i世界坐标系）（对应cubeArray[0][0][0]处），用于构建网格				//Chunk（InitChunk、createChunkMeshData、updateChunkMeshData）

	//由createChunkMeshData初始化,chunkVertexs,chunkIndexs,IndexOffset都能判断区块是否应该加载网格，34立方区块全空或全实不加载网格
	std::vector<terrainVertex::vertexNode> chunkVertexs{};			//区块网格顶点数组																	//Plate（createPlate、joinTerrainMesh、updatePlate、updatePlatePartMesh），Chunk（createChunkMesh、updateChunkMesh）
	std::vector<uint32_t> chunkIndexs{};							//区块网格索引数组																	//Plate（createPlate、joinTerrainMesh、updatePlate、updatePlatePartMesh），Chunk（createChunkMesh、updateChunkMesh）
	uint32_t indexOffset = 0;										//区块索引偏移量																		//Plate（createPlate、updatePlate），Chunk（createChunkMesh、updateChunkMesh、updateChunkMeshData）



	void InitChunk(const glm::dvec3 origin, const glm::dvec3 chunkPos0) {
		originOffset = origin;
		chunkPos = chunkPos0;
		logicalWorldPos = chunkPos * (double)plateInfo::chunkLength;
		actualWorldPos = glm::ivec3(logicalWorldPos - origin);
	}

	//创建区块网格数据
	void createChunkMeshData() {
		std::unordered_map<glm::ivec3, short> visibleFaceMap[6][plateInfo::chunkLength];	//方块坐标为键，方块ID为值，存储所有可见面，[0,31]
		int chunkLengthMax = plateInfo::chunkLength + 1;									//探极值时，因为31不为空时wSet.max不赋值，所以多探一位32必为空，33做边界值
		int chunkLengthMin = -2;															//坐标迁移到左下角时，同理多探一位-1，-2做边界值



		//获取可见面集合
		for (const glm::ivec3& pos: plateList::cubekMapLoop) {
			short cubeId = cubeArray[pos.x][pos.y][pos.z];	//[1,32]

			if (cubeId != cubeEnum::cubeId::air) {
				glm::ivec3 tempPos = pos - 1;				//从[1,32]偏移到[0,31]

				for (int i = 0; i < 6; i++) {
					//如果该面挨着空气则加载
					if (cubeArray[pos.x + faceInfo::faceCullId[i].x][pos.y + faceInfo::faceCullId[i].y][pos.z + faceInfo::faceCullId[i].z] == cubeEnum::cubeId::air) {
						if (i == 0 || i == 1) {
							visibleFaceMap[i][tempPos.y][tempPos] = cubeId;
						}
						else if (i == 2 || i == 3) {
							visibleFaceMap[i][tempPos.x][tempPos] = cubeId;
						}
						else if (i == 4 || i == 5) {
							visibleFaceMap[i][tempPos.z][tempPos] = cubeId;
						}
					}
				}
			}
		}

		//贪心算法优化可见面集合,宽高都是逻辑概念,不存在空间上的对应关系
		//区块网格由0~5六个朝向组成，每个朝向由0~31三十二层组成
		for (int i = 0; i < 6; i++) {
			for (int j = 0; j < plateInfo::chunkLength; j++) {
				if (!visibleFaceMap[i][j].empty()) {		//[0,31]
					int size = (int)chunkVertexs.size();

					while (!visibleFaceMap[i][j].empty()) {
						std::unordered_map<glm::ivec3, short>& node = visibleFaceMap[i][j];
						glm::ivec3 pos = node.begin()->first;
						short cubeId = node.begin()->second;
						int faceId = cubeList::cubeProperty[cubeId].faceId[i].x + cubeList::cubeProperty[cubeId].faceId[i].y * textureInfo::atlasWidth;

						cubeStruct::traversalNode wSet;
						cubeStruct::traversalNode hSet;

						int w;
						int h;

						if (i == 0 || i == 1) {
							//宽度迁移到左下角
							for (int x = pos.x - 1; x > chunkLengthMin; x--) {
								std::unordered_map<glm::ivec3, short>::iterator it = node.find(glm::ivec3(x, pos.y, pos.z));

								if (it == node.end() || it->second != cubeId) {
									pos.x = x + 1;
									break;
								}
							}

							//高度迁移到左下角
							for (int z = pos.z - 1; z > chunkLengthMin; z--) {
								std::unordered_map<glm::ivec3, short>::iterator it = node.find(glm::ivec3(pos.x, pos.y, z));

								if (it == node.end() || it->second != cubeId) {
									pos.z = z + 1;
									break;
								}
							}

							//探宽度极值
							for (int x = pos.x + 1; x < chunkLengthMax; x++) {
								std::unordered_map<glm::ivec3, short>::iterator it = node.find(glm::ivec3(x, pos.y, pos.z));

								if (it == node.end() || it->second != cubeId) {
									wSet.max = x - pos.x;
									break;
								}
							}

							//探高度极值
							for (int z = pos.z + 1; z < chunkLengthMax; z++) {
								std::unordered_map<glm::ivec3, short>::iterator it = node.find(glm::ivec3(pos.x, pos.y, z));

								if (it == node.end() || it->second != cubeId) {
									hSet.max = z - pos.z;
									break;
								}
							}

							//如果没有探索价值则直接退出
							if (wSet.max == 1 && hSet.max == 1) {
								createChunkMesh(i, faceId, cubeId, pos + actualWorldPos, glm::vec3(1, 1, 1));
								node.erase(pos);
								continue;
							}

							wSet.min = wSet.max;
							hSet.min = hSet.max;

							//如果没有探索价值则直接退出
							if (wSet.max == 1) {
								createChunkMesh(i, faceId, cubeId, pos + actualWorldPos, glm::vec3(1, 1, hSet.max));

								for (int h = pos.z; h < pos.z + hSet.max; h++) {
									node.erase(glm::ivec3(pos.x, pos.y, h));
								}

								continue;
							}
							else if (hSet.max == 1) {
								createChunkMesh(i, faceId, cubeId, pos + actualWorldPos, glm::vec3(wSet.max, 1, 1));

								for (int w = pos.x; w < pos.x + wSet.max; w++) {
									node.erase(glm::ivec3(w, pos.y, pos.z));
								}

								continue;
							}

							//宽度探路
							for (int z = pos.z + 1; z < pos.z + hSet.max; z++) {
								for (int x = pos.x; x < pos.x + wSet.max; x++) {
									std::unordered_map<glm::ivec3, short>::iterator it = node.find(glm::ivec3(x, pos.y, z));

									if (it == node.end() || it->second != cubeId) {
										int temp = x - pos.x;

										if (temp < wSet.min) {
											wSet.min = temp;
										}
										break;
									}
								}
							}

							//高度探路
							for (int x = pos.x + 1; x < pos.x + wSet.max; x++) {
								for (int z = pos.z; z < pos.z + hSet.max; z++) {
									std::unordered_map<glm::ivec3, short>::iterator it = node.find(glm::ivec3(x, pos.y, z));

									if (it == node.end() || it->second != cubeId) {
										int temp = z - pos.z;

										if (temp < hSet.min) {
											hSet.min = temp;
										}
										break;
									}
								}
							}

							//生成矩形
							if (wSet.max * hSet.min > wSet.min * hSet.max) {
								w = wSet.max;
								h = hSet.min;

								for (int z = pos.z; z < pos.z + hSet.min; z++) {
									for (int x = pos.x; x < pos.x + wSet.max; x++) {
										node.erase(glm::ivec3(x, pos.y, z));
									}
								}
							}
							else {
								w = wSet.min;
								h = hSet.max;

								for (int x = pos.x; x < pos.x + wSet.min; x++) {
									for (int z = pos.z; z < pos.z + hSet.max; z++) {
										node.erase(glm::ivec3(x, pos.y, z));
									}
								}
							}

							createChunkMesh(i, faceId, cubeId, pos + actualWorldPos, glm::vec3(w, 1, h));
						}
						else if (i == 2 || i == 3) {
							//宽度迁移到左下角
							for (int y = pos.y - 1; y > chunkLengthMin; y--) {
								std::unordered_map<glm::ivec3, short>::iterator it = node.find(glm::ivec3(pos.x, y, pos.z));

								if (it == node.end() || it->second != cubeId) {
									pos.y = y + 1;
									break;
								}
							}

							//高度迁移到左下角
							for (int z = pos.z - 1; z > chunkLengthMin; z--) {
								std::unordered_map<glm::ivec3, short>::iterator it = node.find(glm::ivec3(pos.x, pos.y, z));

								if (it == node.end() || it->second != cubeId) {
									pos.z = z + 1;
									break;
								}
							}

							//探宽度极值
							for (int y = pos.y + 1; y < chunkLengthMax; y++) {
								std::unordered_map<glm::ivec3, short>::iterator it = node.find(glm::ivec3(pos.x, y, pos.z));

								if (it == node.end() || it->second != cubeId) {
									wSet.max = y - pos.y;
									break;
								}
							}

							//探高度极值
							for (int z = pos.z + 1; z < chunkLengthMax; z++) {
								std::unordered_map<glm::ivec3, short>::iterator it = node.find(glm::ivec3(pos.x, pos.y, z));

								if (it == node.end() || it->second != cubeId) {
									hSet.max = z - pos.z;
									break;
								}
							}

							//如果没有探索价值则直接退出
							if (wSet.max == 1 && hSet.max == 1) {
								createChunkMesh(i, faceId, cubeId, pos + actualWorldPos, glm::vec3(1, 1, 1));
								node.erase(pos);
								continue;
							}

							wSet.min = wSet.max;
							hSet.min = hSet.max;

							//如果没有探索价值则直接退出
							if (wSet.max == 1) {
								createChunkMesh(i, faceId, cubeId, pos + actualWorldPos, glm::vec3(1, 1, hSet.max));

								for (int h = pos.z; h < pos.z + hSet.max; h++) {
									node.erase(glm::ivec3(pos.x, pos.y, h));
								}

								continue;
							}
							else if (hSet.max == 1) {
								createChunkMesh(i, faceId, cubeId, pos + actualWorldPos, glm::vec3(1, wSet.max, 1));

								for (int w = pos.y; w < pos.y + wSet.max; w++) {
									node.erase(glm::ivec3(pos.x, w, pos.z));
								}

								continue;
							}

							//宽度探路
							for (int z = pos.z + 1; z < pos.z + hSet.max; z++) {
								for (int y = pos.y; y < pos.y + wSet.max; y++) {
									std::unordered_map<glm::ivec3, short>::iterator it = node.find(glm::ivec3(pos.x, y, z));

									if (it == node.end() || it->second != cubeId) {
										int temp = y - pos.y;

										if (temp < wSet.min) {
											wSet.min = temp;
										}
										break;
									}
								}
							}

							//高度探路
							for (int y = pos.y + 1; y < pos.y + wSet.max; y++) {
								for (int z = pos.z; z < pos.z + hSet.max; z++) {
									std::unordered_map<glm::ivec3, short>::iterator it = node.find(glm::ivec3(pos.x, y, z));

									if (it == node.end() || it->second != cubeId) {
										int temp = z - pos.z;

										if (temp < hSet.min) {
											hSet.min = temp;
										}
										break;
									}
								}
							}

							//生成矩形
							if (wSet.max * hSet.min > wSet.min * hSet.max) {
								w = wSet.max;
								h = hSet.min;

								for (int z = pos.z; z < pos.z + hSet.min; z++) {
									for (int y = pos.y; y < pos.y + wSet.max; y++) {
										node.erase(glm::ivec3(pos.x, y, z));
									}
								}
							}
							else {
								w = wSet.min;
								h = hSet.max;

								for (int y = pos.y; y < pos.y + wSet.min; y++) {
									for (int z = pos.z; z < pos.z + hSet.max; z++) {
										node.erase(glm::ivec3(pos.x, y, z));
									}
								}
							}

							createChunkMesh(i, faceId, cubeId, pos + actualWorldPos, glm::vec3(1, w, h));
						}
						else if (i == 4 || i == 5) {
							//宽度迁移到左下角
							for (int y = pos.y - 1; y > chunkLengthMin; y--) {
								std::unordered_map<glm::ivec3, short>::iterator it = node.find(glm::ivec3(pos.x, y, pos.z));

								if (it == node.end() || it->second != cubeId) {
									pos.y = y + 1;
									break;
								}
							}

							//高度迁移到左下角
							for (int x = pos.x - 1; x > chunkLengthMin; x--) {
								std::unordered_map<glm::ivec3, short>::iterator it = node.find(glm::ivec3(x, pos.y, pos.z));

								if (it == node.end() || it->second != cubeId) {
									pos.x = x + 1;
									break;
								}
							}

							//探宽度极值
							for (int y = pos.y + 1; y < chunkLengthMax; y++) {
								std::unordered_map<glm::ivec3, short>::iterator it = node.find(glm::ivec3(pos.x, y, pos.z));

								if (it == node.end() || it->second != cubeId) {
									wSet.max = y - pos.y;

									break;
								}
							}

							//探高度极值
							for (int x = pos.x + 1; x < chunkLengthMax; x++) {
								std::unordered_map<glm::ivec3, short>::iterator it = node.find(glm::ivec3(x, pos.y, pos.z));

								if (it == node.end() || it->second != cubeId) {
									hSet.max = x - pos.x;
									break;
								}
							}

							//如果没有探索价值则直接退出
							if (wSet.max == 1 && hSet.max == 1) {
								createChunkMesh(i, faceId, cubeId, pos + actualWorldPos, glm::vec3(1, 1, 1));
								node.erase(pos);
								continue;
							}

							wSet.min = wSet.max;
							hSet.min = hSet.max;

							//如果没有探索价值则直接退出
							if (wSet.max == 1) {
								createChunkMesh(i, faceId, cubeId, pos + actualWorldPos, glm::vec3(hSet.max, 1, 1));

								for (int h = pos.x; h < pos.x + hSet.max; h++) {
									node.erase(glm::ivec3(h, pos.y, pos.z));
								}

								continue;
							}
							else if (hSet.max == 1) {
								createChunkMesh(i, faceId, cubeId, pos + actualWorldPos, glm::vec3(1, wSet.max, 1));

								for (int w = pos.y; w < pos.y + wSet.max; w++) {
									node.erase(glm::ivec3(pos.x, w, pos.z));
								}

								continue;
							}

							//宽度探路
							for (int x = pos.x + 1; x < pos.x + hSet.max; x++) {
								for (int y = pos.y; y < pos.y + wSet.max; y++) {
									std::unordered_map<glm::ivec3, short>::iterator it = node.find(glm::ivec3(x, y, pos.z));

									if (it == node.end() || it->second != cubeId) {
										int temp = y - pos.y;

										if (temp < wSet.min) {
											wSet.min = temp;
										}
										break;
									}
								}
							}

							//高度探路
							for (int y = pos.y + 1; y < pos.y + wSet.max; y++) {
								for (int x = pos.x; x < pos.x + hSet.max; x++) {
									std::unordered_map<glm::ivec3, short>::iterator it = node.find(glm::ivec3(x, y, pos.z));

									if (it == node.end() || it->second != cubeId) {
										int temp = x - pos.x;

										if (temp < hSet.min) {
											hSet.min = temp;
										}
										break;
									}
								}
							}

							//生成矩形
							if (wSet.max * hSet.min > wSet.min * hSet.max) {
								w = wSet.max;
								h = hSet.min;

								for (int x = pos.x; x < pos.x + hSet.min; x++) {
									for (int y = pos.y; y < pos.y + wSet.max; y++) {
										node.erase(glm::ivec3(x, y, pos.z));
									}
								}
							}
							else {
								w = wSet.min;
								h = hSet.max;

								for (int y = pos.y; y < pos.y + wSet.min; y++) {
									for (int x = pos.x; x < pos.x + hSet.max; x++) {
										node.erase(glm::ivec3(x, y, pos.z));
									}
								}
							}

							createChunkMesh(i, faceId, cubeId, pos + actualWorldPos, glm::vec3(h, w, 1));
						}
					}

					size = (int)chunkVertexs.size() - size;

					chunkMeshSignList[i].list.push_front(plateStruct::chunkMeshSignListNode{ j, size, (int)(size * 1.5f) });
					chunkMeshSignList[i].vertexsSentinel += size;
				}
			}
		}
	}

private:
	plateStruct::chunkMeshSignNode chunkMeshSignList[6];			//区块网格标识列表，按照先0~5面再0~31层的顺序维护网格列表



	//i：		朝向id
	//faceId：	纹理id
	//cubeId:	方块id
	//position：	坐标
	//length：	纹理循环长度
	void createChunkMesh(int i, int faceId, int cubeId, glm::vec3 position, glm::vec3 length) {
		std::vector<terrainVertex::vertexNode> tempVertexs;

		//四个顶点构成一个面，顶点uv绑定应按照Z画法，uv绑定朝向与画Z时相同
		switch (i) {
		case 0: {
			//上, Y+
			tempVertexs = {
				{{vertexInfo::vertexID[5] * length + position}, {vertexInfo::uvID[0] * glm::vec2(length.x, length.z), faceId}, {vertexInfo::normalID[i]}, {cubeList::cubeProperty[cubeId].ambient}, {cubeList::cubeProperty[cubeId].diffuse}, {cubeList::cubeProperty[cubeId].specular}, {cubeList::cubeProperty[cubeId].shininess}},
				{{vertexInfo::vertexID[4] * length + position}, {vertexInfo::uvID[1] * glm::vec2(length.x, length.z), faceId}, {vertexInfo::normalID[i]}, {cubeList::cubeProperty[cubeId].ambient}, {cubeList::cubeProperty[cubeId].diffuse}, {cubeList::cubeProperty[cubeId].specular}, {cubeList::cubeProperty[cubeId].shininess}},
				{{vertexInfo::vertexID[1] * length + position}, {vertexInfo::uvID[2] * glm::vec2(length.x, length.z), faceId}, {vertexInfo::normalID[i]}, {cubeList::cubeProperty[cubeId].ambient}, {cubeList::cubeProperty[cubeId].diffuse}, {cubeList::cubeProperty[cubeId].specular}, {cubeList::cubeProperty[cubeId].shininess}},
				{{vertexInfo::vertexID[0] * length + position}, {vertexInfo::uvID[3] * glm::vec2(length.x, length.z), faceId}, {vertexInfo::normalID[i]}, {cubeList::cubeProperty[cubeId].ambient}, {cubeList::cubeProperty[cubeId].diffuse}, {cubeList::cubeProperty[cubeId].specular}, {cubeList::cubeProperty[cubeId].shininess}}
			};
			break;
		}
		case 1: {
			//下, Y-
			tempVertexs = {
				{{vertexInfo::vertexID[2] * length + position}, {vertexInfo::uvID[0] * glm::vec2(length.x, length.z), faceId}, {vertexInfo::normalID[i]}, {cubeList::cubeProperty[cubeId].ambient}, {cubeList::cubeProperty[cubeId].diffuse}, {cubeList::cubeProperty[cubeId].specular}, {cubeList::cubeProperty[cubeId].shininess}},
				{{vertexInfo::vertexID[3] * length + position}, {vertexInfo::uvID[1] * glm::vec2(length.x, length.z), faceId}, {vertexInfo::normalID[i]}, {cubeList::cubeProperty[cubeId].ambient}, {cubeList::cubeProperty[cubeId].diffuse}, {cubeList::cubeProperty[cubeId].specular}, {cubeList::cubeProperty[cubeId].shininess}},
				{{vertexInfo::vertexID[6] * length + position}, {vertexInfo::uvID[2] * glm::vec2(length.x, length.z), faceId}, {vertexInfo::normalID[i]}, {cubeList::cubeProperty[cubeId].ambient}, {cubeList::cubeProperty[cubeId].diffuse}, {cubeList::cubeProperty[cubeId].specular}, {cubeList::cubeProperty[cubeId].shininess}},
				{{vertexInfo::vertexID[7] * length + position}, {vertexInfo::uvID[3] * glm::vec2(length.x, length.z), faceId}, {vertexInfo::normalID[i]}, {cubeList::cubeProperty[cubeId].ambient}, {cubeList::cubeProperty[cubeId].diffuse}, {cubeList::cubeProperty[cubeId].specular}, {cubeList::cubeProperty[cubeId].shininess}}
			};
			break;
		}
		case 2: {
			//左, X+
			tempVertexs = {
				{{vertexInfo::vertexID[0] * length + position}, {vertexInfo::uvID[0] * glm::vec2(length.z, length.y), faceId}, {vertexInfo::normalID[i]}, {cubeList::cubeProperty[cubeId].ambient}, {cubeList::cubeProperty[cubeId].diffuse}, {cubeList::cubeProperty[cubeId].specular}, {cubeList::cubeProperty[cubeId].shininess}},
				{{vertexInfo::vertexID[4] * length + position}, {vertexInfo::uvID[1] * glm::vec2(length.z, length.y), faceId}, {vertexInfo::normalID[i]}, {cubeList::cubeProperty[cubeId].ambient}, {cubeList::cubeProperty[cubeId].diffuse}, {cubeList::cubeProperty[cubeId].specular}, {cubeList::cubeProperty[cubeId].shininess}},
				{{vertexInfo::vertexID[3] * length + position}, {vertexInfo::uvID[2] * glm::vec2(length.z, length.y), faceId}, {vertexInfo::normalID[i]}, {cubeList::cubeProperty[cubeId].ambient}, {cubeList::cubeProperty[cubeId].diffuse}, {cubeList::cubeProperty[cubeId].specular}, {cubeList::cubeProperty[cubeId].shininess}},
				{{vertexInfo::vertexID[7] * length + position}, {vertexInfo::uvID[3] * glm::vec2(length.z, length.y), faceId}, {vertexInfo::normalID[i]}, {cubeList::cubeProperty[cubeId].ambient}, {cubeList::cubeProperty[cubeId].diffuse}, {cubeList::cubeProperty[cubeId].specular}, {cubeList::cubeProperty[cubeId].shininess}}
			};
			break;
		}
		case 3: {
			//右, X-
			tempVertexs = {
				{{vertexInfo::vertexID[5] * length + position}, {vertexInfo::uvID[0] * glm::vec2(length.z, length.y), faceId}, {vertexInfo::normalID[i]}, {cubeList::cubeProperty[cubeId].ambient}, {cubeList::cubeProperty[cubeId].diffuse}, {cubeList::cubeProperty[cubeId].specular}, {cubeList::cubeProperty[cubeId].shininess}},
				{{vertexInfo::vertexID[1] * length + position}, {vertexInfo::uvID[1] * glm::vec2(length.z, length.y), faceId}, {vertexInfo::normalID[i]}, {cubeList::cubeProperty[cubeId].ambient}, {cubeList::cubeProperty[cubeId].diffuse}, {cubeList::cubeProperty[cubeId].specular}, {cubeList::cubeProperty[cubeId].shininess}},
				{{vertexInfo::vertexID[6] * length + position}, {vertexInfo::uvID[2] * glm::vec2(length.z, length.y), faceId}, {vertexInfo::normalID[i]}, {cubeList::cubeProperty[cubeId].ambient}, {cubeList::cubeProperty[cubeId].diffuse}, {cubeList::cubeProperty[cubeId].specular}, {cubeList::cubeProperty[cubeId].shininess}},
				{{vertexInfo::vertexID[2] * length + position}, {vertexInfo::uvID[3] * glm::vec2(length.z, length.y), faceId}, {vertexInfo::normalID[i]}, {cubeList::cubeProperty[cubeId].ambient}, {cubeList::cubeProperty[cubeId].diffuse}, {cubeList::cubeProperty[cubeId].specular}, {cubeList::cubeProperty[cubeId].shininess}}
			};
			break;
		}
		case 4: {
			//前, Z+
			tempVertexs = {
				{{vertexInfo::vertexID[4] * length + position}, {vertexInfo::uvID[0] * glm::vec2(length.x, length.y), faceId}, {vertexInfo::normalID[i]}, {cubeList::cubeProperty[cubeId].ambient}, {cubeList::cubeProperty[cubeId].diffuse}, {cubeList::cubeProperty[cubeId].specular}, {cubeList::cubeProperty[cubeId].shininess}},
				{{vertexInfo::vertexID[5] * length + position}, {vertexInfo::uvID[1] * glm::vec2(length.x, length.y), faceId}, {vertexInfo::normalID[i]}, {cubeList::cubeProperty[cubeId].ambient}, {cubeList::cubeProperty[cubeId].diffuse}, {cubeList::cubeProperty[cubeId].specular}, {cubeList::cubeProperty[cubeId].shininess}},
				{{vertexInfo::vertexID[7] * length + position}, {vertexInfo::uvID[2] * glm::vec2(length.x, length.y), faceId}, {vertexInfo::normalID[i]}, {cubeList::cubeProperty[cubeId].ambient}, {cubeList::cubeProperty[cubeId].diffuse}, {cubeList::cubeProperty[cubeId].specular}, {cubeList::cubeProperty[cubeId].shininess}},
				{{vertexInfo::vertexID[6] * length + position}, {vertexInfo::uvID[3] * glm::vec2(length.x, length.y), faceId}, {vertexInfo::normalID[i]}, {cubeList::cubeProperty[cubeId].ambient}, {cubeList::cubeProperty[cubeId].diffuse}, {cubeList::cubeProperty[cubeId].specular}, {cubeList::cubeProperty[cubeId].shininess}}
			};
			break;
		}
		case 5: {
			//后, Z-
			tempVertexs = {
				{{vertexInfo::vertexID[1] * length + position}, {vertexInfo::uvID[0] * glm::vec2(length.x, length.y), faceId}, {vertexInfo::normalID[i]}, {cubeList::cubeProperty[cubeId].ambient}, {cubeList::cubeProperty[cubeId].diffuse}, {cubeList::cubeProperty[cubeId].specular}, {cubeList::cubeProperty[cubeId].shininess}},
				{{vertexInfo::vertexID[0] * length + position}, {vertexInfo::uvID[1] * glm::vec2(length.x, length.y), faceId}, {vertexInfo::normalID[i]}, {cubeList::cubeProperty[cubeId].ambient}, {cubeList::cubeProperty[cubeId].diffuse}, {cubeList::cubeProperty[cubeId].specular}, {cubeList::cubeProperty[cubeId].shininess}},
				{{vertexInfo::vertexID[2] * length + position}, {vertexInfo::uvID[2] * glm::vec2(length.x, length.y), faceId}, {vertexInfo::normalID[i]}, {cubeList::cubeProperty[cubeId].ambient}, {cubeList::cubeProperty[cubeId].diffuse}, {cubeList::cubeProperty[cubeId].specular}, {cubeList::cubeProperty[cubeId].shininess}},
				{{vertexInfo::vertexID[3] * length + position}, {vertexInfo::uvID[3] * glm::vec2(length.x, length.y), faceId}, {vertexInfo::normalID[i]}, {cubeList::cubeProperty[cubeId].ambient}, {cubeList::cubeProperty[cubeId].diffuse}, {cubeList::cubeProperty[cubeId].specular}, {cubeList::cubeProperty[cubeId].shininess}}
			};
			break;
		}
		}

		//顶点输入
		chunkVertexs.insert(chunkVertexs.end(), tempVertexs.begin(), tempVertexs.end());
		//索引输入
		chunkIndexs.insert(chunkIndexs.end(), { 0 + indexOffset, 1 + indexOffset, 2 + indexOffset, 2 + indexOffset, 1 + indexOffset, 3 + indexOffset });

		indexOffset += 4;
	}

	//i：		朝向id
	//faceId：	纹理id
	//cubeId:	方块id
	//position：	坐标
	//length：	纹理循环长度
	//vertexsSize：顶点长度
	void updateChunkMesh(int i, int faceId, int cubeId, glm::vec3 position, glm::vec3 length, int vertexsSize) {
		std::vector<terrainVertex::vertexNode> tempVertexs;

		switch (i) {
		case 0: {
			//上, Y+
			tempVertexs = {
				{{vertexInfo::vertexID[5] * length + position}, {vertexInfo::uvID[0] * glm::vec2(length.x, length.z), faceId}, {vertexInfo::normalID[i]}, {cubeList::cubeProperty[cubeId].ambient}, {cubeList::cubeProperty[cubeId].diffuse}, {cubeList::cubeProperty[cubeId].specular}, {cubeList::cubeProperty[cubeId].shininess}},
				{{vertexInfo::vertexID[4] * length + position}, {vertexInfo::uvID[1] * glm::vec2(length.x, length.z), faceId}, {vertexInfo::normalID[i]}, {cubeList::cubeProperty[cubeId].ambient}, {cubeList::cubeProperty[cubeId].diffuse}, {cubeList::cubeProperty[cubeId].specular}, {cubeList::cubeProperty[cubeId].shininess}},
				{{vertexInfo::vertexID[1] * length + position}, {vertexInfo::uvID[2] * glm::vec2(length.x, length.z), faceId}, {vertexInfo::normalID[i]}, {cubeList::cubeProperty[cubeId].ambient}, {cubeList::cubeProperty[cubeId].diffuse}, {cubeList::cubeProperty[cubeId].specular}, {cubeList::cubeProperty[cubeId].shininess}},
				{{vertexInfo::vertexID[0] * length + position}, {vertexInfo::uvID[3] * glm::vec2(length.x, length.z), faceId}, {vertexInfo::normalID[i]}, {cubeList::cubeProperty[cubeId].ambient}, {cubeList::cubeProperty[cubeId].diffuse}, {cubeList::cubeProperty[cubeId].specular}, {cubeList::cubeProperty[cubeId].shininess}}
			};
			break;
		}
		case 1: {
			//下, Y-
			tempVertexs = {
				{{vertexInfo::vertexID[2] * length + position}, {vertexInfo::uvID[0] * glm::vec2(length.x, length.z), faceId}, {vertexInfo::normalID[i]}, {cubeList::cubeProperty[cubeId].ambient}, {cubeList::cubeProperty[cubeId].diffuse}, {cubeList::cubeProperty[cubeId].specular}, {cubeList::cubeProperty[cubeId].shininess}},
				{{vertexInfo::vertexID[3] * length + position}, {vertexInfo::uvID[1] * glm::vec2(length.x, length.z), faceId}, {vertexInfo::normalID[i]}, {cubeList::cubeProperty[cubeId].ambient}, {cubeList::cubeProperty[cubeId].diffuse}, {cubeList::cubeProperty[cubeId].specular}, {cubeList::cubeProperty[cubeId].shininess}},
				{{vertexInfo::vertexID[6] * length + position}, {vertexInfo::uvID[2] * glm::vec2(length.x, length.z), faceId}, {vertexInfo::normalID[i]}, {cubeList::cubeProperty[cubeId].ambient}, {cubeList::cubeProperty[cubeId].diffuse}, {cubeList::cubeProperty[cubeId].specular}, {cubeList::cubeProperty[cubeId].shininess}},
				{{vertexInfo::vertexID[7] * length + position}, {vertexInfo::uvID[3] * glm::vec2(length.x, length.z), faceId}, {vertexInfo::normalID[i]}, {cubeList::cubeProperty[cubeId].ambient}, {cubeList::cubeProperty[cubeId].diffuse}, {cubeList::cubeProperty[cubeId].specular}, {cubeList::cubeProperty[cubeId].shininess}}
			};
			break;
		}
		case 2: {
			//左, X+
			tempVertexs = {
				{{vertexInfo::vertexID[0] * length + position}, {vertexInfo::uvID[0] * glm::vec2(length.z, length.y), faceId}, {vertexInfo::normalID[i]}, {cubeList::cubeProperty[cubeId].ambient}, {cubeList::cubeProperty[cubeId].diffuse}, {cubeList::cubeProperty[cubeId].specular}, {cubeList::cubeProperty[cubeId].shininess}},
				{{vertexInfo::vertexID[4] * length + position}, {vertexInfo::uvID[1] * glm::vec2(length.z, length.y), faceId}, {vertexInfo::normalID[i]}, {cubeList::cubeProperty[cubeId].ambient}, {cubeList::cubeProperty[cubeId].diffuse}, {cubeList::cubeProperty[cubeId].specular}, {cubeList::cubeProperty[cubeId].shininess}},
				{{vertexInfo::vertexID[3] * length + position}, {vertexInfo::uvID[2] * glm::vec2(length.z, length.y), faceId}, {vertexInfo::normalID[i]}, {cubeList::cubeProperty[cubeId].ambient}, {cubeList::cubeProperty[cubeId].diffuse}, {cubeList::cubeProperty[cubeId].specular}, {cubeList::cubeProperty[cubeId].shininess}},
				{{vertexInfo::vertexID[7] * length + position}, {vertexInfo::uvID[3] * glm::vec2(length.z, length.y), faceId}, {vertexInfo::normalID[i]}, {cubeList::cubeProperty[cubeId].ambient}, {cubeList::cubeProperty[cubeId].diffuse}, {cubeList::cubeProperty[cubeId].specular}, {cubeList::cubeProperty[cubeId].shininess}}
			};
			break;
		}
		case 3: {
			//右, X-
			tempVertexs = {
				{{vertexInfo::vertexID[5] * length + position}, {vertexInfo::uvID[0] * glm::vec2(length.z, length.y), faceId}, {vertexInfo::normalID[i]}, {cubeList::cubeProperty[cubeId].ambient}, {cubeList::cubeProperty[cubeId].diffuse}, {cubeList::cubeProperty[cubeId].specular}, {cubeList::cubeProperty[cubeId].shininess}},
				{{vertexInfo::vertexID[1] * length + position}, {vertexInfo::uvID[1] * glm::vec2(length.z, length.y), faceId}, {vertexInfo::normalID[i]}, {cubeList::cubeProperty[cubeId].ambient}, {cubeList::cubeProperty[cubeId].diffuse}, {cubeList::cubeProperty[cubeId].specular}, {cubeList::cubeProperty[cubeId].shininess}},
				{{vertexInfo::vertexID[6] * length + position}, {vertexInfo::uvID[2] * glm::vec2(length.z, length.y), faceId}, {vertexInfo::normalID[i]}, {cubeList::cubeProperty[cubeId].ambient}, {cubeList::cubeProperty[cubeId].diffuse}, {cubeList::cubeProperty[cubeId].specular}, {cubeList::cubeProperty[cubeId].shininess}},
				{{vertexInfo::vertexID[2] * length + position}, {vertexInfo::uvID[3] * glm::vec2(length.z, length.y), faceId}, {vertexInfo::normalID[i]}, {cubeList::cubeProperty[cubeId].ambient}, {cubeList::cubeProperty[cubeId].diffuse}, {cubeList::cubeProperty[cubeId].specular}, {cubeList::cubeProperty[cubeId].shininess}}
			};
			break;
		}
		case 4: {
			//前, Z+
			tempVertexs = {
				{{vertexInfo::vertexID[4] * length + position}, {vertexInfo::uvID[0] * glm::vec2(length.x, length.y), faceId}, {vertexInfo::normalID[i]}, {cubeList::cubeProperty[cubeId].ambient}, {cubeList::cubeProperty[cubeId].diffuse}, {cubeList::cubeProperty[cubeId].specular}, {cubeList::cubeProperty[cubeId].shininess}},
				{{vertexInfo::vertexID[5] * length + position}, {vertexInfo::uvID[1] * glm::vec2(length.x, length.y), faceId}, {vertexInfo::normalID[i]}, {cubeList::cubeProperty[cubeId].ambient}, {cubeList::cubeProperty[cubeId].diffuse}, {cubeList::cubeProperty[cubeId].specular}, {cubeList::cubeProperty[cubeId].shininess}},
				{{vertexInfo::vertexID[7] * length + position}, {vertexInfo::uvID[2] * glm::vec2(length.x, length.y), faceId}, {vertexInfo::normalID[i]}, {cubeList::cubeProperty[cubeId].ambient}, {cubeList::cubeProperty[cubeId].diffuse}, {cubeList::cubeProperty[cubeId].specular}, {cubeList::cubeProperty[cubeId].shininess}},
				{{vertexInfo::vertexID[6] * length + position}, {vertexInfo::uvID[3] * glm::vec2(length.x, length.y), faceId}, {vertexInfo::normalID[i]}, {cubeList::cubeProperty[cubeId].ambient}, {cubeList::cubeProperty[cubeId].diffuse}, {cubeList::cubeProperty[cubeId].specular}, {cubeList::cubeProperty[cubeId].shininess}}
			};
			break;
		}
		case 5: {
			//后, Z-
			tempVertexs = {
				{{vertexInfo::vertexID[1] * length + position}, {vertexInfo::uvID[0] * glm::vec2(length.x, length.y), faceId}, {vertexInfo::normalID[i]}, {cubeList::cubeProperty[cubeId].ambient}, {cubeList::cubeProperty[cubeId].diffuse}, {cubeList::cubeProperty[cubeId].specular}, {cubeList::cubeProperty[cubeId].shininess}},
				{{vertexInfo::vertexID[0] * length + position}, {vertexInfo::uvID[1] * glm::vec2(length.x, length.y), faceId}, {vertexInfo::normalID[i]}, {cubeList::cubeProperty[cubeId].ambient}, {cubeList::cubeProperty[cubeId].diffuse}, {cubeList::cubeProperty[cubeId].specular}, {cubeList::cubeProperty[cubeId].shininess}},
				{{vertexInfo::vertexID[2] * length + position}, {vertexInfo::uvID[2] * glm::vec2(length.x, length.y), faceId}, {vertexInfo::normalID[i]}, {cubeList::cubeProperty[cubeId].ambient}, {cubeList::cubeProperty[cubeId].diffuse}, {cubeList::cubeProperty[cubeId].specular}, {cubeList::cubeProperty[cubeId].shininess}},
				{{vertexInfo::vertexID[3] * length + position}, {vertexInfo::uvID[3] * glm::vec2(length.x, length.y), faceId}, {vertexInfo::normalID[i]}, {cubeList::cubeProperty[cubeId].ambient}, {cubeList::cubeProperty[cubeId].diffuse}, {cubeList::cubeProperty[cubeId].specular}, {cubeList::cubeProperty[cubeId].shininess}}
			};
			break;
		}
		}

		chunkVertexs.insert(chunkVertexs.begin() + vertexsSize, tempVertexs.begin(), tempVertexs.end());
		chunkIndexs.insert(chunkIndexs.end(), { 0 + indexOffset, 1 + indexOffset, 2 + indexOffset, 2 + indexOffset, 1 + indexOffset, 3 + indexOffset });

		indexOffset += 4;
	}

public:
	//todo 不适用于放置方块
	//更新区块网格数据
	void updateChunkMeshData(const std::unordered_set<glm::ivec3>& cubePosSet, const std::unordered_set<glm::ivec2>& abutCubePosSet) {
		std::unordered_map<glm::ivec3, short> visibleFaceMap[6][plateInfo::chunkLength];						//区块截层可见面集合，[0,31]
		std::unordered_set<int> faceSet[6];																		//区块截层集合，根据两个方块列表（cubePosSet、abutCubePosSet），截取需要更新网格的区块层，[0,31]

		//标识各朝向对应的最大层和最小层，为优化区块截层网格的生成服务
		int layerMax[6]{ 0,0,0,0,0,0 };
		int layerMin[6]{ plateInfo::lengthDecrement,plateInfo::lengthDecrement,plateInfo::lengthDecrement,plateInfo::lengthDecrement,plateInfo::lengthDecrement,plateInfo::lengthDecrement };

		//根据cubePosSet，获取faceSet和visibleFaceMap
		for (const auto& node : cubePosSet) {
			for (int i = 0; i < 6; i++) {
				if (i == 0 || i == 1) {
					//获取该层visibleFaceMap
					if (faceSet[i].find(node.y) == faceSet[i].end()) {
						for (int x = 1; x <= plateInfo::chunkLength; x++) {
							for (int z = 1; z <= plateInfo::chunkLength; z++) {
								short cubeId = cubeArray[x][node.y + 1][z];										//xz为[1,32]，node.y需+1从[0,31]偏移到[1,32]

								if (cubeId != cubeEnum::cubeId::air) {
									if (cubeArray[x + faceInfo::faceCullId[i].x][node.y + 1 + faceInfo::faceCullId[i].y][z + faceInfo::faceCullId[i].z] == cubeEnum::cubeId::air) {
										visibleFaceMap[i][node.y][glm::ivec3{ x - 1,node.y,z - 1 }] = cubeId;	//node.y为[0,31]，xz需-1从[1,32]偏移到[0,31]
									}
								}
							}
						}

						faceSet[i].insert(node.y);
					}

					//检测变更方块邻层是否需要更新，如果位于"上0"或"下31"则无需更新
					int y = node.y;

					if (i == 0 && node.y != 0 && cubeArray[node.x + 1][node.y][node.z + 1] != cubeEnum::cubeId::air) {
						y--;
					}
					else if (i == 1 && node.y != plateInfo::lengthDecrement && cubeArray[node.x + 1][node.y + 2][node.z + 1] != cubeEnum::cubeId::air) {
						y++;
					}

					//获取邻层visibleFaceMap
					if (y != node.y) {
						if (faceSet[i].find(y) == faceSet[i].end()) {
							for (int x = 1; x <= plateInfo::chunkLength; x++) {
								for (int z = 1; z <= plateInfo::chunkLength; z++) {
									short cubeId = cubeArray[x][y + 1][z];

									if (cubeId != cubeEnum::cubeId::air) {
										if (cubeArray[x + faceInfo::faceCullId[i].x][y + 1 + faceInfo::faceCullId[i].y][z + faceInfo::faceCullId[i].z] == cubeEnum::cubeId::air) {
											visibleFaceMap[i][y][glm::ivec3{ x - 1,y,z - 1 }] = cubeId;
										}
									}
								}
							}

							faceSet[i].insert(y);
						}
					}
				}
				else if (i == 2 || i == 3) {
					if (faceSet[i].find(node.x) == faceSet[i].end()) {
						for (int y = 1; y <= plateInfo::chunkLength; y++) {
							for (int z = 1; z <= plateInfo::chunkLength; z++) {
								short cubeId = cubeArray[node.x + 1][y][z];

								if (cubeId != cubeEnum::cubeId::air) {
									if (cubeArray[node.x + 1 + faceInfo::faceCullId[i].x][y + faceInfo::faceCullId[i].y][z + faceInfo::faceCullId[i].z] == cubeEnum::cubeId::air) {
										visibleFaceMap[i][node.x][glm::ivec3{ node.x,y - 1,z - 1 }] = cubeId;
									}
								}
							}
						}

						faceSet[i].insert(node.x);
					}

					int x = node.x;

					if (i == 2 && node.x != 0 && cubeArray[node.x][node.y + 1][node.z + 1] != cubeEnum::cubeId::air) {
						x--;
					}
					else if (i == 3 && node.x != plateInfo::lengthDecrement && cubeArray[node.x + 2][node.y + 1][node.z + 1] != cubeEnum::cubeId::air) {
						x++;
					}

					if (x != node.x) {
						if (faceSet[i].find(x) == faceSet[i].end()) {
							for (int y = 1; y <= plateInfo::chunkLength; y++) {
								for (int z = 1; z <= plateInfo::chunkLength; z++) {
									short cubeId = cubeArray[x + 1][y][z];

									if (cubeId != cubeEnum::cubeId::air) {
										if (cubeArray[x + 1 + faceInfo::faceCullId[i].x][y + faceInfo::faceCullId[i].y][z + faceInfo::faceCullId[i].z] == cubeEnum::cubeId::air) {
											visibleFaceMap[i][x][glm::ivec3{ x,y - 1,z - 1 }] = cubeId;
										}
									}
								}
							}

							faceSet[i].insert(x);
						}
					}
				}
				else if (i == 4 || i == 5) {
					if (faceSet[i].find(node.z) == faceSet[i].end()) {
						for (int x = 1; x <= plateInfo::chunkLength; x++) {
							for (int y = 1; y <= plateInfo::chunkLength; y++) {
								short cubeId = cubeArray[x][y][node.z + 1];

								if (cubeId != cubeEnum::cubeId::air) {
									if (cubeArray[x + faceInfo::faceCullId[i].x][y + faceInfo::faceCullId[i].y][node.z + 1 + faceInfo::faceCullId[i].z] == cubeEnum::cubeId::air) {
										visibleFaceMap[i][node.z][glm::ivec3{ x - 1,y - 1,node.z }] = cubeId;
									}
								}
							}
						}

						faceSet[i].insert(node.z);
					}

					int z = node.z;

					if (i == 4 && node.z != 0 && cubeArray[node.x + 1][node.y + 1][node.z] != cubeEnum::cubeId::air) {
						z--;
					}
					else if (i == 5 && node.z != plateInfo::lengthDecrement && cubeArray[node.x + 1][node.y + 1][node.z + 2] != cubeEnum::cubeId::air) {
						z++;
					}

					if (z != node.z) {
						if (faceSet[i].find(z) == faceSet[i].end()) {
							for (int x = 1; x <= plateInfo::chunkLength; x++) {
								for (int y = 1; y <= plateInfo::chunkLength; y++) {
									short cubeId = cubeArray[x][y][z + 1];

									if (cubeId != cubeEnum::cubeId::air) {
										if (cubeArray[x + faceInfo::faceCullId[i].x][y + faceInfo::faceCullId[i].y][z + 1 + faceInfo::faceCullId[i].z] == cubeEnum::cubeId::air) {
											visibleFaceMap[i][z][glm::ivec3{ x - 1,y - 1,z }] = cubeId;
										}
									}
								}
							}

							faceSet[i].insert(z);
						}
					}
				}
			}
		}

		//根据abutCubePosSet，获取faceSet和visibleFaceMap
		for (const auto& node : abutCubePosSet) {
			if (faceSet[node.x].find(node.y) == faceSet[node.x].end()) {
				if (node.x == 0 || node.x == 1) {
					//获取该层visibleFaceMap
					for (int x = 1; x <= plateInfo::chunkLength; x++) {
						for (int z = 1; z <= plateInfo::chunkLength; z++) {
							short cubeId = cubeArray[x][node.y + 1][z];											//xz为[1,32]，node.y需+1从[0,31]偏移到[1,32]

							if (cubeId != cubeEnum::cubeId::air) {
								if (cubeArray[x + faceInfo::faceCullId[node.x].x][node.y + 1 + faceInfo::faceCullId[node.x].y][z + faceInfo::faceCullId[node.x].z] == cubeEnum::cubeId::air) {
									visibleFaceMap[node.x][node.y][glm::ivec3{ x - 1,node.y,z - 1 }] = cubeId;	//node.y为[0,31]，xz需-1从[1,32]偏移到[0,31]
								}
							}
						}
					}
				}
				else if (node.x == 2 || node.x == 3) {
					for (int y = 1; y <= plateInfo::chunkLength; y++) {
						for (int z = 1; z <= plateInfo::chunkLength; z++) {
							short cubeId = cubeArray[node.y + 1][y][z];

							if (cubeId != cubeEnum::cubeId::air) {
								if (cubeArray[node.y + 1 + faceInfo::faceCullId[node.x].x][y + faceInfo::faceCullId[node.x].y][z + faceInfo::faceCullId[node.x].z] == cubeEnum::cubeId::air) {
									visibleFaceMap[node.x][node.y][glm::ivec3{ node.y,y - 1,z - 1 }] = cubeId;
								}
							}
						}
					}
				}
				else if (node.x == 4 || node.x == 5) {
					for (int x = 1; x <= plateInfo::chunkLength; x++) {
						for (int y = 1; y <= plateInfo::chunkLength; y++) {
							short cubeId = cubeArray[x][y][node.y + 1];

							if (cubeId != cubeEnum::cubeId::air) {
								if (cubeArray[x + faceInfo::faceCullId[node.x].x][y + faceInfo::faceCullId[node.x].y][node.y + 1 + faceInfo::faceCullId[node.x].z] == cubeEnum::cubeId::air) {
									visibleFaceMap[node.x][node.y][glm::ivec3{ x - 1,y - 1,node.y }] = cubeId;
								}
							}
						}
					}
				}

				faceSet[node.x].insert(node.y);
			}
		}

		//根据faceSet，清空区块截层网格
		for (int i = 0; i < 6; i++) {
			for (const auto& j : faceSet[i]) {
				for (std::forward_list<plateStruct::chunkMeshSignListNode>::iterator node = chunkMeshSignList[i].list.begin(), prev = chunkMeshSignList[i].list.before_begin(); node != chunkMeshSignList[i].list.end(); prev = node, ++node) {
					if (node->chunkLayer == j) {
						std::forward_list<plateStruct::chunkMeshSignListNode>::iterator p = std::next(node);
						int vertexsSize = 0;

						while (p != chunkMeshSignList[i].list.end()) {
							vertexsSize += p->vertexsSize;
							++p;
						}

						if (i != 0) {
							for (int k = i; k != 0;) {
								k--;
								vertexsSize += chunkMeshSignList[k].vertexsSentinel;
							}
						}

						chunkVertexs.erase(chunkVertexs.begin() + vertexsSize, chunkVertexs.begin() + vertexsSize + node->vertexsSize);
						chunkIndexs.erase(chunkIndexs.end() - node->indexsSize, chunkIndexs.end());
						indexOffset -= node->vertexsSize;

						chunkMeshSignList[i].vertexsSentinel -= node->vertexsSize;
						chunkMeshSignList[i].list.erase_after(prev);

						break;
					}
				}

				if (layerMax[i] < j) {
					layerMax[i] = j;
				}

				if (layerMin[i] > j) {
					layerMin[i] = j;
				}
			}

			faceSet[i].clear();
		}

		//根据visibleFaceMap，生成拼接区块截层网格
		for (int i = 0; i < 6; i++) {
			for (int j = layerMin[i]; j <= layerMax[i]; j++) {
				if (!visibleFaceMap[i][j].empty()) {
					int size = (int)chunkVertexs.size();
					int vertexsSize = 0;
					bool lock = true;

					for (std::forward_list<plateStruct::chunkMeshSignListNode>::iterator node = chunkMeshSignList[i].list.begin(); node != chunkMeshSignList[i].list.end(); ++node) {
						if (node->chunkLayer < j) {
							vertexsSize += node->vertexsSize;
						}
					}

					if (i != 0) {
						for (int k = i; k != 0;) {
							k--;
							vertexsSize += chunkMeshSignList[k].vertexsSentinel;
						}
					}

					while (!visibleFaceMap[i][j].empty()) {
						const int chunkLengthMax = plateInfo::chunkLength + 1;	//探极值时，因为31不为空时wSet.max不赋值，所以多探一位32必为空
						const int chunkLengthMin = -2;							//坐标迁移到左下角时，同理多探一位-1

						std::unordered_map<glm::ivec3, short>& node = visibleFaceMap[i][j];
						glm::ivec3 pos = node.begin()->first;
						short cubeId = node.begin()->second;
						int faceId = cubeList::cubeProperty[cubeId].faceId[i].x + cubeList::cubeProperty[cubeId].faceId[i].y * textureInfo::atlasWidth;

						cubeStruct::traversalNode wSet;
						cubeStruct::traversalNode hSet;

						int w;
						int h;

						if (i == 0 || i == 1) {
							//宽度迁移到左下角
							for (int x = pos.x - 1; x > chunkLengthMin; x--) {
								std::unordered_map<glm::ivec3, short>::iterator it = node.find(glm::ivec3(x, pos.y, pos.z));

								if (it == node.end() || it->second != cubeId) {
									pos.x = x + 1;
									break;
								}
							}

							//高度迁移到左下角
							for (int z = pos.z - 1; z > chunkLengthMin; z--) {
								std::unordered_map<glm::ivec3, short>::iterator it = node.find(glm::ivec3(pos.x, pos.y, z));

								if (it == node.end() || it->second != cubeId) {
									pos.z = z + 1;
									break;
								}
							}

							//探宽度极值
							for (int x = pos.x + 1; x < chunkLengthMax; x++) {
								std::unordered_map<glm::ivec3, short>::iterator it = node.find(glm::ivec3(x, pos.y, pos.z));

								if (it == node.end() || it->second != cubeId) {
									wSet.max = x - pos.x;
									break;
								}
							}

							//探高度极值
							for (int z = pos.z + 1; z < chunkLengthMax; z++) {
								std::unordered_map<glm::ivec3, short>::iterator it = node.find(glm::ivec3(pos.x, pos.y, z));

								if (it == node.end() || it->second != cubeId) {
									hSet.max = z - pos.z;
									break;
								}
							}

							//如果没有探索价值则直接退出
							if (wSet.max == 1 && hSet.max == 1) {
								updateChunkMesh(i, faceId, cubeId, pos + actualWorldPos, glm::vec3(1, 1, 1), vertexsSize);
								node.erase(pos);
								continue;
							}

							wSet.min = wSet.max;
							hSet.min = hSet.max;

							//如果没有探索价值则直接退出
							if (wSet.max == 1) {
								updateChunkMesh(i, faceId, cubeId, pos + actualWorldPos, glm::vec3(1, 1, hSet.max), vertexsSize);

								for (int h = pos.z; h < pos.z + hSet.max; h++) {
									node.erase(glm::ivec3(pos.x, pos.y, h));
								}

								continue;
							}
							else if (hSet.max == 1) {
								updateChunkMesh(i, faceId, cubeId, pos + actualWorldPos, glm::vec3(wSet.max, 1, 1), vertexsSize);

								for (int w = pos.x; w < pos.x + wSet.max; w++) {
									node.erase(glm::ivec3(w, pos.y, pos.z));
								}

								continue;
							}

							//宽度探路
							for (int z = pos.z + 1; z < pos.z + hSet.max; z++) {
								for (int x = pos.x; x < pos.x + wSet.max; x++) {
									std::unordered_map<glm::ivec3, short>::iterator it = node.find(glm::ivec3(x, pos.y, z));

									if (it == node.end() || it->second != cubeId) {
										int temp = x - pos.x;

										if (temp < wSet.min) {
											wSet.min = temp;
										}
										break;
									}
								}
							}

							//高度探路
							for (int x = pos.x + 1; x < pos.x + wSet.max; x++) {
								for (int z = pos.z; z < pos.z + hSet.max; z++) {
									std::unordered_map<glm::ivec3, short>::iterator it = node.find(glm::ivec3(x, pos.y, z));

									if (it == node.end() || it->second != cubeId) {
										int temp = z - pos.z;

										if (temp < hSet.min) {
											hSet.min = temp;
										}
										break;
									}
								}
							}

							//生成矩形
							if (wSet.max * hSet.min > wSet.min * hSet.max) {
								w = wSet.max;
								h = hSet.min;

								for (int z = pos.z; z < pos.z + hSet.min; z++) {
									for (int x = pos.x; x < pos.x + wSet.max; x++) {
										node.erase(glm::ivec3(x, pos.y, z));
									}
								}
							}
							else {
								w = wSet.min;
								h = hSet.max;

								for (int x = pos.x; x < pos.x + wSet.min; x++) {
									for (int z = pos.z; z < pos.z + hSet.max; z++) {
										node.erase(glm::ivec3(x, pos.y, z));
									}
								}
							}

							updateChunkMesh(i, faceId, cubeId, pos + actualWorldPos, glm::vec3(w, 1, h), vertexsSize);
						}
						else if (i == 2 || i == 3) {
							//宽度迁移到左下角
							for (int y = pos.y - 1; y > chunkLengthMin; y--) {
								std::unordered_map<glm::ivec3, short>::iterator it = node.find(glm::ivec3(pos.x, y, pos.z));

								if (it == node.end() || it->second != cubeId) {
									pos.y = y + 1;
									break;
								}
							}

							//高度迁移到左下角
							for (int z = pos.z - 1; z > chunkLengthMin; z--) {
								std::unordered_map<glm::ivec3, short>::iterator it = node.find(glm::ivec3(pos.x, pos.y, z));

								if (it == node.end() || it->second != cubeId) {
									pos.z = z + 1;
									break;
								}
							}

							//探宽度极值
							for (int y = pos.y + 1; y < chunkLengthMax; y++) {
								std::unordered_map<glm::ivec3, short>::iterator it = node.find(glm::ivec3(pos.x, y, pos.z));

								if (it == node.end() || it->second != cubeId) {
									wSet.max = y - pos.y;
									break;
								}
							}

							//探高度极值
							for (int z = pos.z + 1; z < chunkLengthMax; z++) {
								std::unordered_map<glm::ivec3, short>::iterator it = node.find(glm::ivec3(pos.x, pos.y, z));

								if (it == node.end() || it->second != cubeId) {
									hSet.max = z - pos.z;
									break;
								}
							}

							//如果没有探索价值则直接退出
							if (wSet.max == 1 && hSet.max == 1) {
								updateChunkMesh(i, faceId, cubeId, pos + actualWorldPos, glm::vec3(1, 1, 1), vertexsSize);
								node.erase(pos);
								continue;
							}

							wSet.min = wSet.max;
							hSet.min = hSet.max;

							//如果没有探索价值则直接退出
							if (wSet.max == 1) {
								updateChunkMesh(i, faceId, cubeId, pos + actualWorldPos, glm::vec3(1, 1, hSet.max), vertexsSize);

								for (int h = pos.z; h < pos.z + hSet.max; h++) {
									node.erase(glm::ivec3(pos.x, pos.y, h));
								}

								continue;
							}
							else if (hSet.max == 1) {
								updateChunkMesh(i, faceId, cubeId, pos + actualWorldPos, glm::vec3(1, wSet.max, 1), vertexsSize);

								for (int w = pos.y; w < pos.y + wSet.max; w++) {
									node.erase(glm::ivec3(pos.x, w, pos.z));
								}

								continue;
							}

							//宽度探路
							for (int z = pos.z + 1; z < pos.z + hSet.max; z++) {
								for (int y = pos.y; y < pos.y + wSet.max; y++) {
									std::unordered_map<glm::ivec3, short>::iterator it = node.find(glm::ivec3(pos.x, y, z));

									if (it == node.end() || it->second != cubeId) {
										int temp = y - pos.y;

										if (temp < wSet.min) {
											wSet.min = temp;
										}
										break;
									}
								}
							}

							//高度探路
							for (int y = pos.y + 1; y < pos.y + wSet.max; y++) {
								for (int z = pos.z; z < pos.z + hSet.max; z++) {
									std::unordered_map<glm::ivec3, short>::iterator it = node.find(glm::ivec3(pos.x, y, z));

									if (it == node.end() || it->second != cubeId) {
										int temp = z - pos.z;

										if (temp < hSet.min) {
											hSet.min = temp;
										}
										break;
									}
								}
							}

							//生成矩形
							if (wSet.max * hSet.min > wSet.min * hSet.max) {
								w = wSet.max;
								h = hSet.min;

								for (int z = pos.z; z < pos.z + hSet.min; z++) {
									for (int y = pos.y; y < pos.y + wSet.max; y++) {
										node.erase(glm::ivec3(pos.x, y, z));
									}
								}
							}
							else {
								w = wSet.min;
								h = hSet.max;

								for (int y = pos.y; y < pos.y + wSet.min; y++) {
									for (int z = pos.z; z < pos.z + hSet.max; z++) {
										node.erase(glm::ivec3(pos.x, y, z));
									}
								}
							}

							updateChunkMesh(i, faceId, cubeId, pos + actualWorldPos, glm::vec3(1, w, h), vertexsSize);
						}
						else if (i == 4 || i == 5) {
							//宽度迁移到左下角
							for (int y = pos.y - 1; y > chunkLengthMin; y--) {
								std::unordered_map<glm::ivec3, short>::iterator it = node.find(glm::ivec3(pos.x, y, pos.z));

								if (it == node.end() || it->second != cubeId) {
									pos.y = y + 1;
									break;
								}
							}

							//高度迁移到左下角
							for (int x = pos.x - 1; x > chunkLengthMin; x--) {
								std::unordered_map<glm::ivec3, short>::iterator it = node.find(glm::ivec3(x, pos.y, pos.z));

								if (it == node.end() || it->second != cubeId) {
									pos.x = x + 1;
									break;
								}
							}

							//探宽度极值
							for (int y = pos.y + 1; y < chunkLengthMax; y++) {
								std::unordered_map<glm::ivec3, short>::iterator it = node.find(glm::ivec3(pos.x, y, pos.z));

								if (it == node.end() || it->second != cubeId) {
									wSet.max = y - pos.y;

									break;
								}
							}

							//探高度极值
							for (int x = pos.x + 1; x < chunkLengthMax; x++) {
								std::unordered_map<glm::ivec3, short>::iterator it = node.find(glm::ivec3(x, pos.y, pos.z));

								if (it == node.end() || it->second != cubeId) {
									hSet.max = x - pos.x;
									break;
								}
							}

							//如果没有探索价值则直接退出
							if (wSet.max == 1 && hSet.max == 1) {
								updateChunkMesh(i, faceId, cubeId, pos + actualWorldPos, glm::vec3(1, 1, 1), vertexsSize);
								node.erase(pos);
								continue;
							}

							wSet.min = wSet.max;
							hSet.min = hSet.max;

							//如果没有探索价值则直接退出
							if (wSet.max == 1) {
								updateChunkMesh(i, faceId, cubeId, pos + actualWorldPos, glm::vec3(hSet.max, 1, 1), vertexsSize);

								for (int h = pos.x; h < pos.x + hSet.max; h++) {
									node.erase(glm::ivec3(h, pos.y, pos.z));
								}

								continue;
							}
							else if (hSet.max == 1) {
								updateChunkMesh(i, faceId, cubeId, pos + actualWorldPos, glm::vec3(1, wSet.max, 1), vertexsSize);

								for (int w = pos.y; w < pos.y + wSet.max; w++) {
									node.erase(glm::ivec3(pos.x, w, pos.z));
								}

								continue;
							}

							//宽度探路
							for (int x = pos.x + 1; x < pos.x + hSet.max; x++) {
								for (int y = pos.y; y < pos.y + wSet.max; y++) {
									std::unordered_map<glm::ivec3, short>::iterator it = node.find(glm::ivec3(x, y, pos.z));

									if (it == node.end() || it->second != cubeId) {
										int temp = y - pos.y;

										if (temp < wSet.min) {
											wSet.min = temp;
										}
										break;
									}
								}
							}

							//高度探路
							for (int y = pos.y + 1; y < pos.y + wSet.max; y++) {
								for (int x = pos.x; x < pos.x + hSet.max; x++) {
									std::unordered_map<glm::ivec3, short>::iterator it = node.find(glm::ivec3(x, y, pos.z));

									if (it == node.end() || it->second != cubeId) {
										int temp = x - pos.x;

										if (temp < hSet.min) {
											hSet.min = temp;
										}
										break;
									}
								}
							}

							//生成矩形
							if (wSet.max * hSet.min > wSet.min * hSet.max) {
								w = wSet.max;
								h = hSet.min;

								for (int x = pos.x; x < pos.x + hSet.min; x++) {
									for (int y = pos.y; y < pos.y + wSet.max; y++) {
										node.erase(glm::ivec3(x, y, pos.z));
									}
								}
							}
							else {
								w = wSet.min;
								h = hSet.max;

								for (int y = pos.y; y < pos.y + wSet.min; y++) {
									for (int x = pos.x; x < pos.x + hSet.max; x++) {
										node.erase(glm::ivec3(x, y, pos.z));
									}
								}
							}

							updateChunkMesh(i, faceId, cubeId, pos + actualWorldPos, glm::vec3(h, w, 1), vertexsSize);
						}
					}

					std::forward_list<plateStruct::chunkMeshSignListNode>::iterator it = chunkMeshSignList[i].list.before_begin();
					size = (int)chunkVertexs.size() - size;

					//非列头插入
					for (std::forward_list<plateStruct::chunkMeshSignListNode>::iterator node = chunkMeshSignList[i].list.begin(), prev = chunkMeshSignList[i].list.before_begin(); node != chunkMeshSignList[i].list.end(); prev = node, ++node) {
						if (node->chunkLayer < j) {
							chunkMeshSignList[i].list.insert_after(prev, plateStruct::chunkMeshSignListNode{ j, size, (int)(size * 1.5f) });
							lock = false;
							break;
						}

						it = node;
					}

					//列头插入
					if (lock) {
						chunkMeshSignList[i].list.insert_after(it, plateStruct::chunkMeshSignListNode{ j, size, (int)(size * 1.5f) });
					}

					chunkMeshSignList[i].vertexsSentinel += size;
				}
			}
		}
	}
};