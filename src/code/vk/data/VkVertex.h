#pragma once



//pipeTerrain
namespace terrainVertex {
    struct vertexNode {
        glm::vec3 position;     //坐标
        glm::vec3 texCoord;     //uv，xy用于循环平铺，z用于纹理数组索引
        glm::vec3 normal;       //法线

        static std::array<VkVertexInputBindingDescription, 1> getBindingDescription() {
            std::array<VkVertexInputBindingDescription, 1> bindingDescription = {};
            bindingDescription[0].binding = 0;
            bindingDescription[0].stride = sizeof(vertexNode);
            bindingDescription[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return bindingDescription;
        }

        static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
            std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};

            attributeDescriptions[0].binding  = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[0].offset   = offsetof(vertexNode, position);

            attributeDescriptions[1].binding  = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[1].offset   = offsetof(vertexNode, texCoord);

            attributeDescriptions[2].binding  = 0;
            attributeDescriptions[2].location = 2;
            attributeDescriptions[2].format   = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[2].offset   = offsetof(vertexNode, normal);

            return attributeDescriptions;
        }
    };

    std::vector<vertexNode> vertexs[plateEnum::terrainLayer::count];  //顶点数组
    std::vector<uint32_t> indexs[plateEnum::terrainLayer::count];     //索引数组
}

//model
namespace modelVertex {
    struct vertexNode {
        glm::vec3 position;     //坐标
        glm::vec2 texCoord;     //uv
        glm::vec3 normal;       //法线

        static std::array<VkVertexInputBindingDescription, 1> getBindingDescription() {
            std::array<VkVertexInputBindingDescription, 1> bindingDescription = {};
            bindingDescription[0].binding = 0;
            bindingDescription[0].stride = sizeof(vertexNode);
            bindingDescription[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return bindingDescription;
        }

        static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
            std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};

            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(vertexNode, position);

            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(vertexNode, texCoord);

            attributeDescriptions[2].binding = 0;
            attributeDescriptions[2].location = 2;
            attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[2].offset = offsetof(vertexNode, normal);

            return attributeDescriptions;
        }

        //服务loadModel
        bool operator==(const vertexNode& other) const {
            return position == other.position && texCoord == other.texCoord && normal == other.normal;
        }
    };

    //静态模型网格
    std::vector<vertexNode> sVertexs[bufferEnum::procStaticModelId::count];
    std::vector<uint32_t>   sIndexs[bufferEnum::procStaticModelId::count];

    //动态模型网格
    std::vector<std::vector<vertexNode>> dVertexs[bufferEnum::procDynamicModelId::count];
    std::vector<std::vector<uint32_t>>   dIndexs[bufferEnum::procDynamicModelId::count];
}

//服务于model加载
namespace std {
    template<> struct hash<modelVertex::vertexNode> {
        size_t operator()(modelVertex::vertexNode const& vertex) const {
            return (hash<glm::vec3>()(vertex.position) ^ (hash<glm::vec3>()(vertex.normal) << 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 2));
        }
    };
}

//pipeSkybox
namespace skyboxVertex {
    struct vertexNode {
        glm::vec3 position;     //坐标

        static std::array<VkVertexInputBindingDescription, 1> getBindingDescription() {
            std::array<VkVertexInputBindingDescription, 1> bindingDescription = {};
            bindingDescription[0].binding = 0;
            bindingDescription[0].stride = sizeof(vertexNode);
            bindingDescription[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return bindingDescription;
        }

        static std::array<VkVertexInputAttributeDescription, 1> getAttributeDescriptions() {
            std::array<VkVertexInputAttributeDescription, 1> attributeDescriptions = {};

            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(vertexNode, position);

            return attributeDescriptions;
        }
    };

    //顶点数组
    std::vector<vertexNode> vertexs = {
        {glm::vec3{ 16.0f, 16.0f,-16.0f}},
        {glm::vec3{-16.0f, 16.0f,-16.0f}},
        {glm::vec3{-16.0f,-16.0f,-16.0f}},
        {glm::vec3{ 16.0f,-16.0f,-16.0f}},
                    
        {glm::vec3{ 16.0f, 16.0f, 16.0f}},
        {glm::vec3{-16.0f, 16.0f, 16.0f}},
        {glm::vec3{-16.0f,-16.0f, 16.0f}},
        {glm::vec3{ 16.0f,-16.0f, 16.0f}}
    };

    //索引数组
    std::vector<uint32_t> indexs = {
        1,4,5,0,4,1,
        6,3,2,7,3,6,
        3,4,0,7,4,3,
        6,1,5,2,1,6,
        7,5,4,6,5,7,
        2,0,1,3,0,2,
    };
}

//pipeMine
namespace mineVertex {
    struct vertexNode {
        glm::vec3 position;     //坐标

        static std::array<VkVertexInputBindingDescription, 1> getBindingDescription() {
            std::array<VkVertexInputBindingDescription, 1> bindingDescription = {};
            bindingDescription[0].binding   = 0;
            bindingDescription[0].stride    = sizeof(vertexNode);
            bindingDescription[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return bindingDescription;
        }

        static std::array<VkVertexInputAttributeDescription, 1> getAttributeDescriptions() {
            std::array<VkVertexInputAttributeDescription, 1> attributeDescriptions = {};
            attributeDescriptions[0].binding  = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[0].offset   = offsetof(vertexNode, position);

            return attributeDescriptions;
        }
    };

