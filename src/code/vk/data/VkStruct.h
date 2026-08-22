#pragma once



//engine结构空间
namespace engineStruct {
    struct tickNode {
        double time0 = glfwGetTime();
        double time1 = 0.0;
        double dt    = 0.0;
        double fps   = 0.0;

        void createTickNode(double tempFps) {
            fps = tempFps;
        }

        //是否更新
        bool isUpdate() {
            time1 = glfwGetTime();

            if ((dt = time1 - time0) > 1.0 / fps) {
                time0 = time1;

                return true;
            }

            return false;
        }
    };

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

//device结构空间
namespace deviceStruct {
    //队列族结点
    struct queueFamilyNode {
        uint32_t index = VK_QUEUE_FAMILY_IGNORED;               //队列族内队列索引，初始值为忽略
        uint32_t count = 0;                                     //队列族内队列数量，初始值为0
        float priority = 1.0f;                                  //队列族内队列优先级，暂时无用
    };

    //物理设备结点
    struct gpuNode {
        VkPhysicalDevice gpu;                                   //物理设备，一般是显卡
        deviceStruct::queueFamilyNode graphicsQueueFamily{};    //图形队列族
    };
}



//buffer结构空间
namespace bufferStruct {
    //baseBuffer结点
    struct baseBufferNode {
        VkBuffer      buffer{};
        VmaAllocation bufferMem{};



        //创建cpuBuffer
        baseBufferNode(const void* bufferData, const VkDeviceSize& bufferSize, const VkBufferUsageFlags& bufferUsage, const VkMemoryPropertyFlags& memProperty, const VmaAllocator& memAllocator) {
            createBuffer(bufferSize, bufferUsage, memProperty, memAllocator);
            createMemMap(bufferData, bufferSize, memAllocator);
        }

        //baseBufferNode() = delete;
        baseBufferNode() {}



        void clear(const VmaAllocator& memAllocator) {
            vmaDestroyBuffer(memAllocator, buffer, bufferMem);
        }

    private:
        //创建buffer
        void createBuffer(const VkDeviceSize& bufferSize, const VkBufferUsageFlags& bufferUsage, const VkMemoryPropertyFlags& memProperty, const VmaAllocator& memAllocator) {
            VkBufferCreateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size  = bufferSize;
            bufferInfo.usage = bufferUsage;

            VmaAllocationCreateInfo memInfo{};
            memInfo.usage         = VMA_MEMORY_USAGE_UNKNOWN;       //使用 VMA_MEMORY_USAGE_UNKNOWN 将内存属性交由属性参数控制
            memInfo.requiredFlags = memProperty;		            //设置所需的内存属性

            vmaCreateBuffer(memAllocator, &bufferInfo, &memInfo, &buffer, &bufferMem, nullptr);
        }

        //创建memMap
        void createMemMap(const void* bufferData, const VkDeviceSize& bufferSize, const VmaAllocator& memAllocator) {
            void* memMap{};

            vmaMapMemory(memAllocator, bufferMem, &memMap);
            memcpy(memMap, bufferData, (size_t)bufferSize);
            vmaUnmapMemory(memAllocator, bufferMem);
        }
    };

    //imageBuffer结点
    struct imageBufferNode {
        VkImage       image{};          //在renderEnum::renderFrameId::swap_rf类型下无用
        VmaAllocation imageMem{};
        VkImageView   view{};



        //todo 临时
        imageBufferNode() {}

        //创建 arrayLayer = 1 && mipLevel = 1 的图像
        imageBufferNode(const VkFormat& format, const VkExtent2D& swapExtent, const VkSampleCountFlagBits& msaa, const VkImageUsageFlags& usage, const VkImageAspectFlags& aspectFlags, const VkDevice& device, const VmaAllocator& memAllocator) {
            VkImageCreateInfo imageInfo{};
            imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType     = VK_IMAGE_TYPE_2D;
            imageInfo.format        = format;
            imageInfo.extent.width  = swapExtent.width;
            imageInfo.extent.height = swapExtent.height;
            imageInfo.extent.depth  = 1;
            imageInfo.mipLevels     = 1;
            imageInfo.arrayLayers   = 1;
            imageInfo.samples       = msaa;
            imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.usage         = usage;
            imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            VmaAllocationCreateInfo allocInfo{};
            allocInfo.usage         = VMA_MEMORY_USAGE_GPU_ONLY;
            allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

            if (vmaCreateImage(memAllocator, &imageInfo, &allocInfo, &image, &imageMem, nullptr) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "image创建失败");
            }

            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image    = image;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format   = format;
            viewInfo.subresourceRange.aspectMask     = aspectFlags;
            viewInfo.subresourceRange.baseMipLevel   = 0;
            viewInfo.subresourceRange.levelCount     = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount     = 1;

            if (vkCreateImageView(device, &viewInfo, nullptr, &view) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "view创建失败");
            }
        }

        //todo 待续
        //创建 arrayLayer = n && mipLevel = 1 的图像
        imageBufferNode(const VkFormat& format, const VkExtent2D& swapExtent, const VkSampleCountFlagBits& msaa, const uint32_t& arrayLayer, const VkImageUsageFlags& usage, const VkImageAspectFlags& aspectFlags, const VkDevice& device, const VmaAllocator& memAllocator) {
            VkImageCreateInfo imageInfo{};
            imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType     = VK_IMAGE_TYPE_2D;
            imageInfo.format        = format;
            imageInfo.extent.width  = swapExtent.width;
            imageInfo.extent.height = swapExtent.height;
            imageInfo.extent.depth  = 1;
            imageInfo.mipLevels     = 1;
            imageInfo.arrayLayers   = arrayLayer;
            imageInfo.samples       = msaa;
            imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.usage         = usage;
            imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            VmaAllocationCreateInfo allocInfo{};
            allocInfo.usage         = VMA_MEMORY_USAGE_GPU_ONLY;
            allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

            if (vmaCreateImage(memAllocator, &imageInfo, &allocInfo, &image, &imageMem, nullptr) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "image创建失败");
            }

            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image    = image;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format   = format;
            viewInfo.subresourceRange.aspectMask     = aspectFlags;
            viewInfo.subresourceRange.baseMipLevel   = 0;
            viewInfo.subresourceRange.levelCount     = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount     = 1;

            if (vkCreateImageView(device, &viewInfo, nullptr, &view) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "view创建失败");
            }
        }

        //创建 arrayLayer = 1 && mipLevel = n 的图像
        /*imageBufferNode(const VkFormat& format, const VkExtent2D& swapExtent, const VkSampleCountFlagBits& msaa, const uint32_t& mipLevel, const VkImageUsageFlags& usage, const VkImageAspectFlags& aspectFlags, const VkDevice& device, const VmaAllocator& memAllocator) {
            VkImageCreateInfo imageInfo{};
            imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType     = VK_IMAGE_TYPE_2D;
            imageInfo.format        = format;
            imageInfo.extent.width  = swapExtent.width;
            imageInfo.extent.height = swapExtent.height;
            imageInfo.extent.depth  = 1;
            imageInfo.mipLevels     = mipLevel;
            imageInfo.arrayLayers   = 1;
            imageInfo.samples       = msaa;
            imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.usage         = usage;
            imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            VmaAllocationCreateInfo allocInfo{};
            allocInfo.usage         = VMA_MEMORY_USAGE_GPU_ONLY;
            allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

            if (vmaCreateImage(memAllocator, &imageInfo, &allocInfo, &image, &imageMem, nullptr) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "image创建失败");
            }

            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image    = image;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format   = format;
            viewInfo.subresourceRange.aspectMask     = aspectFlags;
            viewInfo.subresourceRange.baseMipLevel   = 0;
            viewInfo.subresourceRange.levelCount     = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount     = 1;

            if (vkCreateImageView(device, &viewInfo, nullptr, &view) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "view创建失败");
            }
        }*/



        //完整清理
        void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
            vkDestroyImageView(device, view, nullptr);
            vmaDestroyImage(memAllocator, image, imageMem);
        }

        //view清理
        void clearView(const VkDevice& device) {
            vkDestroyImageView(device, view, nullptr);
        }

    private:
    };

    //imageBufferList结点
    struct imageBufferListNode {
        std::vector<bufferStruct::imageBufferNode> imageList{};



        //创建swap图像列表
        void createSwapImageList(std::vector<VkImage>& swapImageList, const VkFormat& format, const VkImageAspectFlags& aspectFlags, const VkDevice& device) {
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format   = format;
            viewInfo.subresourceRange.aspectMask     = aspectFlags;
            viewInfo.subresourceRange.baseMipLevel   = 0;
            viewInfo.subresourceRange.levelCount     = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount     = 1;

            imageList.clear();
            imageList.resize(swapImageList.size());

            for (int i = 0; i < swapImageList.size(); i++) {
                viewInfo.image = swapImageList[i];

                if (vkCreateImageView(device, &viewInfo, nullptr, &imageList[i].view) != VK_SUCCESS) {
                    Log::printError(__FILE__, __LINE__, "view创建失败");
                }
            }
        }

        //获取view列表
        std::vector<VkImageView> getViewList() const {
            std::vector<VkImageView> viewList{};

            for (const auto& node : imageList) {
                viewList.push_back(node.view);
            }

            return viewList;
        }



        //完整清理
        void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
            for (bufferStruct::imageBufferNode& node : imageList) {
                node.clear(device, memAllocator);
            }
        }

        //view清理
        void clearView(const VkDevice& device) {
            for (bufferStruct::imageBufferNode& node : imageList) {
                node.clearView(device);
            }
        }
    };

    //textureBuffer结点
    struct textureBufferNode {
        bufferStruct::imageBufferNode imageNode{};  //图像缓冲结点
        uint32_t arrayLayer = 1;                    //纹理数组层数
        uint32_t mipLevel   = 1;                    //mip级别



        //创建textureArray，使用无损压缩
        textureBufferNode(const char* imageFile, const VkFormat& colorFormat, const VkExtent2D& tileExtent, const VkExtent2D& atlasExtent, const VkDevice& device, const VmaAllocator& memAllocator)
            :/*imageNode(colorFormat, tileExtent, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, device, memAllocator), */arrayLayer(atlasExtent.width* atlasExtent.height) {

            ktxTexture2* ktxTexture;
		    if (ktxTexture2_CreateFromNamedFile(imageFile, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture) != KTX_SUCCESS) {
		    	Log::printError(__FILE__, __LINE__, "KTX贴图加载失败");
		    }

		    uint32_t   texWidth    = ktxTexture->baseWidth;
		    uint32_t   texHeight   = ktxTexture->baseHeight;
		    ktx_size_t textureSize = ktxTexture->dataSize;
		    mipLevel = ktxTexture->numLevels;

		    bufferStruct::baseBufferNode cpuBuffer{ ktxTexture->pData, textureSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, memAllocator };
		    
		    //createImage(textureInfo::terrain::tileWidth, textureInfo::terrain::tileHeight, arrayLayer, mipLevel, format, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, imageNode.image, imageNode.imageMem);
            //
		    //transitionImageLayout(imageBuffer.image, arrayLayer, mipLevel, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		    //copyBufferToImageArray(pixel, textureInfo::terrain::tileWidth, textureInfo::terrain::tileHeight, textureInfo::terrain::atlasWidth, textureInfo::terrain::atlasHeight, texWidth, texHeight, arrayLayer, cpuBuffer.buffer, imageBuffer.image);
		    //transitionImageLayout(imageBuffer.image, arrayLayer, mipLevel, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            //
		    //createImageView(imageBuffer.image, imageBuffer.view, arrayLayer, mipLevel, VK_IMAGE_VIEW_TYPE_2D_ARRAY, format, VK_IMAGE_ASPECT_COLOR_BIT);

		    cpuBuffer.clear(memAllocator);
		    ktxTexture2_Destroy(ktxTexture);
        }

        textureBufferNode() = delete;
    };
}

