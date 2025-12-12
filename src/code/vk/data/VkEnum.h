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
            osMirror,   //离屏镜面
            osScence,	//离屏场景，所有渲染结果先集中到此，最后统一提交到交换链场景
            scScence,	//交换链场景

            count
        };
    }

    namespace procDescriptorId {
        enum {
            scence,     //场景描述符
            uiText,     //文本描述符

            count
        };
    }

    //todo 到时候扩展到管线布局？

    namespace procSamplerId {
        enum {
            terrain,    //地形采样器
            sModelN,    //静态模型邻近采样器
            sModelL,    //静态模型线性采样器
            dModelN,    //动态模型邻近采样器
            dModelL,    //动态模型线性采样器
            offScreen,  //离屏渲染采样器
            uiText,     //文本采样器

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