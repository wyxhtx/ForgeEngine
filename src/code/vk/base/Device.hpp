#pragma once



class Device {
public:
    VkSurfaceKHR presentSurface;                                //呈现表面，vulkan和操作系统窗口之间的桥梁，让GPU知道画到哪里去

    std::vector<deviceStruct::gpuNode> gpuCandidateList{};      //gpu候选列表
    int enableGpuIndex = -1;                                    //启用gpu索引
    VkQueue graphicsQueue;                                      //渲染队列

    VkDevice device;                                            //逻辑设备，vulkan和gpu之间的api接口
    VmaAllocator memAllocator;                                  //内存分配器



    Device(GLFWwindow* window) {
        //创建窗口表面
        if (glfwCreateWindowSurface(InitInstance.instance, window, nullptr, &presentSurface) != VK_SUCCESS) {
            Log::printError(__FILE__, __LINE__, "窗口表面创建失败");
        }

        getGpuIndex();
        createDevice(getGpu(), enableConfigInfo::enableDeviceExtensionList);

        createVMA();

        if (debugInfo::LogSwitch) {
            Log::printTitle("————————Device已创建");

            Log::delimiter();
        }
    }

    ~Device() {
        vmaDestroyAllocator(memAllocator);

        vkDestroyDevice(device, nullptr);
        vkDestroySurfaceKHR(InitInstance.instance, presentSurface, nullptr);

        if (debugInfo::LogSwitch) {
            Log::print("Device已清除");
        }
    }

private:
    Instance InitInstance{};



    //获取最优显卡
    void getGpuIndex() {
        //显卡数量
        uint32_t gpuCount = 0;
        vkEnumeratePhysicalDevices(InitInstance.instance, &gpuCount, nullptr);

        if (gpuCount == 0) {
            Log::printError(__FILE__, __LINE__, "无可用显卡");
        }

        //显卡列表
        std::vector<VkPhysicalDevice> gpuList(gpuCount);
        vkEnumeratePhysicalDevices(InitInstance.instance, &gpuCount, gpuList.data());

        //gpuCandidateList赋值
        gpuCandidateList.resize(gpuCount);
        for (uint32_t i = 0; i < gpuCount; i++) {
            gpuCandidateList[i].gpu = gpuList[i];
        }
        
        //显卡信息列表
        std::vector<VkPhysicalDeviceProperties> gpuInfoList(gpuCount);          //todo 到时候看看能不能封装进LogSwitch里面
        if (debugInfo::LogSwitch) {
            Log::printTitle("可用显卡");
            for (uint32_t i = 0; i < gpuCount; i++) {
                vkGetPhysicalDeviceProperties(gpuCandidateList[i].gpu, &gpuInfoList[i]);
                Log::print(gpuInfoList[i].deviceName);
            }
            Log::delimiter();
        }

        //队列族索引
        for (uint32_t i = 0; i < gpuCount; i++) {
            getQueueFamilyIndex(gpuCandidateList[i], gpuInfoList[i], presentSurface);
        }

        //获取最优显卡
        uint32_t maxGraphicsQueueCount = 0;     //最大渲染队列数量
        for (uint32_t i = 0; i < gpuCount; i++) {
            if (maxGraphicsQueueCount < gpuCandidateList[i].graphicsQueueFamily.count) {
                maxGraphicsQueueCount = gpuCandidateList[i].graphicsQueueFamily.count;
                enableGpuIndex = i;
            }
        }

        //显卡支持检测
        gpuSupportCheck(getGpu().gpu, enableConfigInfo::enableDeviceExtensionList);
        if (debugInfo::LogSwitch) {
            Log::printTitle("选择显卡");
            Log::print(gpuInfoList[enableGpuIndex].deviceName);
            Log::delimiter();
        }
    }

    //创建逻辑设备
    void createDevice(const deviceStruct::gpuNode& node, const std::vector<const char*>& enableDeviceExtensionList) {
        //队列创建信息
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueCount       = 1;
        queueCreateInfo.queueFamilyIndex = node.graphicsQueueFamily.index;
        queueCreateInfo.pQueuePriorities = &node.graphicsQueueFamily.priority;

        //获取gpu功能支持
        VkPhysicalDeviceFeatures2 gpuFeatures2{};
        gpuFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        gpuFeatures2.pNext = nullptr;
        vkGetPhysicalDeviceFeatures2(node.gpu, &gpuFeatures2);

        //创建device
        VkDeviceCreateInfo deviceCreateInfo{};
        deviceCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.queueCreateInfoCount    = 1;
        deviceCreateInfo.pQueueCreateInfos       = &queueCreateInfo;
        deviceCreateInfo.enabledExtensionCount   = (uint32_t)enableDeviceExtensionList.size();
        deviceCreateInfo.ppEnabledExtensionNames = enableDeviceExtensionList.data();
        deviceCreateInfo.pNext                   = &gpuFeatures2;
        if (vkCreateDevice(node.gpu, &deviceCreateInfo, nullptr, &device) != VK_SUCCESS) {
            Log::printError(__FILE__, __LINE__, "设备创建失败");
        }

        //创建graphicsQueue
        vkGetDeviceQueue(device, node.graphicsQueueFamily.index, 0, &graphicsQueue);
    }

