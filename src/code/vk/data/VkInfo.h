#pragma once



//engine	信息
namespace engineInfo {
	const bool ktxCacheSwitch  = true;		//ktx缓存开关		（开：首次启动转码并缓存，之后命中缓存；关：每次启动运行期转码 ⇒ 不产生缓存）
	const bool msdfCacheSwitch = true;		//MSDF图集缓存开关	（开：首次烘焙落盘，之后命中缓存；关：每次启动都烘焙 ⇒ 不产生缓存）
}

//debug		信息，全局
namespace debugInfo {
#ifndef FORGE_DEV_BUILD
	const bool LayerSwitch = false;			//校验开关
	const bool LogSwitch   = false;			//调试开关
#else
	const bool LayerSwitch = true;			//校验开关
	const bool LogSwitch   = true;			//调试开关
#endif
}

//window	信息
namespace windowInfo {
	const uint32_t  width  = 800;
	const uint32_t  height = 450;

	const char* engineName = "ForgeEngine";
	const char* gameName   = "Forge";

	//const bool resizableEnabled = true;
	const bool resizableEnabled = false;								//窗口拉伸启用标识														
}

//lock		信息
namespace lockInfo {
	//原子线程锁
	std::mutex queueThreadLock;											//队列线程锁，用于原子同步graphicsQueue队列操作							
	std::mutex logThreadLock;											//日志线程锁，用于原子同步各线程调用日志								
	std::atomic<bool> musicThreadLock = false;							//音乐线程锁，用于原子同步音乐线程										

	//原子函数锁
	std::mutex drawFuncLock;											//用于避免InitSwapchain.drawFrame()和InitBuffer.recreateCmd()的threadId	= (threadId + 1) % 2重建同时执行
	std::atomic<bool> updatePlateFuncLock = false;						//用于实现 submitUpdatePlate 函数原子线程化
	std::atomic<bool> updateCubeFuncLock  = false;						//用于实现 submitUpdateCube  函数原子线程化
	std::atomic<bool> recreateCmdFuncLock = false;						//用于实现 submitRecreateCmd 函数原子线程化

	//原子数据锁
	std::atomic<bool> plateMeshDataLock = false;						//板块网格数据锁，用于原子同步对板块网格的修改操作						
	std::atomic<bool> chunkMapDataLock  = false;						//区块映射数据锁，用于原子同步对区块映射的修改操作						

	//原子启用锁
	std::atomic<bool> logPauseEnableLock = false;						//日志暂停启用锁，用于校验层报错时暂停									



	//获取锁值
	//如果 lock == false，设置 lock = true，返回 true
	//如果 lock == true， 返回 false
	bool getLock(std::atomic<bool>& lock) {
		bool isLock = false;
		return lock.compare_exchange_strong(isLock, true);
	}
}



//vertex	信息
namespace vertexInfo {
	//顶点索引
	const std::vector<glm::vec3> vertexID = {
		//X-----Y-----Z
		{1.0f, 1.0f, 0.0f},			//0																		
		{0.0f, 1.0f, 0.0f},			//1																		
		{0.0f, 0.0f, 0.0f},			//2																		
		{1.0f, 0.0f, 0.0f},			//3																		
		{1.0f, 1.0f, 1.0f},			//4																		
		{0.0f, 1.0f, 1.0f},			//5																		
		{0.0f, 0.0f, 1.0f},			//6																		
		{1.0f, 0.0f, 1.0f }			//7																		
	};

	//uv索引
	const std::vector<glm::vec2> uvID = {
		{ 0.0f, 0.0f },				//0																		
		{ 1.0f, 0.0f },				//1																		
		{ 0.0f, 1.0f },				//2																		
		{ 1.0f, 1.0f }				//3																		
	};

	//法线索引
	const std::vector<glm::vec3> normalID = {
		{ 0.0f, 1.0f,  0.0f },		//0上																	
		{ 0.0f,-1.0f,  0.0f },		//1下																	
		{-1.0f, 0.0f,  0.0f },		//2左																	
		{ 1.0f, 0.0f,  0.0f },		//3右																	
		{ 0.0f, 0.0f,  1.0f },		//4前																	
		{ 0.0f, 0.0f, -1.0f }		//5后																	
	};
}

