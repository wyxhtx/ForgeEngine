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
    const bool LayerSwitch = false;
    const bool LogSwitch = false;
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
    const VkExtent2D windowSize = { windowInfo::width, windowInfo::height };
    const int MAX_FRAMES_IN_FLIGHT = 3;             //同时并行处理的帧数，需跟Swapchain.swapChainImages.size()相同
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
    glm::vec3 globalLight = glm::vec3(1.0f, 1.0f, 1.0f);    //全局光照
    float globaSpecular = 0.8f;                             //全局界面反射
}

namespace bufferInfo {
    uint32_t bufferScenceNodeSize = 4;          //terrain,model,skybox,mine
    uint32_t bufferTextUINodeSize = 1;          //uiText

    const int commandThreadSize = 2;            //命令线程数
}

namespace fpsInfo {
    const double getKeyFPS = 1024;	//获取按键刷新率
    const double cubeFPS = 128;	    //方块刷新率,用于限制由放置破坏方块导致的重建命令缓冲区
    const double uiTextFPS = 2;		//UIText刷新率
}

namespace fileInfo {
    namespace shader {
        static std::string terrainVert = "src/shader/terrain/terrainVert.spv";
        static std::string terrainFrag = "src/shader/terrain/terrainFrag.spv";

        static std::string modelVert = "src/shader/model/modelVert.spv";
        static std::string modelFrag = "src/shader/model/modelFrag.spv";

        static std::string skyboxVert = "src/shader/skybox/skyboxVert.spv";
        static std::string skyboxFrag = "src/shader/skybox/skyboxFrag.spv";

        static std::string mineVert = "src/shader/mine/mineVert.spv";
        static std::string mineFrag = "src/shader/mine/mineFrag.spv";

        static std::string uitextVert = "src/shader/uiText/uitextVert.spv";
        static std::string uitextFrag = "src/shader/uiText/uitextFrag.spv";
    }

    namespace music {
        const char* throughTheMounta = "src/music/background/Through the Mounta.wav";
        //const char* throughTheMounta = "src/music/background/Greedy.wav";
    }

    namespace model {
        static std::string temple = "src/model/temple.obj";
    }

    namespace image {
        const char* cube = "src/image/cube_16_64.ktx2";				//需要同步改terrain.frag里的纹理边长
        const char* temple = "src/image/temple.ktx2";
    }

    namespace storage {
        const char* fullFile = "storage/chunk/full";                        //全存法区块存储路径
        const char* deltaFile = "storage/chunk/delta";                      //变存法区块存储路径
    }

    const char* dll = ".\\src\\library\\dll";
}

namespace threadCommInfo {
    //一个在Swapchain的drawFrame，一个在Buffer的copyBuffer，因为改成单线程重建命令缓冲区，现已弃用
    //std::mutex queueMutex;          //多线程命令缓冲区的重建导致多线程访问渲染队列，使用互斥锁进行同步

    bool plateLock = false;        //板块线程锁
    bool musicLock = false;        //音乐线程锁
}