#pragma once



class Device {
public:
    VkSurfaceKHR surface;
    deviceStruct::deviceNode availableDevices;      //可用显卡库

    VkDevice device;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

    VkQueue graphicsQueue;
    VkQueue presentQueue;

    uint32_t queueFamilyIndex;                      //队列族索引，在pickPhysicalDevice()设置
    VmaAllocator allocator;



    Device(GLFWwindow* window) {
		createSurface(window);

        pickPhysicalDevice();
        createLogicalDevice();

        createVMA();

        if (debugInfo::LogSwitch) {
            Log::printTitle("————————Device已创建");

            Log::delimiter();
        }
	}

	~Device() {
        vmaDestroyAllocator(allocator);
        
        vkDestroyDevice(device, nullptr);
		vkDestroySurfaceKHR(InitInstance.instance, surface, nullptr);

        if (debugInfo::LogSwitch) {
            Log::print("Device已清除");
        }
	}

private:
    Instance InitInstance{};



	//创建窗口表面
	void createSurface(GLFWwindow* window) {
		if (glfwCreateWindowSurface(InitInstance.instance, window, nullptr, &surface) != VK_SUCCESS) {
			Log::printError(__FILE__, __LINE__, "窗口表面创建失败");
            throw;
		}
	}

	//选择显卡
    void pickPhysicalDevice() {
        //---获取可用显卡
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(InitInstance.instance, &deviceCount, nullptr);

        if (deviceCount == 0) {
            Log::printError(__FILE__, __LINE__, "无可用显卡");
            throw;
        }

        availableDevices.physicalDevices.resize(deviceCount);
        availableDevices.queueIndices.resize(deviceCount);
        vkEnumeratePhysicalDevices(InitInstance.instance, &deviceCount, availableDevices.physicalDevices.data());

        //存储可用显卡信息
        std::vector<VkPhysicalDeviceProperties> deviceProperties(deviceCount);

        if (debugInfo::LogSwitch) {
            Log::printTitle("可用显卡");

            for (uint32_t i = 0; i < deviceCount; i++) {
                vkGetPhysicalDeviceProperties(availableDevices.physicalDevices[i], &deviceProperties[i]);

                Log::print(deviceProperties[i].deviceName);
            }

            Log::delimiter();
        }



        //---选择显卡
        //检查显卡是否满足所需的队列族类型，从中选择能满足要求的显卡并顺便取得队列族索引
        for (uint32_t i = 0; i < deviceCount; i++) {
            if (debugInfo::LogSwitch) {
                Log::printTitle(deviceProperties[i].deviceName);
            }

            selectQueueFamilyIndices(availableDevices.physicalDevices[i], availableDevices.queueIndices[i]);
        }

        //遍历显卡队列索引库,启用支持队列数量最多的显卡
        int maxIndices = availableDevices.queueIndices[0].count;

        for (uint32_t i = 1; i < deviceCount; i++) {
            if (maxIndices < availableDevices.queueIndices[i].count) {
                maxIndices = availableDevices.queueIndices[i].count;
            }
        }

        //启用的显卡放在首位
        for (uint32_t i = 0; i < deviceCount; i++) {
            if (maxIndices == availableDevices.queueIndices[i].count) {
                availableDevices.queueIndices.insert(availableDevices.queueIndices.begin(), availableDevices.queueIndices[i]);
                availableDevices.physicalDevices.insert(availableDevices.physicalDevices.begin(), availableDevices.physicalDevices[i]);
                deviceProperties.insert(deviceProperties.begin(), deviceProperties[i]);
                break;
            }
        }

        //首位显卡更多检测
        if (isDeviceSuitable(availableDevices.physicalDevices[0])) {
            if (debugInfo::LogSwitch) {
                Log::printTitle("选择显卡");
                Log::print(deviceProperties[0].deviceName);
                Log::delimiter();
            }

            queueFamilyIndex = availableDevices.queueIndices[0].present;
            physicalDevice = availableDevices.physicalDevices[0];

            return;
        }
        else {
            Log::printError(__FILE__, __LINE__, "首位显卡不适配");
            throw;
        }
    }

