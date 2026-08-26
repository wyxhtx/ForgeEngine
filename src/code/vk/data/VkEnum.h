#pragma once



//engine枚举空间
namespace engineEnum {
    namespace tickEnum {
        enum type {
            key,        //按键获取刷新率
            uiText,     //文本UI刷新率
            plate,      //板块更新刷新率
            cube,       //方块更新刷新率

            count
        };
    }
}



//buffer枚举空间
namespace bufferEnum {
    //imageBuffer枚举
    namespace imageBufferId {
        enum type {
            base,       //基础图像，array = 1, mip = n，model
            array,      //数组图像，array = n, mip = 1，terrain
            text,       //文本图像，array = 1, mip = 1，uiText
            frame,      //帧图像，  array = 1, mip = 1

            count
        };
    }
}

//render枚举空间
namespace renderEnum {
    //render附件枚举
    namespace renderAttId {
        enum type {
            forward_ra,     //前向渲染附件
            deferred_ra,    //延迟渲染附件
            shadowMap_ra,   //阴影映射附件

            count
        };
    }

    //render帧图像枚举
    namespace renderFrameId {
        enum type {
            conv_rf,        //常规：颜色、深度、视图
            swap_rf,        //交换链

            count
        };
    }

    //render模板枚举
    namespace renderTemplateId {
        enum type {
            swapChain_rt,   //交换链
            offScreen_rt,   //离屏
            deferred_rt,    //延迟渲染
            shadowMap_rt,   //阴影映射

            count
        };
    }

    //render实例枚举
    namespace renderInstanceId {
        enum type {
            scScence_ri,    //交换链场景
            osScence_ri,    //离屏场景
            osMirror_ri,    //离屏镜面
            dsScence_ri,    //延迟渲染场景
            shadowMap_ri,   //阴影映射

            count
        };
    }
}

//pipeline枚举空间
namespace pipelineEnum {
    //shader结点枚举
    namespace shaderNodeId {
        namespace vert {
            enum type {
                vert,   //顶点着色器
                frag,   //片段着色器

                count
            };
        }

        namespace mesh {
            enum type {
                task,   //任务着色器
                mesh,   //网格着色器
                frag,   //片段着色器

                count
            };
        }
    }

    //cmdDraw枚举
    namespace cmdDrawId {
        enum type {
            indexed,        //索引绘制
            fullscreen,     //全屏三角形
            text,           //UI文本

            count
        };
    }



    //pipeline实例枚举
    namespace procPipelineId {
        enum type {
            terrain_p,      //地形
            model_p,        //模型
            skybox_p,       //天空盒
            mine_p,         //选择框
            uiText_p,       //文本ui
            osMirror_p,     //离屏镜面
            osScence_p,     //离屏场景
            dsScence_p,	    //延迟渲染场景
            shadowMap_p,    //阴影映射

            count
        };
    }
}



//todo 后续慢慢拆分
//buffer枚举空间
namespace bufferEnum {
    namespace textAlignId {
        enum type {
            left,
            center,
            right
        };
    }

    namespace procDescriptorId {
        enum type {
            scenceForward_d,    //场景前向描述符
            scenceDeferred_d,   //场景延迟描述符，用于将纹理贴图和法线贴图转换为3大附件
            uiText_d,           //文本描述符
            dsScence_d,         //延迟渲染描述符，用于将3大附件和光照处理成最终结果
            shadowMap_d,        //阴影映射描述符

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
            uiText_s,           //文本采样器
            offScreen_s,        //离屏渲染采样器
            shadowMap_s,        //阴影映射采样器

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