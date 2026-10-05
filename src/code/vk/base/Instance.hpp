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
			auto debugCallbackInfo = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));

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
		std::vector<const char*> enableLayerList{};
		if (debugInfo::LayerSwitch) {
			enableLayerList                = instanceLayerSupportCheck(engineList::instanceLayerList, std::size(engineList::instanceLayerList));
			createInfo.pNext               = &debugCreateInfo;
			createInfo.enabledLayerCount   = static_cast<uint32_t>(enableLayerList.size());
			createInfo.ppEnabledLayerNames = enableLayerList.data();
		}
		else {
			createInfo.pNext               = nullptr;
			createInfo.enabledLayerCount   = 0;
			createInfo.ppEnabledLayerNames = nullptr;
		}

		//启用扩展
		std::vector<const char*> extensionList = instanceExtensionSupportCheck(engineList::instanceExtensionList, std::size(engineList::instanceExtensionList));
		createInfo.enabledExtensionCount       = static_cast<uint32_t>(extensionList.size());
		createInfo.ppEnabledExtensionNames     = extensionList.data();

		//创建instance
		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
			Log::printError(__FILE__, __LINE__, "instance创建失败");
		}

		//获取instance扩展函数
		if (debugInfo::LayerSwitch) {
			PFN_vkCreateDebugUtilsMessengerEXT debugCallbackInfo = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));

			if (debugCallbackInfo == nullptr) {
				Log::printError(__FILE__, __LINE__, "vkCreateDebugUtilsMessengerEXT获取失败");
			}
			else if (debugCallbackInfo(instance, &debugCreateInfo, nullptr, &debugCallback) != VK_SUCCESS) {
				Log::printError(__FILE__, __LINE__, "debugCallback创建失败");
			}
		}
	}



	//instance 层支持检查
	std::vector<const char*> instanceLayerSupportCheck(const engineStruct::instanceLayerNode* table, size_t count) {
		uint32_t supportLayerCount = 0;
		vkEnumerateInstanceLayerProperties(&supportLayerCount, nullptr);
		std::vector<VkLayerProperties> supportLayerList(supportLayerCount);
		vkEnumerateInstanceLayerProperties(&supportLayerCount, supportLayerList.data());

		std::vector<const char*> enableList{};
		for (size_t i = 0; i < count; i++) {
			bool isSupport = false;
			for (const VkLayerProperties& supportLayerNode : supportLayerList) {
				if (std::strcmp(table[i].name, supportLayerNode.layerName) == 0) { 
					isSupport = true; 
					break; 
				}
			}

			if (isSupport) { 
				enableList.push_back(table[i].name); 
			}
			else if (table[i].enable) { 
				Log::printError(__FILE__, __LINE__, std::string("缺少启用层支持：") + table[i].name);
			}
			else if (debugInfo::LogSwitch) { 
				Log::print(std::string("层缺失（不启用）：") + table[i].name); 
			}
		}

		return enableList;
	}

	//instance 扩展支持检查
	std::vector<const char*> instanceExtensionSupportCheck(const engineStruct::instanceExtensionNode* table, size_t count) {
		uint32_t                 platformExtensionCount;
		const char**             platformExtensionInfo = glfwGetRequiredInstanceExtensions(&platformExtensionCount);
		std::vector<const char*> extensionList(platformExtensionInfo, platformExtensionInfo + platformExtensionCount);

		uint32_t supportExtensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &supportExtensionCount, nullptr);
		std::vector<VkExtensionProperties> supportExtensionList(supportExtensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &supportExtensionCount, supportExtensionList.data());

		for (size_t i = 0; i < count; i++) {
			if (table[i].layerSwitch && !debugInfo::LayerSwitch) { continue; }

			bool isSupport = false;
			for (const VkExtensionProperties& supportExtensionNode : supportExtensionList) {
				if (std::strcmp(table[i].name, supportExtensionNode.extensionName) == 0) { 
					isSupport = true; 
					break; 
				}
			}

			if (isSupport) { 
				extensionList.push_back(table[i].name); 
			}
			else if (table[i].enable) { 
				Log::printError(__FILE__, __LINE__, std::string("缺少启用instance扩展支持：") + table[i].name); 
			}
			else if (debugInfo::LogSwitch) { 
				Log::print(std::string("instance 扩展缺失（不启用）：") + table[i].name); 
			}
		}

		if (debugInfo::LogSwitch) {
			Log::printTitle("Vulkan Instance 启用扩展");
			for (const char* extensionNode : extensionList) { Log::print(extensionNode); }
			Log::delimiter();
		}

		return extensionList;
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
};