//swapchain结构空间
namespace swapStruct {
    struct swapSupportNode {
        VkSurfaceCapabilitiesKHR        capabilitie{};
        std::vector<VkSurfaceFormatKHR> swapFormat{};
        std::vector<VkPresentModeKHR>   presentMode{};

        VkSwapchainCreateInfoKHR swapCreateInfo{};



        swapSupportNode() {
            swapCreateInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            swapCreateInfo.imageArrayLayers = 1;
            swapCreateInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            swapCreateInfo.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
            swapCreateInfo.clipped          = VK_TRUE;
            swapCreateInfo.oldSwapchain     = VK_NULL_HANDLE;
        }

        //设置交换链支持结点
        void createSwapSupport(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& presentSurface) {
            //表面功能查询
            if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, presentSurface, &capabilitie)) {
                Log::printError(__FILE__, __LINE__, "表面功能获取失败");
            }


            //表面格式查询
            uint32_t formatCount;
            vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, presentSurface, &formatCount, nullptr);
            if (formatCount == 0) {
                Log::printError(__FILE__, __LINE__, "表面格式获取失败");
            }
            swapFormat.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, presentSurface, &formatCount, swapFormat.data());


            //呈现模式查询
            uint32_t presentModeCount;
            vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, presentSurface, &presentModeCount, nullptr);
            if (presentModeCount == 0) {
                Log::printError(__FILE__, __LINE__, "呈现模式获取失败");
            }
            presentMode.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, presentSurface, &presentModeCount, presentMode.data());



            if (debugInfo::LogSwitch) {
                Log::printTitle("交换链配置");

                Log::print("最小图像数量:\t" + std::to_string(capabilitie.minImageCount));
                Log::print("最大图像数量:\t" + std::to_string(capabilitie.maxImageCount));
                Log::print("");

                Log::print("支持格式:");
                for (auto format : swapFormat) {
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

        //设置swapCreateInfo
        void createSwapCreateInfo(const VkSurfaceKHR& presentSurface) {
            VkSurfaceFormatKHR SwapFormat = getSwapFormat();
            swapCreateInfo.surface                = presentSurface;
            swapCreateInfo.minImageCount          = capabilitie.minImageCount + (capabilitie.maxImageCount > capabilitie.minImageCount);
            swapCreateInfo.imageFormat            = SwapFormat.format;
            swapCreateInfo.imageColorSpace        = SwapFormat.colorSpace;
            swapCreateInfo.imageExtent            = getSwapExtent();
            swapCreateInfo.imageSharingMode       = VK_SHARING_MODE_EXCLUSIVE;
            swapCreateInfo.queueFamilyIndexCount  = 0;
            swapCreateInfo.pQueueFamilyIndices    = nullptr;
            swapCreateInfo.preTransform           = capabilitie.currentTransform;
            swapCreateInfo.presentMode            = getSwapPresentMode();
            swapCreateInfo.oldSwapchain           = VK_NULL_HANDLE;
        }

    private:
        //获取交换链绘制范围
        VkExtent2D getSwapExtent() {
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
        VkSurfaceFormatKHR getSwapFormat() {
            //只有一个未定义格式时，返回指定格式
            if (swapFormat.size() == 1 && swapFormat[0].format == VK_FORMAT_UNDEFINED) {
                return { VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
            }

            //含有多个格式支持时，从中挑选返回
            for (VkSurfaceFormatKHR availableFormat : swapFormat) {
                if (availableFormat.format == VK_FORMAT_R8G8B8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                    return availableFormat;
                }
            }

            return swapFormat[0];
        }

        //获取交换链合适的呈现模式
        VkPresentModeKHR getSwapPresentMode() {
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
            return VK_FORMAT_UNDEFINED;
        }

        //获取多重采样
        VkSampleCountFlagBits getMsaa(const VkPhysicalDevice& physicalDevice) {
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



        //重建swapExtent
        void recreateSwapCreateInfo(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& presentSurface, const VkSwapchainKHR& swapchain) {
            if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, presentSurface, &capabilitie)) {
                Log::printError(__FILE__, __LINE__, "重建交换链时表面功能获取失败");
            }

            swapCreateInfo.imageExtent = getSwapExtent();
            swapCreateInfo.oldSwapchain = swapchain;
        }
    };

    //交换链图像列表，服务于renderFrameNode
    std::vector<VkImage> swapImageList{};
}

//render结构空间
namespace renderStruct {
    //render附件结点
    struct renderAttachmentNode {
        std::vector<VkAttachmentDescription>              attDescList{};    //渲染附件描述列表
        std::array<std::vector<VkAttachmentReference>, 3> attRefList{};     //渲染附件引用列表，0 颜色、1 深度、2 解析

        

        renderAttachmentNode(const renderEnum::renderAttId::type& renderAttType, const VkFormat& colorFormat, const VkFormat& depthFormat, const VkSampleCountFlagBits& msaa = VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM, const VkImageLayout& finalLayout = VK_IMAGE_LAYOUT_MAX_ENUM) {
            switch (renderAttType) {
            case renderEnum::renderAttId::forward_ra: {
                //前向渲染附件列表：颜色、深度、解析
                attDescList = {
                    createAttachmentDesc(colorFormat, msaa,                  VK_ATTACHMENT_LOAD_OP_CLEAR,     VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
                    createAttachmentDesc(depthFormat, msaa,                  VK_ATTACHMENT_LOAD_OP_CLEAR,     VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL),
                    createAttachmentDesc(colorFormat, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE,     finalLayout),
                };

                attRefList = {{
                    {createAttachmentRef(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)},
                    {createAttachmentRef(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)},
                    {createAttachmentRef(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)},
                }};

                break;
            }
            case renderEnum::renderAttId::deferred_ra: {
                //延迟渲染附件列表：坐标颜色、法线颜色、漫反射颜色、阴影深度
                attDescList = {
                    createAttachmentDesc(renderInfo::dsPosColorFormat, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
                    createAttachmentDesc(colorFormat, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
                    createAttachmentDesc(colorFormat, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
                    createAttachmentDesc(depthFormat, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL),
                };

                attRefList = {{
                    {createAttachmentRef(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL), createAttachmentRef(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL), createAttachmentRef(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)},
                    {createAttachmentRef(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)},
                    {},
                }};

                break;
            }
            case renderEnum::renderAttId::shadowMap_ra: {
                //阴影映射附件列表：阴影深度
                attDescList = {
                    createAttachmentDesc(depthFormat, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
                };

                attRefList = {{
                    {},
                    {createAttachmentRef(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)},
                    {},
                }};

                break;
            }
            }
        }

        renderAttachmentNode() = delete;

    private:
        uint32_t refIndex = 0;      //refList的索引



        //创建渲染附件描述
        VkAttachmentDescription createAttachmentDesc(const VkFormat& format, const VkSampleCountFlagBits& msaa, const VkAttachmentLoadOp& loadOp, const VkAttachmentStoreOp& storeOp, const VkImageLayout& finalLayout) {
            return {
                .format         = format,                            //附件的图像格式
                .samples        = msaa,                              //多重采样的数量
                .loadOp         = loadOp,                            //渲染前颜色与深度的加载操作
                .storeOp        = storeOp,                           //渲染后颜色与深度的存储操作
                .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,   //渲染前模板的加载操作
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,  //渲染后模板的存储操作
                .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,         //渲染前图像的布局
                .finalLayout    = finalLayout,                       //渲染后图像的布局
            };
        }

        //创建渲染附件引用
        VkAttachmentReference createAttachmentRef(const VkImageLayout& layout) {
            return{
                .attachment = refIndex++,   //附件的索引
                .layout     = layout,       //附件的图像布局
            };
        }
    };

    //render子通道结点
    struct renderSubpassNode {
        /*
        示例链：上阶段    → 本阶段
        依赖 0：外部      → subpass 0     （开始渲染）
        依赖 1：subpass 0 → subpass 1     （子通道间同步）
        依赖 2：subpass 1 → subpass 2     （子通道间同步）
        依赖 3：subpass 2 → 外部          （结束渲染）

        当前链：上阶段    → 本阶段
        依赖 0：外部      → subpass 0     （开始渲染）
        依赖 1：subpass 0 → 外部          （结束渲染）
        */

        std::vector<VkSubpassDescription> subpassList{};        //渲染子通道列表
        std::vector<VkSubpassDependency>  dependencyList{};     //渲染子通道依赖列表，比subpassList多一个，具体参考上述示例



        renderSubpassNode(const renderEnum::renderTemplateId::type& renderTemplateType, const VkPipelineBindPoint& pipeType, const std::array<std::vector<VkAttachmentReference>, 3>& attRefList) {
            subpassList = {
                createSubpass(pipeType, attRefList),
            };

            switch (renderTemplateType) {
            case renderEnum::renderTemplateId::swapChain_rt: {
                dependencyList = {
                    //依赖 0：外部 → subpass 0，等上一帧颜色和深度写完才能开始读写
                    createSubpassDependency(
                        VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                        0,                   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,                                             VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
                    ),
                    //依赖 1：subpass 0 → 外部，确保本帧颜色和深度写完才被下一帧读取
                    createSubpassDependency(
                        0,                   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,                                              VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                        VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,                                                  VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
                    ),
                };

                break;
            }
            case renderEnum::renderTemplateId::offScreen_rt: {
                dependencyList = {
                    //依赖 0：外部 → subpass 0，等上一帧片段着色器结束，开始颜色深度读写
                    createSubpassDependency(
                        VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,                                                                                                  VK_ACCESS_NONE,
                        0,                   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
                    ),
                    //依赖 1：subpass 0 → 外部，等颜色深度写完，片段着色器才能读
                    createSubpassDependency(
                        0,                   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                        VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,                                                                                                  VK_ACCESS_SHADER_READ_BIT
                    ),
                };

                break;
            }
            case renderEnum::renderTemplateId::deferred_rt: {
                dependencyList = {
                    //依赖 0：外部 → subpass 0，等上一帧颜色写完，本帧开始读写颜色附件
                    createSubpassDependency(
                        VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                        0,                   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
                    ),
                    //依赖 1：subpass 0 → 外部，等本帧颜色写完，片段着色器才能读（供光照阶段采样）
                    createSubpassDependency(
                        0,                   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                        VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,         VK_ACCESS_SHADER_READ_BIT
                    ),
                };

                break;
            }
            case renderEnum::renderTemplateId::shadowMap_rt: {
                dependencyList = {
                    //依赖 0：外部 → subpass 0，等上一帧深度写完，本帧开始深度读写
                    createSubpassDependency(
                        VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                        0,                   VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
                    ),
                    //依赖 1：subpass 0 → 外部，等深度写完，片段着色器才能读阴影贴图
                    createSubpassDependency(
                        0,                   VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                        VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,                                                  VK_ACCESS_SHADER_READ_BIT
                    ),
                };

                break;
            }
            }
        }

        renderSubpassNode() = delete;

    private:
        //创建subpass
        VkSubpassDescription createSubpass(const VkPipelineBindPoint& pipeType, const std::array<std::vector<VkAttachmentReference>, 3>& attRefList) {
            return{
                .pipelineBindPoint       = pipeType,                        //管线类型（图形、计算、光追等等）
                .colorAttachmentCount    = (uint32_t)attRefList[0].size(),  //颜色附件数量
                .pColorAttachments       = attRefList[0].data(),            //颜色附件引用
                .pResolveAttachments     = attRefList[2].data(),            //解析附件引用
                .pDepthStencilAttachment = attRefList[1].data(),            //深度附件引用
            };
        }

        //创建subpass依赖
        VkSubpassDependency createSubpassDependency(const uint32_t& srcSubpass, const VkPipelineStageFlags& srcStageMask, const VkAccessFlags& srcAccessMask, const uint32_t& dstSubpass, const VkPipelineStageFlags& dstStageMask, const VkAccessFlags& dstAccessMask) {
            return {
                .srcSubpass      = srcSubpass,                      //上阶段已完成的子通道，（VK_SUBPASS_EXTERNAL：外部、0/1/2：第n个子通道）
                .dstSubpass      = dstSubpass,                      //本阶段要执行的子通道，（VK_SUBPASS_EXTERNAL：外部、0/1/2：第n个子通道）
                .srcStageMask    = srcStageMask,                    //上阶段已完成的管线阶段，（颜色、深度、着色器、全部）
                .dstStageMask    = dstStageMask,                    //本阶段要执行的管线阶段，（颜色、深度、着色器、全部）
                .srcAccessMask   = srcAccessMask,                   //上阶段对附件的内存权限，（颜色读写、深度读写、着色器读写、内存读写、无权限）
                .dstAccessMask   = dstAccessMask,                   //本阶段对附件的内存权限，（颜色读写、深度读写、着色器读写、内存读写、无权限）
                .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,     //依赖类型，（同一区域的像素串行，不同区域的像素可并行）
            };
        }
    };
    
    //render帧结点
    struct renderFrameNode {
        VkFramebuffer         frameBuffer{};    //帧缓冲
        VkRenderPassBeginInfo renderBegin{};    //渲染通道开始信息



        renderFrameNode(const VkRenderPass& render, const std::vector<VkImageView>& frameAttList, const std::vector<VkClearValue>& clearValueList, const VkExtent2D& extent, const VkDevice& device) {
            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass      = render;
            framebufferInfo.attachmentCount = (uint32_t)frameAttList.size();
            framebufferInfo.pAttachments    = frameAttList.data();
            framebufferInfo.width           = extent.width;
            framebufferInfo.height          = extent.height;
            framebufferInfo.layers          = 1;

            if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &frameBuffer) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "frameBuffer创建失败");
            }

            renderBegin.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderBegin.renderPass        = render;
            renderBegin.framebuffer       = frameBuffer;
            renderBegin.renderArea.offset = { 0, 0 };
            renderBegin.renderArea.extent = extent;
            renderBegin.clearValueCount   = (uint32_t)clearValueList.size();
            renderBegin.pClearValues      = clearValueList.data();
        }

        renderFrameNode() = delete;

        void clear(const VkDevice& device) {
            vkDestroyFramebuffer(device, frameBuffer, nullptr);
        }
    };



    //render模板结点
    struct renderTemplateNode {
        renderEnum::renderAttId::type      renderAttType      = renderEnum::renderAttId::count;
        renderEnum::renderTemplateId::type renderTemplateType = renderEnum::renderTemplateId::count;

        VkRenderPass render{};      //渲染通道



        void createRenderTemplateNode(const renderEnum::renderTemplateId::type& tRenderTemplateType, const VkFormat& colorFormat, const VkFormat& depthFormat, const VkSampleCountFlagBits& msaa, const VkDevice& device) {
            switch (tRenderTemplateType) {
            case renderEnum::renderTemplateId::swapChain_rt: {
                renderAttType = renderEnum::renderAttId::forward_ra;

                renderStruct::renderAttachmentNode renderAttachment{ renderAttType, colorFormat, depthFormat, msaa, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR };
                renderStruct::renderSubpassNode    renderSubpass   { tRenderTemplateType, VK_PIPELINE_BIND_POINT_GRAPHICS, renderAttachment.attRefList };

                createRender(renderAttachment.attDescList, renderSubpass.subpassList, renderSubpass.dependencyList, device, "交换链");

                break;
            }
            case renderEnum::renderTemplateId::offScreen_rt: {
                renderAttType = renderEnum::renderAttId::forward_ra;

                renderStruct::renderAttachmentNode renderAttachment{ renderAttType, colorFormat, depthFormat, msaa, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
                renderStruct::renderSubpassNode    renderSubpass   { tRenderTemplateType, VK_PIPELINE_BIND_POINT_GRAPHICS, renderAttachment.attRefList };

                createRender(renderAttachment.attDescList, renderSubpass.subpassList, renderSubpass.dependencyList, device, "离屏");

                break;
            }
            case renderEnum::renderTemplateId::deferred_rt: {
                renderAttType = renderEnum::renderAttId::deferred_ra;

                renderStruct::renderAttachmentNode renderAttachment{ renderAttType, colorFormat, depthFormat };
                renderStruct::renderSubpassNode    renderSubpass   { tRenderTemplateType, VK_PIPELINE_BIND_POINT_GRAPHICS, renderAttachment.attRefList };

                createRender(renderAttachment.attDescList, renderSubpass.subpassList, renderSubpass.dependencyList, device, "延迟着色");

                break;
            }
            case renderEnum::renderTemplateId::shadowMap_rt: {
                renderAttType = renderEnum::renderAttId::shadowMap_ra;

                renderStruct::renderAttachmentNode renderAttachment{ renderAttType, VK_FORMAT_MAX_ENUM, depthFormat };
                renderStruct::renderSubpassNode    renderSubpass   { tRenderTemplateType, VK_PIPELINE_BIND_POINT_GRAPHICS, renderAttachment.attRefList };

                createRender(renderAttachment.attDescList, renderSubpass.subpassList, renderSubpass.dependencyList, device, "阴影映射");

                break;
            }
            }

            renderTemplateType = tRenderTemplateType;
        }

        void clear(const VkDevice& device) {
            vkDestroyRenderPass(device, render, nullptr);
        }

    private:
        void createRender(const std::vector<VkAttachmentDescription>& attDescList, const std::vector<VkSubpassDescription>& subpassList, std::vector<VkSubpassDependency>& dependencyList, const VkDevice& device, const char* name) {
            VkRenderPassCreateInfo createInfo{};
            createInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            createInfo.attachmentCount = (uint32_t)attDescList.size();
            createInfo.pAttachments    = attDescList.data();
            createInfo.subpassCount    = (uint32_t)subpassList.size();
            createInfo.pSubpasses      = subpassList.data();
            createInfo.dependencyCount = (uint32_t)dependencyList.size();
            createInfo.pDependencies   = dependencyList.data();

            if (vkCreateRenderPass(device, &createInfo, nullptr, &render) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, std::string(name) + "渲染通道创建失败");
            }
        }
    };

    //render实例结点
    struct renderInstanceNode {
        std::vector<bufferStruct::imageBufferListNode> frameImageList{};        //帧图像列表
        std::vector<renderStruct::renderFrameNode>     frameBufferList{};       //帧缓冲列表



        void createRenderInstanceNode(const renderStruct::renderTemplateNode& renderTemplate, const VkFormat& colorFormat, const VkFormat& depthFormat, const VkExtent2D& extent, const VkSampleCountFlagBits& msaa, const VkDevice& device, const VmaAllocator& memAllocator) {
            switch (renderTemplate.renderAttType) {
            case renderEnum::renderAttId::forward_ra: {
                clearValueList = { renderInfo::colorClearValue, renderInfo::depthClearValue };
               
                break;
            }
            case renderEnum::renderAttId::deferred_ra: {
                clearValueList = { renderInfo::colorClearValue, renderInfo::colorClearValue, renderInfo::colorClearValue, renderInfo::depthClearValue };

                break;
            }
            case renderEnum::renderAttId::shadowMap_ra: {
                clearValueList = { renderInfo::depthClearValue };

                break;
            }
            }

            switch (renderTemplate.renderTemplateType) {
            case renderEnum::renderTemplateId::swapChain_rt: {
                frameImageList.resize(renderEnum::renderFrameId::count);

                //color、depth
                frameImageList[renderEnum::renderFrameId::conv_rf].imageList = {
                    {colorFormat, extent, msaa, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_ASPECT_COLOR_BIT, device, memAllocator},
                    {depthFormat, extent, msaa, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,                                   VK_IMAGE_ASPECT_DEPTH_BIT, device, memAllocator},
                };

                //swap
                frameImageList[renderEnum::renderFrameId::swap_rf].createSwapImageList(swapStruct::swapImageList, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, device);

                createFrameBufferList(frameImageList[renderEnum::renderFrameId::conv_rf], frameImageList[renderEnum::renderFrameId::swap_rf], renderTemplate.render, extent, device);
                break;
            }
            case renderEnum::renderTemplateId::offScreen_rt: {
                frameImageList.resize(1);

                //color、depth、view
                frameImageList[renderEnum::renderFrameId::conv_rf].imageList = {
                    {colorFormat, extent, msaa,                  VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, device, memAllocator},
                    {depthFormat, extent, msaa,                  VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,                      VK_IMAGE_ASPECT_DEPTH_BIT, device, memAllocator},
                    {colorFormat, extent, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, device, memAllocator},
                };

                createFrameBufferList(frameImageList[renderEnum::renderFrameId::conv_rf], renderTemplate.render, extent, device);
                break;
            }
            case renderEnum::renderTemplateId::deferred_rt: {
                frameImageList.resize(1);

                //position、normal、albedo、depth
                frameImageList[renderEnum::renderFrameId::conv_rf].imageList = {
                    {renderInfo::dsPosColorFormat, extent, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, device, memAllocator},
                    {colorFormat,                  extent, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, device, memAllocator},
                    {colorFormat,                  extent, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, device, memAllocator},
                    {depthFormat,                  extent, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,                      VK_IMAGE_ASPECT_DEPTH_BIT, device, memAllocator},
                };

                createFrameBufferList(frameImageList[renderEnum::renderFrameId::conv_rf], renderTemplate.render, extent, device);
                break;
            }
            case renderEnum::renderTemplateId::shadowMap_rt: {
                frameImageList.resize(1);

                //depth
                frameImageList[renderEnum::renderFrameId::conv_rf].imageList = {
                    {depthFormat, extent, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, device, memAllocator},
                };

                createFrameBufferList(frameImageList[renderEnum::renderFrameId::conv_rf], renderTemplate.render, extent, device);
                break;
            }
            }
        }

        void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
            //清理frameImageList
            frameImageList[renderEnum::renderFrameId::conv_rf].clear(device, memAllocator);

            if (frameImageList.size() > renderEnum::renderFrameId::swap_rf) {
                frameImageList[renderEnum::renderFrameId::swap_rf].clearView(device);
            }
            frameImageList.clear();

            //清理frameBufferList
            for (int i = 0; i < frameBufferList.size(); i++) {
                frameBufferList[i].clear(device);
            }
            frameBufferList.clear();
        }

    private:
        std::vector<VkClearValue> clearValueList{};     //清理值列表

        

        //创建frameBufferList
        void createFrameBufferList(const bufferStruct::imageBufferListNode& conv, const VkRenderPass& render, const VkExtent2D& extent, const VkDevice& device) {
            frameBufferList = {
                {render, conv.getViewList(), clearValueList, extent, device}
            };
        }

        //创建frameBufferList，swap版
        void createFrameBufferList(const bufferStruct::imageBufferListNode& conv, const bufferStruct::imageBufferListNode& swap, const VkRenderPass& render, const VkExtent2D& extent, const VkDevice& device) {
            std::vector<VkImageView> viewList = conv.getViewList();
            viewList.push_back({});     //预留 swap 槽位

            for (int i = 0; i < swap.imageList.size(); i++) {
                viewList.back() = swap.imageList[i].view;
                frameBufferList.push_back(renderStruct::renderFrameNode{ render, viewList, clearValueList, extent, device });
            }
        }
    };
}

//pipeline结构空间
namespace pipelineStruct {
    //shader结点
    struct shaderNode {
        VkShaderModule shader{};                            //着色器
        VkPipelineShaderStageCreateInfo shaderStage{};      //着色器状态



        //创建着色器结点
        void createShaderNode(const VkDevice& device, const std::string& filename, const VkShaderStageFlagBits& stage) {
            //读取着色器文件
            std::ifstream fileData(filename, std::ios::ate | std::ios::binary);
            if (!fileData.is_open()) {
                Log::printError(__FILE__, __LINE__, "着色器文件打开失败");
            }

            //创建着色器代码
            size_t fileSize = (size_t)fileData.tellg();
            std::vector<char> shaderCode(fileSize);
            fileData.seekg(0);
            fileData.read(shaderCode.data(), fileSize);
            fileData.close();

            //创建着色器
            VkShaderModuleCreateInfo createInfo{};
            createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            createInfo.codeSize = shaderCode.size();
            createInfo.pCode    = reinterpret_cast<const uint32_t*>(shaderCode.data());
            if (vkCreateShaderModule(device, &createInfo, nullptr, &shader) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "着色器创建失败");
            }

            //配置着色器状态
            shaderStage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStage.pName  = "main";
            shaderStage.stage  = stage;
            shaderStage.module = shader;
        }

        void clear(const VkDevice& device) {
            vkDestroyShaderModule(device, shader, nullptr);
        }
    };

    //pipeConfig结点
    struct pipeConfigNode {
        std::vector<VkPipelineShaderStageCreateInfo>     shaderStageList{};         //着色阶段列表（顶点模式、网格模式）
        VkPipelineVertexInputStateCreateInfo             vertInputStage{};          //顶点配置
        VkPipelineInputAssemblyStateCreateInfo			 primitiveState{};	        //图元配置
        VkViewport										 viewport{};			    //视口区域
        VkRect2D										 scissor{};				    //裁剪区域
        VkPipelineViewportStateCreateInfo				 viewportState{};		    //视口配置
        VkPipelineRasterizationStateCreateInfo			 rasterizer{};			    //光栅化
        VkPipelineMultisampleStateCreateInfo			 multisampling{};		    //多重采样
        VkPipelineDepthStencilStateCreateInfo			 depthStencil{};		    //深度和模板
        std::vector<VkPipelineColorBlendAttachmentState> colorAttachmentList{};     //颜色附件列表（前向模式、延迟模式、阴影模式）
        VkPipelineColorBlendStateCreateInfo				 colorAttachmentState{};    //颜色附件配置
        std::vector<VkDynamicState>                      pipeDynamicStageList{};    //管线动态阶段列表（Vulkan 1.4 共 48 个）
        VkPipelineDynamicStateCreateInfo                 pipeDynamicStageState{};   //管线动态阶段配置
        VkGraphicsPipelineCreateInfo					 pipeInfo{};			    //管线信息



        void createPipeConfigNode(const std::vector<pipelineStruct::shaderNode>& shaderList, const VkSampleCountFlagBits& msaa, const renderEnum::renderAttId::type& renderAttType, const VkRenderPass& render, const VkPipelineLayout& pipeLayout) {
            //着色阶段列表
            shaderStageList.resize(shaderList.size());
            for (int i = 0; i < shaderList.size(); i++) {
                shaderStageList[i] = shaderList[i].shaderStage;
            }

            //图元配置
			primitiveState.sType					 = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			primitiveState.topology				 = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;		//图元装配方式
			primitiveState.primitiveRestartEnable = VK_FALSE;

            //视口区域
			viewport.x		  = 0.0f;
			viewport.y		  = 0.0f;
			//viewport.width    = (float)extent.width;
			//viewport.height   = (float)extent.height;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;

            //裁剪区域
			scissor.offset = { 0, 0 };
			//scissor.extent = extent;

            //视口配置
			viewportState.sType			= VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportState.viewportCount = 1;
			viewportState.pViewports	= &viewport;
			viewportState.scissorCount	= 1;
			viewportState.pScissors		= &scissor;

            //光栅化
			rasterizer.sType				   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterizer.depthClampEnable		   = VK_FALSE;
			rasterizer.rasterizerDiscardEnable = VK_FALSE;
			rasterizer.polygonMode			   = VK_POLYGON_MODE_FILL;				//决定几何产生图片的内容
			rasterizer.lineWidth			   = 1.0f;
			rasterizer.cullMode				   = VK_CULL_MODE_BACK_BIT;				//面剔除
			rasterizer.frontFace			   = VK_FRONT_FACE_COUNTER_CLOCKWISE;	//面的加载顺序
			rasterizer.depthBiasEnable		   = VK_TRUE;							//启用深度偏移，让mine能覆盖方块
			rasterizer.depthBiasConstantFactor = 1.0f;								//常量偏移量
			rasterizer.depthBiasClamp		   = 0.0f;								//不限制最小偏移
			rasterizer.depthBiasSlopeFactor    = 1.0f;								//基于斜率的深度偏移

            //多重采样
			multisampling.sType					= VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multisampling.sampleShadingEnable	= VK_FALSE;
			multisampling.rasterizationSamples	= msaa;
			multisampling.minSampleShading		= 1.0f;													
			multisampling.pSampleMask			= nullptr;													
			multisampling.alphaToCoverageEnable = VK_FALSE;											
			multisampling.alphaToOneEnable		= VK_FALSE;												

            //深度和模板
			depthStencil.sType				   = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			depthStencil.depthTestEnable	   = VK_TRUE;							//指定是否应该将新的深度缓冲区与深度缓冲区进行比较，以确认是否应该被丢弃
			depthStencil.depthWriteEnable	   = VK_TRUE;							//指定通过深度测试的新的片段深度是否应该被实际写入深度缓冲区
			depthStencil.depthCompareOp		   = VK_COMPARE_OP_LESS;				//指定执行保留或者丢弃片段的比较细节
			depthStencil.depthBoundsTestEnable = VK_FALSE;							//depthBoundsTestEnable, minDepthBounds 和 maxDepthBounds 字段用于可选择的优化深度绑定测试
			depthStencil.minDepthBounds		   = 0.0f;
			depthStencil.maxDepthBounds		   = 1.0f;
			depthStencil.stencilTestEnable	   = VK_FALSE;							//用于配置模版缓冲区的操作
			depthStencil.front				   = {};
			depthStencil.back				   = {};

            //颜色附件列表
            VkPipelineColorBlendAttachmentState colorAttachmentNode{
                .blendEnable         = VK_FALSE,
                .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
                .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
                .colorBlendOp		 = VK_BLEND_OP_ADD,
                .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                .alphaBlendOp		 = VK_BLEND_OP_ADD,
                .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
            };

            switch (renderAttType) {
            case renderEnum::renderAttId::forward_ra:
                colorAttachmentList.push_back(colorAttachmentNode);
                break;
            case renderEnum::renderAttId::deferred_ra:
                colorAttachmentList.push_back(colorAttachmentNode);
                colorAttachmentList.push_back(colorAttachmentNode);
                colorAttachmentList.push_back(colorAttachmentNode);
                break;
            case renderEnum::renderAttId::shadowMap_ra:
                //阴影管线无需颜色附件
                break;
            }

            //颜色附件配置
			colorAttachmentState.sType			   = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			colorAttachmentState.logicOpEnable	   = VK_FALSE;
			colorAttachmentState.logicOp		   = VK_LOGIC_OP_COPY;		
			colorAttachmentState.attachmentCount   = (uint32_t)colorAttachmentList.size();
			colorAttachmentState.pAttachments	   = colorAttachmentList.data();
			colorAttachmentState.blendConstants[0] = 0.0f;				
			colorAttachmentState.blendConstants[1] = 0.0f;				
			colorAttachmentState.blendConstants[2] = 0.0f;				
			colorAttachmentState.blendConstants[3] = 0.0f;				

            //管线信息
			pipeInfo.sType				 = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            pipeInfo.stageCount          = (uint32_t)shaderStageList.size();
            pipeInfo.pStages             = shaderStageList.data();
            pipeInfo.pVertexInputState   = &vertInputStage;
			pipeInfo.pInputAssemblyState = &primitiveState;
			pipeInfo.pViewportState		 = &viewportState;
			pipeInfo.pRasterizationState = &rasterizer;
			pipeInfo.pMultisampleState	 = &multisampling;
			pipeInfo.pDepthStencilState  = &depthStencil;
			pipeInfo.pColorBlendState	 = &colorAttachmentState;
			pipeInfo.pDynamicState		 = nullptr;					
			pipeInfo.layout			     = pipeLayout;
            pipeInfo.renderPass          = render;
			pipeInfo.subpass			 = 0;
			pipeInfo.basePipelineHandle  = VK_NULL_HANDLE;		
			pipeInfo.basePipelineIndex	 = -1;
		}
    
        //设置管线动态处理的阶段
        void setPipeDynamicStage(const std::vector<VkDynamicState>& list) {
            pipeDynamicStageList                    = list;
            pipeDynamicStageState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            pipeDynamicStageState.dynamicStateCount = (uint32_t)pipeDynamicStageList.size();
            pipeDynamicStageState.pDynamicStates    = pipeDynamicStageList.data();

            pipeInfo.pDynamicState = &pipeDynamicStageState;
        }
    };



    //pipeLayout结点
    struct pipeLayoutNode {
        VkPipelineLayout pipeLayout{};      //管线布局



        void createPipeLayoutNode(const VkDevice& device, const VkDescriptorSetLayout& dstLayout) {
            VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
            pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount         = 1;
            pipelineLayoutInfo.pushConstantRangeCount = 0;
            pipelineLayoutInfo.pPushConstantRanges    = 0;
            pipelineLayoutInfo.pSetLayouts            = &dstLayout;

            if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipeLayout) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "管线布局创建失败");
            }
        }

        void clear(const VkDevice& device) {
            vkDestroyPipelineLayout(device, pipeLayout, nullptr);
        }
    };

    //pipe结点
    struct pipelineNode {
        std::vector<pipelineStruct::shaderNode> shaderList{};   //着色器列表
        pipelineStruct::pipeConfigNode          pipeConfig{};   //管线配置
        VkPipeline                              pipeline{};     //管线



        //创建顶点管线结点
        void createPipelineNode(const VkSampleCountFlagBits& msaa, const renderStruct::renderTemplateNode& renderTemplateNode, const pipelineStruct::pipeLayoutNode& pipeLayoutNode, const std::vector<std::string>& shaderFileList, const VkDevice& device) {
            //创建shaderList
            shaderList.resize(shaderFileList.size());
            if (shaderFileList.size() == pipelineEnum::shaderNodeId::vert::count) {
                shaderList[pipelineEnum::shaderNodeId::vert::vert].createShaderNode(device, shaderFileList[0], VK_SHADER_STAGE_VERTEX_BIT);
                shaderList[pipelineEnum::shaderNodeId::vert::frag].createShaderNode(device, shaderFileList[1], VK_SHADER_STAGE_FRAGMENT_BIT);
            }
            else if (shaderFileList.size() == pipelineEnum::shaderNodeId::mesh::count) {
                //todo 待写网格着色
            }

            //创建pipeConfig
            pipeConfig.createPipeConfigNode(shaderList, msaa, renderTemplateNode.renderAttType, renderTemplateNode.render, pipeLayoutNode.pipeLayout);
        }



        void clear(const VkDevice& device) {
            vkDestroyPipeline(device, pipeline, nullptr);

            for (auto shaderNode : shaderList) {
                shaderNode.clear(device);
            }
        }

        void clearSwapChain(const VkDevice& device) {
            vkDestroyPipeline(device, pipeline, nullptr);
        }
    };
}

