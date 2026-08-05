#pragma once



//engine列表空间
namespace engineList {
    struct procTickNode {
        std::vector<engineStruct::tickNode> node{};



        procTickNode() : node(engineEnum::tickEnum::count) {
            node[engineEnum::tickEnum::key]   .createTickNode(1024);
            node[engineEnum::tickEnum::uiText].createTickNode(4);
            node[engineEnum::tickEnum::plate] .createTickNode(4);
            node[engineEnum::tickEnum::cube]  .createTickNode(128);
        }
    };

    procTickNode procTick{};
}



//buffer列表空间
namespace bufferList {
    struct procRenderNode {
        std::vector<renderStruct::renderTemplateNode> nodePass{};           //渲染通道
        std::vector<bufferStruct::renderInfoNode> nodeInfo{};       //渲染通道信息



        procRenderNode(const std::vector<VkImage>& swapImageList) : nodePass(renderEnum::renderTemplateId::count), nodeInfo(renderEnum::renderInstanceId::count) {
            nodeInfo[renderEnum::renderInstanceId::scScence_ri] .createRenderInfoNode({ 105.0f / 255.f, 105.0f / 255.f, 105.0f / 255.f, 1.0f }, { 1.0f, 0 }, renderEnum::renderTemplateId::swapChain_rt);
            nodeInfo[renderEnum::renderInstanceId::osScence_ri] .createRenderInfoNode({ 105.0f / 255.f, 105.0f / 255.f, 105.0f / 255.f, 1.0f }, { 1.0f, 0 }, renderEnum::renderTemplateId::offScreen_rt);
            nodeInfo[renderEnum::renderInstanceId::osMirror_ri] .createRenderInfoNode({ 105.0f / 255.f, 105.0f / 255.f, 105.0f / 255.f, 1.0f }, { 1.0f, 0 }, renderEnum::renderTemplateId::offScreen_rt);
            nodeInfo[renderEnum::renderInstanceId::dsScence_ri] .createRenderInfoNode({ 105.0f / 255.f, 105.0f / 255.f, 105.0f / 255.f, 1.0f }, { 1.0f, 0 }, renderEnum::renderTemplateId::deferred_rt);
            nodeInfo[renderEnum::renderInstanceId::shadowMap_ri].createRenderInfoNode({ 105.0f / 255.f, 105.0f / 255.f, 105.0f / 255.f, 1.0f }, { 1.0f, 0 }, renderEnum::renderTemplateId::shadowMap_rt);

            //todo封装起来？
            nodeInfo[renderEnum::renderInstanceId::scScence_ri].scPtr->swapImageList.resize(swapImageList.size());
            for (int i = 0; i < swapImageList.size(); i++) {
                nodeInfo[renderEnum::renderInstanceId::scScence_ri].scPtr->swapImageList[i].image = swapImageList[i];
            }
        };

        void createRenderPassInfo(const VkExtent2D& scExtent, const VkFormat& colorFormat, const VkFormat& depthFormat, const VkSampleCountFlagBits& msaaSamples, const VkDevice& device, const VmaAllocator& memAllocator) {
            for (int i = 0; i < renderEnum::renderTemplateId::count; i++) {
                nodePass[i].createRenderNode((renderEnum::renderTemplateId::type)i, colorFormat, depthFormat, msaaSamples, device);
            }

            for (int i = 0; i < renderEnum::renderInstanceId::count; i++) {
                if (i == renderEnum::renderInstanceId::shadowMap_ri) {
                    nodeInfo[i].createRenderInfo(VkExtent2D{ renderInfo::shadowMapLength, renderInfo::shadowMapLength }, colorFormat, depthFormat, msaaSamples, nodePass[nodeInfo[i].renderTemplateType].render, device, memAllocator);
                }
                else {
                    nodeInfo[i].createRenderInfo(scExtent, colorFormat, depthFormat, msaaSamples, nodePass[nodeInfo[i].renderTemplateType].render, device, memAllocator);
                }
            }
        }

