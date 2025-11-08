#pragma once



namespace cubeEnum {
	//方块索引
	namespace cubeId {
		enum : short {
			air = -1,			//空气
			grass,				//草方块
			dirt,				//泥土
			oakLog,				//橡树树干
			oakLeaf,			//橡树树叶
			stoneSquared,		//石料

			count				//枚举长度
		};
	}

	//覆盖索引
	namespace coverageId {
		enum : short {
			oakLeaf,			//橡树树叶
			dirt,				//泥土
			grass,				//草方块
			oakLog,				//橡树树干
			stoneSquared		//石料
		};
	}
}