//cmd结构空间
namespace cmdStruct {
    //cmd线程结点
    struct cmdThreadNode {
        std::mutex lock{};      //线程锁
        int current = 0;        //当前线程
        int next    = 0;        //下一线程



        //判断更新
        bool isFlip() const {
            return next != current;
        }

        //进行更新：next切到另一线程
        void flip() {
            std::lock_guard<std::mutex> guard(lock);
            next = (current + 1) % 2;
        }

        //完成更新：当前线程切到next
        void swap() {
            std::lock_guard<std::mutex> guard(lock);
            current = next;
        }
    };

    //cmd缓冲结点
    struct cmdBufferNode {
        VkCommandPool                pool  = VK_NULL_HANDLE;
        VkFence                      fence = VK_NULL_HANDLE;    //本线程命令执行完成信号
        VkCommandBufferAllocateInfo  info{};
        std::vector<VkCommandBuffer> list{};
    
    
    
        //创建cmdPool、info、fence
        cmdBufferNode(const VkDevice& device, const uint32_t& queueFamily, const uint32_t& frameCount) {
            VkCommandPoolCreateInfo poolInfo{};
            poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            poolInfo.queueFamilyIndex = queueFamily;
    
            if (vkCreateCommandPool(device, &poolInfo, nullptr, &pool) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "cmdPool创建失败");
            }
    
