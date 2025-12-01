#pragma once



namespace bufferEnum {
    namespace textAlign {
        enum Type {
            alignLeft,
            alignCenter,
            alignRight
        };
    }

    namespace procSamplerNode {
        enum {
            terrain,      //地形采样器
            modelN,       //模型邻近采样器
            modelL,       //模型线性采样器
            uiText,       //文本采样器

            count
        };
    }

    namespace procStaticModelEnum {
        enum {
            temple,		//神庙

            count
        };
    }

    namespace procDynamicModelEnum {
        enum {
            droid,		//机器人

            count
        };
    }
}