#pragma once



namespace bufferList {
    struct procDescriptorNode {
        bufferStruct::descriptorNode scence;  //场景描述符
        bufferStruct::descriptorNode uiText;  //文本描述符

        void createDescriptorPool(VkDevice& device) {
            /*scence*/
            std::array<bufferStruct::descriptorPoolNode, 4> poolSizeScence{
                bufferStruct::descriptorPoolNode{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,bufferInfo::bufferScenceNodeSize},           //顶点统一缓冲区
                bufferStruct::descriptorPoolNode{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,bufferInfo::bufferScenceNodeSize},           //片段统一缓冲区
                bufferStruct::descriptorPoolNode{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,bufferInfo::bufferScenceNodeSize},   //片段纹理贴图
                bufferStruct::descriptorPoolNode{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,bufferInfo::bufferScenceNodeSize}    //片段高光贴图
            };

            VkDescriptorPoolCreateInfo descriptorPoolScenceInfo{
                .sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                .maxSets        = bufferInfo::bufferScenceNodeSize,                 //poolSizeScence.descriptorCount最好填大点不支持自动分配
                .poolSizeCount  = static_cast<uint32_t>(poolSizeScence.size()),
                .pPoolSizes     = &poolSizeScence[0].node
            };

            if (vkCreateDescriptorPool(device, &descriptorPoolScenceInfo, nullptr, &scence.pool) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "描述符池创建失败");
                throw;
            }



            /*uiText*/
            std::array<bufferStruct::descriptorPoolNode, 1> poolSizeUItext{
                bufferStruct::descriptorPoolNode{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,bufferInfo::bufferTextUINodeSize} 
            };

            VkDescriptorPoolCreateInfo descriptorPoolUItextInfo{
                .sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                .maxSets        = bufferInfo::bufferTextUINodeSize,
                .poolSizeCount  = static_cast<uint32_t>(poolSizeUItext.size()),
                .pPoolSizes     = &poolSizeUItext[0].node
            };