    //顶点数组
    std::vector<vertexNode> vertexs = { 
        {vertexInfo::vertexID[0]},
        {vertexInfo::vertexID[1]},
        {vertexInfo::vertexID[2]},
        {vertexInfo::vertexID[3]},

        {vertexInfo::vertexID[4]},
        {vertexInfo::vertexID[5]},
        {vertexInfo::vertexID[6]},
        {vertexInfo::vertexID[7]}
    };

    //索引数组 面
    /*std::vector<uint32_t> indexs = {
        5,4,1,1,4,0,
        2,3,6,6,3,7,
        0,4,3,3,4,7,
        5,1,6,6,1,2,
        4,5,7,7,5,6,
        1,0,2,2,0,3
    };*/

    //索引数组 线
    std::vector<uint32_t> indexs = {
        0,1,1,5,5,4,4,0,
        6,2,2,3,3,7,7,6,
        7,3,3,0,0,4,4,7,
        1,2,2,6,6,5,5,1,
        2,3,3,0,0,1,1,2,
        4,5,5,6,6,7,7,4
    };
}

//pipeUIText
namespace uiTextVertex {
    struct vertexNode {
        //绑定描述
        static std::array<VkVertexInputBindingDescription, 2> getBindingDescription() {
            std::array<VkVertexInputBindingDescription, 2> vertexInputBindings = {};

            vertexInputBindings[0] = {};
            vertexInputBindings[0].binding = 0;
            vertexInputBindings[0].stride = sizeof(glm::vec4);
            vertexInputBindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            vertexInputBindings[1] = {};
            vertexInputBindings[1].binding = 1;
            vertexInputBindings[1].stride = sizeof(glm::vec4);
            vertexInputBindings[1].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return vertexInputBindings;
        }

        //属性描述
        static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
            std::array<VkVertexInputAttributeDescription, 2> vertexInputAttributes = {};

            vertexInputAttributes[0].location = 0;
            vertexInputAttributes[0].binding = 0;
            vertexInputAttributes[0].format = VK_FORMAT_R32G32_SFLOAT;
            vertexInputAttributes[0].offset = 0;

            vertexInputAttributes[1].location = 1;
            vertexInputAttributes[1].binding = 1;
            vertexInputAttributes[1].format = VK_FORMAT_R32G32_SFLOAT;
            vertexInputAttributes[1].offset = sizeof(glm::vec2);

            return vertexInputAttributes;
        }
    };
}

//pipeOSMirror
namespace osMirrorVertex {
    struct vertexNode {
        glm::vec3 position;

        static std::array<VkVertexInputBindingDescription, 1> getBindingDescription() {
            std::array<VkVertexInputBindingDescription, 1> bindingDescription = {};
            bindingDescription[0].binding   = 0;
            bindingDescription[0].stride    = sizeof(vertexNode);
            bindingDescription[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return bindingDescription;
        }

        static std::array<VkVertexInputAttributeDescription, 1> getAttributeDescriptions() {
            std::array<VkVertexInputAttributeDescription, 1> attributeDescriptions = {};

            attributeDescriptions[0].binding  = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[0].offset   = offsetof(vertexNode, position);

            return attributeDescriptions;
        }
    };

    //顶点数组
    std::vector<vertexNode> vertexs = {
        {vertexInfo::vertexID[0]},
        {vertexInfo::vertexID[1]},
        {vertexInfo::vertexID[2]},
        {vertexInfo::vertexID[3]},

        {vertexInfo::vertexID[4]},
        {vertexInfo::vertexID[5]},
        {vertexInfo::vertexID[6]},
        {vertexInfo::vertexID[7]}
    };

    //索引数组 面
    std::vector<uint32_t> indexs = {
        4,5,7,7,5,6,
    };
}

//offScreen
namespace offScreenVertex {
    struct vertexNode {
        static std::vector<VkVertexInputBindingDescription> getBindingDescription() {
            return {};
        }

        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
            return {};
        }
    };
}

//shadow
namespace shadowVertex {
    struct vertexNode {
        glm::vec3 position;     //坐标

        static std::array<VkVertexInputBindingDescription, 1> getBindingDescription() {
            std::array<VkVertexInputBindingDescription, 1> bindingDescription = {};
            bindingDescription[0].binding   = 0;
            bindingDescription[0].stride    = sizeof(vertexNode);
            bindingDescription[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return bindingDescription;
        }

        static std::array<VkVertexInputAttributeDescription, 1> getAttributeDescriptions() {
            std::array<VkVertexInputAttributeDescription, 1> attributeDescriptions = {};
            attributeDescriptions[0].binding  = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[0].offset   = offsetof(vertexNode, position);

            return attributeDescriptions;
        }
    };

    //地形网格
    std::vector<vertexNode> tVertexs[plateEnum::terrainLayer::count];

    //静态模型网格
    std::vector<vertexNode> sVertexs[bufferEnum::procStaticModelId::count];

    //动态模型网格
    std::vector<std::vector<vertexNode>> dVertexs[bufferEnum::procDynamicModelId::count];



    void loadTerrainVertexs(int layer) {
        auto& sourceVerts = terrainVertex::vertexs[layer];
        auto& targetVerts = tVertexs[layer];

        const size_t vertCount = sourceVerts.size();
        targetVerts.clear();
        targetVerts.reserve(vertCount);

        for (size_t i = 0; i < vertCount; ++i) {
            targetVerts.push_back(vertexNode{ sourceVerts[i].position });
        }
    }
}