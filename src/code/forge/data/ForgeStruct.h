#pragma once



//噪音结构空间
namespace noiseStruct {
	//噪音结点
	struct noiseNode {
		FastNoise::SmartNode<> noise{};
		int seed = 0;



		void create(const char* noiseTree, int tempSeed) {
			noise = FastNoise::NewFromEncodedNodeTree(noiseTree);
			seed  = tempSeed;
		}
	};

	//噪音数据结点
	struct noiseDataNode {
		std::vector<float> data{};
		double value = 0;
	};

	//树结构结点
	struct treeStructureNode {
		int r;          //树冠半径
		int length;     //树干高度
		int leaf;       //树叶方块id
		int trunk;      //树干方块id



		void create(int tempR, int tempLength, int tempLeaf, int tempTrunk) {
			r      = tempR;
			length = tempLength;
			leaf   = tempLeaf;
			trunk  = tempTrunk;
		}
	};
}

//方块结构空间
namespace cubeStruct {
	//贪心算法使用范围（1~32）遍历结点
	struct traversalNode {
		int max = 1;                            //最短1
		int min = plateInfo::chunkLength;       //最长32
	};

	//面朝向结点
	struct faceNode {
		int x;
		int y;
	};

	//方块结点
	struct cubeNode {
		std::vector<faceNode> faceId;   //纹理id
		short coverage;                 //方块覆盖顺序



		void create(const std::vector<faceNode>& tempFaceId, short tempCoverage) {
			faceId   = tempFaceId;
			coverage = tempCoverage;
		}
	};
}

//板块结构空间
namespace plateStruct {
	//地形网格标识结点
	struct terrainMeshSignNode {
		glm::dvec3 chunkPos;
		int vertexsSize;
		int indexsSize;
	};

	//区块网格标识列表结点
	struct chunkMeshSignListNode {
		int chunkLayer;
		int vertexsSize;                                                //标识区块各朝向每层顶点列表长度
		int indexsSize;                                                 //标识区块各朝向每层索引列表长度
	};

	//区块网格标识结点
	struct chunkMeshSignNode {
		std::forward_list<plateStruct::chunkMeshSignListNode> list;	    //标识区块顶点数组
		int vertexsSentinel = 0;                                        //标识区块各朝向顶点列表长度
	};



	//方块坐标结点
	using cubePosNode = std::array<short, 3>;

	//方块坐标哈希
	struct cubePosHash {
		std::size_t operator()(const std::array<short, 3>& array) const noexcept {
			std::size_t seed = 0;
			seed ^= std::hash<short>{}(array[0]) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			seed ^= std::hash<short>{}(array[1]) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			seed ^= std::hash<short>{}(array[2]) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			return seed;
		}
	};

	//链结构区块结点
	struct linkStructChunkNode {
		std::unordered_map<std::array<short, 3>, short, plateStruct::cubePosHash> cubeSet;  //链结构方块数据
		bool isComplete = false;                                                            //标识链结构是否加载，全部压入即为加载
	};

	//链结构管理器结点，structPos->chunkPos->cubeData
	struct linkStructManagerNode {
		std::unordered_map<glm::dvec3, std::unordered_map<glm::dvec3, plateStruct::linkStructChunkNode>> node;
	};

	//聚类分区结点
	struct kMeansNode {
		std::vector<std::vector<glm::dvec3>> node;



