#pragma once



class RenderPass {
public:
	VkRenderPass renderPass;    //指定渲染过程中，所绑定帧缓冲的参数（格式、内存布局）及各个渲染步骤之间的关系
    
    RenderPass(Device& Device) :Device{ Device } {
		createRenderPass();

        if (debugInfo::LogSwitch) {
            Log::printTitle("————————RenderPass已创建");

            Log::delimiter();
        }
	}

	~RenderPass() {
        cleanupSwapChain();

        if (debugInfo::LogSwitch) {
            Log::print("RenderPass已清除");
        }
	}

private:
	Device& Device;



    //对帧缓冲都要干什么的描述
    void createRenderPass() {
        //附件数组
        std::array<VkAttachmentDescription, 3> attachments{};
        // 颜色附件描述
        attachments[0].format = Device.imageFormat;                                    // 颜色附件格式
        attachments[0].samples = Device.msaaSamples;                                   // 多重采样数
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;                           // 清除操作      清除
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;                     // 存储操作      不关心
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;                // 模板加载      不关心
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;              // 模板存储      不关心
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;                      // 初始布局      未定义
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;         // 最终布局      颜色附件
        // 深度附件描述
        attachments[1].format = Device.depthFormat;                                    // 深度附件格式
        attachments[1].samples = Device.msaaSamples;                                   // 多重采样数
        attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;                           // 清除操作      清除
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;                     // 存储操作      不关心
        attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;                // 模板加载      不关心
        attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;              // 模板存储      不关心
        attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;                      // 初始布局      未定义
        attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; // 最终布局      深度附件
        // 多重采样后的像素颜色值的解析或合并过程描述
        attachments[2].format = Device.imageFormat;                                    // 解析后的颜色附件格式
        attachments[2].samples = VK_SAMPLE_COUNT_1_BIT;                                // 采样数
        attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;                       // 加载操作      不关心
        attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;                         // 存储操作      存储
        attachments[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;                // 模板加载      不关心
        attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;              // 模板存储      不关心
        attachments[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;                      // 初始布局      未定义
        attachments[2].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;                  // 最终布局      屏幕呈现

        //子通道描述
        VkAttachmentReference colorReference = { 0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };              //颜色附件引用
        VkAttachmentReference depthReference = { 1,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };      //深度附件引用
        VkAttachmentReference colorAttachmentResolveRef = { 2,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };   //采样附件引用

        std::array<VkSubpassDescription, 1> subpass{};
        subpass[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;     // 管线绑定点
        subpass[0].colorAttachmentCount = 1;                                // 颜色附件数量
        subpass[0].pColorAttachments = &colorReference;                     // 颜色附件引用
        subpass[0].pDepthStencilAttachment = &depthReference;               // 深度附件引用
        subpass[0].pResolveAttachments = &colorAttachmentResolveRef;        // 采样附件引用

        std::array<VkSubpassDependency, 1> dependency{};
        dependency[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency[0].dstSubpass = 0;
        dependency[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependency[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependency[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        //渲染路径创建信息
        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());     // 附件数量
        renderPassInfo.pAttachments = attachments.data();                               // 附件描述数组
        renderPassInfo.subpassCount = static_cast<uint32_t>(subpass.size());            // 子通道数量
        renderPassInfo.pSubpasses = subpass.data();                                     // 子通道描述数组
        renderPassInfo.dependencyCount = static_cast<uint32_t>(dependency.size());    // 依赖关系数量
        renderPassInfo.pDependencies = dependency.data();                             // 依赖关系数组

        //创建渲染路径
        if (vkCreateRenderPass(Device.device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            Log::printError(__FILE__, __LINE__, "渲染路径创建失败");
            throw;
        }
    }
public:
	void cleanupSwapChain() {
		vkDestroyRenderPass(Device.device, renderPass, nullptr);
	}

	void recreateSwapChain() {
		createRenderPass();
	}
};