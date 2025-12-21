#pragma once



namespace deviceStruct {
    struct queueIndiceNode {
        //四大操作支持索引
        int support = -1;

        //呈现索引
        int present = -1;

        //存储显卡使用队列数量
        int count = -1;
    };

    struct deviceNode {
        //显卡
        std::vector<VkPhysicalDevice> physicalDevices;

        //队列
        std::vector<deviceStruct::queueIndiceNode> queueIndices;    //queueIndices[0]经过排序后的最优显卡
    };
}

namespace swapStruct {
    struct scSupportNode {
        VkSurfaceCapabilitiesKHR capabilitie{};
        std::vector<VkSurfaceFormatKHR> scFormat{};
        std::vector<VkPresentModeKHR> presentMode{};

        VkSwapchainCreateInfoKHR scCreateInfo{};



        scSupportNode() {
            scCreateInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            scCreateInfo.imageArrayLayers = 1;
            scCreateInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            scCreateInfo.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
            scCreateInfo.clipped          = VK_TRUE;
            scCreateInfo.oldSwapchain     = VK_NULL_HANDLE;
        }

        //设置交换链支持结点
        void createScSupport(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& surface) {
            //表面功能查询
            if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilitie)) {
                Log::printError(__FILE__, __LINE__, "表面功能获取失败");
                throw;
            }


            //表面格式查询
            uint32_t formatCount;
            vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
            if (formatCount == 0) {
                Log::printError(__FILE__, __LINE__, "表面格式获取失败");
                throw;
            }
            scFormat.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, scFormat.data());


            //呈现模式查询
            uint32_t presentModeCount;
            vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
            if (presentModeCount == 0) {
                Log::printError(__FILE__, __LINE__, "呈现模式获取失败");
                throw;
            }
            presentMode.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentMode.data());



            if (debugInfo::LogSwitch) {
                Log::printTitle("交换链配置");

                Log::print("最小图像数量:\t" + std::to_string(capabilitie.minImageCount));
                Log::print("最大图像数量:\t" + std::to_string(capabilitie.maxImageCount));
                Log::print("");

                Log::print("支持格式:");
                for (auto format : scFormat) {
                    std::cout << "\t" << "format: " << format.format << ", colorSpace: " << format.colorSpace << std::endl;
                }
                Log::print("");

                Log::print("呈现模式:");
                for (auto mode : presentMode) {
                    std::cout << "\t" << "mode: " << mode << std::endl;
                }

                Log::delimiter();
            }
        }

        //设置scCreateInfo
        void createScCreateInfo(const VkSurfaceKHR& surface, const deviceStruct::queueIndiceNode& currentQueue) {
            VkSurfaceFormatKHR scFormat = getScFormat();
            

            scCreateInfo.surface                = surface;
            scCreateInfo.minImageCount          = capabilitie.minImageCount + (capabilitie.maxImageCount > capabilitie.minImageCount);
            scCreateInfo.imageFormat            = scFormat.format;
            scCreateInfo.imageColorSpace        = scFormat.colorSpace;
            scCreateInfo.imageExtent            = getScExtent();

            if (currentQueue.support != currentQueue.present) {
                uint32_t index[] = { (uint32_t)currentQueue.support, (uint32_t)currentQueue.present };

                scCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
                scCreateInfo.queueFamilyIndexCount = 2;
                scCreateInfo.pQueueFamilyIndices = index;
            }
            else {
                scCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
                scCreateInfo.queueFamilyIndexCount = 0;
                scCreateInfo.pQueueFamilyIndices = nullptr;
            }

            scCreateInfo.preTransform           = capabilitie.currentTransform;
            scCreateInfo.presentMode            = getScPresentMode();
            scCreateInfo.oldSwapchain           = VK_NULL_HANDLE;
        }

    private:
        //获取交换链绘制范围
        VkExtent2D getScExtent() {
            //Vulkan特殊约定，如果currentExtent.width为最大值，表示尺寸由应用程序决定，反之使用系统指定的尺寸
            if (capabilitie.currentExtent.width == std::numeric_limits<uint32_t>::max()) {
                VkExtent2D actualExtent = { windowInfo::width, windowInfo::height };

                actualExtent.width = std::max(capabilitie.minImageExtent.width, std::min(capabilitie.maxImageExtent.width, actualExtent.width));
                actualExtent.height = std::max(capabilitie.minImageExtent.height, std::min(capabilitie.maxImageExtent.height, actualExtent.height));

                return actualExtent;
            }
            else {
                return capabilitie.currentExtent;
            }
        }

        //获取交换链格式
        VkSurfaceFormatKHR getScFormat() {
            //只有一个未定义格式时，返回指定格式
            if (scFormat.size() == 1 && scFormat[0].format == VK_FORMAT_UNDEFINED) {
                return { VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
            }

            //含有多个格式支持时，从中挑选返回
            for (VkSurfaceFormatKHR availableFormat : scFormat) {
                if (availableFormat.format == VK_FORMAT_R8G8B8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                    return availableFormat;
                }
            }

            return scFormat[0];
        }

        //获取交换链合适的呈现模式
        VkPresentModeKHR getScPresentMode() {
            for (auto availablePresentMode : presentMode) {
                if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {

                    if (debugInfo::LogSwitch) {
                        Log::print("交换链选择三缓存呈现模式");

                        Log::delimiter();
                    }

                    return availablePresentMode;
                }
                else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
                    if (debugInfo::LogSwitch) {
                        Log::print("交换链选择立即呈现模式");

                        Log::delimiter();
                    }

                    return availablePresentMode;
                }
            }

            if (debugInfo::LogSwitch) {
                Log::print("交换链选择垂直中断呈现模式");

                Log::delimiter();
            }

            return VK_PRESENT_MODE_FIFO_KHR;
        }

    public:
        //获取深度格式
        VkFormat getDepthFormat(const VkPhysicalDevice& physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
            for (VkFormat format : candidates) {
                //获取当前格式的属性信息
                VkFormatProperties props;
                vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

                //检查当前格式是否支持所需的特性和平铺方式
                if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                    //如果当前格式支持最佳平铺并且满足特性要求，则返回该格式
                    return format;
                }
                else if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                    //如果当前格式支持线性平铺并且满足特性要求，则返回该格式
                    return format;
                }
            }

            Log::printError(__FILE__, __LINE__, "深度格式获取失败");
            throw;
        }

        //获取多重采样
        VkSampleCountFlagBits getMsaaSamples(const VkPhysicalDevice& physicalDevice) {
            VkPhysicalDeviceProperties physicalDeviceProperties;
            vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

            VkSampleCountFlags count = std::min(physicalDeviceProperties.limits.framebufferColorSampleCounts, physicalDeviceProperties.limits.framebufferDepthSampleCounts);
            /*if (count & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
            if (count & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
            if (count & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
            if (count & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }*/
            if (count & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
            if (count & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

            return VK_SAMPLE_COUNT_1_BIT;
        }



        //重建scExtent
        void recreateScCreateInfo(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& surface, const VkSwapchainKHR& swapchain) {
            if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilitie)) {
                Log::printError(__FILE__, __LINE__, "重建交换链时表面功能获取失败");
                throw;
            }

            scCreateInfo.imageExtent = getScExtent();
            scCreateInfo.oldSwapchain = swapchain;
        }
    };
}

