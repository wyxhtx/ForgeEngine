#pragma once



//todo 存储暂时用不上的东西
namespace todoInfo {
	int visibleFaceCount;							//区块可见面数量
	int visibleOptimization;						//区块可见面优化数量
}

//板块信息
namespace plateInfo {
	const int groundPlane = 4096;										//地平面
	//const int skyPlane = 2;											//天平面，未正式启用
	//const int sightRange = 2;											//视距		//2.5s左右
	const int sightRange = 8;											//视距		//3.8s左右

	const int chunkLength = 32;											//区块边长	//很多地方都是直接写数字，所以chunkLength变更时很多地方都得改
	const int chunkMapLength = chunkLength + 2;							//区块Map中的边长
	const int lengthIncrement = plateInfo::chunkLength + 1;
	const int lengthDecrement = plateInfo::chunkLength - 1;

	const float cubeSightRange = plateInfo::chunkLength * (plateInfo::sightRange + 2);	//用于updateUniformBuffer
	const int filled = (int)std::pow(plateInfo::chunkMapLength, 3);		//区块边长立方值
	
	//const int originShiftThreshold = (int)pow(2, 5);					//测试用32
	const int originShiftThreshold = (int)pow(2, 8);					//测试用256
	//const int originShiftThreshold = (int)pow(2, 13);					//原点迁移阈值8192，小数点后还有三位能准确表示精度
}

//todo后续写成结构体
//地形信息
namespace terrainInfo {
	const int undulation = 8;			//起伏		
	const int translation = 16;			//平移
}

//贴图信息
namespace textureInfo {
	const int tileWidth = 16;									//贴图宽度
	const int tileHeight = 16;									//贴图高度
	const int atlasWidth = 64;									//图集宽几个贴图
	const int atlasHeight = 32;									//图集高几个贴图
	const uint32_t arrayLayer = atlasWidth * atlasHeight;		//图集层数
}

//面信息
namespace faceInfo {
	//面剔除索引,在此可控制面剔除朝向
	const std::vector<glm::ivec3> faceCullId{
		{ 0, 1, 0},
		{ 0,-1, 0},
		{ 1, 0, 0},
		{-1, 0, 0},
		{ 0, 0, 1},
		{ 0, 0,-1}
	};
}

//存储信息
namespace storageInfo {
	const int cubeArraySize = plateInfo::filled * sizeof(short);		//方块数组大小
	const int thresholdSize = plateInfo::filled / 4;					//存法阈值，r=32时该值为9826，源自plateInfo::filled * sizeof(short) / 4 * sizeof(short)

	const int regionLoopLength = 2;										//地区循环边长，即3x3x3的立方体
	const int regionLength = 32;										//地区边长，至少一倍视距，不然更新板块出bug，不能跟plateInfo::sightRange在一起写死，不然读存区块文件出bug
}

//玩家信息，内部元素大多涉及频繁更新
namespace playerInfo {
	//const double max = pow(2, 31) - 1;				//玩家出生点坐标最大值
	//const double min = -pow(2, 31);					//玩家出生点坐标最小值
	//const glm::dvec3 initSpawnPos = { max, 32, max };

	//const glm::dvec3 initSpawnPos = { 0, 0, 0 };												//玩家出生点（d世界坐标系）
	const glm::dvec3 initSpawnPos = { 0, 32, 0 };												//玩家出生点（d世界坐标系）

	//逻辑原点坐标（d世界坐标系），在逻辑之中表示无限地图
	glm::dvec3 logicOriginPos = glm::floor(initSpawnPos / (double)plateInfo::originShiftThreshold) * (double)plateInfo::originShiftThreshold;

	//todo目前距离不准待日后矫正
	const float interactionRange = 4;					//方块交互距离

	//鼠标按键状态
	bool mouseButtonLeft = false;
}