            info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            info.commandPool        = pool;
            info.commandBufferCount = frameCount;
    
            list.resize(frameCount);
            createCmdList(device);
    
            VkFenceCreateInfo fenceInfo{};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
            if (vkCreateFence(device, &fenceInfo, nullptr, &fence) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "fence创建失败");
            }
        }
    
        cmdBufferNode() = delete;
    
    
    
        //创建cmdList
        void createCmdList(const VkDevice& device) {
            if (vkAllocateCommandBuffers(device, &info, list.data()) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "cmdList创建失败");
            }
        }
    
        //重建cmdList
        void recreateCmdList(const VkDevice& device) {
            clearCmdList(device);
            createCmdList(device);
        }

        //清理cmdPool
        void clearCmdPool(const VkDevice& device) {
            vkDestroyFence(device, fence, nullptr);
            vkDestroyCommandPool(device, pool, nullptr);

            pool  = VK_NULL_HANDLE;
            fence = VK_NULL_HANDLE;
        }

        //清理cmdList
        void clearCmdList(const VkDevice& device) {
            vkFreeCommandBuffers(device, pool, static_cast<uint32_t>(list.size()), list.data());
        }
    
    
    
        //重载
        VkCommandBuffer& operator[](int frameId) { return list[frameId]; }
        size_t           size()            const { return list.size(); }
    };

    //todo后续写成pipe结点
    //cmd配置结点
    struct cmdConfigNode {
        VkPipeline       pipe        = VK_NULL_HANDLE;
        VkPipelineLayout layout      = VK_NULL_HANDLE;
        VkDescriptorSet  desc        = VK_NULL_HANDLE;
        VkBuffer         vertBuffer  = VK_NULL_HANDLE;
        VkBuffer         indexBuffer = VK_NULL_HANDLE;



        //重置
        void reset() {
            pipe        = VK_NULL_HANDLE;
            layout      = VK_NULL_HANDLE;
            desc        = VK_NULL_HANDLE;
            vertBuffer  = VK_NULL_HANDLE;
            indexBuffer = VK_NULL_HANDLE;
        }
    };

    //cmd录制结点
    struct cmdRecorderNode {
        VkCommandBuffer           cmd = VK_NULL_HANDLE;     //命令
        VkCommandBufferBeginInfo  cmdBegin{};               //命令开始
        cmdStruct::cmdConfigNode  cmdConfig{};              //命令配置

        VkViewport                viewport{};               //视口
        VkRect2D                  scissor{};                //裁剪

        std::vector<VkDeviceSize> vertOffset{ 0 };          //顶点偏移
        VkMemoryBarrier           memBarrier{};             //内存屏障



        cmdRecorderNode() {
            cmdBegin.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            cmdBegin.flags            = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
            cmdBegin.pInheritanceInfo = nullptr;

            viewport.x        = 0.0f;
            viewport.y        = 0.0f;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            scissor.offset = { 0, 0 };

            memBarrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
            memBarrier.pNext         = nullptr;
            memBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            memBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        }

        //绑定cmd
        void bind(const VkCommandBuffer& tCmd) {
            cmd = tCmd;
            cmdConfig.reset();
        }

        //重置cmd
        void reset() {
            if (vkResetCommandBuffer(cmd, 0) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "cmd重置失败");
            }
        }

        //开始cmd
        void begin() {
            if (vkBeginCommandBuffer(cmd, &cmdBegin) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "cmd开始失败");
            }
        }

        //结束cmd
        void end() {
            vertOffset = std::vector<VkDeviceSize>{ 0 };

            if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "cmd结束失败");
            }
        }

        //开始渲染
        void beginRender(const VkRenderPassBeginInfo* info) {
            cmdConfig.reset();
            vkCmdBeginRenderPass(cmd, info, VK_SUBPASS_CONTENTS_INLINE);
        }

        //结束渲染
        void endRender() {
            vkCmdEndRenderPass(cmd);
        }



        //绘制
        void draw(const pipelineEnum::cmdDrawId::type& cmdDrawType, const VkPipeline& tPipe, const VkPipelineLayout& tLayout, const VkDescriptorSet& tDesc, const VkBuffer& tVertBuffer = VK_NULL_HANDLE, const VkBuffer& tIndexBuffer = VK_NULL_HANDLE, uint32_t tCount = 0, const VkExtent2D& tExtent = {}) {
            switch (cmdDrawType) {
            case pipelineEnum::cmdDrawId::indexed: {
                bindPipe(tPipe);
                bindLayoutDesc(tLayout, tDesc);
                bindVert(tVertBuffer);
                bindIndex(tIndexBuffer);

                vkCmdDrawIndexed(cmd, tCount, 1, 0, 0, 0);
                break;
            }
            case pipelineEnum::cmdDrawId::fullscreen: {
                viewport.width  = static_cast<float>(tExtent.width);
                viewport.height = static_cast<float>(tExtent.height);
                scissor.extent  = tExtent;

                bindPipe(tPipe);
                bindLayoutDesc(tLayout, tDesc);

                vkCmdSetViewport(cmd, 0, 1, &viewport);
                vkCmdSetScissor(cmd, 0, 1, &scissor);
                vkCmdDraw(cmd, 3, 1, 0, 0);
                break;
            }
            case pipelineEnum::cmdDrawId::text: {
                bindPipe(tPipe);
                bindLayoutDesc(tLayout, tDesc);
                bindVert(tVertBuffer);

                vkCmdBindVertexBuffers(cmd, 1, 1, &tVertBuffer, vertOffset.data());
                for (uint32_t i = 0; i < tCount; i++) {
                    vkCmdDraw(cmd, 4, 1, i * 4, 0);
                }
                break;
            }
            }
        }

        //管线屏障
        void barrier() {
            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 1, &memBarrier, 0, nullptr, 0, nullptr);
        }

    private:
        //绑定pipe
        void bindPipe(const VkPipeline& tPipe) {
            if (cmdConfig.pipe != tPipe) {
                cmdConfig.pipe = tPipe;
                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, cmdConfig.pipe);
            }
        }

        //绑定layout、desc（API 要求同时给）
        void bindLayoutDesc(const VkPipelineLayout& tLayout, const VkDescriptorSet& tDesc) {
            if (cmdConfig.layout != tLayout || cmdConfig.desc != tDesc) {
                cmdConfig.layout = tLayout;
                cmdConfig.desc   = tDesc;
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, cmdConfig.layout, 0, 1, &cmdConfig.desc, 0, nullptr);
            }
        }

        //绑定vert
        void bindVert(const VkBuffer& tVertBuffer) {
            if (cmdConfig.vertBuffer != tVertBuffer) {
                cmdConfig.vertBuffer = tVertBuffer;
                vkCmdBindVertexBuffers(cmd, 0, 1, &cmdConfig.vertBuffer, vertOffset.data());
            }
        }

        //绑定index
        void bindIndex(const VkBuffer& tIndexBuffer) {
            if (cmdConfig.indexBuffer != tIndexBuffer) {
                cmdConfig.indexBuffer = tIndexBuffer;
                vkCmdBindIndexBuffer(cmd, cmdConfig.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
            }
        }
    };



    //cmd结点
    struct cmdNode {
        cmdStruct::cmdThreadNode              thread{};     //线程
        std::vector<cmdStruct::cmdBufferNode> buffer{};     //缓冲
        cmdStruct::cmdRecorderNode            recorder{};   //录制
    
    
    
        cmdNode(const VkDevice& device, const uint32_t& queueFamily, const uint32_t& frameCount) : buffer{ cmdStruct::cmdBufferNode(device, queueFamily, frameCount), cmdStruct::cmdBufferNode(device, queueFamily, frameCount) }{}
    
        cmdNode() = delete;
    
        void clear(const VkDevice& device) {
            for (auto& node : buffer) {
                node.clearCmdPool(device);
            }
        }
    
    

        //录制命令
        template<typename F>
        void record(int threadId, int frameId, F&& func) {
            recorder.bind(buffer[threadId][frameId]);
            recorder.reset();
            recorder.begin();
            func(recorder);
            recorder.end();
        }

        //提交单次命令
        void submitOnce(const VkDevice& device, const VkQueue& graphicsQueue, const VkCommandBuffer& cmd) {
            std::lock_guard<std::mutex> queueGuard(lockInfo::queueThreadLock);
            vkEndCommandBuffer(cmd);
    
            VkSubmitInfo submitInfo{};
            submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers    = &cmd;
    
            vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
            vkQueueWaitIdle(graphicsQueue);
            vkFreeCommandBuffers(device, buffer[0].pool, 1, &cmd);
        }

        //获取当前命令列表
        std::vector<VkCommandBuffer>& getCmdList() {
            return buffer[thread.current].list;
        }
    
        //等待本线程命令执行完成
        void waitFence(const VkDevice& device, int threadId) {
            vkWaitForFences(device, 1, &buffer[threadId].fence, VK_TRUE, UINT64_MAX);       //等待fence
        }



        bool isFlip() const { return thread.isFlip(); }
        void swap()        { thread.swap(); }
        void flip()        { thread.flip(); }
    };
}



