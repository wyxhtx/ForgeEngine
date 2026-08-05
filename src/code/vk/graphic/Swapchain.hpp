#pragma once



class Swapchain {
public:
    VkExtent2D scExtent;                    //绘制区域
    VkFormat colorFormat;                   //颜色格式
    VkFormat depthFormat;                   //深度格式
    VkSampleCountFlagBits msaaSamples;      //多重采样



    Swapchain(Device& Device) :Device{ Device } {
        createScSupport();
        createSwapChain();
        createSyncObjects();

        if (debugInfo::LogSwitch) {
            Log::printTitle("————————Swapchain已创建");

            Log::delimiter();
        }
	}

	~Swapchain() {
        vkDestroySwapchainKHR(Device.device, swapchain, nullptr);

        for (size_t i = 0; i < swapSupport.scCreateInfo.minImageCount; i++) {
            vkDestroySemaphore(Device.device, renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(Device.device, imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(Device.device, inFlightFences[i], nullptr);
        }

        if (debugInfo::LogSwitch) {
            Log::print("Swapchain已清除");
        }
	}

private:
    Device& Device;

    swapStruct::swapSupportNode swapSupport;            //交换链支持
    VkSwapchainKHR swapchain;                           //交换链

    std::vector<VkSemaphore> imageAvailableSemaphores;  //开始渲染
    std::vector<VkSemaphore> renderFinishedSemaphores;  //开始呈现
    std::vector<VkFence> inFlightFences; 
    size_t currentFrame = 0;



    //创建交换链支持结点
    void createScSupport() {
        swapSupport.createScSupport(Device.enableGpu->gpu, Device.presentSurface);
        swapSupport.createScCreateInfo(Device.presentSurface);

        scExtent = swapSupport.scCreateInfo.imageExtent;
        colorFormat = swapSupport.scCreateInfo.imageFormat;
        depthFormat = swapSupport.getDepthFormat(Device.enableGpu->gpu, { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
        msaaSamples = swapSupport.getMsaaSamples(Device.enableGpu->gpu);

        if (debugInfo::LogSwitch) {
            Log::print("最大采样次数:\t" + std::to_string(msaaSamples));
            Log::delimiter();
        }
    }

    //创建交换链
    void createSwapChain() {
        if (vkCreateSwapchainKHR(Device.device, &swapSupport.scCreateInfo, nullptr, &swapchain) != VK_SUCCESS) {
            Log::printError(__FILE__, __LINE__, "交换链创建失败");
        }
    }

    //创建信号量
    void createSyncObjects() {
        imageAvailableSemaphores.resize(swapSupport.scCreateInfo.minImageCount);
        renderFinishedSemaphores.resize(swapSupport.scCreateInfo.minImageCount);
        inFlightFences.resize(swapSupport.scCreateInfo.minImageCount);

        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < swapSupport.scCreateInfo.minImageCount; i++) {
            if (vkCreateSemaphore(Device.device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS || vkCreateSemaphore(Device.device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS || vkCreateFence(Device.device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "信号量创建失败");
            }
        }
    }

public:
    bool drawFrame(std::vector<VkCommandBuffer>& commandBuffer) {
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
            Log::printError(__FILE__, __LINE__, "获取交换链映像失败");
        }

        //队列提交和同步
        VkSemaphore waitSemaphores[]      = { imageAvailableSemaphores[currentFrame] };
        VkSemaphore signalSemaphores[]    = { renderFinishedSemaphores[currentFrame] };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

        VkSubmitInfo submitInfo{};
        submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount   = 1;
        submitInfo.pWaitSemaphores      = waitSemaphores;
        submitInfo.pWaitDstStageMask    = waitStages;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores    = signalSemaphores;
        submitInfo.commandBufferCount   = 1;
        submitInfo.pCommandBuffers      = &commandBuffer[imageIndex];

        lockInfo::queueThreadLock.lock();
        if (vkQueueSubmit(Device.graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
            Log::printError(__FILE__, __LINE__, "提交绘制命令缓冲区失败");
        }

        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores    = signalSemaphores;

        VkSwapchainKHR swapChains[] = { swapchain };
        presentInfo.swapchainCount  = 1;
        presentInfo.pSwapchains     = swapChains;
        presentInfo.pImageIndices   = &imageIndex;
        presentInfo.pResults        = nullptr;

        result = vkQueuePresentKHR(Device.graphicsQueue, &presentInfo);
        lockInfo::queueThreadLock.unlock();

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            return true;
        }
        else if (result != VK_SUCCESS) {
            Log::printError(__FILE__, __LINE__, "呈现交换链图像失败");
        }

        currentFrame = (currentFrame + 1) % swapSupport.scCreateInfo.minImageCount;
        return false;
    }

    //todo
    //获取交换链图像列表，供procRenderNode. 什么什么 创建交换链图像结点
    std::vector<VkImage> getSwapImageList() {
        uint32_t swapImageCount = 0;
        vkGetSwapchainImagesKHR(Device.device, swapchain, &swapImageCount, nullptr);

        std::vector<VkImage> swapImageList(swapImageCount);
        vkGetSwapchainImagesKHR(Device.device, swapchain, &swapImageCount, swapImageList.data());

        return swapImageList;
    }
    


    void recreateSwapChain() {
        swapSupport.recreateScCreateInfo(Device.enableGpu->gpu, Device.presentSurface, swapchain);
        scExtent = swapSupport.scCreateInfo.imageExtent;

        vkQueueWaitIdle(Device.graphicsQueue);

        VkSwapchainKHR tempSwapchain;
        if (vkCreateSwapchainKHR(Device.device, &swapSupport.scCreateInfo, nullptr, &tempSwapchain) != VK_SUCCESS) {
            Log::printError(__FILE__, __LINE__, "重建交换链创建失败");
        }

        vkDestroySwapchainKHR(Device.device, swapchain, nullptr);
        swapchain = tempSwapchain;
    }
};