//light		信息
namespace lightInfo {
	const glm::vec3 sunLightColor = glm::vec3(1.0f, 1.0f, 1.0f);		//太阳平行光颜色														
	const glm::vec3 sunLightDir   = glm::vec3(0.0f,-1.0f, 0.0f);		//太阳平行光方向														
	const float ambientScale      = 0.8f;								//环境光缩放															
	const float diffuseScale      = 0.8f;								//漫反射缩放															
	const float specularScale     = 1.0f;								//镜面反射缩放															
}

//camera	信息
namespace cameraInfo {
	const glm::vec3 worldUp{ 0.0, 1.0, 0.0 };							//y轴做世界坐标系法向量													
}

//file		信息
namespace fileInfo {
	namespace path {
		const std::string assetPath		= "src/assets/";				//全部资产路径	（构建时镜像到 exe 同级的同名相对路径）
		const std::string fontPath		= assetPath + "font/";
		const std::string shaderPath	= assetPath + "shader/";
		const std::string imagePath		= assetPath + "image/";
		const std::string modelPath		= assetPath + "model/";
		const std::string musicPath		= assetPath + "music/";

		const std::string gamePath		= "storage/game/";				//游戏存储路径	（chunk 存档、玩家配置；发布后与 exe 同级）
		const std::string chunkPath		= gamePath + "chunk/";			//区块存档路径
		const std::string localPath		= "storage/local/";				//运行产物路径	（缓存/日志/截图；删了无损失、不随包发布）
		const std::string cachePath		= localPath + "cache/";			//缓存路径		（管线缓存 pipeline.bin）
		const std::string ktxCachePath  = cachePath + "ktx/";			//ktx BC7 缓存	（运行期自建；命中即跳过，删了会重建）
		const std::string msdfCachePath = cachePath + "msdf/";			//MSDF 图集缓存	（运行期自建；命中即跳过，删了会重建）

		const std::string pipeCacheFile = cachePath + "pipeline.bin";	//管线缓存文件	（运行期自建；命中即跳过，删了会重建）
	}

	namespace font {
		const std::string sourceHanSansSC	  = path::fontPath + "SourceHanSansSC/OTF/SimplifiedChinese/SourceHanSansSC-Regular.otf";
		const std::string mapleMonoNormalNFCN = path::fontPath + "MapleMonoNormalNFCN/TTF/MapleMonoNormal-NFCN-Regular.ttf";	//UI 字体（等宽，Nerd Font CN 变体）
		const std::string uiTextCharset       = path::fontPath + "uiText/charset.txt";										//UI 图集字集表（UTF-8，每行汉字）
	}

	namespace shader {
		const std::string terrainTask = path::shaderPath + "terrain/terrainDeferredTask.spv";
		const std::string terrainMesh = path::shaderPath + "terrain/terrainDeferredMesh.spv";
		const std::string terrainVert = path::shaderPath + "terrain/terrainDeferredVert.spv";
		const std::string terrainFrag = path::shaderPath + "terrain/terrainDeferredFrag.spv";

		const std::string pbrVert     = path::shaderPath + "model/pbrDeferredVert.spv";
		const std::string pbrFrag     = path::shaderPath + "model/pbrDeferredFrag.spv";

		const std::string skyboxVert  = path::shaderPath + "skybox/skyboxVert.spv";
		const std::string skyboxFrag  = path::shaderPath + "skybox/skyboxFrag.spv";

		const std::string mineVert    = path::shaderPath + "mine/mineVert.spv";
		const std::string mineFrag    = path::shaderPath + "mine/mineFrag.spv";

		const std::string uiTextVert  = path::shaderPath + "uiText/uiTextVert.spv";
		const std::string uiTextFrag  = path::shaderPath + "uiText/uiTextFrag.spv";