            if (vkCreateDescriptorPool(device, &descriptorPoolUItextInfo, nullptr, &uiText.pool) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "描述符池创建失败");
                throw;
            }
        }

        void createDescriptorSetLayout(VkDevice& device) {
            /*scence*/
            std::array<bufferStruct::descriptorLayoutNode, 4> scenceLayoutBinding{
                bufferStruct::descriptorLayoutNode{0,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,VK_SHADER_STAGE_VERTEX_BIT},
                bufferStruct::descriptorLayoutNode{1,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,VK_SHADER_STAGE_FRAGMENT_BIT},
                bufferStruct::descriptorLayoutNode{2,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,VK_SHADER_STAGE_FRAGMENT_BIT},
                bufferStruct::descriptorLayoutNode{3,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,VK_SHADER_STAGE_FRAGMENT_BIT}
            };

            VkDescriptorSetLayoutCreateInfo scenceLayoutInfo{
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .pNext = nullptr,
                .bindingCount = static_cast<uint32_t>(scenceLayoutBinding.size()),
                .pBindings = &scenceLayoutBinding[0].node
            };

            if (vkCreateDescriptorSetLayout(device, &scenceLayoutInfo, nullptr, &scence.layout) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "scence描述符布局配置失败");
                throw;
            }



            /*uiText*/
            std::array<bufferStruct::descriptorLayoutNode, 1> uiTextLayoutBinding{
                bufferStruct::descriptorLayoutNode{0,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,VK_SHADER_STAGE_FRAGMENT_BIT}
            };

            VkDescriptorSetLayoutCreateInfo uiTextLayoutInfo{
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .pNext = nullptr,
                .bindingCount = static_cast<uint32_t>(uiTextLayoutBinding.size()),
                .pBindings = &uiTextLayoutBinding[0].node
            };

            if (vkCreateDescriptorSetLayout(device, &uiTextLayoutInfo, nullptr, &uiText.layout) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "uiText描述符布局配置失败");
                throw;
            }
        }

        void clear(VkDevice& device) {
            //scence
            vkDestroyDescriptorPool(device, scence.pool, nullptr);
            vkDestroyDescriptorSetLayout(device, scence.layout, nullptr);

            //uiText
            vkDestroyDescriptorPool(device, uiText.pool, nullptr);
            vkDestroyDescriptorSetLayout(device, uiText.layout, nullptr);
        }
    };

    struct procSamplerNode {
        VkSampler node[bufferEnum::procSamplerNode::count];



        void createTextureSampler(VkDevice& device, uint32_t terrainMip, uint32_t sModelMip, uint32_t dModelMip) {
            VkSamplerCreateInfo samplerInfo{};
            {
                samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

                //过滤器即采用何种方式过滤图片
                samplerInfo.minFilter = VK_FILTER_LINEAR;

                //每个轴向使用的寻址模式全部采用镜像循环
                samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
                samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
                samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

                //各向异性过滤器
                samplerInfo.anisotropyEnable = VK_TRUE;
                samplerInfo.maxAnisotropy = 16;

                //采样范围超过图像时候返回的颜色
                samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

                //指定使用的坐标系统，用于访问图像的纹素
                samplerInfo.unnormalizedCoordinates = VK_FALSE;

                //开启比较功能，那么纹素首先和值进行比较，并且比较后的值用于过滤操作
                samplerInfo.compareEnable = VK_FALSE;
                samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

                samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
                samplerInfo.mipLodBias = 0.0f;
                samplerInfo.minLod = 0.0f;
            }

            //terrain
            samplerInfo.magFilter = VK_FILTER_NEAREST;
            samplerInfo.maxLod = static_cast<float>(terrainMip);
            if (vkCreateSampler(device, &samplerInfo, nullptr, &node[bufferEnum::procSamplerNode::terrain]) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "terrain采样器创建失败");
                throw;
            }

            //sModelN
            samplerInfo.maxLod = static_cast<float>(sModelMip);
            if (vkCreateSampler(device, &samplerInfo, nullptr, &node[bufferEnum::procSamplerNode::sModelN]) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "sModelN采样器创建失败");
                throw;
            }

            //sModelL
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            if (vkCreateSampler(device, &samplerInfo, nullptr, &node[bufferEnum::procSamplerNode::sModelL]) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "sModelL采样器创建失败");
                throw;
            }

            //dModelN
            samplerInfo.magFilter = VK_FILTER_NEAREST;
            samplerInfo.maxLod = static_cast<float>(dModelMip);
            if (vkCreateSampler(device, &samplerInfo, nullptr, &node[bufferEnum::procSamplerNode::dModelN]) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "dModelN采样器创建失败");
                throw;
            }

            //dModelL
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            if (vkCreateSampler(device, &samplerInfo, nullptr, &node[bufferEnum::procSamplerNode::dModelL]) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "dModelL采样器创建失败");
                throw;
            }

            //uiText
            samplerInfo.maxLod = 0.0f;
            if (vkCreateSampler(device, &samplerInfo, nullptr, &node[bufferEnum::procSamplerNode::uiText]) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "uiText采样器创建失败");
                throw;
            }

            //offScreen
            samplerInfo.magFilter     = VK_FILTER_LINEAR;                       //放大过滤器
            samplerInfo.minFilter     = VK_FILTER_LINEAR;                       //缩小过滤器
            samplerInfo.mipmapMode    = VK_SAMPLER_MIPMAP_MODE_LINEAR;          //MIP映射模式
            samplerInfo.addressModeU  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;  //U方向寻址模式
            samplerInfo.addressModeV  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;  //V方向寻址模式
            samplerInfo.addressModeW  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;  //W方向寻址模式
            samplerInfo.mipLodBias    = 0.0f;                                   //MIP LOD偏置
            samplerInfo.maxAnisotropy = 1.0f;                                   //最大各向异性
            samplerInfo.minLod        = 0.0f;                                   //最小LOD
            samplerInfo.maxLod        = 1.0f;                                   //最大LOD
            samplerInfo.borderColor   = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;     //边界颜色
            if (vkCreateSampler(device, &samplerInfo, nullptr, &node[bufferEnum::procSamplerNode::offScreen]) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "offScreenPass采样器创建失败");
                throw;
            }
        }

        void clear(VkDevice& device) {
            for (int i = 0; i < bufferEnum::procSamplerNode::count; i++) {
                vkDestroySampler(device, node[i], nullptr);
            }
        }
    };



    struct procModelNode {
        std::array<bufferStruct::staticModelNode, bufferEnum::procStaticModelEnum::count> sNode = []() {
            std::array<bufferStruct::staticModelNode, bufferEnum::procStaticModelEnum::count> array{};

            array[bufferEnum::procStaticModelEnum::temple]  = bufferStruct::staticModelNode{ fileInfo::model::temple,  fileInfo::image::temple,  bufferEnum::procSamplerNode::sModelN };

            return array;
            }();

        std::array<bufferStruct::dynamicModelNode, bufferEnum::procDynamicModelEnum::count> dNode = []() {
            std::array<bufferStruct::dynamicModelNode, bufferEnum::procDynamicModelEnum::count> array{};

            array[bufferEnum::procDynamicModelEnum::droid] = bufferStruct::dynamicModelNode{ fileInfo::model::droid,{fileInfo::image::droidBody,fileInfo::image::droidHead,fileInfo::image::droidMisc},bufferEnum::procSamplerNode::dModelL };

            return array;
            }();

        uint32_t sModelMip = 0;                  //在创建texture时赋值
        uint32_t dModelMip = 0;                  //在创建texture时赋值



        procModelNode() {
            for (int i = 0; i < bufferEnum::procDynamicModelEnum::count; i++) {
                modelVertex::dVertexs[i].resize(dNode[i].node.size());
                modelVertex::dIndexs[i].resize(dNode[i].node.size());
            }
        }

        void getMaxModelMip() {
            std::vector<uint32_t> sMaxMip;
            for (const bufferStruct::staticModelNode& i : sNode) {
                sMaxMip.push_back(i.mipLevel);
            }
            sModelMip = *std::max_element(sMaxMip.begin(), sMaxMip.end());   //使用模型最大mip创建采样器

            std::vector<uint32_t> dMaxMip;
            for (const bufferStruct::dynamicModelNode& i : dNode) {
                dMaxMip.push_back(i.mipLevel);
            }
            dModelMip = *std::max_element(dMaxMip.begin(), dMaxMip.end());
        }

        void clear(VkDevice& device, VmaAllocator allocator) {
            for (int i = 0; i < bufferEnum::procStaticModelEnum::count; i++) {
                sNode[i].clear(device, allocator);
            }

            for (int i = 0; i < bufferEnum::procDynamicModelEnum::count; i++) {
                dNode[i].clear(device, allocator);
            }
        }
    };
}

