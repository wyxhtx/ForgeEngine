#pragma once



//engine信息
namespace engineInfo {
    //层启用，目前只适用于开启校验层
    std::vector<const char*> enableLayerList = {
        "VK_LAYER_KHRONOS_validation"
    };

    //Instance扩展启用
    std::vector<const char*> enableInstanceExtensionList = {
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME
    };

    //Device扩展启用
    std::vector<const char*> enableDeviceExtensionList = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_EXT_MESH_SHADER_EXTENSION_NAME
    };
}

//debug信息，全局
namespace debugInfo {
#ifdef NDEBUG
    //const bool LayerSwitch = false;
    //const bool LogSwitch = false;

    const bool LayerSwitch = true;          //校验开关
    const bool LogSwitch   = true;          //调试开关
#else
    const bool LayerSwitch = true;          //校验开关
    const bool LogSwitch   = true;          //调试开关
#endif
}

//window信息
namespace windowInfo {
    const uint32_t  width  = 800;
    const uint32_t  height = 450;

    const char* engineName = "ForgeEngine";
    const char* gameName   = "Forge";

    //const bool resizableEnabled = true;
    const bool resizableEnabled = false;    //窗口拉伸启用标识
}

//lock信息
namespace lockInfo {
    //原子线程锁
    std::mutex queueThreadLock;                         //队列线程锁，用于原子同步graphicsQueue队列操作
    std::mutex logThreadLock;                           //日志线程锁，用于原子同步各线程调用日志
    std::atomic<bool> musicThreadLock = false;          //音乐线程锁，用于原子同步音乐线程

    //原子函数锁
    std::mutex drawFuncLock;                            //用于避免InitSwapchain.drawFrame()和InitBuffer.recreateCmd()的threadId = (threadId + 1) % 2重建同时执行
    std::atomic<bool> updatePlateFuncLock = false;      //用于实现 submitUpdatePlate 函数原子线程化
    std::atomic<bool> updateCubeFuncLock  = false;      //用于实现 submitUpdateCube  函数原子线程化
    std::atomic<bool> recreateCmdFuncLock = false;      //用于实现 submitRecreateCmd 函数原子线程化

    //原子数据锁
    std::atomic<bool> plateMeshDataLock = false;        //板块网格数据锁，用于原子同步对板块网格的修改操作
    std::atomic<bool> chunkMapDataLock  = false;        //区块映射数据锁，用于原子同步对区块映射的修改操作

    //原子启用锁
    std::atomic<bool> logPauseEnableLock = false;       //日志暂停启用锁，用于校验层报错时暂停



    //获取锁值
    //如果 lock == false，设置 lock = true，返回 true
    //如果 lock == true， 返回 false
    bool getLock(std::atomic<bool>& lock) {
        bool isLock = false;
        return lock.compare_exchange_strong(isLock, true);
    }
}



//vertex信息
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

//light信息
namespace lightInfo {
    const glm::vec3 sunLightColor = glm::vec3(1.0f, 1.0f, 1.0f);    //太阳平行光颜色
    const glm::vec3 sunLightDir   = glm::vec3(0.0f,-1.0f, 0.0f);    //太阳平行光方向
    const float ambientScale      = 0.8f;                           //环境光缩放
    const float diffuseScale      = 0.8f;                           //漫反射缩放
    const float specularScale     = 1.0f;                           //镜面反射缩放
}

//camera信息
namespace cameraInfo {
    const glm::vec3 worldUp{ 0.0, 1.0, 0.0 };    //y轴做世界坐标系法向量
}

//todo 实现对应资产的固定根路径
//file信息
namespace fileInfo {
    namespace shader {
        const std::string terrainTask = "src/assets/0_shader/terrain/terrainDeferredTask.spv";
        const std::string terrainMesh = "src/assets/0_shader/terrain/terrainDeferredMesh.spv";
        const std::string terrainVert = "src/assets/0_shader/terrain/terrainDeferredVert.spv";
        const std::string terrainFrag = "src/assets/0_shader/terrain/terrainDeferredFrag.spv";

        const std::string pbrVert     = "src/assets/0_shader/model/pbrDeferredVert.spv";
        const std::string pbrFrag     = "src/assets/0_shader/model/pbrDeferredFrag.spv";

        const std::string skyboxVert  = "src/assets/0_shader/skybox/skyboxVert.spv";
        const std::string skyboxFrag  = "src/assets/0_shader/skybox/skyboxFrag.spv";

        const std::string mineVert    = "src/assets/0_shader/mine/mineVert.spv";
        const std::string mineFrag    = "src/assets/0_shader/mine/mineFrag.spv";