		const std::string osMirrorVert = path::shaderPath + "osMirror/osMirrorVert.spv";
		const std::string osMirrorFrag = path::shaderPath + "osMirror/osMirrorFrag.spv";

		const std::string osScenceVert = path::shaderPath + "osScence/osScenceVert.spv";
		const std::string osScenceFrag = path::shaderPath + "osScence/osScenceFrag.spv";

		const std::string dsScenceVert = path::shaderPath + "dsScence/dsScenceVert.spv";
		const std::string dsScenceFrag = path::shaderPath + "dsScence/dsScenceFrag.spv";

		const std::string shadowVert   = path::shaderPath + "shadow/shadowVert.spv";
		const std::string shadowFrag   = path::shaderPath + "shadow/shadowFrag.spv";
	}

	namespace image {
		//todo 待整合，着色器，贴图，贴图信息
		const std::string cube       = path::imagePath + "terrain/cube_16_64.ktx2";

		const std::string temple     = path::imagePath + "model/temple/baseColor.ktx2";
		const std::string droidBodyC = path::imagePath + "model/droid/Body_baseColor.ktx2";
		const std::string droidHeadC = path::imagePath + "model/droid/Head_baseColor.ktx2";
		const std::string droidMiscC = path::imagePath + "model/droid/Misc_baseColor.ktx2";
		//const std::string droidBodyN = path::imagePath + "model/droid/Body_normal.ktx2";
		//const std::string droidHeadN = path::imagePath + "model/droid/Head_normal.ktx2";
		//const std::string droidMiscN = path::imagePath + "model/droid/Misc_normal.ktx2";
	}

	namespace model {
		const std::string temple = path::modelPath + "temple/scene.gltf";
		const std::string droid  = path::modelPath + "droid/scene.gltf";
		//const std::string nextTemp = path::model + "nextTemp/scene.gltf";

		//模型路径表：索引 = bufferEnum::procStaticModelId / procDynamicModelId（节点里不再各存一份 modelFile）
		const std::vector<std::string> staticList{ temple };
		const std::vector<std::string> dynamicList{ droid };
	}

	namespace music {
		const std::string throughTheMounta = path::musicPath + "background/Through the Mounta.wav";
		//const std::string throughTheMounta = path::musicPath + "background/Greedy.wav";
	}

	namespace storage {
		const std::string fullFile  = path::chunkPath + "full";    //全存法区块存储路径
		const std::string deltaFile = path::chunkPath + "delta";   //变存法区块存储路径
	}
}

//texture	信息
namespace textureInfo {
	namespace terrain {
		const VkFormat   colorFormat = VK_FORMAT_R8G8B8A8_UNORM;		//颜色格式																
		const VkExtent2D tileExtent  = { 16, 16 };						//纹理尺寸																
		const VkExtent2D atlasExtent = { 64, 32 };						//图集尺寸																
	}
}

//spirv		信息
namespace spirvInfo {
	//SPIR-V 字节码头共 5 字：魔数、版本、生成器、id 上限、schema
	const uint32_t magicNumber = 0x07230203;

	//SPIR-V 的 id 0 恒非法，故取全 1 当"未定义"哨兵
	const uint32_t invalidId = 0xFFFFFFFF;

	//图像维度：子通道数据即输入附件
	const uint32_t dimSubpassData = 6;

	//图像采样标记为 2 时是存储图像
	const uint32_t sampledStorage = 2;

