#pragma once



class Noise {
public:
	FastNoiseLite terrain;											//地形噪音
	FastNoiseLite tree;												//树噪音

	//FastNoiseLite bigS;												//巨构噪音



	Noise(int initSeed) :seed{ initSeed } {
		createNoise();

        if (debugInfo::LogSwitch) {
            Log::printTitle("————————Noise已创建");

            Log::delimiter();
        }
    }

    ~Noise() {
        if (debugInfo::LogSwitch) {
            Log::print("Noise已清除");
        }
    }

private:
	int seed = 0;

    void createNoise() {		
		terrain.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);	//噪音类型
		terrain.SetSeed(seed);											//种子
		terrain.SetFrequency(0.01f);									//缩放
		terrain.SetFractalType(FastNoiseLite::FractalType_FBm);			//分形类型
		terrain.SetFractalOctaves(2);									//分形次数
		terrain.SetFractalLacunarity(3);								//分形特征尺度

		tree.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2S);
		tree.SetSeed(seed + 1);
		tree.SetFrequency(0.5f);
		tree.SetFractalType(FastNoiseLite::FractalType_FBm);
		tree.SetFractalOctaves(2);
		tree.SetFractalLacunarity(3);

		/*bigS.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2S);
		bigS.SetSeed(seed + 2);
		bigS.SetFrequency(0.2f);*/
    }
};