namespace bufferStruct {
    struct commandNode {
        VkCommandPool commandPool;
        std::vector<VkCommandBuffer> commandBuffer[bufferInfo::commandThreadSize];     //使用双重命令缓冲防止更新区块卡顿

        int currentThreadId = 0;						                                //当前地形线程命令缓冲区id
        uint32_t processingThreadSize = 1;                                              //背后线程数量,避免上个线程没结束就开启新线程

        VkCommandBufferAllocateInfo allocInfo{};
        VkCommandBufferBeginInfo beginInfo{};

        std::vector<VkDeviceSize> vertexOffset{ 0 };	//绑定管线过程中指定各顶点缓冲区的偏移量



        void createCommandPool(const VkDevice& device, uint32_t queueFamilyIndex) {
            VkCommandPoolCreateInfo poolInfo{};
            poolInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolInfo.queueFamilyIndex   = queueFamilyIndex;
            poolInfo.flags              = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

            if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "命令池创建失败");
                throw;
            }
        }

        void initCommandBuffer(const uint32_t frameSize) {
            for (int i = 0; i < bufferInfo::commandThreadSize; i++) {
                commandBuffer[i].resize(frameSize);
            }

            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandPool = commandPool;
            allocInfo.commandBufferCount = uint32_t(frameSize);

            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
            beginInfo.pInheritanceInfo = nullptr;
        }

        void beginCommandBuffer(int threadID, int frameID) {
            vkResetCommandBuffer(commandBuffer[threadID][frameID], 0);
            vkBeginCommandBuffer(commandBuffer[threadID][frameID], &beginInfo);
        }

        void endCommandBuffer(int threadID, int frameID) {
            vertexOffset = std::vector<VkDeviceSize>{ 0 };

            if (vkEndCommandBuffer(commandBuffer[threadID][frameID]) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "记录命令缓冲区失败");
            }
        }

        void createScenceCommandBuffer(int threadID, int frameID, const VkPipeline& pipeline, const VkPipelineLayout& pipelineLayout, const VkDescriptorSet& descriptorSet, const VkBuffer& vertexBuffer, const VkBuffer& indexBuffer, uint32_t indexSize) {
            vkCmdBindPipeline(commandBuffer[threadID][frameID], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
            vkCmdBindDescriptorSets(commandBuffer[threadID][frameID], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
            vkCmdBindVertexBuffers(commandBuffer[threadID][frameID], 0, 1, &vertexBuffer, vertexOffset.data());
            vkCmdBindIndexBuffer(commandBuffer[threadID][frameID], indexBuffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(commandBuffer[threadID][frameID], indexSize, 1, 0, 0, 0);
        }

        //全屏三角形
        void createScenceCommandBuffer(int threadID, int frameID, const VkPipeline& pipeline, const VkPipelineLayout& pipelineLayout, const VkDescriptorSet& descriptorSet, const VkExtent2D& scExtent) {
            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = (float)scExtent.width;
            viewport.height = (float)scExtent.height;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;

            VkRect2D scissor{};
            scissor.offset = { 0, 0 };
            scissor.extent = scExtent;

            vkCmdBindPipeline(commandBuffer[threadID][frameID], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
            vkCmdSetViewport(commandBuffer[threadID][frameID], 0, 1, &viewport);
            vkCmdSetScissor(commandBuffer[threadID][frameID], 0, 1, &scissor);
            vkCmdBindDescriptorSets(commandBuffer[threadID][frameID], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
            vkCmdDraw(commandBuffer[threadID][frameID], 3, 1, 0, 0);
        }

        void createUITextCommandBuffer(int threadID, int frameID, const VkPipeline& pipeline, const VkPipelineLayout& pipelineLayout, const VkDescriptorSet& descriptorSet, const VkBuffer& vertexBuffer, uint32_t numLetters) {
            vkCmdBindPipeline(commandBuffer[threadID][frameID], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
            vkCmdBindDescriptorSets(commandBuffer[threadID][frameID], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);
            vkCmdBindVertexBuffers(commandBuffer[threadID][frameID], 0, 1, &vertexBuffer, vertexOffset.data());
            vkCmdBindVertexBuffers(commandBuffer[threadID][frameID], 1, 1, &vertexBuffer, vertexOffset.data());

            for (uint32_t j = 0; j < numLetters; j++) {
                vkCmdDraw(commandBuffer[threadID][frameID], 4, 1, j * 4, 0);		//不用循环文本底部有连线
            }
        }

        void free(const VkDevice& device, int threadID) {
            vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffer[threadID].size()), commandBuffer[threadID].data());
        }

        void clear(const VkDevice& device) {
            vkDestroyCommandPool(device, commandPool, nullptr);
        }
    };

    struct showUITextNode {
        glm::dvec3 pos;
        glm::dvec3 chunkPos;
        glm::ivec3 minepos{};
    };



    /*缓冲结点*/
    struct imageNode {
        VkImage image;
        VmaAllocation imageMemory;
        VkImageView view;
    };

    struct bufferNode {
        VkBuffer buffer;
        VmaAllocation bufferMemory;
    };

    struct baseUBO {
        glm::mat4 model;            //模型矩阵，用于将模型变换到世界坐标系
        glm::mat4 modelTI;          //模型转置逆矩阵，用于计算光照法线
        glm::mat4 view;             //视图矩阵，用于将世界坐标系变换到观察者坐标系
        glm::mat4 proj;             //投影矩阵，用于将观察者坐标系变换到裁剪坐标系
    };

    //注意着色器内存对齐
    struct lightUBO {
        glm::vec3 viewPos;          //观察者坐标，用于计算镜面反射
        float currentHour;          //当前小时，通过gameTime(游戏运行时长)%24得到，用于周期性效果

        glm::vec3 gLightColor;      //全局光照颜色，决定环境光和漫反射基础颜色
        float ambientScale;         //环境光缩放，决定环境光强度

        glm::vec3 gLightPos;        //全局光照坐标，点光源坐标，决定漫反射效果
        float diffuseScale;         //漫反射缩放，决定漫反射强度

        glm::vec3 gLightDir;        //全局光照方向，平行光坐标，决定漫反射效果
        float specularScale;        //镜面反射缩放，决定镜面反射强度

        glm::vec3 flashPos;         //聚光灯坐标
        float flashCutOff;          //聚光灯内圆锥

        glm::vec3 flashDir;         //聚光灯方向
        float outerCutOff;          //聚光灯外圆锥
    };



    /*管线缓冲区*/
    struct terrainBufferNode {
        //网格缓冲
        bufferNode vertex[bufferInfo::commandThreadSize][terrainEnum::terrainLayer::count];
        bufferNode index[bufferInfo::commandThreadSize][terrainEnum::terrainLayer::count];

        //纹理贴图缓冲
        const char* textureFile;
        imageNode texture;

        //高光贴图缓冲
        const char* specularFile;
        imageNode specular;

        //纹理参数
        bufferEnum::procSamplerId::type samplerId;
        uint32_t arrayLayer;
        uint32_t mipLevel = 1;

        //统一缓冲
        bufferNode uniform;
        baseUBO ubo;
        bufferNode lightUniform;
        lightUBO lightUbo;

        //描述符缓冲
        VkDescriptorSet descriptorSet;

        //管线索引
        pipelineEnum::procPipelineId::type pipeId;



        terrainBufferNode(pipelineEnum::procPipelineId::type pId, bufferEnum::procSamplerId::type tempSamplerId) {
            textureFile = fileInfo::image::cube;
            specularFile = fileInfo::image::cubeSpecular;
            arrayLayer = textureInfo::arrayLayer;

            samplerId = tempSamplerId;
            pipeId = pId;
        }

        terrainBufferNode() = delete;

        //清理结构体，descriptorSet由描述符池管理，无需手动清理
        void clear(const VkDevice& device, const VmaAllocator& allocator) {
            for (int thread = 0; thread < bufferInfo::commandThreadSize; thread++) {
                for (int layer = 0; layer < terrainEnum::terrainLayer::count; layer++) {
                    vmaDestroyBuffer(allocator, vertex[thread][layer].buffer, vertex[thread][layer].bufferMemory);
                    vmaDestroyBuffer(allocator, index[thread][layer].buffer, index[thread][layer].bufferMemory);
                }
            }

            vmaDestroyImage(allocator, texture.image, texture.imageMemory);
            vkDestroyImageView(device, texture.view, nullptr);

            vmaDestroyImage(allocator, specular.image, specular.imageMemory);
            vkDestroyImageView(device, specular.view, nullptr);

            vmaDestroyBuffer(allocator, uniform.buffer, uniform.bufferMemory);
            vmaDestroyBuffer(allocator, lightUniform.buffer, lightUniform.bufferMemory);
        }
    };

    struct staticModelNode {
        std::string modelFile;
        bufferNode vertex;
        bufferNode index;

        const char* textureFile;
        imageNode texture;
        //const char* normalFile;
        //imageNode normal;

        bufferEnum::procSamplerId::type samplerId;
        uint32_t mipLevel;                  //在创建texture时赋值

        bufferNode uniform;
        baseUBO ubo;
        bufferNode lightUniform;
        lightUBO lightUbo;

        VkDescriptorSet descriptorSet;



        void create(const std::string tempModelFile, const char* tempTextureFile, /*const char* tempNormalFile,*/ bufferEnum::procSamplerId::type tempSamplerId) {
            modelFile   = tempModelFile;
            textureFile = tempTextureFile;
            //normalFile  = tempNormalFile;
            samplerId   = tempSamplerId;
        }

        void clear(const VkDevice& device, const VmaAllocator& allocator) {
            vmaDestroyBuffer(allocator, vertex.buffer, vertex.bufferMemory);
            vmaDestroyBuffer(allocator, index.buffer, index.bufferMemory);

            vmaDestroyImage(allocator, texture.image, texture.imageMemory);
            vkDestroyImageView(device, texture.view, nullptr);

            //vmaDestroyImage(allocator, normal.image, normal.imageMemory);
            //vkDestroyImageView(device, normal.view, nullptr);

            vmaDestroyBuffer(allocator, uniform.buffer, uniform.bufferMemory);
            vmaDestroyBuffer(allocator, lightUniform.buffer, lightUniform.bufferMemory);
        }
    };

    struct dynamicModelSubNode {
        bufferNode vertex;
        bufferNode index;

        const char* textureFile;
        imageNode texture;
        //const char* normalFile;
        //imageNode normal;

        bufferNode uniform;
        baseUBO ubo;

        VkDescriptorSet descriptorSet;



        void clear(const VkDevice& device, const VmaAllocator& allocator) {
            vmaDestroyBuffer(allocator, vertex.buffer, vertex.bufferMemory);
            vmaDestroyBuffer(allocator, index.buffer, index.bufferMemory);

            vmaDestroyImage(allocator, texture.image, texture.imageMemory);
            vkDestroyImageView(device, texture.view, nullptr);

            //vmaDestroyImage(allocator, normal.image, normal.imageMemory);
            //vkDestroyImageView(device, normal.view, nullptr);

            vmaDestroyBuffer(allocator, uniform.buffer, uniform.bufferMemory);
        }
    };

    struct dynamicModelNode {
        std::string modelFile;
        std::vector<dynamicModelSubNode> node;

        bufferNode lightUniform;
        lightUBO lightUbo;

        bufferEnum::procSamplerId::type samplerId;
        uint32_t mipLevel;



        void create(const std::string tempModelFile, std::vector<const char*> tempTextureFile, /*std::vector<const char*> tempNormalFile,*/ bufferEnum::procSamplerId::type tempSamplerId) {
            modelFile = tempModelFile;

            node.resize(tempTextureFile.size());
            for (int i = 0; i < tempTextureFile.size(); i++) {
                node[i].textureFile = tempTextureFile[i];
                //node[i].normalFile = tempNormalFile[i];
            }

            samplerId = tempSamplerId;
        }

        void clear(const VkDevice& device, const VmaAllocator& allocator) {
            for (int i = 0; i < node.size(); i++) {
                node[i].clear(device, allocator);
            }

            vmaDestroyBuffer(allocator, lightUniform.buffer, lightUniform.bufferMemory);
        }
    };

    struct skyboxBufferNode {
        bufferNode vertex;
        bufferNode index;

        bufferNode uniform;
        baseUBO ubo;
        bufferNode lightUniform;
        lightUBO lightUbo;

        VkDescriptorSet descriptorSet;

        pipelineEnum::procPipelineId::type pipeId;



        skyboxBufferNode(pipelineEnum::procPipelineId::type pId) {
            pipeId = pId;
        }

        skyboxBufferNode() = delete;

        void clear(const VkDevice& device, const VmaAllocator& allocator) {
            vmaDestroyBuffer(allocator, vertex.buffer, vertex.bufferMemory);
            vmaDestroyBuffer(allocator, index.buffer, index.bufferMemory);

            vmaDestroyBuffer(allocator, uniform.buffer, uniform.bufferMemory);
            vmaDestroyBuffer(allocator, lightUniform.buffer, lightUniform.bufferMemory);
        }
    };

    struct mineBufferNode {
        bufferNode vertex;
        bufferNode index;

        bufferNode uniform;
        baseUBO ubo;

        VkDescriptorSet descriptorSet;

        pipelineEnum::procPipelineId::type pipeId;



        mineBufferNode(pipelineEnum::procPipelineId::type pId) {
            pipeId = pId;
        }

        mineBufferNode() = delete;

        void clear(const VkDevice& device, const VmaAllocator& allocator) {
            vmaDestroyBuffer(allocator, vertex.buffer, vertex.bufferMemory);
            vmaDestroyBuffer(allocator, index.buffer, index.bufferMemory);

            vmaDestroyBuffer(allocator, uniform.buffer, uniform.bufferMemory);
        }
    };

    struct osMirrorBufferNode {
        bufferNode vertex;
        bufferNode index;

        bufferEnum::procSamplerId::type samplerId;

        bufferNode uniform;
        baseUBO ubo;
        bufferNode lightUniform;
        lightUBO lightUbo;

        VkDescriptorSet descriptorSet;

        pipelineEnum::procPipelineId::type pipeId;



        osMirrorBufferNode(pipelineEnum::procPipelineId::type pId, bufferEnum::procSamplerId::type tempSamplerId) {
            samplerId = tempSamplerId;
            pipeId = pId;
        }

        osMirrorBufferNode() = delete;

        void clear(const VkDevice& device, const VmaAllocator& allocator) {
            vmaDestroyBuffer(allocator, vertex.buffer, vertex.bufferMemory);
            vmaDestroyBuffer(allocator, index.buffer, index.bufferMemory);

            vmaDestroyBuffer(allocator, uniform.buffer, uniform.bufferMemory);
            vmaDestroyBuffer(allocator, lightUniform.buffer, lightUniform.bufferMemory);
        }
    };

    struct osScenceBufferNode {
        bufferEnum::procSamplerId::type samplerId;

        VkDescriptorSet descriptorSet;

        pipelineEnum::procPipelineId::type pipeId;



        osScenceBufferNode(pipelineEnum::procPipelineId::type pId, bufferEnum::procSamplerId::type tempSamplerId) {
            samplerId = tempSamplerId;
            pipeId = pId;
        }

        osScenceBufferNode() = delete;

        void clear(const VkDevice& device, const VmaAllocator& allocator) {

        }
    };

    struct dsScenceBufferNode {
        bufferEnum::procSamplerId::type samplerId;

        bufferNode lightUniform;
        lightUBO lightUbo;

        VkDescriptorSet descriptorSet;

        pipelineEnum::procPipelineId::type pipeId;



        dsScenceBufferNode(pipelineEnum::procPipelineId::type pId, bufferEnum::procSamplerId::type tempSamplerId) {
            samplerId = tempSamplerId;
            pipeId = pId;
        }

        dsScenceBufferNode() = delete;

        void clear(const VkDevice& device, const VmaAllocator& allocator) {
            vmaDestroyBuffer(allocator, lightUniform.buffer, lightUniform.bufferMemory);
        }
    };

    struct uiTextBufferNode {
        bufferNode vertex;

        imageNode texture;
        stb_fontchar* stbFontData = new stb_fontchar[STB_FONT_consolas_24_latin1_NUM_CHARS];    //字符数据
        glm::vec4* mapped = nullptr;
        uint32_t numLetters;
        bufferEnum::procSamplerId::type samplerId;

        VkDescriptorSet descriptorSet;

        pipelineEnum::procPipelineId::type pipeId;



        uiTextBufferNode(pipelineEnum::procPipelineId::type pId, bufferEnum::procSamplerId::type tempSamplerId) {
            samplerId = tempSamplerId;
            pipeId = pId;
        }

        uiTextBufferNode() = delete;

        void beginTextUpdate(VmaAllocator allocator) {
            vmaMapMemory(allocator, vertex.bufferMemory, (void**)&mapped);
            numLetters = 0;
        }

        void addText(std::string texts, float x, float y, bufferEnum::textAlignId::type align) {
            // 字符的起始索引
            const uint32_t firstChar = STB_FONT_consolas_24_latin1_FIRST_CHAR;

            // 以窗口大小为单位，计算字符宽度和高度
            const float charW = 1.5f * 1.0f / windowInfo::width;
            const float charH = 1.5f * 1.0f / windowInfo::height;

            // 窗口的宽度和高度
            x = (x / windowInfo::width * 2.0f) - 1.0f;
            y = (y / windowInfo::height * 2.0f) - 1.0f;

            // 计算文本宽度
            float textWidth = 0;
            for (char letter : texts) {
                stb_fontchar* charData = &stbFontData[(uint32_t)letter - firstChar];
                textWidth += charData->advance * charW;
            }

            // 根据对齐方式调整x坐标
            switch (align) {
            case bufferEnum::textAlignId::right:
                x -= textWidth;
                break;
            case bufferEnum::textAlignId::center:
                x -= textWidth / 2.0f;
                break;
            case bufferEnum::textAlignId::left:
                break;
            }

            // 为每个字符生成uv映射的四边形
            for (char letter : texts) {
                // 创建一个指向stb_fontchar类型数据的指针
                stb_fontchar* charData = &stbFontData[(uint32_t)letter - firstChar];

                // 设置四边形的uv坐标和位置
                mapped->x = (x + (float)charData->x0 * charW);
                mapped->y = (y + (float)charData->y0 * charH);
                mapped->z = charData->s0;
                mapped->w = charData->t0;
                mapped++;

                mapped->x = (x + (float)charData->x1 * charW);
                mapped->y = (y + (float)charData->y0 * charH);
                mapped->z = charData->s1;
                mapped->w = charData->t0;
                mapped++;

                mapped->x = (x + (float)charData->x0 * charW);
                mapped->y = (y + (float)charData->y1 * charH);
                mapped->z = charData->s0;
                mapped->w = charData->t1;
                mapped++;

                mapped->x = (x + (float)charData->x1 * charW);
                mapped->y = (y + (float)charData->y1 * charH);
                mapped->z = charData->s1;
                mapped->w = charData->t1;
                mapped++;

                x += charData->advance * charW;

                numLetters++;
            }
        }

        void endTextUpdate(const VmaAllocator& allocator) {
            vmaUnmapMemory(allocator, vertex.bufferMemory);
            mapped = nullptr;
        }

        void clear(const VkDevice& device, const VmaAllocator& allocator) {
            vmaDestroyBuffer(allocator, vertex.buffer, vertex.bufferMemory);

            vmaDestroyImage(allocator, texture.image, texture.imageMemory);
            vkDestroyImageView(device, texture.view, nullptr);
        }
    };



    struct scFramebufferNode {
        bufferStruct::imageNode color;
        bufferStruct::imageNode depth;
        std::vector<VkImage> scImage;       //由交换链负责释放
        std::vector<VkImageView> scView;

        std::vector<VkFramebuffer> framebuffer;

        

        void clear(const VkDevice& device, const VmaAllocator& allocator) {
            vkDestroyImageView(device, color.view, nullptr);
            vmaDestroyImage(allocator, color.image, color.imageMemory);

            vkDestroyImageView(device, depth.view, nullptr);
            vmaDestroyImage(allocator, depth.image, depth.imageMemory);

            for (int i = 0; i < scView.size(); i++) {
                vkDestroyImageView(device, scView[i], nullptr);
            }

            for (int i = 0; i < framebuffer.size(); i++) {
                vkDestroyFramebuffer(device, framebuffer[i], nullptr);
            }
        }
    };

    struct osFramebufferNode {
        bufferStruct::imageNode color;
        bufferStruct::imageNode depth;
        bufferStruct::imageNode osView;
        
        VkFramebuffer framebuffer;



        void clear(const VkDevice& device, const VmaAllocator& allocator) {
            vkDestroyImageView(device, color.view, nullptr);
            vmaDestroyImage(allocator, color.image, color.imageMemory);

            vkDestroyImageView(device, depth.view, nullptr);
            vmaDestroyImage(allocator, depth.image, depth.imageMemory);

            vkDestroyImageView(device, osView.view, nullptr);
            vmaDestroyImage(allocator, osView.image, osView.imageMemory);

            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
    };

    struct dsFramebufferNode {
        bufferStruct::imageNode position;  //位置附件（世界空间坐标）
        bufferStruct::imageNode normal;    //法线附件（世界空间法线）
        bufferStruct::imageNode albedo;    //反照率附件（颜色+高光系数）
        bufferStruct::imageNode depth;     //深度附件

        VkFramebuffer framebuffer;



        void clear(const VkDevice& device, const VmaAllocator& allocator) {
            vkDestroyImageView(device, position.view, nullptr);
            vmaDestroyImage(allocator, position.image, position.imageMemory);

            vkDestroyImageView(device, normal.view, nullptr);
            vmaDestroyImage(allocator, normal.image, normal.imageMemory);

            vkDestroyImageView(device, albedo.view, nullptr);
            vmaDestroyImage(allocator, albedo.image, albedo.imageMemory);

            vkDestroyImageView(device, depth.view, nullptr);
            vmaDestroyImage(allocator, depth.image, depth.imageMemory);

            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
    };

    struct renderNode {
        VkRenderPass render;
        


        void createRenderNode(int id, const VkFormat& colorFormat, const VkFormat& depthFormat, const VkSampleCountFlagBits& msaaSamples, const VkDevice& device) {
            if (id == bufferEnum::frameNodeId::sc) {
                createScRenderPass(colorFormat, depthFormat, msaaSamples, device);
            }
            else if (id == bufferEnum::frameNodeId::os) {
                createOsRenderPass(colorFormat, depthFormat, msaaSamples, device);
            }
            else if (id == bufferEnum::frameNodeId::ds) {
                createDsRenderPass(depthFormat, device);
            }
            else {
                Log::printError(__FILE__, __LINE__, "该帧缓冲类型不存在: \t" + std::to_string(id));
            }
        }

        void clear(const VkDevice& device) {
            vkDestroyRenderPass(device, render, nullptr);
        }

    private:
        //创建交换链渲染通道
        void createScRenderPass(const VkFormat& colorFormat, const VkFormat& depthFormat, const VkSampleCountFlagBits& msaaSamples, const VkDevice& device) {
            std::array<VkAttachmentDescription, 3> attachment{};                                                //附件描述
            getForwardAttachment(attachment, colorFormat, depthFormat, msaaSamples, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);  //最终布局-屏幕呈现

            //子通道描述
            VkAttachmentReference colorReference = { 0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };              //颜色附件引用
            VkAttachmentReference depthReference = { 1,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };      //深度附件引用
            VkAttachmentReference colorAttachmentResolveRef = { 2,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };   //采样附件引用

            std::array<VkSubpassDescription, 1> subpass{};
            subpass[0].pipelineBindPoint        = VK_PIPELINE_BIND_POINT_GRAPHICS;                  //管线绑定点
            subpass[0].colorAttachmentCount     = 1;                                                //颜色附件数量
            subpass[0].pColorAttachments        = &colorReference;                                  //颜色附件引用
            subpass[0].pDepthStencilAttachment  = &depthReference;                                  //深度附件引用
            subpass[0].pResolveAttachments      = &colorAttachmentResolveRef;                       //采样附件引用

            std::array<VkSubpassDependency, 1> dependency{};
            dependency[0].srcSubpass    = VK_SUBPASS_EXTERNAL;
            dependency[0].dstSubpass    = 0;
            dependency[0].srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            dependency[0].dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependency[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            dependency[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            //渲染路径创建信息
            VkRenderPassCreateInfo renderPassInfo{};
            renderPassInfo.sType            = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            renderPassInfo.attachmentCount  = static_cast<uint32_t>(attachment.size());             //附件数量
            renderPassInfo.pAttachments     = attachment.data();                                    //附件描述数组
            renderPassInfo.subpassCount     = static_cast<uint32_t>(subpass.size());                //子通道数量
            renderPassInfo.pSubpasses       = subpass.data();                                       //子通道描述数组
            renderPassInfo.dependencyCount  = static_cast<uint32_t>(dependency.size());             //依赖关系数量
            renderPassInfo.pDependencies    = dependency.data();                                    //依赖关系数组

            //创建渲染路径
            if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &render) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "交换链渲染通道创建失败");
                throw;
            }
        }

        //创建离屏渲染通道
        void createOsRenderPass(const VkFormat& colorFormat, const VkFormat& depthFormat, const VkSampleCountFlagBits& msaaSamples, const VkDevice& device) {
            std::array<VkAttachmentDescription, 3> attachment{};                                                        //附件描述
            getForwardAttachment(attachment, colorFormat, depthFormat, msaaSamples, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL); //最终布局-着色器只读

            //附件引用
            VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };				//颜色附件引用
            VkAttachmentReference depthReference = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };		//深度附件引用
            VkAttachmentReference colorAttachmentResolveRef = { 2,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };   //采样附件引用

            //子通道描述
            std::array<VkSubpassDescription, 1> subpass{};
            subpass[0].pipelineBindPoint        = VK_PIPELINE_BIND_POINT_GRAPHICS;  //图形管道绑定点
            subpass[0].colorAttachmentCount     = 1;								//颜色附件数量
            subpass[0].pColorAttachments        = &colorReference;					//颜色附件
            subpass[0].pDepthStencilAttachment  = &depthReference;					//深度模板附件
            subpass[0].pResolveAttachments      = &colorAttachmentResolveRef;		//采样附件引用

            //使用子通道依赖进行布局转换
            std::array<VkSubpassDependency, 2> dependencies{};
            //第一个依赖：从外部到子通道0
            dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;							//源子通道-外部
            dependencies[0].dstSubpass      = 0;											//目标子通道-子通道0
            dependencies[0].srcStageMask    = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;		//源阶段-片段着色器
            dependencies[0].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;	//目标阶段-颜色输出、深度测试
            dependencies[0].srcAccessMask   = VK_ACCESS_NONE_KHR;							//源访问-无需特定访问
            dependencies[0].dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;	//目标访问-颜色、深度读写
            dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;					//依赖标志-按区域
            //第二个依赖：从子通道0到外部
            dependencies[1].srcSubpass      = 0;											//源子通道-子通道0
            dependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;							//目标子通道-外部
            dependencies[1].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;	//源阶段-颜色输出、深度测试
            dependencies[1].dstStageMask    = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;		//目标阶段-片段着色器
            dependencies[1].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;	//源访问-颜色、深度读写
            dependencies[1].dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;					//目标访问-内存读取
            dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;					//依赖标志-按区域

            //创建实际的渲染通道
            VkRenderPassCreateInfo renderPassInfo{};
            renderPassInfo.sType            = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            renderPassInfo.attachmentCount  = static_cast<uint32_t>(attachment.size());	    //附件数量
            renderPassInfo.pAttachments     = attachment.data();							//附件描述
            renderPassInfo.subpassCount     = static_cast<uint32_t>(subpass.size());		//子通道数量
            renderPassInfo.pSubpasses       = subpass.data();     							//子通道描述
            renderPassInfo.dependencyCount  = static_cast<uint32_t>(dependencies.size());	//依赖数量
            renderPassInfo.pDependencies    = dependencies.data();							//依赖

            if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &render) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "离屏渲染通道创建失败");
                throw;
            }
        }

        //创建延迟着色渲染通道
        void createDsRenderPass(const VkFormat& depthFormat, const VkDevice& device) {
            std::array<VkAttachmentDescription, 4> attachment{};
            getDeferredAttachment(attachment, depthFormat);

            //颜色附件引用
            std::array<VkAttachmentReference, 3> colorReferences{};
            colorReferences[0] = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };  //位置
            colorReferences[1] = { 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };  //法线
            colorReferences[2] = { 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };  //反照率

            //深度附件引用
            VkAttachmentReference depthReference = { 3, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

            //子通道描述
            VkSubpassDescription subpass{};
            subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.pColorAttachments       = colorReferences.data();
            subpass.colorAttachmentCount    = static_cast<uint32_t>(colorReferences.size());
            subpass.pDepthStencilAttachment = &depthReference;

            //子通道依赖
            std::array<VkSubpassDependency, 2> dependencies{};

            //从外部到本子通道
            dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
            dependencies[0].dstSubpass      = 0;
            dependencies[0].srcStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            dependencies[0].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependencies[0].srcAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
            dependencies[0].dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

            //从本子通道到外部
            dependencies[1].srcSubpass      = 0;
            dependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
            dependencies[1].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependencies[1].dstStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            dependencies[1].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dependencies[1].dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
            dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

            VkRenderPassCreateInfo renderPassInfo{};
            renderPassInfo.sType            = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            renderPassInfo.attachmentCount  = static_cast<uint32_t>(attachment.size());
            renderPassInfo.pAttachments     = attachment.data();
            renderPassInfo.subpassCount     = 1;
            renderPassInfo.pSubpasses       = &subpass;
            renderPassInfo.dependencyCount  = static_cast<uint32_t>(dependencies.size());
            renderPassInfo.pDependencies    = dependencies.data();

            if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &render) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "延迟着色渲染通道创建失败");
                throw;
            }
        }



        //创建前向附件
        void getForwardAttachment(std::array<VkAttachmentDescription, 3>& attachment, const VkFormat& colorFormat, const VkFormat& depthFormat, const VkSampleCountFlagBits& msaaSamples, const VkImageLayout& finalLayout) {
            //颜色附件描述
            attachment[0].format           = colorFormat;									        //颜色附件格式
            attachment[0].samples          = msaaSamples;									        //采样数-多重采样
            attachment[0].loadOp           = VK_ATTACHMENT_LOAD_OP_CLEAR;						    //加载操作-清除
            attachment[0].storeOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;					    //存储操作-不关心
            attachment[0].stencilLoadOp    = VK_ATTACHMENT_LOAD_OP_DONT_CARE;					    //模板加载-不关心
            attachment[0].stencilStoreOp   = VK_ATTACHMENT_STORE_OP_DONT_CARE;					    //模板存储-不关心
            attachment[0].initialLayout    = VK_IMAGE_LAYOUT_UNDEFINED;						        //初始布局-未定义
            attachment[0].finalLayout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;			    //最终布局-颜色附件
            //深度附件描述
            attachment[1].format           = depthFormat;									        //深度附件格式
            attachment[1].samples          = msaaSamples;									        //采样数-多重采样
            attachment[1].loadOp           = VK_ATTACHMENT_LOAD_OP_CLEAR;						    //加载操作-清除
            attachment[1].storeOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;					    //存储操作-不关心
            attachment[1].stencilLoadOp    = VK_ATTACHMENT_LOAD_OP_DONT_CARE;					    //模板加载-不关心
            attachment[1].stencilStoreOp   = VK_ATTACHMENT_STORE_OP_DONT_CARE;					    //模板存储-不关心
            attachment[1].initialLayout    = VK_IMAGE_LAYOUT_UNDEFINED;							    //初始布局-未定义
            attachment[1].finalLayout      = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;      //最终布局-深度附件
            //多重采样后的像素颜色值的解析或合并过程描述
            attachment[2].format           = colorFormat;									        //解析附件格式
            attachment[2].samples          = VK_SAMPLE_COUNT_1_BIT;                                 //采样数-单采样（将4个采样点合并为1个最终颜色值）
            attachment[2].loadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;                       //加载操作-不关心
            attachment[2].storeOp          = VK_ATTACHMENT_STORE_OP_STORE;                          //存储操作-存储
            attachment[2].stencilLoadOp    = VK_ATTACHMENT_LOAD_OP_DONT_CARE;                       //模板加载-不关心
            attachment[2].stencilStoreOp   = VK_ATTACHMENT_STORE_OP_DONT_CARE;                      //模板存储-不关心
            attachment[2].initialLayout    = VK_IMAGE_LAYOUT_UNDEFINED;                             //初始布局-未定义
            attachment[2].finalLayout      = finalLayout;                                           //最终布局
        }

        //创建延迟附件
        void getDeferredAttachment(std::array<VkAttachmentDescription, 4>& attachment, const VkFormat& depthFormat) {
            //位置附件(世界空间位置)
            attachment[0].format            = VK_FORMAT_R16G16B16A16_SFLOAT;    //16位浮点格式
            attachment[0].samples           = VK_SAMPLE_COUNT_1_BIT;
            attachment[0].loadOp            = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachment[0].storeOp           = VK_ATTACHMENT_STORE_OP_STORE;
            attachment[0].stencilLoadOp     = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment[0].stencilStoreOp    = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachment[0].initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
            attachment[0].finalLayout       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            //法线附件(世界空间法线)
            attachment[1].format            = VK_FORMAT_R16G16B16A16_SFLOAT;    //16位浮点格式
            attachment[1].samples           = VK_SAMPLE_COUNT_1_BIT;
            attachment[1].loadOp            = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachment[1].storeOp           = VK_ATTACHMENT_STORE_OP_STORE;
            attachment[1].stencilLoadOp     = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment[1].stencilStoreOp    = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachment[1].initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
            attachment[1].finalLayout       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            //反照率附件(颜色+高光)
            attachment[2].format            = VK_FORMAT_R8G8B8A8_UNORM;         //8位无符号归一化
            attachment[2].samples           = VK_SAMPLE_COUNT_1_BIT;
            attachment[2].loadOp            = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachment[2].storeOp           = VK_ATTACHMENT_STORE_OP_STORE;
            attachment[2].stencilLoadOp     = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment[2].stencilStoreOp    = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachment[2].initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
            attachment[2].finalLayout       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            //深度附件
            attachment[3].format            = depthFormat;
            attachment[3].samples           = VK_SAMPLE_COUNT_1_BIT;
            attachment[3].loadOp            = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachment[3].storeOp           = VK_ATTACHMENT_STORE_OP_STORE;
            attachment[3].stencilLoadOp     = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment[3].stencilStoreOp    = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachment[3].initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
            attachment[3].finalLayout       = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }
    };

    struct renderInfoNode {
        std::array<VkClearValue, 2> clearValueForward;     //解析附件不需要清除值
        std::array<VkClearValue, 4> clearValueDeferred;

        bufferEnum::frameNodeId::type frameNodeType;
        std::variant<scFramebufferNode, osFramebufferNode, dsFramebufferNode> frameNode;
        scFramebufferNode* scPtr = nullptr;
        osFramebufferNode* osPtr = nullptr;
        dsFramebufferNode* dsPtr = nullptr;

        VkRenderPassBeginInfo renderInfo;



        void createRenderInfoNode(const VkClearColorValue& tempColor, const VkClearDepthStencilValue& tempDepthStencil, bufferEnum::frameNodeId::type tempFrameNodeType) {
            clearValueForward[0].color        = tempColor;
            clearValueForward[1].depthStencil = tempDepthStencil;

            clearValueDeferred[0].color        = tempColor;
            clearValueDeferred[1].color        = tempColor;
            clearValueDeferred[2].color        = tempColor;
            clearValueDeferred[3].depthStencil = tempDepthStencil;

            renderInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderInfo.renderArea.offset = { 0, 0 };

            frameNodeType = tempFrameNodeType;

            if (tempFrameNodeType == bufferEnum::frameNodeId::sc) {
                frameNode = scFramebufferNode{};
                scPtr = &std::get<bufferStruct::scFramebufferNode>(frameNode);

                renderInfo.clearValueCount = static_cast<uint32_t>(clearValueForward.size());
                renderInfo.pClearValues = clearValueForward.data();
            }
            else if(tempFrameNodeType == bufferEnum::frameNodeId::os){
                frameNode = osFramebufferNode{};
                osPtr = &std::get<bufferStruct::osFramebufferNode>(frameNode);

                renderInfo.clearValueCount = static_cast<uint32_t>(clearValueForward.size());
                renderInfo.pClearValues = clearValueForward.data();
            }
            else if (tempFrameNodeType == bufferEnum::frameNodeId::ds) {
                frameNode = dsFramebufferNode{};
                dsPtr = &std::get<bufferStruct::dsFramebufferNode>(frameNode);

                renderInfo.clearValueCount = static_cast<uint32_t>(clearValueDeferred.size());
                renderInfo.pClearValues = clearValueDeferred.data();
            }
            else {
                Log::printError(__FILE__, __LINE__, "该帧缓冲类型不存在: \t" + std::to_string(tempFrameNodeType));
            }
        }

        void createRenderInfo(const VkExtent2D& scExtent, const VkFormat& colorFormat, const VkFormat& depthFormat, const VkSampleCountFlagBits& msaaSamples, const VkRenderPass& render, const VkDevice& device, const VmaAllocator& allocator) {
            if (frameNodeType == bufferEnum::frameNodeId::sc) {
                createScFramebuffer(scExtent, render, device);
            }
            else if (frameNodeType == bufferEnum::frameNodeId::os) {
                createImageNode(osPtr->color, scExtent.width, scExtent.height, colorFormat, msaaSamples, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, device, allocator);
                createImageNode(osPtr->depth, scExtent.width, scExtent.height, depthFormat, msaaSamples, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, device, allocator);
                createImageNode(osPtr->osView, scExtent.width, scExtent.height, colorFormat, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, device, allocator);

                createOsFramebuffer(scExtent, render, device);
            }
            else if (frameNodeType == bufferEnum::frameNodeId::ds) {
                createImageNode(dsPtr->position, scExtent.width, scExtent.height, VK_FORMAT_R16G16B16A16_SFLOAT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, device, allocator);
                createImageNode(dsPtr->normal,   scExtent.width, scExtent.height, VK_FORMAT_R16G16B16A16_SFLOAT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, device, allocator);
                createImageNode(dsPtr->albedo,   scExtent.width, scExtent.height, VK_FORMAT_R8G8B8A8_UNORM,      VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, device, allocator);
                createImageNode(dsPtr->depth,    scExtent.width, scExtent.height, depthFormat,                   VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, device, allocator);

                createDsFramebuffer(scExtent, render, device);
            }
            else {
                Log::printError(__FILE__, __LINE__, "该帧缓冲类型不存在: \t" + std::to_string(frameNodeType));
            }

            renderInfo.renderPass = render;
            renderInfo.renderArea.extent = scExtent;
        }

        void clear(const VkDevice& device, const VmaAllocator& allocator) {
            std::visit([&device, &allocator](auto& node) {node.clear(device, allocator); }, frameNode);
        }

    private:
        //创建交换链帧缓冲
        void createScFramebuffer(const VkExtent2D& scExtent, const VkRenderPass& render, const VkDevice& device) {
	    	VkFramebufferCreateInfo framebufferInfo{};
	    	framebufferInfo.sType	   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	    	framebufferInfo.renderPass = render;
	    	framebufferInfo.width	   = scExtent.width;
	    	framebufferInfo.height     = scExtent.height;
	    	framebufferInfo.layers	   = 1;

            bufferStruct::scFramebufferNode& tempFrameNode = std::get<bufferStruct::scFramebufferNode>(frameNode);
            tempFrameNode.framebuffer.resize(tempFrameNode.scView.size());

	    	for (size_t i = 0; i < tempFrameNode.scView.size(); i++) {
	    		std::array<VkImageView, 3> attachments{ tempFrameNode.color.view,tempFrameNode.depth.view,tempFrameNode.scView[i] };
	    		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	    		framebufferInfo.pAttachments	= attachments.data();

	    		if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &tempFrameNode.framebuffer[i]) != VK_SUCCESS) {
	    			Log::printError(__FILE__, __LINE__, "交换链帧缓冲创建失败");
	    			throw;
	    		}
	    	}
	    }

        //创建离屏帧缓冲
        void createOsFramebuffer(const VkExtent2D& scExtent, const VkRenderPass& render, const VkDevice& device) {
            bufferStruct::osFramebufferNode& tempFrameNode = std::get<bufferStruct::osFramebufferNode>(frameNode);
            std::array<VkImageView, 3> attachments{ tempFrameNode.color.view,tempFrameNode.depth.view,tempFrameNode.osView.view };

		    VkFramebufferCreateInfo framebufferInfo{};
		    framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		    framebufferInfo.renderPass		= render;	                                    //渲染通道
		    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());	//附件数量
		    framebufferInfo.pAttachments	= attachments.data();							//附件
		    framebufferInfo.width			= scExtent.width;				                //宽度
		    framebufferInfo.height			= scExtent.height;				                //高度
		    framebufferInfo.layers			= 1;											//层数

		    if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &tempFrameNode.framebuffer) != VK_SUCCESS) {
		    	Log::printError(__FILE__, __LINE__, "离屏帧缓冲创建失败");
		    	throw;
		    }
            
            renderInfo.framebuffer = tempFrameNode.framebuffer;
        }

        //创建延迟着色帧缓冲
        void createDsFramebuffer(const VkExtent2D& scExtent, const VkRenderPass& render, const VkDevice& device) {
            bufferStruct::dsFramebufferNode& tempFrameNode = std::get<bufferStruct::dsFramebufferNode>(frameNode);
            std::array<VkImageView, 4> attachments{ tempFrameNode.position.view, tempFrameNode.normal.view,tempFrameNode.albedo.view,tempFrameNode.depth.view };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass      = render;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments    = attachments.data();
            framebufferInfo.width           = scExtent.width;
            framebufferInfo.height          = scExtent.height;
            framebufferInfo.layers          = 1;

            if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &tempFrameNode.framebuffer) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "延迟着色帧缓冲创建失败");
                throw;
            }

            renderInfo.framebuffer = tempFrameNode.framebuffer;
        }
        


        void createImageNode(imageNode& image, uint32_t width, uint32_t height, VkFormat format, VkSampleCountFlagBits numSamples, VkImageUsageFlags usage, VkImageAspectFlags aspectFlags, const VkDevice& device, const VmaAllocator& allocator) {
            VkImageCreateInfo imageInfo = {};
            imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType     = VK_IMAGE_TYPE_2D;
            imageInfo.format        = format;
            imageInfo.extent.width  = width;
            imageInfo.extent.height = height;
            imageInfo.extent.depth  = 1;
            imageInfo.mipLevels     = 1;
            imageInfo.arrayLayers   = 1;
            imageInfo.samples       = numSamples;
            imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage         = usage;
            imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

            VmaAllocationCreateInfo allocInfo = {};
            allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
            allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

            if (vmaCreateImage(allocator, &imageInfo, &allocInfo, &image.image, &image.imageMemory, nullptr) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "图像创建失败");
                throw;
            }

            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image      = image.image;
            viewInfo.viewType   = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format     = format;
            viewInfo.subresourceRange.aspectMask     = aspectFlags;
            viewInfo.subresourceRange.baseMipLevel   = 0;
            viewInfo.subresourceRange.levelCount     = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount     = 1;

            if (vkCreateImageView(device, &viewInfo, nullptr, &image.view) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "图像视图创建失败");
                throw;
            }
        }
    
    public:
        void cleanupSwapChain(const VkDevice& device, const VmaAllocator& allocator) {
            std::visit([&device, &allocator](auto& node) {node.clear(device, allocator); }, frameNode);
        }

        void setScFramebuffer(int i) {
            renderInfo.framebuffer = scPtr->framebuffer[i];
        }
    };



    struct descriptorConfigNode {
        VkDescriptorPoolSize         pool{};
        VkDescriptorSetLayoutBinding layout{};
        VkWriteDescriptorSet         write{};



        descriptorConfigNode(uint32_t id, VkDescriptorType type, VkShaderStageFlags stage, uint32_t count) {
            pool.type = type;
            pool.descriptorCount = count;

            layout.descriptorCount = 1;
            layout.pImmutableSamplers = nullptr;
            layout.binding = id;
            layout.descriptorType = type;
            layout.stageFlags = stage;

            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstArrayElement = 0;
            write.descriptorCount = 1;
            write.dstBinding = id;
            write.descriptorType = type;

            bufferInfo.offset = 0;
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }

        descriptorConfigNode() = delete;

        const VkWriteDescriptorSet& setWrite(const VkDescriptorSet& dstSet, const VkBuffer& buffer, const VkDeviceSize& range) {
            bufferInfo.buffer = buffer;
            bufferInfo.range = range;

            write.dstSet = dstSet;
            write.pBufferInfo = &bufferInfo;

            return write;
        }

        const VkWriteDescriptorSet& setWrite(const VkDescriptorSet& dstSet, const VkSampler& sampler, const VkImageView& imageView) {
            imageInfo.sampler = sampler;
            imageInfo.imageView = imageView;

            write.dstSet = dstSet;
            write.pImageInfo = &imageInfo;

            return write;
        }

    private:
        VkDescriptorBufferInfo bufferInfo{};
        VkDescriptorImageInfo imageInfo{};
    };

    struct descriptorNode {
        std::vector<bufferStruct::descriptorConfigNode> config{};

        VkDescriptorPool pool{};            //描述符池
        VkDescriptorSetLayout layout{};     //描述符布局



        void createDescriptorNode(const VkDevice& device) {
            createPool(device);
            createLayout(device);
        }

        void clear(const VkDevice& device) {
            vkDestroyDescriptorPool(device, pool, nullptr);
            vkDestroyDescriptorSetLayout(device, layout, nullptr);
        }

    private:
        void createPool(const VkDevice& device) {
            std::vector<VkDescriptorPoolSize> poolSize{};
            for (const auto& i : config) {
                poolSize.push_back(i.pool);
            }

            VkDescriptorPoolCreateInfo poolInfo{
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                .maxSets = config[0].pool.descriptorCount,
                .poolSizeCount = static_cast<uint32_t>(poolSize.size()),
                .pPoolSizes = poolSize.data()
            };

            if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &pool) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "描述符池创建失败");
            }
        }

        void createLayout(const VkDevice& device) {
            std::vector<VkDescriptorSetLayoutBinding> layoutBinding{};
            for (const auto& i : config) {
                layoutBinding.push_back(i.layout);
            }

            VkDescriptorSetLayoutCreateInfo layoutInfo{
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .pNext = nullptr,
                .bindingCount = static_cast<uint32_t>(layoutBinding.size()),
                .pBindings = layoutBinding.data()
            };

            if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &layout) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "描述符布局配置失败");
                throw;
            }
        }
    };
}