        bufferStruct::scFramebufferNode& getScFramebufferNode(renderEnum::renderInstanceId::type procRenderId) {
            if (debugInfo::LogSwitch) {
                if (nodeInfo[procRenderId].renderTemplateType != renderEnum::renderTemplateId::swapChain_rt) {
                    Log::printError(__FILE__, __LINE__, "帧缓冲结点类型不符");
                }
            }

            return *nodeInfo[procRenderId].scPtr;
        }

        bufferStruct::osFramebufferNode& getOsFramebufferNode(renderEnum::renderInstanceId::type procRenderId) {
            if (debugInfo::LogSwitch) {
                if (nodeInfo[procRenderId].renderTemplateType != renderEnum::renderTemplateId::offScreen_rt) {
                    Log::printError(__FILE__, __LINE__, "帧缓冲结点类型不符");
                }
            }

            return *nodeInfo[procRenderId].osPtr;
        }

        bufferStruct::dsFramebufferNode& getDsFramebufferNode(renderEnum::renderInstanceId::type procRenderId) {
            if (debugInfo::LogSwitch) {
                if (nodeInfo[procRenderId].renderTemplateType != renderEnum::renderTemplateId::deferred_rt) {
                    Log::printError(__FILE__, __LINE__, "帧缓冲结点类型不符");
                }
            }

            return *nodeInfo[procRenderId].dsPtr;
        }

        bufferStruct::shFramebufferNode& getShFramebufferNode(renderEnum::renderInstanceId::type procRenderId) {
            if (debugInfo::LogSwitch) {
                if (nodeInfo[procRenderId].renderTemplateType != renderEnum::renderTemplateId::shadowMap_rt) {
                    Log::printError(__FILE__, __LINE__, "帧缓冲结点类型不符");
                }
            }

            return *nodeInfo[procRenderId].shPtr;
        }

        void clearSwapChain(const VkDevice& device, const VmaAllocator& memAllocator) {
            for (int i = 0; i < renderEnum::renderInstanceId::count; i++) {
                nodeInfo[i].clearSwapChain(device, memAllocator);
            }
        }

        void recreateSwapChain(const std::vector<VkImage>& swapImageList, const VkExtent2D& scExtent, const VkFormat& colorFormat, const VkFormat& depthFormat, const VkSampleCountFlagBits& msaaSamples, const VkDevice& device, const VmaAllocator& memAllocator) {
            //todo封装起来？
            nodeInfo[renderEnum::renderInstanceId::scScence_ri].scPtr->swapImageList.resize(swapImageList.size());
            for (int i = 0; i < swapImageList.size(); i++) {
                nodeInfo[renderEnum::renderInstanceId::scScence_ri].scPtr->swapImageList[i].image = swapImageList[i];
            }
            
            for (int i = 0; i < renderEnum::renderInstanceId::count; i++) {
                if (i == renderEnum::renderInstanceId::shadowMap_ri) {
                    nodeInfo[i].createRenderInfo(VkExtent2D{ renderInfo::shadowMapLength, renderInfo::shadowMapLength }, colorFormat, depthFormat, msaaSamples, nodePass[nodeInfo[i].renderTemplateType].render, device, memAllocator);
                }
                else {
                    nodeInfo[i].createRenderInfo(scExtent, colorFormat, depthFormat, msaaSamples, nodePass[nodeInfo[i].renderTemplateType].render, device, memAllocator);
                }
            }
        }

        void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
            for (int i = 0; i < renderEnum::renderTemplateId::count; i++) {
                nodePass[i].clear(device);
            }

