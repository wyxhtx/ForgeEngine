#pragma once



namespace bufferList {
    struct procRenderNode {
        std::array<bufferStruct::renderNode, bufferEnum::frameNodeId::count> nodePass{};
        std::array<bufferStruct::renderInfoNode, bufferEnum::procRenderId::count> node{};



        procRenderNode() {
            node[bufferEnum::procRenderId::scScence].createRenderInfoNode({ 105.0f / 255.f, 105.0f / 255.f, 105.0f / 255.f, 1.0f }, { 1.0f, 0 }, bufferEnum::frameNodeId::sc);
            node[bufferEnum::procRenderId::osScence].createRenderInfoNode({ 105.0f / 255.f, 105.0f / 255.f, 105.0f / 255.f, 1.0f }, { 1.0f, 0 }, bufferEnum::frameNodeId::os);
            node[bufferEnum::procRenderId::osMirror].createRenderInfoNode({ 105.0f / 255.f, 105.0f / 255.f, 105.0f / 255.f, 1.0f }, { 1.0f, 0 }, bufferEnum::frameNodeId::os);
            node[bufferEnum::procRenderId::dsScence].createRenderInfoNode({ 105.0f / 255.f, 105.0f / 255.f, 105.0f / 255.f, 1.0f }, { 1.0f, 0 }, bufferEnum::frameNodeId::ds);
        };

        void createRenderPassInfo(const VkExtent2D& scExtent, const VkFormat& colorFormat, const VkFormat& depthFormat, const VkSampleCountFlagBits& msaaSamples, const VkDevice& device, const VmaAllocator& allocator) {
            for (int i = 0; i < bufferEnum::frameNodeId::count; i++) {
                nodePass[i].createRenderNode(i, colorFormat, depthFormat, msaaSamples, device);
            }

            for (int i = 0; i < bufferEnum::procRenderId::count; i++) {
                node[i].createRenderInfo(scExtent, colorFormat, depthFormat, msaaSamples, nodePass[node[i].frameNodeType].render, device, allocator);
            }
        }

        bufferStruct::scFramebufferNode& getScFramebufferNode(bufferEnum::procRenderId::type procRenderId) {
            if (debugInfo::LogSwitch) {
                if (node[procRenderId].frameNodeType != bufferEnum::frameNodeId::sc) {
                    Log::printError(__FILE__, __LINE__, "帧缓冲结点类型不符");
                }
            }

            return *node[procRenderId].scPtr;
        }

        bufferStruct::osFramebufferNode& getOsFramebufferNode(bufferEnum::procRenderId::type procRenderId) {
            if (debugInfo::LogSwitch) {
                if (node[procRenderId].frameNodeType != bufferEnum::frameNodeId::os) {
                    Log::printError(__FILE__, __LINE__, "帧缓冲结点类型不符");
                }
            }

            return *node[procRenderId].osPtr;
        }

        bufferStruct::dsFramebufferNode& getDsFramebufferNode(bufferEnum::procRenderId::type procRenderId) {
            if (debugInfo::LogSwitch) {
                if (node[procRenderId].frameNodeType != bufferEnum::frameNodeId::ds) {
                    Log::printError(__FILE__, __LINE__, "帧缓冲结点类型不符");
                }
            }

            return *node[procRenderId].dsPtr;
        }

        void cleanupSwapChain(const VkDevice& device, const VmaAllocator& allocator) {
            for (int i = 0; i < bufferEnum::procRenderId::count; i++) {
                node[i].cleanupSwapChain(device, allocator);
            }
        }

        void recreateSwapChain(const VkExtent2D& scExtent, const VkFormat& colorFormat, const VkFormat& depthFormat, const VkSampleCountFlagBits& msaaSamples, const VkDevice& device, const VmaAllocator& allocator) {
            for (int i = 0; i < bufferEnum::procRenderId::count; i++) {
                node[i].createRenderInfo(scExtent, colorFormat, depthFormat, msaaSamples, nodePass[node[i].frameNodeType].render, device, allocator);
            }
        }

        void clear(const VkDevice& device, const VmaAllocator& allocator) {
            for (int i = 0; i < bufferEnum::frameNodeId::count; i++) {
                nodePass[i].clear(device);
            }

            for (int i = 0; i < bufferEnum::procRenderId::count; i++) {
                node[i].clear(device, allocator);
            }
        }
    };

    struct procDescriptorNode {
        VkDescriptorSetAllocateInfo allocInfo{};
        bufferEnum::procDescriptorId::type nodeId;

        std::array<bufferStruct::descriptorNode, bufferEnum::procDescriptorId::count> node{};



        procDescriptorNode(const VkDevice& device) {
            //allocInfo
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.pNext = nullptr;
            allocInfo.descriptorSetCount = 1;

            //node
            node[bufferEnum::procDescriptorId::scenceForward].config = {
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT,   bufferInfo::scenceForwardSize},    //顶点模型缓冲区描述符
                {1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_FRAGMENT_BIT, bufferInfo::scenceForwardSize},    //片段光照缓冲区描述符
                {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, bufferInfo::scenceForwardSize},    //片段纹理采样器描述符
                {3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, bufferInfo::scenceForwardSize}     //片段高光采样器描述符
            };

            node[bufferEnum::procDescriptorId::scenceDeferred].config = {
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT,   bufferInfo::scenceDeferredSize},   //顶点模型
                {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, bufferInfo::scenceDeferredSize},   //片段纹理
                {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, bufferInfo::scenceDeferredSize}    //片段法线
            };

