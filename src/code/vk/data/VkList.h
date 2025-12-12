#pragma once



namespace bufferList {
    struct procRenderNode {
        std::array<bufferStruct::renderNode, bufferEnum::procRenderId::count> node{};



        procRenderNode() {
            node[bufferEnum::procRenderId::osMirror].createRenderNode({ 105.0f / 255.f, 105.0f / 255.f, 105.0f / 255.f, 1.0f }, { 1.0f, 0 }, bufferEnum::frameNodeType::os);
            node[bufferEnum::procRenderId::osScence].createRenderNode({ 105.0f / 255.f, 105.0f / 255.f, 105.0f / 255.f, 1.0f }, { 1.0f, 0 }, bufferEnum::frameNodeType::os);
            node[bufferEnum::procRenderId::scScence].createRenderNode({ 105.0f / 255.f, 105.0f / 255.f, 105.0f / 255.f, 1.0f }, { 1.0f, 0 }, bufferEnum::frameNodeType::sc);
        };

        void createRenderPassInfo(const VkExtent2D& scExtent, const VkFormat& colorFormat, const VkFormat& depthFormat, const VkSampleCountFlagBits& msaaSamples, const VkDevice& device) {
            for (int i = 0; i < bufferEnum::procRenderId::count; i++) {
                node[i].createRenderPassInfo(scExtent, colorFormat, depthFormat, msaaSamples, device);
            }
        }

        bufferStruct::scFramebufferNode& getScFramebufferNode(int procRenderId) {
            if (node[procRenderId].frameNodeType != bufferEnum::frameNodeType::sc) {
                Log::printError(__FILE__, __LINE__, "帧缓冲结点类型不符");
            }

            return *node[procRenderId].scPtr;
        }

        bufferStruct::osFramebufferNode& getOsFramebufferNode(int procRenderId) {
            if (node[procRenderId].frameNodeType != bufferEnum::frameNodeType::os) {
                Log::printError(__FILE__, __LINE__, "帧缓冲结点类型不符");
            }

            return *node[procRenderId].osPtr;
        }

        void cleanupSwapChain(const VkDevice& device, const VmaAllocator& allocator) {
            for (int i = 0; i < bufferEnum::procRenderId::count; i++) {
                node[i].cleanupSwapChain(device, allocator);
            }
        }

        void recreateSwapChain(const VkExtent2D& scExtent, const VkDevice& device) {
            for (int i = 0; i < bufferEnum::procRenderId::count; i++) {
                node[i].recreateSwapChain(scExtent, device);
            }
        }

        void clear(const VkDevice& device, const VmaAllocator& allocator) {
            for (int i = 0; i < bufferEnum::procRenderId::count; i++) {
                node[i].clear(device, allocator);
            }
        }
    };

    struct procDescriptorNode {
        VkDescriptorSetAllocateInfo allocInfo{};
        int nodeId = bufferEnum::procDescriptorId::count + 1;   //用于简化getWrite

        std::array<bufferStruct::descriptorNode, bufferEnum::procDescriptorId::count> node{};



        procDescriptorNode(const VkDevice& device) {
            //allocInfo
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.pNext = nullptr;
            allocInfo.descriptorSetCount = 1;

            //node
            node[bufferEnum::procDescriptorId::scence].config = {
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT,   bufferInfo::bufferScenceNodeSize},     //顶点模型缓冲区描述符
                {1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_FRAGMENT_BIT, bufferInfo::bufferScenceNodeSize},     //片段光照缓冲区描述符
                {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, bufferInfo::bufferScenceNodeSize},     //片段纹理采样器描述符
                {3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, bufferInfo::bufferScenceNodeSize}      //片段高光采样器描述符
            };

            node[bufferEnum::procDescriptorId::uiText].config = {
                {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, bufferInfo::bufferUINodeSize}          //片段纹理采样器描述符
            };

            for (int i = 0; i < bufferEnum::procDescriptorId::count; i++) {
                node[i].createDescriptorNode(device);
            }
        }

