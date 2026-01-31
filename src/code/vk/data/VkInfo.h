#pragma once



namespace enableConfigInfo {
    //层启用，目前只适用于开启校验层
    std::vector<const char*> enableLayer = {
        "VK_LAYER_KHRONOS_validation"
    };

    //Instance扩展启用
    std::vector<const char*> enableInstanceExtension = {
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME
    };

    //Device扩展启用
    std::vector<const char*> enableDeviceExtension = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
}

namespace debugInfo {
#ifdef NDEBUG
    //const bool LayerSwitch = false;
    //const bool LogSwitch = false;

    const bool LayerSwitch = true;          //校验开关
    const bool LogSwitch = true;            //调试开关
#else
    const bool LayerSwitch = true;          //校验开关
    const bool LogSwitch = true;            //调试开关
#endif
}

namespace windowInfo {
    //Window:
    const uint32_t  width = 800;
    const uint32_t  height = 450;

    const char* engineName = "ForgeEngine";
    const char* gameName = "Forge";

    //const bool resizableEnabled = true;
    const bool resizableEnabled = false;    //窗口拉伸启用标识
}

namespace swapchainInfo {
    const VkExtent2D windowkExtent = { windowInfo::width, windowInfo::height };
}

namespace vertexInfo {
    //顶点索引
    const std::vector<glm::vec3> vertexID = {
        //X-----Y-----Z
        {1.0f, 1.0f, 0.0f}, //0
        {0.0f, 1.0f, 0.0f}, //1
        {0.0f, 0.0f, 0.0f}, //2
        {1.0f, 0.0f, 0.0f}, //3
        {1.0f, 1.0f, 1.0f}, //4
        {0.0f, 1.0f, 1.0f}, //5
        {0.0f, 0.0f, 1.0f}, //6
        {1.0f, 0.0f, 1.0f } //7
    };

    //uv索引
    const std::vector<glm::vec2> uvID = {
        { 0.0f, 0.0f },   //0
        { 1.0f, 0.0f },   //1
        { 0.0f, 1.0f },   //2
        { 1.0f, 1.0f }    //3
    };

    //法线索引
    const std::vector<glm::vec3> normalID = {
        { 0.0f, 1.0f,  0.0f }, //0上
        { 0.0f,-1.0f,  0.0f }, //1下
        {-1.0f, 0.0f,  0.0f }, //2左
        { 1.0f, 0.0f,  0.0f }, //3右
        { 0.0f, 0.0f,  1.0f }, //4前
        { 0.0f, 0.0f, -1.0f }  //5后
    };
}

namespace coordsInfo {
    const glm::vec3 worldUp{ 0.0, 1.0, 0.0 };    //y轴做世界坐标系法向量
}

namespace lightInfo {
    const glm::vec3 sunLightColor = glm::vec3(1.0f, 1.0f, 1.0f);    //太阳平行光颜色
    const glm::vec3 sunLightDir   = glm::vec3(0.0f,-1.0f, 0.0f);    //太阳平行光方向
    const float ambientScale      = 0.4f;                           //环境光缩放
    const float diffuseScale      = 0.8f;                           //漫反射缩放
    const float specularScale     = 1.0f;                           //镜面反射缩放
}

namespace bufferInfo {
    const int commandThreadCount = 2;           //命令线程数

    const uint32_t scenceForwardCount  = 4;     //scenceForward描述符池上限，  skybox，mine，osMirror，osScence
    const uint32_t scenceDeferredCount = 5;     //scenceDeferred描述符池上限， terrain，m_temple，m_droid*3
    const uint32_t uiTextCount         = 1;     //uiText描述符池上限，         uiText
    const uint32_t dsScenceCount       = 1;     //dsScence描述符池上限，       dsScence
    const uint32_t shadowCount         = 6;     //shadow描述符池上限，         terrain*2，m_temple，m_droid*3

    VkFormat dsPosFormat = VK_FORMAT_R32G32B32A32_SFLOAT;