            for (int i = 0; i < renderEnum::renderInstanceId::count; i++) {
                nodeInfo[i].clear(device, memAllocator);
            }
        }
    };

    struct procDescriptorNode {
        VkDescriptorSetAllocateInfo allocInfo{};
        bufferEnum::procDescriptorId::type nodeId;

        std::vector<bufferStruct::descriptorNode> node{};



        procDescriptorNode(const VkDevice& device) : node(bufferEnum::procDescriptorId::count) {
            //allocInfo
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.pNext = nullptr;
            allocInfo.descriptorSetCount = 1;

            //scenceForward_d
            node[bufferEnum::procDescriptorId::scenceForward_d].config = {
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT,   bufferInfo::scenceForwardCount},    //顶点模型缓冲区描述符
                {1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_FRAGMENT_BIT, bufferInfo::scenceForwardCount},    //片段光照缓冲区描述符
                {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, bufferInfo::scenceForwardCount},    //片段纹理采样器描述符
            };

            //scenceDeferred_d
            node[bufferEnum::procDescriptorId::scenceDeferred_d].config = {
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT,   bufferInfo::scenceDeferredCount},   //顶点模型
                {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, bufferInfo::scenceDeferredCount},   //片段纹理
            };

            //uiText_d
            node[bufferEnum::procDescriptorId::uiText_d].config = {
                {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, bufferInfo::uiTextCount}            //片段纹理采样器描述符
            };

            //dsScence_d
            node[bufferEnum::procDescriptorId::dsScence_d].config = {
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_FRAGMENT_BIT, bufferInfo::dsScenceCount},         //片段光照统一缓冲
                {1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_FRAGMENT_BIT, bufferInfo::dsScenceCount},         //片段阴影统一缓冲
                {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, bufferInfo::dsScenceCount},         //片段坐标
                {3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, bufferInfo::dsScenceCount},         //片段法线
                {4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, bufferInfo::dsScenceCount},         //片段反射
                {5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, bufferInfo::dsScenceCount}          //片段阴影
            };

            //shadowMap_d
            node[bufferEnum::procDescriptorId::shadowMap_d].config = {
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, bufferInfo::shadowCount}     //顶点阴影mvp映射
            };

            for (int i = 0; i < bufferEnum::procDescriptorId::count; i++) {
                node[i].createDescriptorNode(device);
            }
        }

        void setAllocInfo(bufferEnum::procDescriptorId::type id) {
            allocInfo.descriptorPool = node[id].pool;
            allocInfo.pSetLayouts    = &node[id].layout;

            nodeId = id;
        }

        const VkWriteDescriptorSet& getWrite(int writeId, const VkDescriptorSet& dstSet, const VkBuffer& buffer, const VkDeviceSize& range) {
            return node[nodeId].config[writeId].setWrite(dstSet, buffer, range);
        }

        const VkWriteDescriptorSet& getWrite(int writeId, const VkDescriptorSet& dstSet, const VkSampler& sampler, const VkImageView& imageView) {
            return node[nodeId].config[writeId].setWrite(dstSet, sampler, imageView);
        }

        void clear(const VkDevice& device) {
            for (int i = 0; i < bufferEnum::procDescriptorId::count; i++) {
                node[i].clear(device);
            }
        }
    };

    struct procSamplerNode {
        std::vector<VkSampler> node{};



        procSamplerNode() : node(bufferEnum::procSamplerId::count) {}

        void createTextureSampler(const VkDevice& device, uint32_t terrainMip, uint32_t sModelMip, uint32_t dModelMip) {
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
                samplerInfo.maxAnisotropy    = 16.0f;

                //采样范围超过图像时候返回的颜色
                samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

                //指定使用的坐标系统，用于访问图像的纹素
                samplerInfo.unnormalizedCoordinates = VK_FALSE;

                //开启比较功能，那么纹素首先和值进行比较，并且比较后的值用于过滤操作
                samplerInfo.compareEnable = VK_FALSE;
                samplerInfo.compareOp     = VK_COMPARE_OP_ALWAYS;

                samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
                samplerInfo.mipLodBias = 0.0f;
                samplerInfo.minLod     = 0.0f;
            }

            //terrain_s
            samplerInfo.magFilter = VK_FILTER_NEAREST;
            samplerInfo.maxLod    = static_cast<float>(terrainMip);
            if (vkCreateSampler(device, &samplerInfo, nullptr, &node[bufferEnum::procSamplerId::terrain_s]) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "terrain采样器创建失败");
            }

            //staticModel_sn
            samplerInfo.magFilter = VK_FILTER_NEAREST;
            samplerInfo.maxLod    = static_cast<float>(sModelMip);
            if (vkCreateSampler(device, &samplerInfo, nullptr, &node[bufferEnum::procSamplerId::staticModel_sn]) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "sModelN采样器创建失败");
            }

            //staticModel_sl
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            if (vkCreateSampler(device, &samplerInfo, nullptr, &node[bufferEnum::procSamplerId::staticModel_sl]) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "sModelL采样器创建失败");
            }

            //dynamicModel_sn
            samplerInfo.magFilter = VK_FILTER_NEAREST;
            samplerInfo.maxLod    = static_cast<float>(dModelMip);
            if (vkCreateSampler(device, &samplerInfo, nullptr, &node[bufferEnum::procSamplerId::dynamicModel_sn]) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "dModelN采样器创建失败");
            }

            //dynamicModel_sl
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            if (vkCreateSampler(device, &samplerInfo, nullptr, &node[bufferEnum::procSamplerId::dynamicModel_sl]) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "dModelL采样器创建失败");
            }

            //uiText_s
            samplerInfo.magFilter     = VK_FILTER_LINEAR;
            samplerInfo.maxAnisotropy = 16.0f;
            if (vkCreateSampler(device, &samplerInfo, nullptr, &node[bufferEnum::procSamplerId::uiText_s]) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "uiText采样器创建失败");
            }

            //offScreen_s
            samplerInfo.magFilter     = VK_FILTER_LINEAR;
            samplerInfo.maxLod        = 0.0f;
            samplerInfo.maxAnisotropy = 1.0f;
            if (vkCreateSampler(device, &samplerInfo, nullptr, &node[bufferEnum::procSamplerId::offScreen_s]) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "offScreen采样器创建失败");
            }

            //shadowMap_s
            samplerInfo.magFilter        = VK_FILTER_NEAREST;
            samplerInfo.addressModeU     = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.addressModeV     = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.addressModeW     = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.anisotropyEnable = VK_FALSE;
            samplerInfo.maxAnisotropy    = 0;
            samplerInfo.mipmapMode       = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerInfo.mipLodBias       = 0;
            samplerInfo.minLod           = 0.0f;
            samplerInfo.maxLod           = 0.0f;
            if (vkCreateSampler(device, &samplerInfo, nullptr, &node[bufferEnum::procSamplerId::shadowMap_s]) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "shadow采样器创建失败");
            }
        }

        void clear(const VkDevice& device) {
            for (int i = 0; i < bufferEnum::procSamplerId::count; i++) {
                vkDestroySampler(device, node[i], nullptr);
            }
        }
    };



    struct procModelNode {
        std::vector<bufferStruct::staticModelNode>  sNode{};
        std::vector<bufferStruct::dynamicModelNode> dNode{};

        uint32_t sModelMip = 0;                  //在创建texture时赋值
        uint32_t dModelMip = 0;                  //在创建texture时赋值

        bool isEnable;
        pipelineEnum::procPipelineId::type pipeId;



        procModelNode(bool enable, pipelineEnum::procPipelineId::type pId) : sNode(bufferEnum::procStaticModelId::count), dNode(bufferEnum::procDynamicModelId::count) {
            sNode[bufferEnum::procStaticModelId::temple_m].create(fileInfo::model::temple, fileInfo::image::temple, /*fileInfo::image::temple,*/ bufferEnum::procSamplerId::staticModel_sn, glm::dvec3(0, 32, 0));

            dNode[bufferEnum::procDynamicModelId::droid_m].create(fileInfo::model::droid, { fileInfo::image::droidBodyC,fileInfo::image::droidHeadC,fileInfo::image::droidMiscC }, /*{ fileInfo::image::droidBodyN,fileInfo::image::droidHeadN,fileInfo::image::droidMiscN },*/ bufferEnum::procSamplerId::dynamicModel_sl, glm::dvec3(0, 32, 0));



            for (int i = 0; i < bufferEnum::procDynamicModelId::count; i++) {
                modelVertex::dVertexs[i].resize(dNode[i].node.size());
                modelVertex::dIndexs[i].resize(dNode[i].node.size());
            }

            isEnable = enable;
            pipeId   = pId;
        }

        procModelNode() = delete;

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

        void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
            for (int i = 0; i < bufferEnum::procStaticModelId::count; i++) {
                sNode[i].clear(device, memAllocator);
            }

            for (int i = 0; i < bufferEnum::procDynamicModelId::count; i++) {
                dNode[i].clear(device, memAllocator);
            }
        }
    };

    struct procShadowNode {
        bufferStruct::shadowTerrainBufferNode tShadowNode{};                    //地形阴影
        std::vector<bufferStruct::shadowBufferNode> sShadowNode{};              //静态模型阴影
        std::vector<std::vector<bufferStruct::shadowBufferNode>> dShadowNode{}; //动态模型阴影

        //辅助shadowMvp计算
        glm::mat4 shadowProj{};
        glm::mat4 shadowVP{};

        //程序化索引
        bool isEnable;
        pipelineEnum::procPipelineId::type pipeId;
        bufferEnum::procSamplerId::type samplerId;



        procShadowNode(bool enable, pipelineEnum::procPipelineId::type pId, bufferEnum::procSamplerId::type sId) : sShadowNode(bufferEnum::procStaticModelId::count), dShadowNode(bufferEnum::procDynamicModelId::count) {
            isEnable  = enable;
            pipeId    = pId;
            samplerId = sId;
        }

        procShadowNode() = delete;

        void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
            tShadowNode.clear(device, memAllocator);

            for (int i = 0; i < sShadowNode.size(); i++) {
                sShadowNode[i].clear(device, memAllocator);
            }

            for (int i = 0; i < dShadowNode.size(); i++) {
                for (int j = 0; j < dShadowNode[i].size(); j++) {
                    dShadowNode[i][j].clear(device, memAllocator);
                }
            }
        }
    };
}