	//SPIR-V 指令操作码（只列用到的）
	namespace opCode {
		const uint32_t name             = 5;     //OpName：取变量名，仅用于日志
		const uint32_t entryPoint       = 15;    //OpEntryPoint：取执行模型，校验文件与阶段是否对应
		const uint32_t typeFloat        = 22;    //OpTypeFloat：浮点标量，顶点输入的对拍要用
		const uint32_t typeVector       = 23;    //OpTypeVector：向量类型，取分量数与标量类型
		const uint32_t typeImage        = 25;    //OpTypeImage：图像类型
		const uint32_t typeSampler      = 26;    //OpTypeSampler：采样器类型
		const uint32_t typeSampledImage = 27;    //OpTypeSampledImage：组合采样器类型
		const uint32_t typeArray        = 28;    //OpTypeArray：定长数组，取描述符数组长度
		const uint32_t typeRuntimeArray = 29;    //OpTypeRuntimeArray：运行期数组，长度按 1 处理
		const uint32_t typeStruct       = 30;    //OpTypeStruct：结构体，区分统一缓冲与存储缓冲
		const uint32_t typePointer      = 32;    //OpTypePointer：取存储类与指向类型
		const uint32_t constant         = 43;    //OpConstant：取数组长度常量
		const uint32_t variable         = 59;    //OpVariable：全局变量，即描述符本体
		const uint32_t decorate         = 71;    //OpDecorate：取 set、binding、Block
	}

	//SPIR-V 装饰编号（只列用到的）
	namespace decoration {
		const uint32_t block         = 2;    //Block：统一缓冲结构体
		const uint32_t bufferBlock   = 3;    //BufferBlock：存储缓冲结构体（旧写法）
		const uint32_t location      = 30;   //Location：顶点输入位置
		const uint32_t binding       = 33;   //Binding：绑定号
		const uint32_t descriptorSet = 34;   //DescriptorSet：描述符集号
	}

	//SPIR-V 存储类编号（只列用到的）
	namespace storageClass {
		const uint32_t uniformConstant = 0;    //采样器与图像
		const uint32_t input           = 1;    //顶点/片段输入
		const uint32_t uniform         = 2;    //统一缓冲、旧写法存储缓冲
		const uint32_t storageBuffer   = 12;   //存储缓冲
	}

	//SPIR-V 执行模型编号（只列本引擎用到的阶段）
	namespace executionModel {
		const uint32_t vertex   = 0;     //顶点
		const uint32_t fragment = 4;     //片段
		const uint32_t compute  = 5;     //计算
		const uint32_t task     = 5267;  //任务（网格管线）
		const uint32_t mesh     = 5268;  //网格（网格管线）
	}
}



//render	信息
namespace renderInfo {
	const VkFormat   dsPosColorFormat = VK_FORMAT_R32G32B32A32_SFLOAT;	//延迟渲染坐标颜色附件格式
	const VkExtent2D shadowMapExtent  = { 2048, 2048 };					//阴影映射尺寸															

	const VkClearValue colorClearValue = { .color = {105.0f / 255.f, 105.0f / 255.f, 105.0f / 255.f, 1.0f } };      //颜色清理值
	const VkClearValue depthClearValue = { .depthStencil = { 1.0f, 0u } };                                          //深度清理值
}

//pipeline	信息
namespace pipelineInfo {
	//管线 id → 日志名字（顺序必须与 pipelineEnum::procPipelineId 一致）
	const std::array<std::string, pipelineEnum::procPipelineId::enumCount> procPipelineName{
		"terrain", "model", "skybox", "mine", "uiText", "osMirror", "osScence", "dsScence", "shadowMap"
	};

	//描述符 id → 日志名字（顺序必须与 descEnum::descType 一致）
	const std::array<std::string, descEnum::descType::enumCount> procDescName{ 
		"scenceForward", "scenceDeferred", "uiText", "dsScence", "shadowMap" 
	};

	const int commandThreadCount	   = 2;								//命令线程数		

	const uint32_t scenceForwardCount  = 4;								//scenceForward描述符池上限，	skybox，mine，osMirror，osScence
	const uint32_t scenceDeferredCount = 5;								//scenceDeferred描述符池上限，	terrain，m_temple，m_droid*3
	const uint32_t uiTextCount         = 1;								//uiText描述符池上限，			uiText
	const uint32_t dsScenceCount       = 1;								//dsScence描述符池上限，		dsScence
	const uint32_t shadowCount         = 6;								//shadow描述符池上限，			terrain*2，m_temple，m_droid*3
	
	const uint32_t uiTextGlyphCount    = 512;							//uiText 顶点缓冲字形上限，	6 顶点/字形
}