        const std::string uitextVert  = "src/assets/0_shader/uiText/uitextVert.spv";
        const std::string uitextFrag  = "src/assets/0_shader/uiText/uitextFrag.spv";

        const std::string osMirrorVert = "src/assets/0_shader/osMirror/osMirrorVert.spv";
        const std::string osMirrorFrag = "src/assets/0_shader/osMirror/osMirrorFrag.spv";

        const std::string osScenceVert = "src/assets/0_shader/osScence/osScenceVert.spv";
        const std::string osScenceFrag = "src/assets/0_shader/osScence/osScenceFrag.spv";

        const std::string dsScenceVert = "src/assets/0_shader/dsScence/dsScenceVert.spv";
        const std::string dsScenceFrag = "src/assets/0_shader/dsScence/dsScenceFrag.spv";

        const std::string shadowVert   = "src/assets/0_shader/shadow/shadowVert.spv";
        const std::string shadowFrag   = "src/assets/0_shader/shadow/shadowFrag.spv";
    }

    namespace image {
        //todo 待整合，着色器，贴图，贴图信息
        const char* cube       = "src/assets/1_image/terrain/cube_16_64.ktx2";

        const char* temple     = "src/assets/1_image/model/temple/baseColor.ktx2";
        const char* droidBodyC = "src/assets/1_image/model/droid/Body_baseColor.ktx2";
        const char* droidHeadC = "src/assets/1_image/model/droid/Head_baseColor.ktx2";
        const char* droidMiscC = "src/assets/1_image/model/droid/Misc_baseColor.ktx2";
        //const char* droidBodyN = "src/assets/1_image/model/droid/Body_normal.ktx2";
        //const char* droidHeadN = "src/assets/1_image/model/droid/Head_normal.ktx2";
        //const char* droidMiscN = "src/assets/1_image/model/droid/Misc_normal.ktx2";
    }

    namespace model {
        const std::string temple = "src/assets/2_model/temple/scene.gltf";
        const std::string droid  = "src/assets/2_model/droid/scene.gltf";
        //const std::string nextTemp = "src/assets/2_model/nextTemp/scene.gltf";
    }

    namespace music {
        const char* throughTheMounta = "src/assets/3_music/background/Through the Mounta.wav";
        //const char* throughTheMounta = "src/music/background/Greedy.wav";
    }

    namespace storage {
        const char* fullFile  = "storage/chunk/full";       //全存法区块存储路径
        const char* deltaFile = "storage/chunk/delta";      //变存法区块存储路径
    }



}

//texture信息
namespace textureInfo {
    namespace terrain {
        //todo 待删除
	    const int tileWidth   = 16;							    //纹理宽度
	    const int tileHeight  = 16;							    //纹理高度
	    const int atlasWidth  = 64;							    //图集宽几个纹理
	    const int atlasHeight = 32;							    //图集高几个纹理
	    const uint32_t arrayLayer = atlasWidth * atlasHeight;   //图集层数

        //todo
        const VkFormat   colorFormat = VK_FORMAT_R8G8B8A8_UNORM;    //颜色格式
        const VkExtent2D tileExtent  = { 16, 16 };                  //纹理尺寸
        const VkExtent2D atlasExtent = { 64, 32 };                  //图集尺寸
    }
}



//render信息
namespace renderInfo {
    const VkFormat   dsPosColorFormat = VK_FORMAT_R32G32B32A32_SFLOAT;      //延迟渲染坐标颜色附件格式
    const VkExtent2D shadowMapExtent  = { 2048, 2048 };                     //阴影映射尺寸

    const VkClearValue colorClearValue = { .color = {105.0f / 255.f, 105.0f / 255.f, 105.0f / 255.f, 1.0f } };      //颜色清理值
    const VkClearValue depthClearValue = { .depthStencil = { 1.0f, 0u } };                                          //深度清理值
}

//buffer信息
namespace bufferInfo {
    const int commandThreadCount = 2;           //命令线程数

    const uint32_t scenceForwardCount  = 4;     //scenceForward描述符池上限，  skybox，mine，osMirror，osScence
    const uint32_t scenceDeferredCount = 5;     //scenceDeferred描述符池上限， terrain，m_temple，m_droid*3
    const uint32_t uiTextCount         = 1;     //uiText描述符池上限，         uiText
    const uint32_t dsScenceCount       = 1;     //dsScence描述符池上限，       dsScence
    const uint32_t shadowCount         = 6;     //shadow描述符池上限，         terrain*2，m_temple，m_droid*3
}