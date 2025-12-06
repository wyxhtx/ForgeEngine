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
            terrain,    //地形采样器
            sModelN,    //静态模型邻近采样器
            sModelL,    //静态模型线性采样器
            dModelN,    //动态模型邻近采样器
            dModelL,    //动态模型线性采样器
            uiText,     //文本采样器
            offScreen,  //离屏渲染采样器

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