            node[bufferEnum::procDescriptorId::dsScence].config = {
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT,   bufferInfo::dsScenceSize},         //顶点光照
                {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, bufferInfo::dsScenceSize},         //片段坐标
                {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, bufferInfo::dsScenceSize},         //片段法线
                {3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, bufferInfo::dsScenceSize}          //片段反射
            };

            node[bufferEnum::procDescriptorId::uiText].config = {
                {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, bufferInfo::uiTextSize}            //片段纹理采样器描述符
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

        pipelineEnum::procPipelineId::type pipeId;



        procModelNode(pipelineEnum::procPipelineId::type pId) {
            //todo temple没有法线贴图，暂时拿fileInfo::image::temple凑数
            sNode[bufferEnum::procStaticModelId::temple].create(fileInfo::model::temple, fileInfo::image::temple, fileInfo::image::temple, bufferEnum::procSamplerId::sModelN);

            dNode[bufferEnum::procDynamicModelId::droid].create(fileInfo::model::droid, { fileInfo::image::droidBodyC,fileInfo::image::droidHeadC,fileInfo::image::droidMiscC }, { fileInfo::image::droidBodyN,fileInfo::image::droidHeadN,fileInfo::image::droidMiscN }, bufferEnum::procSamplerId::dModelL);



            for (int i = 0; i < bufferEnum::procDynamicModelId::count; i++) {
                modelVertex::dVertexs[i].resize(dNode[i].node.size());
                modelVertex::dIndexs[i].resize(dNode[i].node.size());
            }

            pipeId = pId;
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
        std::array<pipelineStruct::pipeLayoutNode, bufferEnum::procDescriptorId::count> nodeLayout{};
        std::array<pipelineStruct::pipelineNode, pipelineEnum::procPipelineId::count> node{};



        procPipelineNode() {
            node[pipelineEnum::procPipelineId::terrain] .createPipelineNode(bufferEnum::procDescriptorId::scenceForward,  fileInfo::shader::terrainVert,  fileInfo::shader::terrainFrag);
            node[pipelineEnum::procPipelineId::model]   .createPipelineNode(bufferEnum::procDescriptorId::scenceDeferred, fileInfo::shader::pbrVert,      fileInfo::shader::pbrFrag);
            node[pipelineEnum::procPipelineId::skybox]  .createPipelineNode(bufferEnum::procDescriptorId::scenceForward,  fileInfo::shader::skyboxVert,   fileInfo::shader::skyboxFrag);
            node[pipelineEnum::procPipelineId::mine]    .createPipelineNode(bufferEnum::procDescriptorId::scenceForward,  fileInfo::shader::mineVert,     fileInfo::shader::mineFrag);
            node[pipelineEnum::procPipelineId::osMirror].createPipelineNode(bufferEnum::procDescriptorId::scenceForward,  fileInfo::shader::osMirrorVert, fileInfo::shader::osMirrorFrag);
            node[pipelineEnum::procPipelineId::osScence].createPipelineNode(bufferEnum::procDescriptorId::scenceForward,  fileInfo::shader::osScenceVert, fileInfo::shader::osScenceFrag);
            node[pipelineEnum::procPipelineId::dsScence].createPipelineNode(bufferEnum::procDescriptorId::dsScence,       fileInfo::shader::dsScenceVert, fileInfo::shader::dsScenceFrag);
            node[pipelineEnum::procPipelineId::uiText]  .createPipelineNode(bufferEnum::procDescriptorId::uiText,         fileInfo::shader::uitextVert,   fileInfo::shader::uitextFrag);
        }

        void createPipeLayout(const VkDevice& device, std::array<bufferStruct::descriptorNode, bufferEnum::procDescriptorId::count>& dstNode) {
            for (size_t i = 0; i < bufferEnum::procDescriptorId::count; i++) {
                nodeLayout[i].createPipeLayoutNode(device, dstNode[i].layout);
            }
        }

        template<typename VertexType>
        void createPipeline(const VkDevice& device, pipelineEnum::procPipelineId::type id, std::array<VkPipelineShaderStageCreateInfo, 2>& shader, VkPipelineVertexInputStateCreateInfo& input, VkGraphicsPipelineCreateInfo& pipelineInfo) {
            shader[0] = loadShader(device, node[id].vertFile, VK_SHADER_STAGE_VERTEX_BIT);
            shader[1] = loadShader(device, node[id].fragFile, VK_SHADER_STAGE_FRAGMENT_BIT);
            
            auto binding   = VertexType::getBindingDescription();
            auto attribute = VertexType::getAttributeDescriptions();

            input.vertexBindingDescriptionCount   = uint32_t(binding.size());
            input.vertexAttributeDescriptionCount = uint32_t(attribute.size());
            input.pVertexBindingDescriptions      = binding.data();
            input.pVertexAttributeDescriptions    = attribute.data();

            pipelineInfo.layout = getPipeLayout(id);

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
            for (size_t i = 0; i < bufferEnum::procDescriptorId::count; i++) {
                nodeLayout[i].clear(device);
            }

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

    public:
        VkPipelineLayout& getPipeLayout(pipelineEnum::procPipelineId::type id) {
            return nodeLayout[node[id].pipeLayoutId].pipeLayout;
        }

        VkPipeline& getPipeline(pipelineEnum::procPipelineId::type id) {
            return node[id].pipeline;
        }
    };
}