//buffer结构空间， todo后续慢慢拆分
namespace bufferStruct {
    //ui信息结点
    struct uiTextInfoNode {
        glm::dvec3 pos;
        glm::dvec3 chunkPos;
        glm::ivec3 minepos{};
    };



    //点光源结点
    struct pointLightNode {
        glm::vec3 position;     //位置
        float radius;           //半径

        glm::vec3 color;        //颜色
        float placeholder;      //占位符
    };

    //平行光结点
    struct dirLightNode {
        glm::vec3 direction;    //方向
        float scale;            //强度

        glm::vec3 color;        //颜色
        float placeholder;      //占位符
    };

    //聚光灯结点
    struct spotLightNode {
        glm::vec3 position;     //位置
        float inner;            //内圆锥

        glm::vec3 color;        //颜色
        float outer;            //外圆锥

        glm::vec3 direction;    //方向
        float placeholder;      //占位符
    };

    //mvp统一结点
    struct mvpUBO {
        glm::mat4 model;            //模型矩阵，用于将模型变换到世界坐标系
        glm::mat4 modelTI;          //模型转置逆矩阵，用于计算光照法线
        glm::mat4 view;             //视图矩阵，用于将世界坐标系变换到观察者坐标系
        glm::mat4 proj;             //投影矩阵，用于将观察者坐标系变换到裁剪坐标系
    };