    const int shadowMapCount  = 3;              //阴影映射数量，三大光源阴影贴图数量和，一个点光源要6张贴图，平行光和聚光灯要一张贴图
}

namespace fpsInfo {
    const double getKeyFPS = 1024;	//获取按键刷新率
    const double cubeFPS = 128;	    //方块刷新率,用于限制由放置破坏方块导致的重建命令缓冲区
    const double uiTextFPS = 2;		//UIText刷新率
}

namespace fileInfo {
    namespace shader {
        //const std::string terrainVert = "src/shader/terrain/terrainForwardVert.spv";
        //const std::string terrainFrag = "src/shader/terrain/terrainForwardFrag.spv";

        const std::string terrainVert = "src/shader/terrain/terrainDeferredVert.spv";
        const std::string terrainFrag = "src/shader/terrain/terrainDeferredFrag.spv";

        //const std::string pbrVert = "src/shader/model/pbrForwardVert.spv";
        //const std::string pbrFrag = "src/shader/model/pbrForwardFrag.spv";

        const std::string pbrVert = "src/shader/model/pbrDeferredVert.spv";
        const std::string pbrFrag = "src/shader/model/pbrDeferredFrag.spv";

        const std::string skyboxVert = "src/shader/skybox/skyboxVert.spv";
        const std::string skyboxFrag = "src/shader/skybox/skyboxFrag.spv";

        const std::string mineVert = "src/shader/mine/mineVert.spv";
        const std::string mineFrag = "src/shader/mine/mineFrag.spv";

        const std::string uitextVert = "src/shader/uiText/uitextVert.spv";
        const std::string uitextFrag = "src/shader/uiText/uitextFrag.spv";

        const std::string osMirrorVert = "src/shader/osMirror/osMirrorVert.spv";
        const std::string osMirrorFrag = "src/shader/osMirror/osMirrorFrag.spv";

        const std::string osScenceVert = "src/shader/osScence/osScenceVert.spv";
        const std::string osScenceFrag = "src/shader/osScence/osScenceFrag.spv";

        const std::string dsScenceVert = "src/shader/dsScence/dsScenceVert.spv";
        const std::string dsScenceFrag = "src/shader/dsScence/dsScenceFrag.spv";

        const std::string shadowVert = "src/shader/shadow/shadowVert.spv";
        const std::string shadowGeom = "src/shader/shadow/shadowGeom.spv";
    }

    namespace music {
        const char* throughTheMounta = "src/music/background/Through the Mounta.wav";
        //const char* throughTheMounta = "src/music/background/Greedy.wav";
    }

    namespace model {
        const std::string temple  = "src/model/temple/scene.gltf";
        const std::string droid   = "src/model/droid/scene.gltf";
        //const std::string nextTemp = "src/model/nextTemp/scene.gltf";
    }

    namespace image {
        const char* cube       = "src/image/terrain/cube_16_64.ktx2";

        const char* temple     = "src/image/model/temple/baseColor.ktx2";
        const char* droidBodyC = "src/image/model/droid/Body_baseColor.ktx2";
        const char* droidHeadC = "src/image/model/droid/Head_baseColor.ktx2";
        const char* droidMiscC = "src/image/model/droid/Misc_baseColor.ktx2";
        //const char* droidBodyN = "src/image/model/droid/Body_normal.ktx2";
        //const char* droidHeadN = "src/image/model/droid/Head_normal.ktx2";
        //const char* droidMiscN = "src/image/model/droid/Misc_normal.ktx2";
    }

    namespace storage {
        const char* fullFile = "storage/chunk/full";                        //全存法区块存储路径
        const char* deltaFile = "storage/chunk/delta";                      //变存法区块存储路径
    }

    const char* dll = ".\\src\\library\\dll";
}

namespace threadCommInfo {
    bool plateLock = false;        //板块线程锁
    bool musicLock = false;        //音乐线程锁
}