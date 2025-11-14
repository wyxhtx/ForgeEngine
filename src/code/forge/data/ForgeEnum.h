#pragma once



//方块枚举
namespace cubeEnum {
	//方块索引
	namespace cubeId {
		enum : short {
			air = -1,			//空气
			grass,				//草方块
			dirt,				//泥土
			oakTrunk,			//橡树树干
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
			oakTrunk,			//橡树树干
			stoneSquared		//石料
		};
	}
}

//地形枚举
namespace terrainEnum {
	//地形层级
	namespace terrainLayer {
		enum {
			Layer_0,			//Layer_0：可破坏方块、	无lod合并、	  服务于玩家快速破坏方块
			Layer_1,			//Layer_1：不可破坏方块、	从1x到32x合并、服务于玩家观赏地形

			count
		};
	}

	//todo 待实现
	/*
	namespace terrainLod {		//地形lod，即Layer_1的lod
		enum {
			Lod_0,              //无合并
			Lod_1,              //2x合并
			Lod_2,              //4x合并
			Lod_3,              //8x合并
			Lod_4,              //16x合并
			Lod_5,              //32x合并

			count
		};
	}
	*/

	//地形索引
	namespace terrainId {
		enum {
			plain,		//平原

			count
		};
	}
}

//地形结构枚举
namespace terrainStructureEnum {
	//树索引
	namespace treeId {
		enum {
			oak,		//橡树

			count
		};
	}
}