namespace pipelineList {
    struct pipelineLayout {
        VkPipelineLayout scence;        //场景管线布局
        VkPipelineLayout uiText;        //文本管线布局
    };

    struct pipeline {
        VkPipeline terrain;     //地形管线
        VkPipeline model;       //模型管线
        VkPipeline skybox;      //天空盒管线
        VkPipeline mine;        //挖掘选择框管线
        VkPipeline offScreen;   //离屏渲染管线
        VkPipeline uiText;      //文本管线
    };

    struct procPipelineNode {
        //pipelineLayouts和pipelines方便传参
        pipelineList::pipelineLayout pipelineLayout;
        pipelineList::pipeline pipeline;

        pipelineStruct::pipelineNode terrain    { pipelineLayout.scence,pipeline.terrain };
        pipelineStruct::pipelineNode model      { pipelineLayout.scence,pipeline.model };
        pipelineStruct::pipelineNode skybox     { pipelineLayout.scence,pipeline.skybox };
        pipelineStruct::pipelineNode mine       { pipelineLayout.scence,pipeline.mine };
        pipelineStruct::pipelineNode offScreen  { pipelineLayout.scence,pipeline.offScreen };
        pipelineStruct::pipelineNode uiText     { pipelineLayout.uiText,pipeline.uiText };



        void cleanupSwapChain(VkDevice& device) {
            vkDestroyPipelineLayout(device, pipelineLayout.scence, nullptr);
            vkDestroyPipelineLayout(device, pipelineLayout.uiText, nullptr);

            vkDestroyPipeline(device, terrain.pipeline, nullptr);
            vkDestroyPipeline(device, model.pipeline, nullptr);
            vkDestroyPipeline(device, skybox.pipeline, nullptr);
            vkDestroyPipeline(device, mine.pipeline, nullptr);
            vkDestroyPipeline(device, offScreen.pipeline, nullptr);
            vkDestroyPipeline(device, uiText.pipeline, nullptr);
        }
    };
}