		void kMeans(const std::vector<glm::dvec3>& chunkLoop, const int& maxThreadCount) {
			const int ndim = 3;                                     //三维
			const int nobs = static_cast<int>(chunkLoop.size());    //长度
			const int ncenters = maxThreadCount;                    //分区

			std::vector<double> matrix(ndim * nobs);



			node.resize(maxThreadCount);

			//将glm::dvec3数组转换为库要求的列优先矩阵格式 (ndim x nobs)
			for (int i = 0; i < nobs; ++i) {
				matrix[i * ndim + 0] = chunkLoop[i].x;
				matrix[i * ndim + 1] = chunkLoop[i].y;
				matrix[i * ndim + 2] = chunkLoop[i].z;
			}

			//将矩阵转换为K-means算法需要的矩阵格式
			kmeans::SimpleMatrix<int, double> kmat(ndim, nobs, matrix.data());

			//执行聚类算法，使用kmeans++初始化和Lloyd算法优化
			kmeans::Results<int, int, double> res = kmeans::compute(kmat, kmeans::InitializeKmeanspp<int, double, int, double>(), kmeans::RefineLloyd<int, double, int, double>(), ncenters);

			for (int i = 0; i < nobs; ++i) {
				node[res.clusters[i]].push_back(chunkLoop[i]);
			}
		}
	};
}

//存储结构空间
namespace storageStruct {
	//保存文件结点，自动创建父路径并打开文件
	struct saveFileNode : std::ofstream {
		saveFileNode(const std::filesystem::path& path) {
			std::filesystem::create_directories(path.parent_path());    //如果当前文件父路径不存在则创建，如果存在则不操作
			open(path, std::ios::binary);                               //打开文件
		}



		//写入单数据
		template<typename T>
		void writeData(const T& data) {
			write(reinterpret_cast<const char*>(&data), sizeof(T));
		}

		//写入数据块
		template<typename T>
		void writeData(const T* data, std::streamsize size) {
			write(reinterpret_cast<const char*>(data), size);
		}
	};

	//读取文件结点
	struct readFileNode : std::ifstream {
		readFileNode(const std::filesystem::path& path) {
			open(path, std::ios::binary);                               //打开文件
		}



		//读取单数据
		template<typename T>
		void readData(T& data) {
			read(reinterpret_cast<char*>(&data), sizeof(T));
		}

		//读取数据块
		template<typename T>
		void readData(T* data, std::streamsize size) {
			read(reinterpret_cast<char*>(data), size);
		}

		//返回读取数据
		template<typename T>
		T readValue() {
			T value{};
			read(reinterpret_cast<char*>(&value), sizeof(T));
			return value;
		}
	};
}



//玩家结构空间
namespace playerStruct {
	//摄像机结点
	struct cameraNode {
		glm::mat4 view;                         //视图
		glm::quat rota;                         //角度
		glm::dvec3 pos;                         //逻辑坐标,计算时必须用glm::floor来偏移出正确结果,不偏移的话xyz任意一条为0时其上下两部分为0.5和-0.5,此时使用(int)强转结果都为0
		glm::vec3 front;                        //前向向量
	};

	//方块选择框结点
	struct selectBoxNode {
		glm::vec3 minepos;                     //挖掘坐标
		float isVisible = 0;                   //挖掘框是否可见
	};

	//方块选择框踪迹结点
	struct selectBoxTraceNode {
		glm::dvec3 initPos;	        //起始坐标（d世界坐标系）
		glm::dvec3 chunkPos;        //区块坐标（区块坐标系）
		glm::ivec3 cubePos;         //方块坐标（方块坐标系）
	};
}

//gameDebug结构空间
namespace gameDebugStruct {
	//临界方块检测结构体
	struct extremeCubeNode {
		glm::dvec3 pos;     //方块坐标
		short oldId;        //本方块id
		short newId;        //临方块id



		struct Hash {
			size_t operator()(const extremeCubeNode& node) const {
				size_t h = std::hash<glm::dvec3>()(node.pos);
				h ^= std::hash<short>()(node.oldId) + 0x9e3779b9 + (h << 6) + (h >> 2);
				h ^= std::hash<short>()(node.newId) + 0x9e3779b9 + (h << 6) + (h >> 2);
				return h;
			}
		};

		bool operator==(const extremeCubeNode& other) const {
			return pos == other.pos && oldId == other.oldId && newId == other.newId;
		}
	}; 
}