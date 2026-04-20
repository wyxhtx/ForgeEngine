#pragma once



//面信息
namespace faceInfo {
	//面剔除索引，用于控制面剔除朝向
	const std::vector<glm::ivec3> faceCullId{
		{ 0, 1, 0},
		{ 0,-1, 0},
		{ 1, 0, 0},
		{-1, 0, 0},
		{ 0, 0, 1},
		{ 0, 0,-1}
	};
}

//板块信息
namespace plateInfo {
	const int groundPlane = 4096;														//地平面
	//const int skyPlane = 2;															//天平面，未正式启用
	//const int sightRange = 8;															//视距
	const int sightRange = 8;															//视距

	const int chunkLength = 32;															//区块边长
	const int cubeArrayLength = chunkLength + 2;										//方块数组边长
	const int lengthIncrement = plateInfo::chunkLength + 1;								//区块边长增一，用于方块数组边缘检测
	const int lengthDecrement = plateInfo::chunkLength - 1;								//区块边长减一，用于方块数组边缘检测
	const int cubeArraySquare = (int)std::pow(plateInfo::cubeArrayLength, 2);			//方块数组边长平方值
	const int cubeArrayVolume = (int)std::pow(plateInfo::cubeArrayLength, 3);			//方块数组边长立方值

	const float perspSightRange = plateInfo::chunkLength * (plateInfo::sightRange + 2);	//方块透视投影视距半径，用于设置远裁剪面半径
	const float orthoSightRange = plateInfo::chunkLength * plateInfo::sightRange;		//方块正交投影视距半径，用于设置远裁剪面半径

	const int originShiftThreshold = (int)pow(2, 8);									//测试用256
	//const int originShiftThreshold = (int)pow(2, 13);									//原点迁移阈值8192，小数点后还有三位能准确表示精度

	const int terrainLayerSight[forgeEnum::terrainLayer::count] = { 4,plateInfo::sightRange };	//地形管线各层视距列表
}

//噪音信息
namespace noiseInfo {
	const int worldSeed = 0;

	const double noiseExtreme = pow(2, 23);		//噪音极值，因为fastnoise2精度问题，为确保结果正确，大于该值时从GenPositionArray2D换成GenSingle2D运算
}

//存储信息
namespace storageInfo {
	const int cubeArraySize = plateInfo::cubeArrayVolume * sizeof(short);				//方块数组大小
	const int thresholdSize = plateInfo::cubeArrayVolume / 4;							//存法阈值，r=32时该值为9826，源自plateInfo::cubeArrayVolume * sizeof(short) / 4 * sizeof(short)

	const int regionLoopLength = 2;														//地区循环边长，即3x3x3的立方体
	const int regionLength = 32;														//地区边长，至少一倍视距，不然更新板块出错，不能跟视距写死，不然视距改变后读存区块出错
}

//贴图信息
namespace textureInfo {
	const int tileWidth = 16;									//贴图宽度
	const int tileHeight = 16;									//贴图高度
	const int atlasWidth = 64;									//图集宽几个贴图
	const int atlasHeight = 32;									//图集高几个贴图
	const uint32_t arrayLayer = atlasWidth * atlasHeight;		//图集层数
}

//玩家信息，内部元素大多涉及频繁更新
namespace playerInfo {
	//const double max = pow(2, 31) - 1;				//玩家出生点坐标最大值
	//const double min = -pow(2, 31);					//玩家出生点坐标最小值
	//const glm::dvec3 initSpawnPos = { max, 32, max };

	const glm::dvec3 initSpawnPos = { 0, 32, 0 };		//玩家出生点（d世界坐标系）

	//逻辑原点坐标（d世界坐标系），在逻辑之中表示无限地图，用于updateUniformBuffer正确计算板块mvp
	glm::dvec3 logicOriginPos = glm::floor(initSpawnPos / (double)plateInfo::originShiftThreshold) * (double)plateInfo::originShiftThreshold;

	//todo目前距离不准待日后矫正
	const float cubeInteractionRange = 16;				//方块交互半径

	//鼠标按键状态
	std::atomic<bool> mouseButtonLeft  = false;			//鼠标左键
	std::atomic<bool> mouseButtonRight = false;			//鼠标右键
}