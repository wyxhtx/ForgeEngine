#pragma once



namespace bufferList {
    struct descriptor {
        bufferStruct::descriptorNode scence;  //场景描述符
        bufferStruct::descriptorNode uiText;  //文本描述符

        void createDescriptorPool(VkDevice& device) {
            VkDescriptorPoolCreateInfo descriptorPoolInfo{};
            descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;

            //scence
            {
                std::array<VkDescriptorPoolSize, 3> poolSizeScence = {};				
                poolSizeScence[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;				//顶点统一缓冲区
                poolSizeScence[0].descriptorCount = bufferInfo::bufferScenceNodeSize;	
                poolSizeScence[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;				//片段统一缓冲区
                poolSizeScence[1].descriptorCount = bufferInfo::bufferScenceNodeSize;	
                poolSizeScence[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;		//片段纹理采样器
                poolSizeScence[2].descriptorCount = bufferInfo::bufferScenceNodeSize;

                descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizeScence.size());
                descriptorPoolInfo.pPoolSizes = poolSizeScence.data();
                descriptorPoolInfo.maxSets = bufferInfo::bufferScenceNodeSize;          //poolSizeScence.descriptorCount最好填大点不支持自动分配
            }

            if (vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &scence.pool) != VK_SUCCESS) {
                Log::error(__FILE__, __LINE__, "描述符池创建失败");
                throw;
            }

            //uiText
            {
                std::array<VkDescriptorPoolSize, 1> poolSizeUItext{};
                poolSizeUItext[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                poolSizeUItext[0].descriptorCount = bufferInfo::bufferTextUINodeSize;

                descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizeUItext.size());
                descriptorPoolInfo.pPoolSizes = poolSizeUItext.data();
                descriptorPoolInfo.maxSets = bufferInfo::bufferTextUINodeSize;
            }

            if (vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &uiText.pool) != VK_SUCCESS) {
                Log::error(__FILE__, __LINE__, "描述符池创建失败");
                throw;
            }
        }

        void createDescriptorSetLayout(VkDevice& device) {
            //创建描述符布局
            VkDescriptorSetLayoutCreateInfo layoutInfo{};
            layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.pNext = nullptr;

            //scence
            {
                std::array<VkDescriptorSetLayoutBinding, 3> bindingOpacScence = {};         //描述符布局绑定描述符
                bindingOpacScence[0].binding = 0;					                        //顶点统一缓冲区
                bindingOpacScence[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                bindingOpacScence[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
                bindingOpacScence[0].descriptorCount = 1;
                bindingOpacScence[0].pImmutableSamplers = nullptr;
                bindingOpacScence[1].binding = 1;                                           //片段统一缓冲区
                bindingOpacScence[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                bindingOpacScence[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                bindingOpacScence[1].descriptorCount = 1;
                bindingOpacScence[1].pImmutableSamplers = nullptr;
                bindingOpacScence[2].binding = 2;					                        //片段纹理采样器
                bindingOpacScence[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                bindingOpacScence[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                bindingOpacScence[2].descriptorCount = 1;
                bindingOpacScence[2].pImmutableSamplers = nullptr;

                layoutInfo.bindingCount = static_cast<uint32_t>(bindingOpacScence.size());
                layoutInfo.pBindings = bindingOpacScence.data();
            }

            if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &scence.layout) != VK_SUCCESS) {
                Log::error(__FILE__, __LINE__, "描述符布局配置失败");
                throw;
            }

            //uiText
            {
                std::array<VkDescriptorSetLayoutBinding, 1> bindingUItext{};
                bindingUItext[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                bindingUItext[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                bindingUItext[0].binding = 0;
                bindingUItext[0].descriptorCount = 1;

                layoutInfo.bindingCount = static_cast<uint32_t>(bindingUItext.size());
                layoutInfo.pBindings = bindingUItext.data();
            }

            if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &uiText.layout) != VK_SUCCESS) {
                Log::error(__FILE__, __LINE__, "描述符布局配置失败");
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
                Log::error(__FILE__, __LINE__, "地形贴图采样器创建失败");
                throw;
            }

            //model
            samplerInfo.maxLod = static_cast<float>(modelMipLevel);

            if (vkCreateSampler(device, &samplerInfo, nullptr, &model) != VK_SUCCESS) {
                Log::error(__FILE__, __LINE__, "模型贴图采样器创建失败");
                throw;
            }

            //uiText
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            samplerInfo.minFilter = VK_FILTER_LINEAR;

            samplerInfo.maxLod = 0.0f;

            if (vkCreateSampler(device, &samplerInfo, nullptr, &uiText) != VK_SUCCESS) {
                Log::error(__FILE__, __LINE__, "文本贴图采样器创建失败");
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
        VkPipeline terrain[pipelineEnum::terrain::count];   //地形管线
        VkPipeline model;                                   //模型管线
        VkPipeline skybox;                                  //天空盒管线
        VkPipeline mine;                                    //挖掘选择框管线
        VkPipeline uiText;                                  //文本管线
    };

    struct procPipeline {
        //pipelineLayouts和pipelines用于一次传整个列表
        pipelineList::pipelineLayout pipelineLayouts;
        pipelineList::pipeline pipelines;

        pipelineStruct::pipelineNode terrain{
            pipelineLayouts.scence,pipelines.terrain[pipelineEnum::terrain::Lod_0] 
        };



        pipelineStruct::pipelineNode model      { pipelineLayouts.scence,pipelines.model };
        pipelineStruct::pipelineNode skybox     { pipelineLayouts.scence,pipelines.skybox };
        pipelineStruct::pipelineNode mine       { pipelineLayouts.scence,pipelines.mine };
        pipelineStruct::pipelineNode uiText     { pipelineLayouts.uiText,pipelines.uiText };

        void cleanupSwapChain(VkDevice& device) {
            vkDestroyPipelineLayout(device, pipelineLayouts.scence, nullptr);
            vkDestroyPipelineLayout(device, pipelineLayouts.uiText, nullptr);

            vkDestroyPipeline(device, terrain.pipeline, nullptr);
            vkDestroyPipeline(device, model.pipeline, nullptr);
            vkDestroyPipeline(device, skybox.pipeline, nullptr);
            vkDestroyPipeline(device, mine.pipeline, nullptr);
            vkDestroyPipeline(device, uiText.pipeline, nullptr);
        }
    };
}