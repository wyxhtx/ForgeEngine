#pragma once



namespace bufferEnum {
    namespace textAlignId {
        enum Type {
            alignLeft,
            alignCenter,
            alignRight
        };
    }

    namespace frameNodeType {
        enum {
            sc,	        //交换链
            os	        //离屏
        };
    }

    namespace procRenderId {
        enum {
            scScence,	//场景交换链
            osScence,	//场景离屏，所有渲染结果先集中到此，最后统一提交到交换链

            //osMirror,   //镜面离屏，todo其余处理好后再实现

            count
        };
    }

    namespace procSamplerId {
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

    namespace procStaticModelId {
        enum {
            temple,		//神庙

            count
        };
    }

    namespace procDynamicModelId {
        enum {
            droid,		//机器人

            count
        };
    }
}