//pipeline列表空间
namespace pipelineList {
    struct procPipelineNode {
        std::vector<pipelineStruct::pipeLayoutNode> nodeLayout{};
        std::vector<pipelineStruct::pipelineNode>   node{};



        procPipelineNode(const VkDevice& device, const std::vector<bufferStruct::descriptorNode>& dstList, const std::vector<renderStruct::renderTemplateNode>& renderList, const VkSampleCountFlagBits& msaa) : nodeLayout(bufferEnum::procDescriptorId::count), node(pipelineEnum::procPipelineId::count) {
            /*配置nodeLayout*/
            for (size_t i = 0; i < bufferEnum::procDescriptorId::count; i++) {
                nodeLayout[i].createPipeLayoutNode(device, dstList[i].layout);
            }

            /*配置node*/
            //node[pipelineEnum::procPipelineId::terrain_p] .createMeshPipelineNode(device, extent, msaa, bufferEnum::procDescriptorId::scenceDeferred_d, fileInfo::shader::terrainTask, fileInfo::shader::terrainMesh, fileInfo::shader::terrainFrag);
            node[pipelineEnum::procPipelineId::terrain_p]  .createPipelineNode(device, VK_SAMPLE_COUNT_1_BIT, renderEnum::renderAttId::deferred_ra,  renderList[renderEnum::renderTemplateId::deferred_rt],  nodeLayout[bufferEnum::procDescriptorId::scenceDeferred_d], { fileInfo::shader::terrainVert,  fileInfo::shader::terrainFrag  });
            node[pipelineEnum::procPipelineId::model_p]    .createPipelineNode(device, VK_SAMPLE_COUNT_1_BIT, renderEnum::renderAttId::deferred_ra,  renderList[renderEnum::renderTemplateId::deferred_rt],  nodeLayout[bufferEnum::procDescriptorId::scenceDeferred_d], { fileInfo::shader::pbrVert,      fileInfo::shader::pbrFrag      });
            node[pipelineEnum::procPipelineId::skybox_p]   .createPipelineNode(device, VK_SAMPLE_COUNT_1_BIT, renderEnum::renderAttId::deferred_ra,  renderList[renderEnum::renderTemplateId::deferred_rt],  nodeLayout[bufferEnum::procDescriptorId::scenceForward_d],  { fileInfo::shader::skyboxVert,   fileInfo::shader::skyboxFrag   });
            node[pipelineEnum::procPipelineId::mine_p]     .createPipelineNode(device, VK_SAMPLE_COUNT_1_BIT, renderEnum::renderAttId::deferred_ra,  renderList[renderEnum::renderTemplateId::deferred_rt],  nodeLayout[bufferEnum::procDescriptorId::scenceForward_d],  { fileInfo::shader::mineVert,     fileInfo::shader::mineFrag     });
            node[pipelineEnum::procPipelineId::uiText_p]   .createPipelineNode(device, VK_SAMPLE_COUNT_1_BIT, renderEnum::renderAttId::deferred_ra,  renderList[renderEnum::renderTemplateId::deferred_rt],  nodeLayout[bufferEnum::procDescriptorId::uiText_d],         { fileInfo::shader::uitextVert,   fileInfo::shader::uitextFrag   });
            node[pipelineEnum::procPipelineId::osMirror_p] .createPipelineNode(device, msaa,                  renderEnum::renderAttId::forward_ra,   renderList[renderEnum::renderTemplateId::offScreen_rt], nodeLayout[bufferEnum::procDescriptorId::scenceForward_d],  { fileInfo::shader::osMirrorVert, fileInfo::shader::osMirrorFrag });
            node[pipelineEnum::procPipelineId::osScence_p] .createPipelineNode(device, msaa,                  renderEnum::renderAttId::forward_ra,   renderList[renderEnum::renderTemplateId::swapChain_rt], nodeLayout[bufferEnum::procDescriptorId::scenceForward_d],  { fileInfo::shader::osScenceVert, fileInfo::shader::osScenceFrag });
            node[pipelineEnum::procPipelineId::dsScence_p] .createPipelineNode(device, msaa,                  renderEnum::renderAttId::forward_ra,   renderList[renderEnum::renderTemplateId::swapChain_rt], nodeLayout[bufferEnum::procDescriptorId::dsScence_d],       { fileInfo::shader::dsScenceVert, fileInfo::shader::dsScenceFrag });
            node[pipelineEnum::procPipelineId::shadowMap_p].createPipelineNode(device, VK_SAMPLE_COUNT_1_BIT, renderEnum::renderAttId::shadowMap_ra, renderList[renderEnum::renderTemplateId::shadowMap_rt], nodeLayout[bufferEnum::procDescriptorId::shadowMap_d],      { fileInfo::shader::shadowVert,   fileInfo::shader::shadowFrag   });

            /*配置node.pipeConfig*/
            //depthStencil 实现只在空白处显示
            node[pipelineEnum::procPipelineId::skybox_p].pipeConfig.depthStencil.depthWriteEnable = VK_FALSE;
            node[pipelineEnum::procPipelineId::skybox_p].pipeConfig.depthStencil.depthCompareOp   = VK_COMPARE_OP_LESS_OR_EQUAL;

            //primitiveState 线段图元，rasterizer 线填充
            node[pipelineEnum::procPipelineId::mine_p].pipeConfig.primitiveState.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
            node[pipelineEnum::procPipelineId::mine_p].pipeConfig.rasterizer.polygonMode  = VK_POLYGON_MODE_LINE;
            node[pipelineEnum::procPipelineId::mine_p].pipeConfig.rasterizer.lineWidth    = 1.0f;

            //primitiveState 条带图元，rasterizer 顺时针，depthStencil 实现始终渲染在最上层
            node[pipelineEnum::procPipelineId::uiText_p].pipeConfig.primitiveState.topology       = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
            node[pipelineEnum::procPipelineId::uiText_p].pipeConfig.rasterizer.frontFace          = VK_FRONT_FACE_CLOCKWISE;
            node[pipelineEnum::procPipelineId::uiText_p].pipeConfig.depthStencil.depthTestEnable  = VK_FALSE;
            node[pipelineEnum::procPipelineId::uiText_p].pipeConfig.depthStencil.depthWriteEnable = VK_FALSE;

            //pipeDynamicStageState 动态视口与动态裁剪，rasterizer 剔除正面
            node[pipelineEnum::procPipelineId::osScence_p].pipeConfig.setPipeDynamicStage({ VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR });
            node[pipelineEnum::procPipelineId::osScence_p].pipeConfig.rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;

            //pipeDynamicStageState 动态视口与动态裁剪，rasterizer 剔除正面
            node[pipelineEnum::procPipelineId::dsScence_p].pipeConfig.setPipeDynamicStage({ VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR });
            node[pipelineEnum::procPipelineId::dsScence_p].pipeConfig.rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;

            //depthStencil 防止照射面被误判为阴影
            node[pipelineEnum::procPipelineId::shadowMap_p].pipeConfig.depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

            /*创建pipeCache*/
            createPipelineCache(device);
        }

