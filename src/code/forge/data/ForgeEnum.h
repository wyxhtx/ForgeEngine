#pragma once



namespace cubeEnum {
	//方块索引
	namespace cubeId {
		enum {
			air = -1,			//空气
			grass,				//草方块
			dirt,				//泥土
			oakLog,				//橡树树干
			oakLeaf,			//橡树树叶
			stoneSquared		//石料
		};
	}

	//覆盖索引
	namespace coverageId {
		enum {
			oakLeaf,			//橡树树叶
			dirt,				//泥土
			grass,				//草方块
			oakLog,				//橡树树干
			stoneSquared		//石料
		};
	}
}