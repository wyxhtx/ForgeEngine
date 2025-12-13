#pragma once



namespace bufferEnum {
    namespace textAlignId {
        enum type {
            left,
            center,
            right
        };
    }

    //一种帧结点类型意味着一种renderPass，各种离屏可以共用同一种renderPass
    namespace frameNodeId {
        enum type {
            os,	        //离屏
            sc,	        //交换链

            count
        };
    }

    namespace procRenderId {
        enum type {
            osMirror,   //离屏镜面
            osScence,	//离屏场景，所有渲染结果先集中到此，最后统一提交到交换链场景
            scScence,	//交换链场景

            count
        };
    }

    namespace procDescriptorId {
        enum type {
            scence,     //场景描述符
            uiText,     //文本描述符

            count
        };
    }

    namespace procSamplerId {
        enum type {
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
        enum type {
            temple,		//神庙

            count
        };
    }

    namespace procDynamicModelId {
        enum type {
            droid,		//机器人

            count
        };
    }
}

namespace pipelineEnum {
    namespace procPipelineId {
        enum type {
            terrain,    //地形
            model,      //模型
            skybox,     //天空盒
            mine,       //选择框
            osMirror,   //离屏镜面
            osScence,   //离屏场景
            uiText,     //文本ui

            count
        };
    }
}