        procPipelineNode() = delete;

        

        //创建管线
        template<typename VertexType>
        void createPipeline(const VkDevice& device, pipelineEnum::procPipelineId::type id, const VkExtent2D& extent) {
            //配置vertInputStage
            auto binding   = VertexType::getBindingDescription();
            auto attribute = VertexType::getAttributeDescriptions();

            VkPipelineVertexInputStateCreateInfo vertInputStage{};
            vertInputStage.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertInputStage.vertexBindingDescriptionCount   = uint32_t(binding.size());
            vertInputStage.vertexAttributeDescriptionCount = uint32_t(attribute.size());
            vertInputStage.pVertexBindingDescriptions      = binding.data();
            vertInputStage.pVertexAttributeDescriptions    = attribute.data();

            //配置pipeConfig
            pipelineStruct::pipeConfigNode& pipeConfig = node[id].pipeConfig;
            pipeConfig.viewport.width  = (float)extent.width;
            pipeConfig.viewport.height = (float)extent.height;
            pipeConfig.scissor.extent  = extent;
            pipeConfig.pipeInfo.pVertexInputState = &vertInputStage;

            //创建pipeline
            if (vkCreateGraphicsPipelines(device, pipeCache, 1, &pipeConfig.pipeInfo, nullptr, &node[id].pipeline) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "管线创建失败");
            }
        }
        