    //创建逻辑设备
    void createLogicalDevice() {
        float queuePriority = 1.0f;

        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfo.queueFamilyIndex = availableDevices.queueIndices[0].support;

        //特征链在deviceCreateInfo.pNext启用
        VkPhysicalDeviceFeatures2 physicalDeviceFeatures2 = {};
        physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        physicalDeviceFeatures2.pNext = nullptr;
        vkGetPhysicalDeviceFeatures2(physicalDevice, &physicalDeviceFeatures2);
        
        //普通链在 deviceCreateInfo.pEnabledFeatures启用
        /*VkPhysicalDeviceFeatures physicalDeviceFeatures = {};
        vkGetPhysicalDeviceFeatures(physicalDevice, &physicalDeviceFeatures);*/

        //如果 pNext 链包含 VkPhysicalDeviceFeatures2 结构体，则 pEnabledFeatures 必须为 NULL
        VkDeviceCreateInfo deviceCreateInfo{};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.queueCreateInfoCount = 1;
        deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
        deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(enableConfigInfo::enableDeviceExtension.size());
        deviceCreateInfo.ppEnabledExtensionNames = enableConfigInfo::enableDeviceExtension.data();
        deviceCreateInfo.pNext = &physicalDeviceFeatures2;
        //deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures;

        if (vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device) != VK_SUCCESS) {
            Log::printError(__FILE__, __LINE__, "设备创建失败");
            throw;
        }

        vkGetDeviceQueue(device, availableDevices.queueIndices[0].support, 0, &graphicsQueue);
        vkGetDeviceQueue(device, availableDevices.queueIndices[0].present, 0, &presentQueue);

        return;
    }

    //创建内存管理
    void createVMA() {
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = physicalDevice;
        allocatorInfo.device = device;
        allocatorInfo.instance = InitInstance.instance;

        vmaCreateAllocator(&allocatorInfo, &allocator);
    }



    //挑选显卡队列索引
    void selectQueueFamilyIndices(VkPhysicalDevice device, deviceStruct::queueIndiceNode& queueIndices) {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        if (!queueFamilyCount) {
            Log::printError(__FILE__, __LINE__, "无可用队列族");
            throw;
        }

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        if (debugInfo::LogSwitch) {
            Log::printTitle("队列族支持操作");

            for (uint32_t i = 0; i < queueFamilyCount; i++) {
                VkQueueFamilyProperties queueFamily = queueFamilies[i];

                std::cout << "\t" << "族: " << i << std::endl;
                std::cout << "\t" << "数量: " << queueFamily.queueCount << std::endl;
                std::cout << "\t" << "掩码: " << queueFamily.queueFlags << std::endl;

                if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                    std::cout << "\t" << "图形";
                }

                if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
                    std::cout << "\t" << "计算";
                }

                if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) {
                    std::cout << "\t" << "传输";
                }

                if (queueFamily.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) {
                    std::cout << "\t" << "稀疏";
                }

                if (queueFamily.queueFlags & VK_QUEUE_PROTECTED_BIT) {
                    std::cout << "\t" << "保护";
                }

                if (queueFamily.queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR) {
                    std::cout << "\t" << "视频解码";
                }

                if (queueFamily.queueFlags & VK_QUEUE_OPTICAL_FLOW_BIT_NV) {
                    std::cout << "\t" << "光流计算";
                }

                std::cout << std::endl << std::endl;
            }
        }

        //队列操作支持检测
        for (uint32_t i = 0; i < queueFamilyCount; i++) {
            if (queueFamilies[i].queueFlags == 15) {
                queueIndices.support = i;
                queueIndices.count = queueFamilies[i].queueCount;

                VkBool32 presentSupport = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

                if (presentSupport) {
                    if (debugInfo::LogSwitch) {
                        Log::printTitle("待选显卡队列");
                        Log::print("族:");
                        Log::print(queueIndices.support);

                        Log::print("数量:");
                        Log::print(queueIndices.count);
                        Log::delimiter();
                    }

                    queueIndices.present = i;
                    return;
                }
            }
        }

        Log::printError(__FILE__, __LINE__, "无适配队列族");
        throw;
    }
    
    //显卡是否适配应用程序
    bool isDeviceSuitable(VkPhysicalDevice device) {
        //设备扩展支持检测
        bool extensionsSupported = checkDeviceExtensionSupport(device);

        //各向异性支持检测
        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

        return extensionsSupported && supportedFeatures.samplerAnisotropy;
    }



    //检测设备扩展启用
    bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(enableConfigInfo::enableDeviceExtension.begin(), enableConfigInfo::enableDeviceExtension.end());

        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }
};