    //前向光照统一结点
    struct forwardLightUBO {
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

    //延迟光照统一结点
    struct deferredLightUBO {
        glm::vec3 viewPos;          //观察者坐标，用于计算镜面反射
        float currentHour;          //当前小时，通过gameTime(游戏运行时长)%24得到，用于周期性效果
        
        float ambientScale;         //环境光强度
        float diffuseScale;         //漫反射强度
        float specularScale;        //镜面反射强度
        float placeholder;          //占位符
        
        pointLightNode pointLightList[1];
        dirLightNode   dirLightList[1];
        spotLightNode  spotLightList[1];
    };

    //延迟阴影统一结点
    struct deferredShadowUBO {
        glm::mat4 shadowMvp;        //阴影mvp
        glm::vec2 shadowExtent;     //阴影贴图大小
    };

    //阴影mvp统一结点
    struct shadowMvpUBO {
        glm::mat4 mvp;              //阴影mvp变换
    };



    /*管线缓冲区*/
    struct terrainBufferNode {
        //todo
        textureBufferNode temp_texture;                                                                  //纹理结点，构造时初始化



        terrainBufferNode(const bool& tEnable, const pipelineEnum::procPipelineId::type& tPipeType, const bufferEnum::procSamplerId::type& tSamplerType, const char* imageFile, const VkFormat& colorFormat, const VkExtent2D& tileExtent, const VkExtent2D& atlasExtent, const VkDevice& device, const VmaAllocator& memAllocator)
            :isEnable(tEnable), pipeType(tPipeType), samplerType(tSamplerType), temp_texture(imageFile, colorFormat, tileExtent, atlasExtent, device, memAllocator) {

            textureFile = imageFile;
            arrayLayer  = atlasExtent.width * atlasExtent.height;
        }

