#pragma once



namespace bufferList {
    struct descriptor {
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

    struct sampler {
        VkSampler terrain;      //地形采样器
        VkSampler model;        //模型采样器
        VkSampler uiText;       //文本采样器

        void createTextureSampler(VkDevice& device, uint32_t terrainMipLevel, uint32_t modelMipLevel) {
            VkSamplerCreateInfo samplerInfo{};
            {
                samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

                //过滤器即采用何种方式过滤图片
                //指定纹素放大和缩小内插值方式
                samplerInfo.magFilter = VK_FILTER_NEAREST;
                samplerInfo.minFilter = VK_FILTER_NEAREST;

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
            samplerInfo.maxLod = static_cast<float>(terrainMipLevel);

            if (vkCreateSampler(device, &samplerInfo, nullptr, &terrain) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "terrain采样器创建失败");
                throw;
            }

            //model
            samplerInfo.maxLod = static_cast<float>(modelMipLevel);

            if (vkCreateSampler(device, &samplerInfo, nullptr, &model) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "model采样器创建失败");
                throw;
            }

            //uiText
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            samplerInfo.minFilter = VK_FILTER_LINEAR;

            samplerInfo.maxLod = 0.0f;

            if (vkCreateSampler(device, &samplerInfo, nullptr, &uiText) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "uiText采样器创建失败");
                throw;
            }
        }

        void clear(VkDevice& device) {
            vkDestroySampler(device, terrain, nullptr);
            vkDestroySampler(device, model, nullptr);
            vkDestroySampler(device, uiText, nullptr);
        }
    };
}

namespace pipelineList {
    struct pipelineLayout {
        VkPipelineLayout scence;        //场景管线布局
        VkPipelineLayout uiText;        //文本管线布局
    };

    struct pipeline {
        VkPipeline terrain[terrainEnum::terrainLayer::count];   //地形管线
        VkPipeline model;                                       //模型管线
        VkPipeline skybox;                                      //天空盒管线
        VkPipeline mine;                                        //挖掘选择框管线
        VkPipeline uiText;                                      //文本管线
    };

    struct procPipeline {
        //pipelineLayouts和pipelines方便传参
        pipelineList::pipelineLayout pipelineLayouts;
        pipelineList::pipeline pipelines;

        pipelineStruct::pipelineNode terrain[terrainEnum::terrainLayer::count]{
            {pipelineLayouts.scence,pipelines.terrain[terrainEnum::terrainLayer::Layer_0]},
            {pipelineLayouts.scence,pipelines.terrain[terrainEnum::terrainLayer::Layer_1]}
        };

        pipelineStruct::pipelineNode model  { pipelineLayouts.scence,pipelines.model };
        pipelineStruct::pipelineNode skybox { pipelineLayouts.scence,pipelines.skybox };
        pipelineStruct::pipelineNode mine   { pipelineLayouts.scence,pipelines.mine };
        pipelineStruct::pipelineNode uiText { pipelineLayouts.uiText,pipelines.uiText };



        void cleanupSwapChain(VkDevice& device) {
            vkDestroyPipelineLayout(device, pipelineLayouts.scence, nullptr);
            vkDestroyPipelineLayout(device, pipelineLayouts.uiText, nullptr);

            for (int layer = 0; layer < terrainEnum::terrainLayer::count; layer++) {
                vkDestroyPipeline(device, terrain[layer].pipeline, nullptr);
            }

            vkDestroyPipeline(device, model.pipeline, nullptr);
            vkDestroyPipeline(device, skybox.pipeline, nullptr);
            vkDestroyPipeline(device, mine.pipeline, nullptr);
            vkDestroyPipeline(device, uiText.pipeline, nullptr);
        }
    };
}