namespace pipelineStruct {
    struct pipeLayoutNode {
        VkPipelineLayout pipeLayout{};      //管线布局



        void createPipeLayoutNode(const VkDevice& device, VkDescriptorSetLayout& layout) {
            VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount = 1;
            pipelineLayoutInfo.pushConstantRangeCount = 0;
            pipelineLayoutInfo.pPushConstantRanges = 0;
            pipelineLayoutInfo.pSetLayouts = &layout;

            if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipeLayout) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "管线布局创建失败");
            }
        }

        void clear(const VkDevice& device) {
            vkDestroyPipelineLayout(device, pipeLayout, nullptr);
        }
    };

    struct pipelineNode {
        std::string vertFile{};
        std::string fragFile{};
        
        bufferEnum::procDescriptorId::type pipeLayoutId;
        VkPipeline pipeline{};



        void createPipelineNode(bufferEnum::procDescriptorId::type id, std::string vert, std::string frag) {
            pipeLayoutId = id;
            vertFile     = vert;
            fragFile     = frag;
        }

        void clear(const VkDevice& device) {
            vkDestroyPipeline(device, pipeline, nullptr);
        }
    };
}

namespace fpsStruct {
    struct tickNode {
        double time0 = glfwGetTime();
        double time1 = 0.0;
        double dt = 0.0;

        //是否更新
        bool isUpdate(double fps) {
            time1 = glfwGetTime();

            if ((dt = time1 - time0) > 1.0 / fps) {
                time0 = time1;

                return true;
            }

            return false;
        }
    };
}

namespace timeStruct {
    struct timeNode {
        std::chrono::high_resolution_clock::time_point start{};
        std::chrono::duration<double, std::milli> duration{};

        timeNode() {
            start = std::chrono::high_resolution_clock::now();
        }

        std::string end() {
            duration = std::chrono::high_resolution_clock::now() - start;
            return std::to_string(duration.count());
        }
    };
}