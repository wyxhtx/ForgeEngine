#pragma once



namespace plateStruct {
    //板块网格标识结点
    struct plateMeshSignNode {
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
}

namespace terrainStruct {
    struct treeNode {
        int r;
        int length;
    };
}

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
        std::vector<faceNode> faceId;       //纹理id
        glm::vec3 light;                    //光照属性

        short hardness;                     //硬度，决定结构覆盖顺序

        //todo 暂时无用
        //bool isTransparent;               //是否透明，决定加载管线
    };
}

namespace noiseStruct {
    //噪音起伏undulation，噪音平移translation，噪音生成密度density，
    //density，应大于等于translation - undulation，小于等于translation + undulation - 2
    //density，从translation - undulation到translation + undulation - 2递减，中间值位于translation - 1
    //density，满足上述条件，改变u的值对结果没有影响，如1u，16t，15d和2u，16t，15d和4u，16t，15d的结果都是一样的

    struct treeNode {
        int undulation;		    //起伏
        int translation;	    //平移
        int density;	        //密度
    };
}

namespace storageStruct {
    //变存法存储结点
    //存储玩家变更方块的数组坐标和方块id
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
    struct CameraNode {
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