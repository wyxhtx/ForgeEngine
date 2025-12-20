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
            sc,	        //交换链    swapChain
            os,	        //离屏      offScreen
            ds,         //延迟着色  deferredShading
            //dl,         //延迟光照  deferredLighting

            count
        };
    }

    namespace procRenderId {
        enum type {
            scScence,	//交换链场景
            osScence,	//离屏场景，所有渲染结果先集中到此，最后统一提交到交换链场景
            osMirror,   //离屏镜面
            dsScence,	//延迟着色场景

            count
        };
    }

    namespace procDescriptorId {
        enum type {
            scenceForward,      //场景前向描述符
            scenceDeferred,     //场景延迟描述符，用于将纹理贴图和法线贴图转换为3大附件
            dsScence,           //延迟着色描述符，用于将3大附件和光照处理成最终结果
            uiText,             //文本描述符

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
            dsScence,	//延迟着色场景
            uiText,     //文本ui

            count
        };
    }
}