        void setAllocInfo(int id) {
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
        VkSampler node[bufferEnum::procSamplerId::count];



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
                samplerInfo.maxAnisotropy = 16.0f;

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
            if (vkCreateSampler(device, &samplerInfo, nullptr, &node[bufferEnum::procSamplerId::terrain]) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "terrain采样器创建失败");
                throw;
            }

            //sModelN
            samplerInfo.maxLod = static_cast<float>(sModelMip);
            if (vkCreateSampler(device, &samplerInfo, nullptr, &node[bufferEnum::procSamplerId::sModelN]) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "sModelN采样器创建失败");
                throw;
            }

            //sModelL
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            if (vkCreateSampler(device, &samplerInfo, nullptr, &node[bufferEnum::procSamplerId::sModelL]) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "sModelL采样器创建失败");
                throw;
            }

            //dModelN
            samplerInfo.magFilter = VK_FILTER_NEAREST;
            samplerInfo.maxLod = static_cast<float>(dModelMip);
            if (vkCreateSampler(device, &samplerInfo, nullptr, &node[bufferEnum::procSamplerId::dModelN]) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "dModelN采样器创建失败");
                throw;
            }

            //dModelL
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            if (vkCreateSampler(device, &samplerInfo, nullptr, &node[bufferEnum::procSamplerId::dModelL]) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "dModelL采样器创建失败");
                throw;
            }

            //offScreen
            samplerInfo.maxLod = 0.0f;
            samplerInfo.maxAnisotropy = 1.0f;
            if (vkCreateSampler(device, &samplerInfo, nullptr, &node[bufferEnum::procSamplerId::offScreen]) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "offScreen采样器创建失败");
                throw;
            }

            //uiText
            samplerInfo.maxAnisotropy = 16.0f;
            if (vkCreateSampler(device, &samplerInfo, nullptr, &node[bufferEnum::procSamplerId::uiText]) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "uiText采样器创建失败");
                throw;
            }
        }

        void clear(const VkDevice& device) {
            for (int i = 0; i < bufferEnum::procSamplerId::count; i++) {
                vkDestroySampler(device, node[i], nullptr);
            }
        }
    };



    struct procModelNode {
        std::array<bufferStruct::staticModelNode, bufferEnum::procStaticModelId::count> sNode{};
        std::array<bufferStruct::dynamicModelNode, bufferEnum::procDynamicModelId::count> dNode{};

        uint32_t sModelMip = 0;                  //在创建texture时赋值
        uint32_t dModelMip = 0;                  //在创建texture时赋值



        procModelNode() {
            sNode[bufferEnum::procStaticModelId::temple].create(fileInfo::model::temple, fileInfo::image::temple, bufferEnum::procSamplerId::sModelN);

            dNode[bufferEnum::procDynamicModelId::droid].create(fileInfo::model::droid, { fileInfo::image::droidBody,fileInfo::image::droidHead,fileInfo::image::droidMisc }, bufferEnum::procSamplerId::dModelL);



            for (int i = 0; i < bufferEnum::procDynamicModelId::count; i++) {
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

        void clear(const VkDevice& device, const VmaAllocator& allocator) {
            for (int i = 0; i < bufferEnum::procStaticModelId::count; i++) {
                sNode[i].clear(device, allocator);
            }

            for (int i = 0; i < bufferEnum::procDynamicModelId::count; i++) {
                dNode[i].clear(device, allocator);
            }
        }
    };
}

namespace pipelineList {
    struct procPipelineNode {
        std::array<pipelineStruct::pipelineNode, pipelineEnum::procPipelineId::count> node{};



        procPipelineNode() {
            node[pipelineEnum::procPipelineId::terrain] .createPipelineNode(bufferEnum::procDescriptorId::scence, fileInfo::shader::terrainVert,  fileInfo::shader::terrainFrag);
            node[pipelineEnum::procPipelineId::model]   .createPipelineNode(bufferEnum::procDescriptorId::scence, fileInfo::shader::pbrVert,      fileInfo::shader::pbrFrag);
            node[pipelineEnum::procPipelineId::skybox]  .createPipelineNode(bufferEnum::procDescriptorId::scence, fileInfo::shader::skyboxVert,   fileInfo::shader::skyboxFrag);
            node[pipelineEnum::procPipelineId::mine]    .createPipelineNode(bufferEnum::procDescriptorId::scence, fileInfo::shader::mineVert,     fileInfo::shader::mineFrag);
            node[pipelineEnum::procPipelineId::osMirror].createPipelineNode(bufferEnum::procDescriptorId::scence, fileInfo::shader::osMirrorVert, fileInfo::shader::osMirrorFrag);
            node[pipelineEnum::procPipelineId::osScence].createPipelineNode(bufferEnum::procDescriptorId::scence, fileInfo::shader::osScenceVert, fileInfo::shader::osScenceFrag);
            node[pipelineEnum::procPipelineId::uiText]  .createPipelineNode(bufferEnum::procDescriptorId::uiText, fileInfo::shader::uitextVert,   fileInfo::shader::uitextFrag);
        }

        template<typename VertexType>
        void createPipeline(const VkDevice& device, int id, std::array<VkPipelineShaderStageCreateInfo, 2>& shader, VkPipelineVertexInputStateCreateInfo& input, VkGraphicsPipelineCreateInfo& pipelineInfo) {
            shader[0] = loadShader(device, node[id].vertFile, VK_SHADER_STAGE_VERTEX_BIT);
            shader[1] = loadShader(device, node[id].fragFile, VK_SHADER_STAGE_FRAGMENT_BIT);
            
            auto binding   = VertexType::getBindingDescription();
            auto attribute = VertexType::getAttributeDescriptions();

            input.vertexBindingDescriptionCount   = uint32_t(binding.size());
            input.vertexAttributeDescriptionCount = uint32_t(attribute.size());
            input.pVertexBindingDescriptions      = binding.data();
            input.pVertexAttributeDescriptions    = attribute.data();

            if (vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineInfo, nullptr, &node[id].pipeline) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "管线创建失败");
                throw;
            }
        }

        void cleanupSwapChain(const VkDevice& device) {
            for (int i = 0; i < pipelineEnum::procPipelineId::count; i++) {
                node[i].clear(device);
            }
        }

        void clear(const VkDevice& device) {
            for (int i = 0; i < pipelineEnum::procPipelineId::count; i++) {
                node[i].clear(device);
            }

            vkDestroyPipelineCache(device, pipelineCache, nullptr);

            for (auto shaderModule : shaderModule) {
                vkDestroyShaderModule(device, shaderModule, nullptr);
            }
        }

    private:
        VkPipelineCache pipelineCache;
        std::vector<VkShaderModule> shaderModule;



        void createPipelineCache(const VkDevice& device){
            VkPipelineCacheCreateInfo pipelineCacheCreateInfo{};
            pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

            if (vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &pipelineCache)) {
                Log::printError(__FILE__, __LINE__, "管线缓存创建失败");
                throw;
            }
        }

        VkPipelineShaderStageCreateInfo loadShader(const VkDevice& device, std::string fileName, VkShaderStageFlagBits stage) {
            shaderModule.push_back(createShaderModule(device, readFile(fileName)));

            VkPipelineShaderStageCreateInfo shaderStage = {};
            shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStage.stage = stage;
            shaderStage.module = shaderModule.back();
            shaderStage.pName = "main";
            return shaderStage;
        }



        VkShaderModule createShaderModule(const VkDevice& device, const std::vector<char>& code) {
            VkShaderModuleCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            createInfo.codeSize = code.size();
            createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

            VkShaderModule shaderModule;
            if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "着色器创建失败");
                throw;
            }

            return shaderModule;
        }

        static std::vector<char> readFile(const std::string& filename) {
            std::ifstream file(filename, std::ios::ate | std::ios::binary);

            if (!file.is_open()) {
                Log::printError(__FILE__, __LINE__, "着色器文件打开失败");
                throw;
            }

            size_t fileSize = (size_t)file.tellg();
            std::vector<char> buffer(fileSize);

            file.seekg(0);
            file.read(buffer.data(), fileSize);
            file.close();

            return buffer;
        }
    };
}