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
            sc_f,	        //交换链    swapChain
            os_f,	        //离屏      offScreen
            ds_f,           //延迟着色  deferredShading

            count
        };
    }

    namespace procRenderId {
        enum type {
            scScence_r,	    //交换链场景
            osScence_r,	    //离屏场景，所有渲染结果先集中到此，最后统一提交到交换链场景
            osMirror_r,     //离屏镜面
            dsScence_r,	    //延迟着色场景

            count
        };
    }

    namespace procDescriptorId {
        enum type {
            scenceForward_d,      //场景前向描述符
            scenceDeferred_d,     //场景延迟描述符，用于将纹理贴图和法线贴图转换为3大附件
            dsScence_d,           //延迟着色描述符，用于将3大附件和光照处理成最终结果
            uiText_d,             //文本描述符

            count
        };
    }

    namespace procSamplerId {
        enum type {
            terrain_s,          //地形采样器
            staticModel_sn,     //静态模型邻近采样器
            staticModel_sl,     //静态模型线性采样器
            dynamicModel_sn,    //动态模型邻近采样器
            dynamicModel_sl,    //动态模型线性采样器
            offScreen_s,        //离屏渲染采样器
            uiText_s,           //文本采样器

            count
        };
    }

    namespace procStaticModelId {
        enum type {
            temple_m,   //神庙

            count
        };
    }

    namespace procDynamicModelId {
        enum type {
            droid_m,		//机器人

            count
        };
    }
}

namespace pipelineEnum {
    namespace procPipelineId {
        enum type {
            terrain_p,      //地形
            model_p,        //模型
            skybox_p,       //天空盒
            mine_p,         //选择框
            osMirror_p,     //离屏镜面
            osScence_p,     //离屏场景
            dsScence_p,	    //延迟着色场景
            uiText_p,       //文本ui

            count
        };
    }
}