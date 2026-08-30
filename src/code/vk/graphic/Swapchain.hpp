#pragma once



class Swapchain {
public:
    VkFormat colorFormat;           //颜色格式
    VkFormat depthFormat;           //深度格式
    VkExtent2D swapExtent;          //绘制区域
    VkSampleCountFlagBits msaa;     //多重采样



    Swapchain(Device& Device) :Device{ Device } {
        createSwapSupport();
        createSwapChain();

        createSyncObjects();
        createSwapImageList();

        if (debugInfo::LogSwitch) {
            Log::printTitle("————————Swapchain已创建");

            Log::delimiter();
        }
	}

	~Swapchain() {
        vkDestroySwapchainKHR(Device.device, swapchain, nullptr);

        for (size_t i = 0; i < swapSupport.swapCreateInfo.minImageCount; i++) {
            vkDestroySemaphore(Device.device, renderDone[i], nullptr);
            vkDestroySemaphore(Device.device, imageDone[i], nullptr);
        }

        if (debugInfo::LogSwitch) {
            Log::print("Swapchain已清除");
        }
	}

private:
    Device& Device;

    swapStruct::swapSupportNode swapSupport;        //交换链支持
    VkSwapchainKHR swapchain;                       //交换链

    std::vector<VkSemaphore> imageDone;             //图像完成，开始渲染
    std::vector<VkSemaphore> renderDone;            //渲染完成，开始呈现
    size_t currentFrame = 0;



    //创建交换链支持结点
    void createSwapSupport() {
        swapSupport.createSwapSupport(Device.enableGpu->gpu, Device.presentSurface);
        swapSupport.createSwapCreateInfo(Device.presentSurface);

        colorFormat = swapSupport.swapCreateInfo.imageFormat;
        depthFormat = swapSupport.getDepthFormat(Device.enableGpu->gpu, { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
        swapExtent  = swapSupport.swapCreateInfo.imageExtent;
        msaa        = swapSupport.getMsaa(Device.enableGpu->gpu);

        if (debugInfo::LogSwitch) {
            Log::print("最大采样次数:\t" + std::to_string(msaa));
            Log::delimiter();
        }
    }

    //创建交换链
    void createSwapChain() {
        if (vkCreateSwapchainKHR(Device.device, &swapSupport.swapCreateInfo, nullptr, &swapchain) != VK_SUCCESS) {
            Log::printError(__FILE__, __LINE__, "交换链创建失败");
        }
    }



    //创建信号量
    void createSyncObjects() {
        imageDone .resize(swapSupport.swapCreateInfo.minImageCount);
        renderDone.resize(swapSupport.swapCreateInfo.minImageCount);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        for (size_t i = 0; i < swapSupport.swapCreateInfo.minImageCount; i++) {
            if (vkCreateSemaphore(Device.device, &semaphoreInfo, nullptr, &imageDone[i]) != VK_SUCCESS ||
                vkCreateSemaphore(Device.device, &semaphoreInfo, nullptr, &renderDone[i]) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "信号量创建失败");
            }
        }
    }

    //创建交换链图像列表
    void createSwapImageList() {
        uint32_t swapImageCount = 0;
        vkGetSwapchainImagesKHR(Device.device, swapchain, &swapImageCount, nullptr);

        swapStruct::swapImageList.clear();
        swapStruct::swapImageList.resize(swapImageCount);
        vkGetSwapchainImagesKHR(Device.device, swapchain, &swapImageCount, swapStruct::swapImageList.data());
    }

public:
    bool drawFrame(std::vector<VkCommandBuffer>& cmdList, VkFence cmdFence) {
        vkWaitForFences(Device.device, 1, &cmdFence, VK_TRUE, std::numeric_limits<uint64_t>::max());
        vkResetFences  (Device.device, 1, &cmdFence);
    
        //从交换链中获取图像
        uint32_t imageIndex = 0;
        switch (vkAcquireNextImageKHR(Device.device, swapchain, UINT64_MAX, imageDone[currentFrame], VK_NULL_HANDLE, &imageIndex)) {
        case VK_SUCCESS:                //正常
        case VK_SUBOPTIMAL_KHR:         //正常
            break;
        case VK_ERROR_OUT_OF_DATE_KHR:  //重建
            return true;
        default:                        //报错
            Log::printError(__FILE__, __LINE__, "获取交换链映像失败");
            break;
        }
    


        //队列提交
        lockInfo::queueThreadLock.lock();
        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo submitInfo{};
        submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount   = 1;
        submitInfo.pWaitSemaphores      = &imageDone[currentFrame];
        submitInfo.pWaitDstStageMask    = &waitStage;
        submitInfo.commandBufferCount   = 1;
        submitInfo.pCommandBuffers      = &cmdList[imageIndex];
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores    = &renderDone[currentFrame];
    
        if (vkQueueSubmit(Device.graphicsQueue, 1, &submitInfo, cmdFence) != VK_SUCCESS) {
            Log::printError(__FILE__, __LINE__, "命令提交失败");
        }
    
        //队列呈现
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores    = &renderDone[currentFrame];
        presentInfo.swapchainCount     = 1;
        presentInfo.pSwapchains        = &swapchain;
        presentInfo.pImageIndices      = &imageIndex;
        presentInfo.pResults           = nullptr;
    
        switch (vkQueuePresentKHR(Device.graphicsQueue, &presentInfo)) {
        case VK_ERROR_OUT_OF_DATE_KHR:      //swapchain与Device.presentSurface不兼容
        case VK_SUBOPTIMAL_KHR:             //不是最优配置
            lockInfo::queueThreadLock.unlock();
            return true;
        case VK_SUCCESS:                    //正常
            break;
        default:                            //报错
            Log::printError(__FILE__, __LINE__, "图像呈现失败");
            break;
        }
        lockInfo::queueThreadLock.unlock();
    
    
    
        currentFrame = (currentFrame + 1) % swapSupport.swapCreateInfo.minImageCount;
        return false;
    }

    void recreateSwapChain() {
        swapSupport.recreateSwapCreateInfo(Device.enableGpu->gpu, Device.presentSurface, swapchain);
        swapExtent = swapSupport.swapCreateInfo.imageExtent;

        vkQueueWaitIdle(Device.graphicsQueue);

        VkSwapchainKHR tempSwapchain;
        if (vkCreateSwapchainKHR(Device.device, &swapSupport.swapCreateInfo, nullptr, &tempSwapchain) != VK_SUCCESS) {
            Log::printError(__FILE__, __LINE__, "重建交换链创建失败");
        }

        vkDestroySwapchainKHR(Device.device, swapchain, nullptr);
        swapchain = tempSwapchain;

        createSwapImageList();
    }
};