        void clearSwapChain(const VkDevice& device) {
            for (int i = 0; i < pipelineEnum::procPipelineId::count; i++) {
                node[i].clearSwapChain(device);
            }
        }

        void clear(const VkDevice& device) {
            for (size_t i = 0; i < bufferEnum::procDescriptorId::count; i++) {
                nodeLayout[i].clear(device);
            }

            for (int i = 0; i < pipelineEnum::procPipelineId::count; i++) {
                node[i].clear(device);
            }

            vkDestroyPipelineCache(device, pipeCache, nullptr);
        }

    private:
        VkPipelineCache pipeCache;



        //创建pipeCache
        void createPipelineCache(const VkDevice& device){
            VkPipelineCacheCreateInfo pipeCacheInfo{};
            pipeCacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

            if (vkCreatePipelineCache(device, &pipeCacheInfo, nullptr, &pipeCache)) {
                Log::printError(__FILE__, __LINE__, "管线缓存创建失败");
            }
        }

    public:
        VkPipelineLayout& getPipeLayout(pipelineEnum::procPipelineId::type id) {
            return node[id].pipeConfig.pipeInfo.layout;
        }

        VkPipeline& getPipeline(pipelineEnum::procPipelineId::type id) {
            return node[id].pipeline;
        }
    };
}