        terrainBufferNode() = delete;



        //---
        //todo baseBufferNode    mesh[bufferInfo::commandThreadCount][plateEnum::terrainLayer::count];     //网格结点
        baseBufferNode vertex[bufferInfo::commandThreadCount][plateEnum::terrainLayer::count];
        baseBufferNode index[bufferInfo::commandThreadCount][plateEnum::terrainLayer::count];

        //纹理缓冲
        const char* textureFile;
        imageBufferNode texture;

        //纹理参数
        uint32_t arrayLayer;
        uint32_t mipLevel = 1;

        //统一缓冲
        baseBufferNode uniform;
        mvpUBO ubo;

        //描述缓冲
        VkDescriptorSet descriptorSet;

        //程序化索引
        bool isEnable;
        //todo 改成type
        pipelineEnum::procPipelineId::type pipeType;  //管线索引
        bufferEnum::procSamplerId::type samplerType;  //采样索引



        //清理结构体，descriptorSet由描述符池管理，无需手动清理
        void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
            for (int thread = 0; thread < bufferInfo::commandThreadCount; thread++) {
                for (int layer = 0; layer < plateEnum::terrainLayer::count; layer++) {
                    vmaDestroyBuffer(memAllocator, vertex[thread][layer].buffer, vertex[thread][layer].bufferMem);
                    vmaDestroyBuffer(memAllocator, index[thread][layer].buffer, index[thread][layer].bufferMem);
                }
            }

            vmaDestroyImage(memAllocator, texture.image, texture.imageMem);
            vkDestroyImageView(device, texture.view, nullptr);

            vmaDestroyBuffer(memAllocator, uniform.buffer, uniform.bufferMem);
        }
    };

    struct staticModelNode {
        //模型文件
        std::string modelFile;

        //网格缓冲
        baseBufferNode vertex;
        baseBufferNode index;

        //纹理缓冲
        const char* textureFile;
        imageBufferNode texture;
        //const char* normalFile;
        //imageBufferNode normal;

        //纹理参数
        uint32_t mipLevel;                  //在创建texture时赋值

        //统一缓冲
        baseBufferNode uniform;
        mvpUBO ubo;

        //描述缓冲
        VkDescriptorSet descriptorSet;

        //程序化索引
        bufferEnum::procSamplerId::type samplerId;

        //用于屏蔽超出视觉范围模型的命令提交和统一缓冲更新
        glm::dvec3 modelWorldPos;           //模型世界坐标（d世界坐标系），在初始化时赋值
        glm::dvec3 currentWorldPos;         //当前世界坐标（d世界坐标系），在更新统一缓冲时赋值
        bool isVisible = true;              //模型可见标识，在提交命令时赋值



        void create(const std::string tempModelFile, const char* tempTextureFile, /*const char* tempNormalFile,*/ bufferEnum::procSamplerId::type sId, const glm::dvec3& tempModelWorldPos) {
            modelFile     = tempModelFile;
            textureFile   = tempTextureFile;
            //normalFile  = tempNormalFile;
            samplerId     = sId;
            modelWorldPos = tempModelWorldPos;
        }

        void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
            vmaDestroyBuffer(memAllocator, vertex.buffer, vertex.bufferMem);
            vmaDestroyBuffer(memAllocator, index.buffer, index.bufferMem);

            vmaDestroyImage(memAllocator, texture.image, texture.imageMem);
            vkDestroyImageView(device, texture.view, nullptr);

            //vmaDestroyImage(memAllocator, normal.image, normal.imageMem);
            //vkDestroyImageView(device, normal.view, nullptr);

