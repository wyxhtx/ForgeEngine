#pragma once



namespace cubeStruct {
    //贪心算法使用范围（1~32）遍历节点
    struct traversalNode {
        int max = 1;                            //最短1
        int min = plateInfo::chunkLength;       //最长32
    };

    //面朝向结点
    struct faceNode {
        int x;
        int y;
    };

    //方块属性结点
    struct propertyNode {
        std::vector<faceNode> faceId;   //纹理id
        short coverage;                 //方块覆盖顺序

        glm::vec3 ambient;              //材质环境光
        glm::vec3 diffuse;              //材质漫反射
        glm::vec3 specular;             //材质镜面反射
        float shininess;                //材质反光度
    };
}

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

    //结构管理器结点
    struct structManagerNode {
        std::unordered_map<glm::dvec3, std::unordered_set<glm::ivec4>> node;
    };

    //结构分区管理器结点
    struct structPartManagerNode {
        std::unordered_map<glm::dvec3, std::unordered_set<glm::ivec4>> pending;     //未加载
        std::unordered_map<glm::dvec3, std::unordered_set<glm::ivec4>> active;      //已加载
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

//地形结构结构体
namespace terrainStructureStruct {
    //树结构结点
    struct treeStructureNode {
        int r;          //树冠半径
        int length;     //树干高度
        int leaf;       //树叶
        int trunk;      //树干
    };
}

namespace noiseStruct {
    //噪音起伏undulation，噪音平移translation，噪音生成密度density，
    //density，应大于等于translation - undulation，小于等于translation + undulation - 2
    //density，从translation - undulation到translation + undulation - 2递减，中间值位于translation - 1
    //density，满足上述条件，改变u的值对结果没有影响，如1u，16t，15d和2u，16t，15d和4u，16t，15d的结果都是一样的

    //地形噪音参数结点
    struct terrainParamNode {
        int undulation;		    //起伏
        int translation;	    //平移
    };

    //树噪音参数结点
    struct treeParamNode {
        int undulation;		    //起伏
        int translation;	    //平移
        int density;	        //密度
    };
}

namespace storageStruct {
    //变存法存储结点，存储玩家变更方块的方块数组坐标和方块id
    struct cubeDeltaNode {
        short data[4];

        cubeDeltaNode(short x = 0, short y = 0, short z = 0, short id = cubeEnum::cubeId::air) {
            data[0] = x; data[1] = y; data[2] = z; data[3] = id;
        }

        bool operator==(const cubeDeltaNode& node) const {
            return data[0] == node.data[0] && data[1] == node.data[1] && data[2] == node.data[2] && data[3] == node.data[3];
        }
    };

    struct cubeDeltaNodeHash {
        std::size_t operator()(const cubeDeltaNode& node) const {
            std::size_t seed = 0;

            seed ^= std::hash<short>{}(node.data[0]) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= std::hash<short>{}(node.data[1]) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= std::hash<short>{}(node.data[2]) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= std::hash<short>{}(node.data[3]) + 0x9e3779b9 + (seed << 6) + (seed >> 2);

            return seed;
        }
    };
}

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
}