    //创建内存管理
    void createVMA() {
        VmaAllocatorCreateInfo allocatorInfo{};
        allocatorInfo.physicalDevice = getGpu().gpu;
        allocatorInfo.device         = device;
        allocatorInfo.instance       = InitInstance.instance;
        vmaCreateAllocator(&allocatorInfo, &memAllocator);
    }



    //获取最优队列族
    void getQueueFamilyIndex(deviceStruct::gpuNode& node, const VkPhysicalDeviceProperties& gpuInfo, const VkSurfaceKHR& presentSurface) {
        //队列族数量
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(node.gpu, &queueFamilyCount, nullptr);

        if (queueFamilyCount == 0) {
            Log::printError(__FILE__, __LINE__, "无可用队列族");
        }

        //队列族信息
        std::vector<VkQueueFamilyProperties> queueFamilieList(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(node.gpu, &queueFamilyCount, queueFamilieList.data());

        if (debugInfo::LogSwitch) {
            Log::printTitle(gpuInfo.deviceName);
            Log::printTitle("队列族支持操作");

            struct { VkQueueFlags flag; const char* name; } flagTable[] = {
                { VK_QUEUE_GRAPHICS_BIT,         "图形" },
                { VK_QUEUE_COMPUTE_BIT,          "计算" },
                { VK_QUEUE_TRANSFER_BIT,         "传输" },
                { VK_QUEUE_SPARSE_BINDING_BIT,   "稀疏" },
                { VK_QUEUE_PROTECTED_BIT,        "保护" },
                { VK_QUEUE_VIDEO_DECODE_BIT_KHR, "视频解码" },
                { VK_QUEUE_OPTICAL_FLOW_BIT_NV,  "光流计算" },
            };

            for (uint32_t i = 0; i < queueFamilyCount; i++) {
                VkQueueFamilyProperties queueFamilyInfo = queueFamilieList[i];
                std::cout << std::left << "\t" << "族: " << std::setw(3) << i << "\t" << "数量: " << std::setw(4) << queueFamilyInfo.queueCount << "\t" << "掩码: " << std::setw(5) << queueFamilyInfo.queueFlags << "\t";

                for (const auto& entry : flagTable) {
                    if (queueFamilyInfo.queueFlags & entry.flag) {
                        std::cout << std::setw(12) << std::left << entry.name;
                    }
                }

                std::cout << std::endl << std::endl;
            }
        }

        //获取最优队列族
        for (uint32_t i = 0; i < queueFamilyCount; i++) {
            VkQueueFamilyProperties queueFamilyInfo = queueFamilieList[i];

            if (queueFamilyInfo.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                VkBool32 isSupport = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(node.gpu, i, presentSurface, &isSupport);  //检测队列族能否向窗口呈现图像

                if (isSupport) {
                    node.graphicsQueueFamily.index = i;
                    node.graphicsQueueFamily.count = queueFamilyInfo.queueCount;

                    if (debugInfo::LogSwitch) {
                        Log::printTitle("待选显卡队列");
                        std::cout << "\t" << "族: " << node.graphicsQueueFamily.index << "\t" << "数量: " << node.graphicsQueueFamily.count << std::endl;
                        Log::delimiter();
                    }

                    return;
                }
            }
        }

        Log::printError(__FILE__, __LINE__, "无适配队列族");
    }

    //显卡支持检测
    void gpuSupportCheck(const VkPhysicalDevice& gpu, const std::vector<const char*>& enableDeviceExtensionList) {
        //支持device扩展数量
        uint32_t supportExtensionCount;
        vkEnumerateDeviceExtensionProperties(gpu, nullptr, &supportExtensionCount, nullptr);

        //支持device扩展信息
        std::vector<VkExtensionProperties> supportExtensionList(supportExtensionCount);
        vkEnumerateDeviceExtensionProperties(gpu, nullptr, &supportExtensionCount, supportExtensionList.data());

        //启用扩展支持查询
        for (const auto& enableDeviceExtensionNode : enableDeviceExtensionList) {
            bool isSupport = false;     //支持标识

            for (const auto& supportExtensionNode : supportExtensionList) {
                if (std::strcmp(enableDeviceExtensionNode, supportExtensionNode.extensionName) == 0) {
                    isSupport = true;
                    break;
                }
            }

            if (!isSupport) {
                Log::print(enableDeviceExtensionNode);
                Log::printError(__FILE__, __LINE__, "缺少启用device扩展支持");
            }
        }

        //显卡功能支持
        VkPhysicalDeviceFeatures gpuFeaturesSupported;
        vkGetPhysicalDeviceFeatures(gpu, &gpuFeaturesSupported);

        //几何着色器支持检测
        if (!gpuFeaturesSupported.geometryShader) {
            Log::printError(__FILE__, __LINE__, "设备不支持几何着色器");
        }
    }

public:
    //获取gpu
    const deviceStruct::gpuNode& getGpu() {
        return gpuCandidateList[enableGpuIndex];
    }
};