            vmaDestroyBuffer(memAllocator, uniform.buffer, uniform.bufferMem);
        }
    };

    struct dynamicModelSubNode {
        //网格缓冲
        baseBufferNode vertex;
        baseBufferNode index;

        //纹理缓冲
        const char* textureFile;
        imageBufferNode texture;
        //const char* normalFile;
        //imageBufferNode normal;

        //统一缓冲
        baseBufferNode uniform;
        mvpUBO ubo;

        //描述缓冲
        VkDescriptorSet descriptorSet;



        void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
            vmaDestroyBuffer(memAllocator, vertex.buffer, vertex.bufferMem);
            vmaDestroyBuffer(memAllocator, index.buffer, index.bufferMem);

            vmaDestroyImage(memAllocator, texture.image, texture.imageMem);
            vkDestroyImageView(device, texture.view, nullptr);

            //vmaDestroyImage(memAllocator, normal.image, normal.imageMem);
            //vkDestroyImageView(device, normal.view, nullptr);

            vmaDestroyBuffer(memAllocator, uniform.buffer, uniform.bufferMem);
        }
    };

    struct dynamicModelNode {
        //模型文件
        std::string modelFile;

        //动态模型结点列表
        std::vector<dynamicModelSubNode> node;

        //纹理参数
        uint32_t mipLevel;

        //程序化索引
        bufferEnum::procSamplerId::type samplerId;

        //用于屏蔽超出视觉范围模型的命令提交和统一缓冲更新
        glm::dvec3 modelWorldPos;           //模型世界坐标（d世界坐标系），在初始化时赋值
        glm::dvec3 currentWorldPos;         //当前世界坐标（d世界坐标系），在更新统一缓冲时赋值
        bool isVisible = true;              //模型可见标识，在提交命令时赋值



        void create(const std::string tempModelFile, std::vector<const char*> tempTextureFile, /*std::vector<const char*> tempNormalFile,*/ bufferEnum::procSamplerId::type sId, const glm::dvec3& tempModelWorldPos) {
            modelFile = tempModelFile;

            node.resize(tempTextureFile.size());
            for (int i = 0; i < tempTextureFile.size(); i++) {
                node[i].textureFile = tempTextureFile[i];
                //node[i].normalFile = tempNormalFile[i];
            }

            samplerId = sId;
            modelWorldPos = tempModelWorldPos;
        }

        void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
            for (int i = 0; i < node.size(); i++) {
                node[i].clear(device, memAllocator);
            }
        }
    };

    struct skyboxBufferNode {
        //网格缓冲
        baseBufferNode vertex;
        baseBufferNode index;

        //统一缓冲
        baseBufferNode uniform;
        mvpUBO ubo;
        baseBufferNode lightUniform;
        forwardLightUBO lightUbo;

        //描述缓冲
        VkDescriptorSet descriptorSet;

        //程序化索引
        bool isEnable;
        pipelineEnum::procPipelineId::type pipeId;



        skyboxBufferNode(bool enable, pipelineEnum::procPipelineId::type pId) {
            isEnable = enable;
            pipeId   = pId;
        }

        skyboxBufferNode() = delete;

        void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
            vmaDestroyBuffer(memAllocator, vertex.buffer, vertex.bufferMem);
            vmaDestroyBuffer(memAllocator, index.buffer, index.bufferMem);

            vmaDestroyBuffer(memAllocator, uniform.buffer, uniform.bufferMem);
            vmaDestroyBuffer(memAllocator, lightUniform.buffer, lightUniform.bufferMem);
        }
    };

    struct mineBufferNode {
        //网格缓冲
        baseBufferNode vertex;
        baseBufferNode index;

        //统一缓冲
        baseBufferNode uniform;
        mvpUBO ubo;

        //描述缓冲
        VkDescriptorSet descriptorSet;

        //程序化索引
        bool isEnable;
        pipelineEnum::procPipelineId::type pipeId;



        mineBufferNode(bool enable, pipelineEnum::procPipelineId::type pId) {
            isEnable = enable;
            pipeId   = pId;
        }

        mineBufferNode() = delete;

        void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
            vmaDestroyBuffer(memAllocator, vertex.buffer, vertex.bufferMem);
            vmaDestroyBuffer(memAllocator, index.buffer, index.bufferMem);

            vmaDestroyBuffer(memAllocator, uniform.buffer, uniform.bufferMem);
        }
    };

    struct uiTextBufferNode {
        //网格缓冲
        baseBufferNode vertex;

        //纹理缓冲
        imageBufferNode texture;
        stb_fontchar* stbFontData = new stb_fontchar[STB_FONT_consolas_24_latin1_NUM_CHARS];    //字符数据

        //纹理参数
        glm::vec4* mapped = nullptr;
        uint32_t numLetters;

        //描述缓冲
        VkDescriptorSet descriptorSet;

        //程序化索引
        bool isEnable;
        pipelineEnum::procPipelineId::type pipeId;
        bufferEnum::procSamplerId::type samplerId;



        uiTextBufferNode(bool enable, pipelineEnum::procPipelineId::type pId, bufferEnum::procSamplerId::type sId) {
            isEnable  = enable;
            pipeId    = pId;
            samplerId = sId;
        }

        uiTextBufferNode() = delete;

        void beginTextUpdate(VmaAllocator memAllocator) {
            vmaMapMemory(memAllocator, vertex.bufferMem, (void**)&mapped);
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

        void endTextUpdate(const VmaAllocator& memAllocator) {
            vmaUnmapMemory(memAllocator, vertex.bufferMem);
            mapped = nullptr;
        }

        void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
            vmaDestroyBuffer(memAllocator, vertex.buffer, vertex.bufferMem);

            vmaDestroyImage(memAllocator, texture.image, texture.imageMem);
            vkDestroyImageView(device, texture.view, nullptr);
        }
    };

    struct osMirrorBufferNode {
        //网格缓冲
        baseBufferNode vertex;
        baseBufferNode index;

        //统一缓冲
        baseBufferNode uniform;
        mvpUBO ubo;
        baseBufferNode lightUniform;
        forwardLightUBO lightUbo;

        //描述缓冲
        VkDescriptorSet descriptorSet;

        //程序化索引
        bool isEnable;
        pipelineEnum::procPipelineId::type pipeId;
        bufferEnum::procSamplerId::type samplerId;



        osMirrorBufferNode(bool enable, pipelineEnum::procPipelineId::type pId, bufferEnum::procSamplerId::type sId) {
            isEnable  = enable;
            pipeId    = pId;
            samplerId = sId;
        }

        osMirrorBufferNode() = delete;

        void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
            vmaDestroyBuffer(memAllocator, vertex.buffer, vertex.bufferMem);
            vmaDestroyBuffer(memAllocator, index.buffer, index.bufferMem);

            vmaDestroyBuffer(memAllocator, uniform.buffer, uniform.bufferMem);
            vmaDestroyBuffer(memAllocator, lightUniform.buffer, lightUniform.bufferMem);
        }
    };

    struct osScenceBufferNode {
        //描述缓冲
        VkDescriptorSet descriptorSet;

        //程序化索引
        bool isEnable;
        pipelineEnum::procPipelineId::type pipeId;
        bufferEnum::procSamplerId::type samplerId;



        osScenceBufferNode(bool enable, pipelineEnum::procPipelineId::type pId, bufferEnum::procSamplerId::type sId) {
            isEnable  = enable;
            pipeId    = pId;
            samplerId = sId;
        }

        osScenceBufferNode() = delete;

        void clear(const VkDevice& device, const VmaAllocator& memAllocator) {

        }
    };

    struct dsScenceBufferNode {
        //统一缓冲
        baseBufferNode lightUniform;
        deferredLightUBO lightUbo;
        baseBufferNode shadowUniform;
        deferredShadowUBO shadowUbo;

        //描述缓冲
        VkDescriptorSet descriptorSet;

        //程序化索引
        bool isEnable;
        pipelineEnum::procPipelineId::type pipeId;
        bufferEnum::procSamplerId::type samplerId;



        dsScenceBufferNode(bool enable, pipelineEnum::procPipelineId::type pId, bufferEnum::procSamplerId::type sId) {
            isEnable  = enable;
            pipeId    = pId;
            samplerId = sId;
        }

        dsScenceBufferNode() = delete;

        void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
            vmaDestroyBuffer(memAllocator, lightUniform.buffer,  lightUniform.bufferMem);
            vmaDestroyBuffer(memAllocator, shadowUniform.buffer, shadowUniform.bufferMem);
        }
    };

    struct shadowTerrainBufferNode {
        //网格缓冲，索引直接用源网格的
        baseBufferNode vertex[bufferInfo::commandThreadCount][plateEnum::terrainLayer::count];

        //统一缓冲
        baseBufferNode shadowMvpUniform;
        shadowMvpUBO shadowMvpUbo;

        //描述缓冲
        VkDescriptorSet descriptorSet;



        void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
            for (int thread = 0; thread < bufferInfo::commandThreadCount; thread++) {
                for (int layer = 0; layer < plateEnum::terrainLayer::count; layer++) {
                    vmaDestroyBuffer(memAllocator, vertex[thread][layer].buffer, vertex[thread][layer].bufferMem);
                }
            }

            vmaDestroyBuffer(memAllocator, shadowMvpUniform.buffer, shadowMvpUniform.bufferMem);
        }
    };

    struct shadowBufferNode {
        //网格缓冲，索引直接用源网格的
        baseBufferNode vertex;

        //统一缓冲
        baseBufferNode shadowMvpUniform;
        shadowMvpUBO shadowMvpUbo;

        //描述缓冲
        VkDescriptorSet descriptorSet;



        void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
            vmaDestroyBuffer(memAllocator, vertex.buffer, vertex.bufferMem);

            vmaDestroyBuffer(memAllocator, shadowMvpUniform.buffer, shadowMvpUniform.bufferMem);
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
            layout.binding        = id;
            layout.descriptorType = type;
            layout.stageFlags     = stage;

            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstArrayElement = 0;
            write.descriptorCount = 1;
            write.dstBinding      = id;
            write.descriptorType  = type;

            bufferInfo.offset = 0;
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }

        descriptorConfigNode() = delete;

        const VkWriteDescriptorSet& setWrite(const VkDescriptorSet& dstSet, const VkBuffer& buffer, const VkDeviceSize& range) {
            bufferInfo.buffer = buffer;
            bufferInfo.range  = range;

            write.dstSet      = dstSet;
            write.pBufferInfo = &bufferInfo;

            return write;
        }

        const VkWriteDescriptorSet& setWrite(const VkDescriptorSet& dstSet, const VkSampler& sampler, const VkImageView& imageView) {
            imageInfo.sampler   = sampler;
            imageInfo.imageView = imageView;

            write.dstSet     = dstSet;
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
            }
        }
    };
}