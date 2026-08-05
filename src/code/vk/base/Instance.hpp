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
            auto debugCallbackInfo = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

            if (debugCallbackInfo != nullptr) {
                debugCallbackInfo(instance, debugCallback, nullptr);
            }
        }

       vkDestroyInstance(instance, nullptr);

       if (debugInfo::LogSwitch) {
           Log::print("Instance已清除");
       }
    }

private:
    VkDebugUtilsMessengerEXT debugCallback;            //校验层回调

    

    //初始化Instance
    void createInstance() {
        //vulkan版本号
        uint32_t apiVersion = VK_API_VERSION_1_0;       //默认赋值1.0
        vkEnumerateInstanceVersion(&apiVersion);        //获取实际版本号

        if (debugInfo::LogSwitch) {
            Log::printTitle("Vulkan API 版本号");
            std::cout << "\t" << VK_API_VERSION_MAJOR(apiVersion) << "." << VK_API_VERSION_MINOR(apiVersion) << "." << VK_API_VERSION_PATCH(apiVersion) << std::endl;
            Log::delimiter();
        }

        //应用信息
        VkApplicationInfo appInfo{};
        appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pNext              = nullptr;
        appInfo.pApplicationName   = windowInfo::gameName;
        appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 0, 0, 1);
        appInfo.pEngineName        = windowInfo::engineName;
        appInfo.engineVersion      = VK_MAKE_API_VERSION(0, 0, 0, 1);
        appInfo.apiVersion         = apiVersion;

        //instance创建信息
        VkInstanceCreateInfo createInfo{};
        createInfo.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.flags            = 0;
        createInfo.pApplicationInfo = &appInfo;
        
        //debugCallback配置
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        debugCreateInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugCreateInfo.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugCreateInfo.pfnUserCallback = debugCallbackFunc;

        //启用层
        if (debugInfo::LayerSwitch) {
            enableLayerListSupport(engineInfo::enableLayerList);
            createInfo.pNext               = &debugCreateInfo;
            createInfo.enabledLayerCount   = (uint32_t)engineInfo::enableLayerList.size();
            createInfo.ppEnabledLayerNames = engineInfo::enableLayerList.data();
        }
        else {
            createInfo.pNext               = nullptr;
            createInfo.enabledLayerCount   = 0;
            createInfo.ppEnabledLayerNames = nullptr;
        }

        //启用扩展
        std::vector<const char*> extensionList = enableInstanceExtensionListSupport(engineInfo::enableInstanceExtensionList);
        createInfo.enabledExtensionCount       = (uint32_t)extensionList.size();
        createInfo.ppEnabledExtensionNames     = extensionList.data();

        //创建instance
        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            Log::printError(__FILE__, __LINE__, "instance创建失败");
        }

        //获取instance扩展函数
        if (debugInfo::LayerSwitch) {
            PFN_vkCreateDebugUtilsMessengerEXT debugCallbackInfo = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

            if (debugCallbackInfo == nullptr) {
                Log::printError(__FILE__, __LINE__, "vkCreateDebugUtilsMessengerEXT获取失败");
            }
            else if (debugCallbackInfo(instance, &debugCreateInfo, nullptr, &debugCallback) != VK_SUCCESS) {
                Log::printError(__FILE__, __LINE__, "debugCallback创建失败");
            }
        }
    }



    //回调处理
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallbackFunc(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
        std::cerr << "校验层: " << std::endl << pCallbackData->pMessage << std::endl << "------" << std::endl;

        //Pipeline生命周期中，校验层报错时暂停
        while (lockInfo::logPauseEnableLock.load()) {
            std::this_thread::yield();
        };

        return VK_FALSE;
    }

    //enableLayerList支持
    void enableLayerListSupport(const std::vector<const char*>& enableLayerList) {
        //支持层数量
        uint32_t supportLayerCount;
        vkEnumerateInstanceLayerProperties(&supportLayerCount, nullptr);

        //支持层信息
        std::vector<VkLayerProperties> supportLayerList(supportLayerCount);
        vkEnumerateInstanceLayerProperties(&supportLayerCount, supportLayerList.data());

        //输出支持层与启用层
        if (debugInfo::LogSwitch) {
            //Log::printTitle("Vulkan Instance 支持层");
            //for (const auto& supportLayerNode : supportLayerList) {
            //    Log::print(supportLayerNode.layerName);
            //    Log::print(supportLayerNode.description);
            //    Log::print("------");
            //}
            //Log::delimiter();

            Log::printTitle("Vulkan Instance 启用层");
            for (const auto& enableLayerNode : enableLayerList) {
                Log::print(enableLayerNode);
            }
            Log::delimiter();
        }

        //启用层支持查询
        for (const auto& enableLayerNode : enableLayerList) {
            bool isSupport = false;     //支持标识

            for (const auto& supportLayerNode : supportLayerList) {
                if (std::strcmp(enableLayerNode, supportLayerNode.layerName) == 0) {
                    isSupport = true;
                    break;
                }
            }

            if (!isSupport) {
                Log::print(enableLayerNode);
                Log::printError(__FILE__, __LINE__, "缺少启用层支持");
            }
        }
    }

    //enableInstanceExtensionList支持
    std::vector<const char*> enableInstanceExtensionListSupport(const std::vector<const char*>& enableInstanceExtensionList) {
        uint32_t                 platformExtensionCount;                                                                            //平台扩展数量
        const char**             platformExtensionInfo = glfwGetRequiredInstanceExtensions(&platformExtensionCount);                //平台扩展信息
        std::vector<const char*> platformExtensionList(platformExtensionInfo, platformExtensionInfo + platformExtensionCount);      //平台扩展列表
    
        if (debugInfo::LayerSwitch) {
            platformExtensionList.insert(platformExtensionList.end(), enableInstanceExtensionList.begin(), enableInstanceExtensionList.end());
        }

        //支持instance扩展数量
        uint32_t supportExtensionCount;
        vkEnumerateInstanceExtensionProperties(nullptr, &supportExtensionCount, nullptr);
    
        //支持instance扩展信息
        std::vector<VkExtensionProperties> supportExtensionList(supportExtensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &supportExtensionCount, supportExtensionList.data());
    
        //输出支持扩展与启用扩展
        if (debugInfo::LogSwitch) {
            //Log::printTitle("Vulkan Instance 支持扩展");
            //for (const auto& supportExtensionNode : supportExtensionList) {
            //    Log::print(supportExtensionNode.extensionName);
            //    Log::print("------");
            //}
            //Log::delimiter();
    
            Log::printTitle("Vulkan Instance 启用扩展");
            for (const auto& platformExtensionNode : platformExtensionList) {
                Log::print(platformExtensionNode);
            }
            Log::delimiter();
        }
    
        //启用扩展支持查询
        for (const auto& platformExtensionNode : platformExtensionList) {
            bool isSupport = false;     //支持标识
    
            for (const auto& supportExtensionNode : supportExtensionList) {
                if (std::strcmp(platformExtensionNode, supportExtensionNode.extensionName) == 0) {
                    isSupport = true;
                    break;
                }
            }
    
            if (!isSupport) {
                Log::print(platformExtensionNode);
                Log::printError(__FILE__, __LINE__, "缺少启用instance扩展支持");
            }
        }

        return platformExtensionList;
    }
};