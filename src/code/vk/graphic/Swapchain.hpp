#pragma once



class Swapchain {
public:
    std::vector<VkImageView> swapChainImageViews;       //为帧缓冲服务



    Swapchain(Device& Device) :Device{ Device } {
        createSwapChain();
        createImageViews();
        createSyncObjects();

        findDepthFormat();

        if (debugInfo::LogSwitch) {
            Log::printTitle("————————Swapchain已创建");

            Log::delimiter();
        }
	}

	~Swapchain() {
        for (size_t i = 0; i < swapchainInfo::MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(Device.device, renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(Device.device, imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(Device.device, inFlightFences[i], nullptr);
        }

        cleanupSwapChain();

        vkDestroySwapchainKHR(Device.device, swapchain, nullptr);

        if (debugInfo::LogSwitch) {
            Log::print("Swapchain已清除");
        }
	}
    
private:
    Device& Device;

    VkSwapchainCreateInfoKHR swapchainCreateInfo{};
    swapStruct::swapchainSupportNode swapchainSupport;
    VkSwapchainKHR swapchain;

    std::vector<VkImage> swapChainImages;
    VkImageViewCreateInfo imageViewCreateInfo{};        //swapChainImageViews创建信息

    std::vector<VkSemaphore> imageAvailableSemaphores;  //开始渲染
    std::vector<VkSemaphore> renderFinishedSemaphores;  //开始呈现
    std::vector<VkFence> inFlightFences; 
    size_t currentFrame = 0;



    //创建交换链
    void createSwapChain() {
        swapchainSupport = Device.getSwapChainSupport();

        VkSurfaceFormatKHR format = chooseFormat(swapchainSupport.formats);
        VkPresentModeKHR mode = chooseMode(swapchainSupport.presentModes);
        VkExtent2D extent = chooseExtent(swapchainSupport.capabilities);
        
        swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainCreateInfo.surface = Device.surface;

        //如果最大数量与最小数量不等，那么使用最小数量 + 1,相等则直接使用最小数量
        uint32_t imageCount = swapchainSupport.capabilities.minImageCount + (swapchainSupport.capabilities.maxImageCount > swapchainSupport.capabilities.minImageCount);

        //Log::print(imageCount);

        swapchainCreateInfo.minImageCount = imageCount;
        swapchainCreateInfo.imageFormat = format.format;
        swapchainCreateInfo.imageColorSpace = format.colorSpace;
        swapchainCreateInfo.imageExtent = extent;
        swapchainCreateInfo.imageArrayLayers = 1;
        swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        if (Device.availableDevices.queueIndices[0].support != Device.availableDevices.queueIndices[0].present) {
            uint32_t index[] = { (uint32_t)Device.availableDevices.queueIndices[0].support, (uint32_t)Device.availableDevices.queueIndices[0].present };

            swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            swapchainCreateInfo.queueFamilyIndexCount = 2;
            swapchainCreateInfo.pQueueFamilyIndices = index;
        }
        else {
            swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            swapchainCreateInfo.queueFamilyIndexCount = 0;
            swapchainCreateInfo.pQueueFamilyIndices = nullptr;
        }

        swapchainCreateInfo.preTransform = swapchainSupport.capabilities.currentTransform;

        swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        swapchainCreateInfo.presentMode = mode;
        swapchainCreateInfo.clipped = VK_TRUE;

        swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(Device.device, &swapchainCreateInfo, nullptr, &swapchain) != VK_SUCCESS) {
            Log::error(__FILE__, __LINE__, "交换链创建失败");
        }
        


        Device.imageFormat = format.format;
        Device.swapChainExtent = extent;

        vkGetSwapchainImagesKHR(Device.device, swapchain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);

        vkGetSwapchainImagesKHR(Device.device, swapchain, &imageCount, swapChainImages.data());
        swapChainImageViews.resize(swapChainImages.size());
    }

    //创建图像窗口
    void createImageViews() {
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = Device.imageFormat;

        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;



        for (size_t i = 0; i < swapChainImages.size(); i++) {
            imageViewCreateInfo.image = swapChainImages[i];

            if (vkCreateImageView(Device.device, &imageViewCreateInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
                Log::error(__FILE__, __LINE__, "图像窗口创建失败");
            }
        }
    }

    //创建信号量
    void createSyncObjects() {
        imageAvailableSemaphores.resize(swapchainInfo::MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(swapchainInfo::MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(swapchainInfo::MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < swapchainInfo::MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(Device.device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS || vkCreateSemaphore(Device.device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS || vkCreateFence(Device.device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
                Log::error(__FILE__, __LINE__, "信号量创建失败");
                throw;
            }
        }
    }

    //获取深度格式
    void findDepthFormat() {
        Device.depthFormat = findSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }



    //选择合适的表面格式
    VkSurfaceFormatKHR chooseFormat(std::vector<VkSurfaceFormatKHR> availableFormats) {
        //只有一个未定义格式时,返回指定格式
        if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
            return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
        }

        //含有多个格式支持时,从中挑选返回
        for (VkSurfaceFormatKHR availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    //选择合适的呈现模式
    VkPresentModeKHR chooseMode(std::vector<VkPresentModeKHR> availablePresentModes) {
        for (auto availablePresentMode : availablePresentModes) {
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

    //选择交换范围
    VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR capabilities) {
        if (capabilities.currentExtent.width == std::numeric_limits<uint32_t>::max()) {
            VkExtent2D actualExtent = { windowInfo::width, windowInfo::height };

            actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
            actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

            return actualExtent;
        }
        else {
            return capabilities.currentExtent;
        }
    }

    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
        for (VkFormat format : candidates) {
            // 获取当前格式的属性信息
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(Device.physicalDevice, format, &props);

            // 检查当前格式是否支持所需的特性和平铺方式
            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                // 如果当前格式支持线性平铺并且满足特性要求，则返回该格式
                return format;
            }
            else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                // 如果当前格式支持最佳平铺并且满足特性要求，则返回该格式
                return format;
            }
        }

        Log::error(__FILE__, __LINE__, "深度格式获取失败");
        throw;
    }

public:
    bool drawFrame(std::vector<VkCommandBuffer>& commandBuffers) {
        //std::lock_guard<std::mutex> lock(threadCommInfo::queueMutex);
        vkWaitForFences(Device.device, 1, &inFlightFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
        vkResetFences(Device.device, 1, &inFlightFences[currentFrame]);

        //从交换链中获取图像
        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(Device.device, swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
        
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            //重建交换链
            return true;
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            Log::error(__FILE__, __LINE__, "获取交换链映像失败");
            throw;
        }

        //队列提交和同步
        VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
        VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

        if (vkQueueSubmit(Device.graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
            Log::error(__FILE__, __LINE__, "提交绘制命令缓冲区失败");
            throw;
        }

        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = { swapchain };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr; // Optional

        result = vkQueuePresentKHR(Device.presentQueue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            return true;
        }
        else if (result != VK_SUCCESS) {
            Log::error(__FILE__, __LINE__, "呈现交换链图像失败");
            throw;
        }

        currentFrame = (currentFrame + 1) % swapchainInfo::MAX_FRAMES_IN_FLIGHT;
        return false;
    }
    


    void cleanupSwapChain() {
        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            vkDestroyImageView(Device.device, swapChainImageViews[i], nullptr);
        }
    }

    void recreateSwapChain() {
        //重建交换链
        if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(Device.physicalDevice, Device.surface, &swapchainSupport.capabilities)) {
            Log::error(__FILE__, __LINE__, "重建交换链时表面功能获取失败");
            throw;
        }

        swapchainCreateInfo.imageExtent = swapchainSupport.capabilities.currentExtent;
        swapchainCreateInfo.oldSwapchain = swapchain;

        vkQueueWaitIdle(Device.graphicsQueue);

        VkSwapchainKHR tempSwapchain;

        if (vkCreateSwapchainKHR(Device.device, &swapchainCreateInfo, nullptr, &tempSwapchain) != VK_SUCCESS) {
            Log::error(__FILE__, __LINE__, "重建交换链创建失败");
            throw;
        }

        vkDestroySwapchainKHR(Device.device, swapchain, nullptr);
        swapchain = tempSwapchain;

        //重获交换链支持
        VkSurfaceFormatKHR format = chooseFormat(swapchainSupport.formats);
        VkExtent2D extent = chooseExtent(swapchainSupport.capabilities);

        Device.imageFormat = format.format;
        Device.swapChainExtent = extent;

        vkGetSwapchainImagesKHR(Device.device, swapchain, &swapchainCreateInfo.minImageCount, nullptr);
        swapChainImages.resize(swapchainCreateInfo.minImageCount);

        vkGetSwapchainImagesKHR(Device.device, swapchain, &swapchainCreateInfo.minImageCount, swapChainImages.data());
        swapChainImageViews.resize(swapChainImages.size());

        //重建图像视图
        imageViewCreateInfo.format = Device.imageFormat;
        //重建深度视图
        findDepthFormat();

        for (size_t i = 0; i < swapChainImages.size(); i++) {
            imageViewCreateInfo.image = swapChainImages[i];

            if (vkCreateImageView(Device.device, &imageViewCreateInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
                Log::error(__FILE__, __LINE__, "图像窗口创建失败");
            }
        }
    }
};