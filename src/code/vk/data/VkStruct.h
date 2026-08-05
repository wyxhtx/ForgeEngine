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



//swapchain结构空间
namespace swapStruct {
    struct swapSupportNode {
        VkSurfaceCapabilitiesKHR capabilitie{};
        std::vector<VkSurfaceFormatKHR> scFormat{};
        std::vector<VkPresentModeKHR> presentMode{};

        VkSwapchainCreateInfoKHR scCreateInfo{};



        swapSupportNode() {
            scCreateInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            scCreateInfo.imageArrayLayers = 1;
            scCreateInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            scCreateInfo.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
            scCreateInfo.clipped          = VK_TRUE;
            scCreateInfo.oldSwapchain     = VK_NULL_HANDLE;
        }

        //设置交换链支持结点
        void createScSupport(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& presentSurface) {
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
            scFormat.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, presentSurface, &formatCount, scFormat.data());


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
        void createScCreateInfo(const VkSurfaceKHR& presentSurface) {
            VkSurfaceFormatKHR scFormat = getScFormat();
            scCreateInfo.surface                = presentSurface;
            scCreateInfo.minImageCount          = capabilitie.minImageCount + (capabilitie.maxImageCount > capabilitie.minImageCount);
            scCreateInfo.imageFormat            = scFormat.format;
            scCreateInfo.imageColorSpace        = scFormat.colorSpace;
            scCreateInfo.imageExtent            = getScExtent();
            scCreateInfo.imageSharingMode       = VK_SHARING_MODE_EXCLUSIVE;
            scCreateInfo.queueFamilyIndexCount  = 0;
            scCreateInfo.pQueueFamilyIndices    = nullptr;
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
            return VK_FORMAT_UNDEFINED;
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
        void recreateScCreateInfo(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& presentSurface, const VkSwapchainKHR& swapchain) {
            if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, presentSurface, &capabilitie)) {
                Log::printError(__FILE__, __LINE__, "重建交换链时表面功能获取失败");
            }

            scCreateInfo.imageExtent = getScExtent();
            scCreateInfo.oldSwapchain = swapchain;
        }
    };
}

//前向声明buffer的缓冲结点
namespace bufferStruct {
    //图像缓冲结点
    struct imageNode {
        VkImage       image;
        VmaAllocation imageMemory;
        VkImageView   view;
    };

    //通用缓冲结点
    struct bufferNode {
        VkBuffer      buffer;
        VmaAllocation bufferMemory;
    };
}

//render结构空间
namespace renderStruct {
    //renderPass附件结构体
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

    //renderPass子通道结构体
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

    //renderPass帧图像结构体
    struct renderFrameNode {
        std::vector<std::vector<bufferStruct::imageNode>> frameImageList{};      //帧图像列表
        std::vector<VkFramebuffer>                        frameBufferList{};     //帧缓冲列表



        renderFrameNode() {

        }

        //renderFrameNode() = delete;



        void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
            for (bufferStruct::imageNode& node : frameImageList[renderEnum::renderFrameId::conv_rf]) {
                vkDestroyImageView(device,    node.view,  nullptr);
                vmaDestroyImage(memAllocator, node.image, node.imageMemory);
            }

            for (bufferStruct::imageNode& node : frameImageList[renderEnum::renderFrameId::swap_rf]){
                vkDestroyImageView(device, node.view, nullptr);
            }

