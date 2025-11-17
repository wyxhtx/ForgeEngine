#pragma once



class Instance {
public:
    VkInstance instance;



    Instance() {
        createInstance();

        if (debugInfo::LogSwitch) {
            Log::printTitle("————————Instance已创建");

            Log::delimiter();
        }
    }

    ~Instance() {
       if (debugInfo::LayerSwitch) {
            destroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
       }

       vkDestroyInstance(instance, nullptr);

       if (debugInfo::LogSwitch) {
           Log::print("Instance已清除");
       }
    }

private:
    uint32_t APIVersion;                            //vulkan版本号
    VkDebugUtilsMessengerEXT debugMessenger;

    

    //初始化Instance
    void createInstance() {
        //获取版本信息
        APIVersion = VK_API_VERSION_1_0;
        if (vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkEnumerateInstanceVersion")) {
            vkEnumerateInstanceVersion(&APIVersion);
        }

        if (debugInfo::LogSwitch) {
            Log::printTitle("Vulkan API 版本号");
            std::cout << "\t" << VK_VERSION_MAJOR(APIVersion) << "." << VK_VERSION_MINOR(APIVersion) << "." << VK_VERSION_PATCH(APIVersion) << std::endl;
            Log::delimiter();
        }

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pNext = nullptr;
        appInfo.pApplicationName = windowInfo::gameName;
        appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
        appInfo.pEngineName = windowInfo::engineName;
        appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);
        appInfo.apiVersion = APIVersion;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.flags = 0;
        createInfo.pApplicationInfo = &appInfo;

        //启用层
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (debugInfo::LayerSwitch) {
            //检测是否支持所有层
            checkLayer();
            populateDebugMessengerCreateInfo(debugCreateInfo);

            createInfo.enabledLayerCount = static_cast<uint32_t>(enableConfigInfo::enableLayer.size());
            createInfo.ppEnabledLayerNames = enableConfigInfo::enableLayer.data();
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;

        }
        else {
            createInfo.enabledLayerCount = 0;
            createInfo.ppEnabledLayerNames = nullptr;
            createInfo.pNext = nullptr;
        }

        //启用扩展
        std::vector<const char*> extensions = getInstanceExtension();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            Log::printError(__FILE__, __LINE__, "实例创建失败");
            throw;
        }

        if (debugInfo::LayerSwitch && (createDebugUtilsMessengerEXT(instance, &debugCreateInfo, nullptr, &debugMessenger) != VK_SUCCESS)) {
            Log::printError(__FILE__, __LINE__, "配置回调失败");
            throw;
        }
    }



    void checkLayer() {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        if (layerCount == 0) {
            Log::print("无启用层");
            return;
        }

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        //输出支持层
        if (debugInfo::LogSwitch) {
            Log::printTitle("Vulkan_Instance 层支持");
            for (const auto& layer : availableLayers) {
                Log::print(layer.layerName);
                Log::print(layer.description);
                Log::print("------");
            }
            Log::delimiter();

            Log::printTitle("Vulkan_Instance 层启用");
            for (const auto& layer : enableConfigInfo::enableLayer) {
                Log::print(layer);
            }
            Log::delimiter();
        }

        //支持查询
        for (const auto& enable : enableConfigInfo::enableLayer) {
            bool found = false;

            for (const auto& layer : availableLayers) {
                if (std::strcmp(enable, layer.layerName) == 0) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                Log::print(enable);
                Log::printError(__FILE__, __LINE__, "层缺少支持");
                throw;
            }
        }
    }

    std::vector<const char*> getInstanceExtension() {
        //输出Vulkan支持的Instance扩展
        if (debugInfo::LogSwitch) {
            uint32_t extensionCount;
            vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

            std::vector<VkExtensionProperties> availableExtension(extensionCount);
            vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtension.data());

            Log::printTitle("Vulkan_Instance 扩展支持");

            for (const auto& extension : availableExtension) {
                Log::print(extension.extensionName);
                Log::print("------");
            }

            Log::delimiter();
        }

        uint32_t extensionCount;        //设备支持的扩展数量
        const char** extensionInfo;     //设备支持的扩展信息

        extensionInfo = glfwGetRequiredInstanceExtensions(&extensionCount);
        std::vector<const char*> extensionList(extensionInfo, extensionInfo + extensionCount);      //设备支持列表

        if (debugInfo::LayerSwitch) {
            extensionList.insert(extensionList.end(), enableConfigInfo::enableInstanceExtension.begin(), enableConfigInfo::enableInstanceExtension.end());
        }

        if (debugInfo::LogSwitch) {
            Log::printTitle("Vulkan_Instance 扩展启用");

            //输出Vulkan支持的Instance扩展
            for (int i = 0; i < extensionList.size(); i++) {
                Log::print(extensionList[i]);
            }

            Log::delimiter();
        }

        return extensionList;
    }

    //填充调试信使创建信息
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
    }

    //创建回调
    VkResult createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        }
        else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }



    //处理回调
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
        std::cerr << "校验层: " << pCallbackData->pMessage << std::endl << "------" << std::endl;
        return VK_FALSE;
    }

    //销毁回调
    void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance, debugMessenger, pAllocator);
        }
    }
};