            for (int i = 0; i < frameBufferList.size(); i++) {
                vkDestroyFramebuffer(device, frameBufferList[i], nullptr);
            }
        }
    };

    /*
    //todo 都改成带Image的
    struct scFramebufferNode {
        bufferStruct::imageNode color;                          //颜色图像
        bufferStruct::imageNode depth;                          //深度图像
        std::vector<bufferStruct::imageNode> swapImageList;     //交换链图像

        std::vector<VkFramebuffer> framebuffer;



        void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
            vkDestroyImageView(device, color.view, nullptr);
            vmaDestroyImage(memAllocator, color.image, color.imageMemory);

            vkDestroyImageView(device, depth.view, nullptr);
            vmaDestroyImage(memAllocator, depth.image, depth.imageMemory);

            //通过vkGetSwapchainImagesKHR获取的VkImage，会随着swapchain的销毁自动释放
            for (int i = 0; i < swapImageList.size(); i++) {
                vkDestroyImageView(device, swapImageList[i].view, nullptr);
            }
            swapImageList.clear();

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



        void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
            vkDestroyImageView(device, color.view, nullptr);
            vmaDestroyImage(memAllocator, color.image, color.imageMemory);

            vkDestroyImageView(device, depth.view, nullptr);
            vmaDestroyImage(memAllocator, depth.image, depth.imageMemory);

            vkDestroyImageView(device, osView.view, nullptr);
            vmaDestroyImage(memAllocator, osView.image, osView.imageMemory);

            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
    };

    struct dsFramebufferNode {
        bufferStruct::imageNode position;  //位置附件（世界空间坐标）
        bufferStruct::imageNode normal;    //法线附件（世界空间法线）
        bufferStruct::imageNode albedo;    //反照率附件（颜色+高光系数）
        bufferStruct::imageNode depth;     //深度附件

        VkFramebuffer framebuffer;



        void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
            vkDestroyImageView(device, position.view, nullptr);
            vmaDestroyImage(memAllocator, position.image, position.imageMemory);

            vkDestroyImageView(device, normal.view, nullptr);
            vmaDestroyImage(memAllocator, normal.image, normal.imageMemory);

            vkDestroyImageView(device, albedo.view, nullptr);
            vmaDestroyImage(memAllocator, albedo.image, albedo.imageMemory);

            vkDestroyImageView(device, depth.view, nullptr);
            vmaDestroyImage(memAllocator, depth.image, depth.imageMemory);

            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
    };

    struct shFramebufferNode {
        bufferStruct::imageNode depth;      //深度附件

        VkFramebuffer framebuffer;



        void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
            vkDestroyImageView(device, depth.view, nullptr);
            vmaDestroyImage(memAllocator, depth.image, depth.imageMemory);

            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
    };
    //
    */


    //renderPass模板结构体
    struct renderTemplateNode {
        VkRenderPass render;



        void createRenderNode(const renderEnum::renderTemplateId::type& renderTemplateType, const VkFormat& colorFormat, const VkFormat& depthFormat, const VkSampleCountFlagBits& msaa, const VkDevice& device) {
            switch (renderTemplateType) {
            case renderEnum::renderTemplateId::swapChain_rt: {
                renderStruct::renderAttachmentNode renderAttachment{ renderEnum::renderAttId::forward_ra, colorFormat, depthFormat, msaa, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR };
                renderStruct::renderSubpassNode    renderSubpass   { renderTemplateType, VK_PIPELINE_BIND_POINT_GRAPHICS, renderAttachment.attRefList };

                createRenderPass(renderAttachment.attDescList, renderSubpass.subpassList, renderSubpass.dependencyList, device, "交换链");
                break;
            }
            case renderEnum::renderTemplateId::offScreen_rt: {
                renderStruct::renderAttachmentNode renderAttachment{ renderEnum::renderAttId::forward_ra, colorFormat, depthFormat, msaa, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
                renderStruct::renderSubpassNode    renderSubpass   { renderTemplateType, VK_PIPELINE_BIND_POINT_GRAPHICS, renderAttachment.attRefList };

                createRenderPass(renderAttachment.attDescList, renderSubpass.subpassList, renderSubpass.dependencyList, device, "离屏");
                break;
            }
            case renderEnum::renderTemplateId::deferred_rt: {
                renderStruct::renderAttachmentNode renderAttachment{ renderEnum::renderAttId::deferred_ra, colorFormat, depthFormat };
                renderStruct::renderSubpassNode    renderSubpass   { renderTemplateType, VK_PIPELINE_BIND_POINT_GRAPHICS, renderAttachment.attRefList };

                createRenderPass(renderAttachment.attDescList, renderSubpass.subpassList, renderSubpass.dependencyList, device, "延迟着色");
                break;
            }
            case renderEnum::renderTemplateId::shadowMap_rt: {
                renderStruct::renderAttachmentNode renderAttachment{ renderEnum::renderAttId::shadowMap_ra, VK_FORMAT_MAX_ENUM, depthFormat };
                renderStruct::renderSubpassNode    renderSubpass   { renderTemplateType, VK_PIPELINE_BIND_POINT_GRAPHICS, renderAttachment.attRefList };

                createRenderPass(renderAttachment.attDescList, renderSubpass.subpassList, renderSubpass.dependencyList, device, "阴影映射");
                break;
            }
            }
        }

        void clear(const VkDevice& device) {
            vkDestroyRenderPass(device, render, nullptr);
        }

    private:
        void createRenderPass(const std::vector<VkAttachmentDescription>& attDescList, const std::vector<VkSubpassDescription>& subpassList, std::vector<VkSubpassDependency>& dependencyList, const VkDevice& device, const char* name) {
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

    //todo
    //renderPass实例结构体
    struct renderInstanceNode {
        //todo
        //std::vector<VkClearValue> clearValue;       //清理值，决定渲染清空后的背景颜色



        renderInstanceNode(const renderEnum::renderAttId::type& renderAttType, const renderEnum::renderTemplateId::type& renderTemplateType) {

        }
    };



    /*---
    struct renderInfoNode {
        std::array<VkClearValue, 2> clearValueForward;      //前向清理（解析附件不需要清除值）
        std::array<VkClearValue, 4> clearValueDeferred;     //延迟清理
        std::array<VkClearValue, 1> clearValueShadow;       //阴影清理

        renderEnum::renderTemplateId::type renderTemplateType;
        std::variant<scFramebufferNode, osFramebufferNode, dsFramebufferNode, shFramebufferNode> frameNode;
        scFramebufferNode* scPtr = nullptr;
        osFramebufferNode* osPtr = nullptr;
        dsFramebufferNode* dsPtr = nullptr;
        shFramebufferNode* shPtr = nullptr;

        VkRenderPassBeginInfo renderInfo;



        void createRenderInfoNode(const VkClearColorValue& color, const VkClearDepthStencilValue& depthStencil, renderEnum::renderTemplateId::type tRenderTemplateType) {
            clearValueForward[0].color = color;
            clearValueForward[1].depthStencil = depthStencil;

            clearValueDeferred[0].color = color;
            clearValueDeferred[1].color = color;
            clearValueDeferred[2].color = color;
            clearValueDeferred[3].depthStencil = depthStencil;

            clearValueShadow[0].depthStencil = depthStencil;

            renderInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderInfo.renderArea.offset = { 0, 0 };

            renderTemplateType = tRenderTemplateType;

            if (tRenderTemplateType == renderEnum::renderTemplateId::swapChain_rt) {
                frameNode = scFramebufferNode{};
                scPtr = &std::get<bufferStruct::scFramebufferNode>(frameNode);

                renderInfo.clearValueCount = static_cast<uint32_t>(clearValueForward.size());
                renderInfo.pClearValues = clearValueForward.data();
            }
            else if (tRenderTemplateType == renderEnum::renderTemplateId::offScreen_rt) {
                frameNode = osFramebufferNode{};
                osPtr = &std::get<bufferStruct::osFramebufferNode>(frameNode);

                renderInfo.clearValueCount = static_cast<uint32_t>(clearValueForward.size());
                renderInfo.pClearValues = clearValueForward.data();
            }
            else if (tRenderTemplateType == renderEnum::renderTemplateId::deferred_rt) {
                frameNode = dsFramebufferNode{};
                dsPtr = &std::get<bufferStruct::dsFramebufferNode>(frameNode);

                renderInfo.clearValueCount = static_cast<uint32_t>(clearValueDeferred.size());
                renderInfo.pClearValues = clearValueDeferred.data();
            }
            else if (tRenderTemplateType == renderEnum::renderTemplateId::shadowMap_rt) {
                frameNode = shFramebufferNode{};
                shPtr = &std::get<bufferStruct::shFramebufferNode>(frameNode);

                renderInfo.clearValueCount = static_cast<uint32_t>(clearValueShadow.size());
                renderInfo.pClearValues = clearValueShadow.data();
            }
            else {
                Log::printError(__FILE__, __LINE__, "该帧缓冲类型不存在: \t" + std::to_string(tRenderTemplateType));
            }
        }

        void createRenderInfo(const VkExtent2D& scExtent, const VkFormat& colorFormat, const VkFormat& depthFormat, const VkSampleCountFlagBits& msaa, const VkRenderPass& render, const VkDevice& device, const VmaAllocator& memAllocator) {
            if (renderTemplateType == renderEnum::renderTemplateId::swapChain_rt) {
                createImageNode(scPtr->color, scExtent.width, scExtent.height, colorFormat, msaa, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_ASPECT_COLOR_BIT, device, memAllocator);
                createImageNode(scPtr->depth, scExtent.width, scExtent.height, depthFormat, msaa, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, device, memAllocator);
                createImageNode(scPtr->swapImageList, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, device);

                createScFramebuffer(scExtent, render, device);
            }
            else if (renderTemplateType == renderEnum::renderTemplateId::offScreen_rt) {
                createImageNode(osPtr->color, scExtent.width, scExtent.height, colorFormat, msaa, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, device, memAllocator);
                createImageNode(osPtr->depth, scExtent.width, scExtent.height, depthFormat, msaa, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, device, memAllocator);
                createImageNode(osPtr->osView, scExtent.width, scExtent.height, colorFormat, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, device, memAllocator);

                createOsFramebuffer(scExtent, render, device);
            }
            else if (renderTemplateType == renderEnum::renderTemplateId::deferred_rt) {
                createImageNode(dsPtr->position, scExtent.width, scExtent.height, renderInfo::dsPosColorFormat, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, device, memAllocator);
                createImageNode(dsPtr->normal, scExtent.width, scExtent.height, colorFormat, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, device, memAllocator);
                createImageNode(dsPtr->albedo, scExtent.width, scExtent.height, colorFormat, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, device, memAllocator);
                createImageNode(dsPtr->depth, scExtent.width, scExtent.height, depthFormat, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, device, memAllocator);

                createDsFramebuffer(scExtent, render, device);
            }
            else if (renderTemplateType == renderEnum::renderTemplateId::shadowMap_rt) {
                createImageNode(shPtr->depth, scExtent.width, scExtent.height, depthFormat, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, device, memAllocator);

                createShFramebuffer(scExtent, render, device);
            }
            else {
                Log::printError(__FILE__, __LINE__, "该帧缓冲类型不存在: \t" + std::to_string(renderTemplateType));
            }

            renderInfo.renderPass = render;
            renderInfo.renderArea.extent = scExtent;
        }

        void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
            std::visit([&device, &memAllocator](auto& node) {node.clear(device, memAllocator); }, frameNode);
        }

    private:
        //创建交换链帧缓冲
        void createScFramebuffer(const VkExtent2D& scExtent, const VkRenderPass& render, const VkDevice& device) {
            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = render;
            framebufferInfo.width = scExtent.width;
            framebufferInfo.height = scExtent.height;
            framebufferInfo.layers = 1;

            bufferStruct::scFramebufferNode& tempFrameNode = std::get<bufferStruct::scFramebufferNode>(frameNode);
            tempFrameNode.framebuffer.resize(tempFrameNode.swapImageList.size());

            for (size_t i = 0; i < tempFrameNode.swapImageList.size(); i++) {
                std::array<VkImageView, 3> attachments{ tempFrameNode.color.view,tempFrameNode.depth.view,tempFrameNode.swapImageList[i].view };
                framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
                framebufferInfo.pAttachments = attachments.data();

                if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &tempFrameNode.framebuffer[i]) != VK_SUCCESS) {
                    Log::printError(__FILE__, __LINE__, "交换链帧缓冲创建失败");
                }
            }
        }

        //创建离屏帧缓冲
        void createOsFramebuffer(const VkExtent2D& scExtent, const VkRenderPass& render, const VkDevice& device) {
            bufferStruct::osFramebufferNode& tempFrameNode = std::get<bufferStruct::osFramebufferNode>(frameNode);
            std::array<VkImageView, 3> attachments{ tempFrameNode.color.view,tempFrameNode.depth.view,tempFrameNode.osView.view };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = render;	                                    //渲染通道
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());	//附件数量
            framebufferInfo.pAttachments = attachments.data();							//附件
            framebufferInfo.width = scExtent.width;				                //宽度
            framebufferInfo.height = scExtent.height;				                //高度
            framebufferInfo.layers = 1;											//层数

            if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &tempFrameNode.framebuffer) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "离屏帧缓冲创建失败");
            }

            renderInfo.framebuffer = tempFrameNode.framebuffer;
        }

        //创建延迟渲染帧缓冲
        void createDsFramebuffer(const VkExtent2D& scExtent, const VkRenderPass& render, const VkDevice& device) {
            bufferStruct::dsFramebufferNode& tempFrameNode = std::get<bufferStruct::dsFramebufferNode>(frameNode);
            std::array<VkImageView, 4> attachments{ tempFrameNode.position.view, tempFrameNode.normal.view,tempFrameNode.albedo.view,tempFrameNode.depth.view };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = render;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = scExtent.width;
            framebufferInfo.height = scExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &tempFrameNode.framebuffer) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "延迟渲染帧缓冲创建失败");
            }

            renderInfo.framebuffer = tempFrameNode.framebuffer;
        }

        //创建阴影映射帧缓冲
        void createShFramebuffer(const VkExtent2D& scExtent, const VkRenderPass& render, const VkDevice& device) {
            bufferStruct::shFramebufferNode& tempFrameNode = std::get<bufferStruct::shFramebufferNode>(frameNode);

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = render;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = &tempFrameNode.depth.view;
            framebufferInfo.width = scExtent.width;
            framebufferInfo.height = scExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &tempFrameNode.framebuffer) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "阴影映射帧缓冲创建失败");
            }

            renderInfo.framebuffer = tempFrameNode.framebuffer;
        }



        //创建图像结点
        void createImageNode(bufferStruct::imageNode& image, uint32_t width, uint32_t height, VkFormat format, VkSampleCountFlagBits numSamples, VkImageUsageFlags usage, VkImageAspectFlags aspectFlags, const VkDevice& device, const VmaAllocator& memAllocator) {
            VkImageCreateInfo imageInfo = {};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.format = format;
            imageInfo.extent.width = width;
            imageInfo.extent.height = height;
            imageInfo.extent.depth = 1;
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.samples = numSamples;
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage = usage;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VmaAllocationCreateInfo allocInfo = {};
            allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
            allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

            if (vmaCreateImage(memAllocator, &imageInfo, &allocInfo, &image.image, &image.imageMemory, nullptr) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "图像创建失败");
            }

            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = image.image;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = format;
            viewInfo.subresourceRange.aspectMask = aspectFlags;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(device, &viewInfo, nullptr, &image.view) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "图像视图创建失败");
            }
        }

        //创建交换链图像结点
        void createImageNode(std::vector<bufferStruct::imageNode>& swapImageList, VkFormat format, VkImageAspectFlags aspectFlags, const VkDevice& device) {
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = format;
            viewInfo.subresourceRange.aspectMask = aspectFlags;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            for (auto& imageNode : swapImageList) {
                viewInfo.image = imageNode.image;

                if (vkCreateImageView(device, &viewInfo, nullptr, &imageNode.view) != VK_SUCCESS) {
                    Log::printError(__FILE__, __LINE__, "图像视图创建失败");
                }
            }
        }

    public:
        void clearSwapChain(const VkDevice& device, const VmaAllocator& memAllocator) {
            std::visit([&device, &memAllocator](auto& node) {node.clear(device, memAllocator); }, frameNode);
        }

        void setScFramebuffer(int i) {
            renderInfo.framebuffer = scPtr->framebuffer[i];
        }
    };
    */
}

//buffer结构空间
namespace bufferStruct {
    //命令结点
    struct commandNode {
        VkCommandPool                commandPool;                                           //命令池
        std::vector<VkCommandBuffer> commandBuffer[bufferInfo::commandThreadCount];         //使用双重命令缓冲防止更新区块卡顿



        //命令创建
        void cmdCreate(const VkDevice& device, const uint32_t queueFamilyIndex, const uint32_t frameSize) {
            //创建commandPool
            VkCommandPoolCreateInfo poolInfo{};
            poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            poolInfo.queueFamilyIndex = queueFamilyIndex;
            if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "命令池创建失败");
            }

            //配置commandBuffer
            for (int i = 0; i < bufferInfo::commandThreadCount; i++) {
                commandBuffer[i].resize(frameSize);
            }

            //配置allocInfo
            allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandPool        = commandPool;
            allocInfo.commandBufferCount = frameSize;

            //配置beginInfo
            beginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags            = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
            beginInfo.pInheritanceInfo = nullptr;

            viewport.x        = 0.0f;
            viewport.y        = 0.0f;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;

            scissor.offset = { 0, 0 };

            memBarrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
            memBarrier.pNext         = nullptr;
            memBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;    //等待颜色写入完成
            memBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;               //让着色器能读取

            loadExtFunc(device);
        }

        //命令池释放
        void cmdPoolFree(const VkDevice& device) {
            vkDestroyCommandPool(device, commandPool, nullptr);
        }

        //命令释放
        void cmdFree(const VkDevice& device, const int threadID) {
            vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffer[threadID].size()), commandBuffer[threadID].data());
        }



        //命令分配
        void cmdAllocate(const VkDevice& device, const int threadId) {
            if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffer[threadId].data()) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "命令分配失败");
            }
        }

        //命令重置
        void cmdReset(const int threadID, const int frameID) {
            if (vkResetCommandBuffer(commandBuffer[threadID][frameID], 0) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "命令重置失败");
            }
        }

        //命令开始
        void cmdBegin(const int threadID, const int frameID) {
            if (vkBeginCommandBuffer(commandBuffer[threadID][frameID], &beginInfo) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "命令开始失败");
            }
        }

        //命令结束
        void cmdEnd(const int threadID, const int frameID) {
            vertOffset = std::vector<VkDeviceSize>{ 0 };

            if (vkEndCommandBuffer(commandBuffer[threadID][frameID]) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "命令结束失败");
            }
        }



        //命令录制，通用
        void cmdRecord(int threadID, int frameID, const VkPipeline& pipeline, const VkPipelineLayout& pipelineLayout, const VkDescriptorSet& descriptorSet, const VkBuffer& vertexBuffer, const VkBuffer& indexBuffer, uint32_t indexSize) {
            vkCmdBindPipeline(      commandBuffer[threadID][frameID], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
            vkCmdBindDescriptorSets(commandBuffer[threadID][frameID], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
            vkCmdBindVertexBuffers( commandBuffer[threadID][frameID], 0, 1, &vertexBuffer, vertOffset.data());
            vkCmdBindIndexBuffer(   commandBuffer[threadID][frameID], indexBuffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(       commandBuffer[threadID][frameID], indexSize, 1, 0, 0, 0);
        }

        //命令录制，全屏三角形
        void cmdRecord(int threadID, int frameID, const VkPipeline& pipeline, const VkPipelineLayout& pipelineLayout, const VkDescriptorSet& descriptorSet, const VkExtent2D& scExtent) {
            viewport.width  = (float)scExtent.width;
            viewport.height = (float)scExtent.height;

            scissor.extent = scExtent;

            vkCmdBindPipeline(      commandBuffer[threadID][frameID], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
            vkCmdSetViewport(       commandBuffer[threadID][frameID], 0, 1, &viewport);
            vkCmdSetScissor(        commandBuffer[threadID][frameID], 0, 1, &scissor);
            vkCmdBindDescriptorSets(commandBuffer[threadID][frameID], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
            vkCmdDraw(              commandBuffer[threadID][frameID], 3, 1, 0, 0);
        }

        //命令录制，ui
        void cmdRecord(int threadID, int frameID, const VkPipeline& pipeline, const VkPipelineLayout& pipelineLayout, const VkDescriptorSet& descriptorSet, const VkBuffer& vertexBuffer, uint32_t numLetters) {
            vkCmdBindPipeline(      commandBuffer[threadID][frameID], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
            vkCmdBindDescriptorSets(commandBuffer[threadID][frameID], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);
            vkCmdBindVertexBuffers( commandBuffer[threadID][frameID], 0, 1, &vertexBuffer, vertOffset.data());
            vkCmdBindVertexBuffers( commandBuffer[threadID][frameID], 1, 1, &vertexBuffer, vertOffset.data());

            for (uint32_t j = 0; j < numLetters; j++) {
                vkCmdDraw(commandBuffer[threadID][frameID], 4, 1, j * 4, 0);		//不用循环文本底部有连线
            }
        }



        //命令队列提交
        void cmdQueueSubmit(VkDevice& device, VkQueue& graphicsQueue, VkCommandBuffer& tempCommandBuffer) {
            VkSubmitInfo submitInfo{};
            submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers    = &tempCommandBuffer;

            std::lock_guard<std::mutex> lock(lockInfo::queueThreadLock);
            vkEndCommandBuffer(tempCommandBuffer);

            vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
            vkQueueWaitIdle(graphicsQueue);

            vkFreeCommandBuffers(device, commandPool, 1, &tempCommandBuffer);
        }

        //命令管线屏障
        void cmdBarrier(int threadID, int frameID) {
            vkCmdPipelineBarrier(commandBuffer[threadID][frameID], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 1, &memBarrier, 0, nullptr, 0, nullptr);
        }



        int currentThreadId = 0;						                                    //当前帧命令缓冲区线程id
        int nextThreadId = currentThreadId;						                            //下一帧命令缓冲区线程id
        std::unique_ptr<std::mutex> nextThreadIdDataLock = std::make_unique<std::mutex>();  //nextThreadId数据锁

        //判断命令缓冲区线程是否需要更新
        bool isUpdate() {
            return nextThreadId != currentThreadId;
        }

        //设置当前帧命令缓冲区线程id
        void setCurrentThreadId() {
            nextThreadIdDataLock->lock();
            currentThreadId = nextThreadId;
            nextThreadIdDataLock->unlock();
        }

        //设置下一帧命令缓冲区线程id
        void setNextThreadId() {
            nextThreadIdDataLock->lock();
            nextThreadId = (currentThreadId + 1) % 2;
            nextThreadIdDataLock->unlock();
        }

    private:
        VkCommandBufferAllocateInfo allocInfo{};                    //命令的创建信息
        VkCommandBufferBeginInfo beginInfo{};                       //命令的录制信息

        VkViewport viewport{};
        VkRect2D scissor{};

        VkMemoryBarrier memBarrier{}; 

        std::vector<VkDeviceSize> vertOffset{ 0 };                  //顶点偏移量，绑定管线过程中指定各顶点缓冲区的偏移量

        PFN_vkCmdDrawMeshTasksEXT vkCmdDrawMeshTasksEXT = nullptr;  //网格着色器绘制函数



        //加载扩展函数
        void loadExtFunc(const VkDevice& device) {
            //网格着色器绘制函数
            vkCmdDrawMeshTasksEXT = (PFN_vkCmdDrawMeshTasksEXT)vkGetDeviceProcAddr(device, "vkCmdDrawMeshTasksEXT");
            if (vkCmdDrawMeshTasksEXT == nullptr) {
                Log::printError(__FILE__, __LINE__, "vkCmdDrawMeshTasksEXT获取失败");
            }
        }
    };

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
        //网格缓冲
        bufferNode vertex[bufferInfo::commandThreadCount][plateEnum::terrainLayer::count];
        bufferNode index[bufferInfo::commandThreadCount][plateEnum::terrainLayer::count];

        //纹理缓冲
        const char* textureFile;
        imageNode texture;

        //纹理参数
        uint32_t arrayLayer;
        uint32_t mipLevel = 1;

        //统一缓冲
        bufferNode uniform;
        mvpUBO ubo;

        //描述缓冲
        VkDescriptorSet descriptorSet;

        //程序化索引
        bool isEnable;
        pipelineEnum::procPipelineId::type pipeId;  //管线索引
        bufferEnum::procSamplerId::type samplerId;  //采样索引



        terrainBufferNode(bool enable, pipelineEnum::procPipelineId::type pId, bufferEnum::procSamplerId::type sId) {
            textureFile = fileInfo::image::cube;
            arrayLayer = textureInfo::terrain::arrayLayer;

            isEnable  = enable;
            pipeId    = pId;
            samplerId = sId;
        }

        terrainBufferNode() = delete;

        //清理结构体，descriptorSet由描述符池管理，无需手动清理
        void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
            for (int thread = 0; thread < bufferInfo::commandThreadCount; thread++) {
                for (int layer = 0; layer < plateEnum::terrainLayer::count; layer++) {
                    vmaDestroyBuffer(memAllocator, vertex[thread][layer].buffer, vertex[thread][layer].bufferMemory);
                    vmaDestroyBuffer(memAllocator, index[thread][layer].buffer, index[thread][layer].bufferMemory);
                }
            }

            vmaDestroyImage(memAllocator, texture.image, texture.imageMemory);
            vkDestroyImageView(device, texture.view, nullptr);

            vmaDestroyBuffer(memAllocator, uniform.buffer, uniform.bufferMemory);
        }
    };

    struct staticModelNode {
        //模型文件
        std::string modelFile;

        //网格缓冲
        bufferNode vertex;
        bufferNode index;

        //纹理缓冲
        const char* textureFile;
        imageNode texture;
        //const char* normalFile;
        //imageNode normal;

        //纹理参数
        uint32_t mipLevel;                  //在创建texture时赋值

        //统一缓冲
        bufferNode uniform;
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
            vmaDestroyBuffer(memAllocator, vertex.buffer, vertex.bufferMemory);
            vmaDestroyBuffer(memAllocator, index.buffer, index.bufferMemory);

            vmaDestroyImage(memAllocator, texture.image, texture.imageMemory);
            vkDestroyImageView(device, texture.view, nullptr);

            //vmaDestroyImage(memAllocator, normal.image, normal.imageMemory);
            //vkDestroyImageView(device, normal.view, nullptr);

            vmaDestroyBuffer(memAllocator, uniform.buffer, uniform.bufferMemory);
        }
    };

    struct dynamicModelSubNode {
        //网格缓冲
        bufferNode vertex;
        bufferNode index;

        //纹理缓冲
        const char* textureFile;
        imageNode texture;
        //const char* normalFile;
        //imageNode normal;

        //统一缓冲
        bufferNode uniform;
        mvpUBO ubo;

        //描述缓冲
        VkDescriptorSet descriptorSet;



        void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
            vmaDestroyBuffer(memAllocator, vertex.buffer, vertex.bufferMemory);
            vmaDestroyBuffer(memAllocator, index.buffer, index.bufferMemory);

            vmaDestroyImage(memAllocator, texture.image, texture.imageMemory);
            vkDestroyImageView(device, texture.view, nullptr);

            //vmaDestroyImage(memAllocator, normal.image, normal.imageMemory);
            //vkDestroyImageView(device, normal.view, nullptr);

            vmaDestroyBuffer(memAllocator, uniform.buffer, uniform.bufferMemory);
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
        bufferNode vertex;
        bufferNode index;

        //统一缓冲
        bufferNode uniform;
        mvpUBO ubo;
        bufferNode lightUniform;
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
            vmaDestroyBuffer(memAllocator, vertex.buffer, vertex.bufferMemory);
            vmaDestroyBuffer(memAllocator, index.buffer, index.bufferMemory);

            vmaDestroyBuffer(memAllocator, uniform.buffer, uniform.bufferMemory);
            vmaDestroyBuffer(memAllocator, lightUniform.buffer, lightUniform.bufferMemory);
        }
    };

    struct mineBufferNode {
        //网格缓冲
        bufferNode vertex;
        bufferNode index;

        //统一缓冲
        bufferNode uniform;
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
            vmaDestroyBuffer(memAllocator, vertex.buffer, vertex.bufferMemory);
            vmaDestroyBuffer(memAllocator, index.buffer, index.bufferMemory);

            vmaDestroyBuffer(memAllocator, uniform.buffer, uniform.bufferMemory);
        }
    };

    struct uiTextBufferNode {
        //网格缓冲
        bufferNode vertex;

        //纹理缓冲
        imageNode texture;
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
            vmaMapMemory(memAllocator, vertex.bufferMemory, (void**)&mapped);
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
            vmaUnmapMemory(memAllocator, vertex.bufferMemory);
            mapped = nullptr;
        }

        void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
            vmaDestroyBuffer(memAllocator, vertex.buffer, vertex.bufferMemory);

            vmaDestroyImage(memAllocator, texture.image, texture.imageMemory);
            vkDestroyImageView(device, texture.view, nullptr);
        }
    };

    struct osMirrorBufferNode {
        //网格缓冲
        bufferNode vertex;
        bufferNode index;

        //统一缓冲
        bufferNode uniform;
        mvpUBO ubo;
        bufferNode lightUniform;
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
            vmaDestroyBuffer(memAllocator, vertex.buffer, vertex.bufferMemory);
            vmaDestroyBuffer(memAllocator, index.buffer, index.bufferMemory);

            vmaDestroyBuffer(memAllocator, uniform.buffer, uniform.bufferMemory);
            vmaDestroyBuffer(memAllocator, lightUniform.buffer, lightUniform.bufferMemory);
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
        bufferNode lightUniform;
        deferredLightUBO lightUbo;
        bufferNode shadowUniform;
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
            vmaDestroyBuffer(memAllocator, lightUniform.buffer,  lightUniform.bufferMemory);
            vmaDestroyBuffer(memAllocator, shadowUniform.buffer, shadowUniform.bufferMemory);
        }
    };

    struct shadowTerrainBufferNode {
        //网格缓冲，索引直接用源网格的
        bufferNode vertex[bufferInfo::commandThreadCount][plateEnum::terrainLayer::count];

        //统一缓冲
        bufferNode shadowMvpUniform;
        shadowMvpUBO shadowMvpUbo;

        //描述缓冲
        VkDescriptorSet descriptorSet;



        void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
            for (int thread = 0; thread < bufferInfo::commandThreadCount; thread++) {
                for (int layer = 0; layer < plateEnum::terrainLayer::count; layer++) {
                    vmaDestroyBuffer(memAllocator, vertex[thread][layer].buffer, vertex[thread][layer].bufferMemory);
                }
            }

            vmaDestroyBuffer(memAllocator, shadowMvpUniform.buffer, shadowMvpUniform.bufferMemory);
        }
    };

    struct shadowBufferNode {
        //网格缓冲，索引直接用源网格的
        bufferNode vertex;

        //统一缓冲
        bufferNode shadowMvpUniform;
        shadowMvpUBO shadowMvpUbo;

        //描述缓冲
        VkDescriptorSet descriptorSet;



        void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
            vmaDestroyBuffer(memAllocator, vertex.buffer, vertex.bufferMemory);

            vmaDestroyBuffer(memAllocator, shadowMvpUniform.buffer, shadowMvpUniform.bufferMemory);
        }
    };



    //todo 都改成带Image的
    struct scFramebufferNode {
        bufferStruct::imageNode color;                          //颜色图像
        bufferStruct::imageNode depth;                          //深度图像
        std::vector<bufferStruct::imageNode> swapImageList;     //交换链图像

        std::vector<VkFramebuffer> framebuffer;

        

        void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
            vkDestroyImageView(device, color.view, nullptr);
            vmaDestroyImage(memAllocator, color.image, color.imageMemory);

            vkDestroyImageView(device, depth.view, nullptr);
            vmaDestroyImage(memAllocator, depth.image, depth.imageMemory);

            //通过vkGetSwapchainImagesKHR获取的VkImage，会随着swapchain的销毁自动释放
            for (int i = 0; i < swapImageList.size(); i++) {
                vkDestroyImageView(device, swapImageList[i].view, nullptr);
            }
            swapImageList.clear();

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



        void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
            vkDestroyImageView(device, color.view, nullptr);
            vmaDestroyImage(memAllocator, color.image, color.imageMemory);

            vkDestroyImageView(device, depth.view, nullptr);
            vmaDestroyImage(memAllocator, depth.image, depth.imageMemory);

            vkDestroyImageView(device, osView.view, nullptr);
            vmaDestroyImage(memAllocator, osView.image, osView.imageMemory);

            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
    };

    struct dsFramebufferNode {
        bufferStruct::imageNode position;  //位置附件（世界空间坐标）
        bufferStruct::imageNode normal;    //法线附件（世界空间法线）
        bufferStruct::imageNode albedo;    //反照率附件（颜色+高光系数）
        bufferStruct::imageNode depth;     //深度附件

        VkFramebuffer framebuffer;



        void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
            vkDestroyImageView(device, position.view, nullptr);
            vmaDestroyImage(memAllocator, position.image, position.imageMemory);

            vkDestroyImageView(device, normal.view, nullptr);
            vmaDestroyImage(memAllocator, normal.image, normal.imageMemory);

            vkDestroyImageView(device, albedo.view, nullptr);
            vmaDestroyImage(memAllocator, albedo.image, albedo.imageMemory);

            vkDestroyImageView(device, depth.view, nullptr);
            vmaDestroyImage(memAllocator, depth.image, depth.imageMemory);

            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
    };

    struct shFramebufferNode {
        bufferStruct::imageNode depth;      //深度附件

        VkFramebuffer framebuffer;



        void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
            vkDestroyImageView(device, depth.view, nullptr);
            vmaDestroyImage(memAllocator, depth.image, depth.imageMemory);

            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
    };

    struct renderInfoNode {
        std::array<VkClearValue, 2> clearValueForward;      //前向清理（解析附件不需要清除值）
        std::array<VkClearValue, 4> clearValueDeferred;     //延迟清理
        std::array<VkClearValue, 1> clearValueShadow;       //阴影清理

        renderEnum::renderTemplateId::type renderTemplateType;
        std::variant<scFramebufferNode, osFramebufferNode, dsFramebufferNode, shFramebufferNode> frameNode;
        scFramebufferNode* scPtr = nullptr;
        osFramebufferNode* osPtr = nullptr;
        dsFramebufferNode* dsPtr = nullptr;
        shFramebufferNode* shPtr = nullptr;

        VkRenderPassBeginInfo renderInfo;



        void createRenderInfoNode(const VkClearColorValue& color, const VkClearDepthStencilValue& depthStencil, renderEnum::renderTemplateId::type tRenderTemplateType) {
            clearValueForward[0].color        = color;
            clearValueForward[1].depthStencil = depthStencil;

            clearValueDeferred[0].color        = color;
            clearValueDeferred[1].color        = color;
            clearValueDeferred[2].color        = color;
            clearValueDeferred[3].depthStencil = depthStencil;

            clearValueShadow[0].depthStencil = depthStencil;

            renderInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderInfo.renderArea.offset = { 0, 0 };

            renderTemplateType = tRenderTemplateType;

            if (tRenderTemplateType == renderEnum::renderTemplateId::swapChain_rt) {
                frameNode = scFramebufferNode{};
                scPtr     = &std::get<bufferStruct::scFramebufferNode>(frameNode);

                renderInfo.clearValueCount = static_cast<uint32_t>(clearValueForward.size());
                renderInfo.pClearValues    = clearValueForward.data();
            }
            else if(tRenderTemplateType == renderEnum::renderTemplateId::offScreen_rt){
                frameNode = osFramebufferNode{};
                osPtr     = &std::get<bufferStruct::osFramebufferNode>(frameNode);

                renderInfo.clearValueCount = static_cast<uint32_t>(clearValueForward.size());
                renderInfo.pClearValues    = clearValueForward.data();
            }
            else if (tRenderTemplateType == renderEnum::renderTemplateId::deferred_rt) {
                frameNode = dsFramebufferNode{};
                dsPtr     = &std::get<bufferStruct::dsFramebufferNode>(frameNode);

                renderInfo.clearValueCount = static_cast<uint32_t>(clearValueDeferred.size());
                renderInfo.pClearValues    = clearValueDeferred.data();
            }
            else if (tRenderTemplateType == renderEnum::renderTemplateId::shadowMap_rt) {
                frameNode = shFramebufferNode{};
                shPtr     = &std::get<bufferStruct::shFramebufferNode>(frameNode);
                
                renderInfo.clearValueCount = static_cast<uint32_t>(clearValueShadow.size());
                renderInfo.pClearValues    = clearValueShadow.data();
            }
            else {
                Log::printError(__FILE__, __LINE__, "该帧缓冲类型不存在: \t" + std::to_string(tRenderTemplateType));
            }
        }

        void createRenderInfo(const VkExtent2D& scExtent, const VkFormat& colorFormat, const VkFormat& depthFormat, const VkSampleCountFlagBits& msaa, const VkRenderPass& render, const VkDevice& device, const VmaAllocator& memAllocator) {
            if (renderTemplateType == renderEnum::renderTemplateId::swapChain_rt) {
                createImageNode(scPtr->color, scExtent.width, scExtent.height, colorFormat, msaa, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_ASPECT_COLOR_BIT, device, memAllocator);
                createImageNode(scPtr->depth, scExtent.width, scExtent.height, depthFormat, msaa, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,                                   VK_IMAGE_ASPECT_DEPTH_BIT, device, memAllocator);
                createImageNode(scPtr->swapImageList, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, device);

                createScFramebuffer(scExtent, render, device);
            }
            else if (renderTemplateType == renderEnum::renderTemplateId::offScreen_rt) {
                createImageNode(osPtr->color,   scExtent.width, scExtent.height, colorFormat, msaa, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, device, memAllocator);
                createImageNode(osPtr->depth,   scExtent.width, scExtent.height, depthFormat, msaa, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,                      VK_IMAGE_ASPECT_DEPTH_BIT, device, memAllocator);
                createImageNode(osPtr->osView,  scExtent.width, scExtent.height, colorFormat, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, device, memAllocator);

                createOsFramebuffer(scExtent, render, device);
            }
            else if (renderTemplateType == renderEnum::renderTemplateId::deferred_rt) {
                createImageNode(dsPtr->position, scExtent.width, scExtent.height, renderInfo::dsPosColorFormat, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, device, memAllocator);
                createImageNode(dsPtr->normal,   scExtent.width, scExtent.height, colorFormat,             VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, device, memAllocator);
                createImageNode(dsPtr->albedo,   scExtent.width, scExtent.height, colorFormat,             VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, device, memAllocator);
                createImageNode(dsPtr->depth,    scExtent.width, scExtent.height, depthFormat,             VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, device, memAllocator);

                createDsFramebuffer(scExtent, render, device);
            }
            else if (renderTemplateType == renderEnum::renderTemplateId::shadowMap_rt) {
                createImageNode(shPtr->depth, scExtent.width, scExtent.height, depthFormat, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, device, memAllocator);

                createShFramebuffer(scExtent, render, device);
            }
            else {
                Log::printError(__FILE__, __LINE__, "该帧缓冲类型不存在: \t" + std::to_string(renderTemplateType));
            }

            renderInfo.renderPass = render;
            renderInfo.renderArea.extent = scExtent;
        }

        void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
            std::visit([&device, &memAllocator](auto& node) {node.clear(device, memAllocator); }, frameNode);
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
            tempFrameNode.framebuffer.resize(tempFrameNode.swapImageList.size());

	    	for (size_t i = 0; i < tempFrameNode.swapImageList.size(); i++) {
	    		std::array<VkImageView, 3> attachments{ tempFrameNode.color.view,tempFrameNode.depth.view,tempFrameNode.swapImageList[i].view };
	    		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	    		framebufferInfo.pAttachments	= attachments.data();

	    		if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &tempFrameNode.framebuffer[i]) != VK_SUCCESS) {
	    			Log::printError(__FILE__, __LINE__, "交换链帧缓冲创建失败");
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
		    }
            
            renderInfo.framebuffer = tempFrameNode.framebuffer;
        }

        //创建延迟渲染帧缓冲
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
                Log::printError(__FILE__, __LINE__, "延迟渲染帧缓冲创建失败");
            }

            renderInfo.framebuffer = tempFrameNode.framebuffer;
        }

        //创建阴影映射帧缓冲
        void createShFramebuffer(const VkExtent2D& scExtent, const VkRenderPass& render, const VkDevice& device) {
            bufferStruct::shFramebufferNode& tempFrameNode = std::get<bufferStruct::shFramebufferNode>(frameNode);

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass      = render;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments    = &tempFrameNode.depth.view;
            framebufferInfo.width           = scExtent.width;
            framebufferInfo.height          = scExtent.height;
            framebufferInfo.layers          = 1;

            if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &tempFrameNode.framebuffer) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "阴影映射帧缓冲创建失败");
            }

            renderInfo.framebuffer = tempFrameNode.framebuffer;
        }



        //创建图像结点
        void createImageNode(bufferStruct::imageNode& image, uint32_t width, uint32_t height, VkFormat format, VkSampleCountFlagBits numSamples, VkImageUsageFlags usage, VkImageAspectFlags aspectFlags, const VkDevice& device, const VmaAllocator& memAllocator) {
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
            allocInfo.usage         = VMA_MEMORY_USAGE_GPU_ONLY;
            allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

            if (vmaCreateImage(memAllocator, &imageInfo, &allocInfo, &image.image, &image.imageMemory, nullptr) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "图像创建失败");
            }

            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image    = image.image;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format   = format;
            viewInfo.subresourceRange.aspectMask     = aspectFlags;
            viewInfo.subresourceRange.baseMipLevel   = 0;
            viewInfo.subresourceRange.levelCount     = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount     = 1;

            if (vkCreateImageView(device, &viewInfo, nullptr, &image.view) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "图像视图创建失败");
            }
        }

        //创建交换链图像结点
        void createImageNode(std::vector<bufferStruct::imageNode>& swapImageList, VkFormat format, VkImageAspectFlags aspectFlags, const VkDevice& device) {
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format   = format;
            viewInfo.subresourceRange.aspectMask     = aspectFlags;
            viewInfo.subresourceRange.baseMipLevel   = 0;
            viewInfo.subresourceRange.levelCount     = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount     = 1;

            for (auto& imageNode: swapImageList) {
                viewInfo.image = imageNode.image;

                if (vkCreateImageView(device, &viewInfo, nullptr, &imageNode.view) != VK_SUCCESS) {
                    Log::printError(__FILE__, __LINE__, "图像视图创建失败");
                }
            }
        }

    public:
        void clearSwapChain(const VkDevice& device, const VmaAllocator& memAllocator) {
            std::visit([&device, &memAllocator](auto& node) {node.clear(device, memAllocator); }, frameNode);
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

//pipeline结构空间
namespace pipelineStruct {
    //着色器结点
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

    //管线配置结点
    struct pipeConfigNode {
        std::vector<VkPipelineShaderStageCreateInfo>     shaderStageList{};         //着色阶段列表（顶点模式、网格模式）
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
    
        void setPipeDynamicStage(const std::vector<VkDynamicState>& list) {
            pipeDynamicStageList                    = list;
            pipeDynamicStageState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            pipeDynamicStageState.dynamicStateCount = (uint32_t)pipeDynamicStageList.size();
            pipeDynamicStageState.pDynamicStates    = pipeDynamicStageList.data();

            pipeInfo.pDynamicState = &pipeDynamicStageState;
        }
    };



    //管线布局结点
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

    //管线结点
    struct pipelineNode {
        std::vector<pipelineStruct::shaderNode> shaderList{};   //着色器列表
        pipelineStruct::pipeConfigNode          pipeConfig{};   //管线配置
        VkPipeline                              pipeline{};     //管线



        //创建顶点管线结点
        void createPipelineNode(const VkDevice& device, const VkSampleCountFlagBits& msaa, const renderEnum::renderAttId::type& renderAttType, const renderStruct::renderTemplateNode& renderTemplateNode, const pipelineStruct::pipeLayoutNode& pipeLayoutNode, const std::vector<std::string>& shaderFileList) {
            //创建vertShaderList
            shaderList.resize(shaderFileList.size());
            if (shaderFileList.size() == pipelineEnum::shaderNodeId::vert::count) {
                shaderList[pipelineEnum::shaderNodeId::vert::vert].createShaderNode(device, shaderFileList[0], VK_SHADER_STAGE_VERTEX_BIT);
                shaderList[pipelineEnum::shaderNodeId::vert::frag].createShaderNode(device, shaderFileList[1], VK_SHADER_STAGE_FRAGMENT_BIT);
            }
            else if (shaderFileList.size() == pipelineEnum::shaderNodeId::mesh::count) {
                //todo 待写网格着色
            }

            //创建pipeConfig
            pipeConfig.createPipeConfigNode(shaderList, msaa, renderAttType, renderTemplateNode.render, pipeLayoutNode.pipeLayout);
        }



        void clearSwapChain(const VkDevice& device) {
            vkDestroyPipeline(device, pipeline, nullptr);
        }

        void clear(const VkDevice& device) {
            vkDestroyPipeline(device, pipeline, nullptr);

            for (auto shaderNode : shaderList) {
                shaderNode.clear(device);
            }
        }
    };
}