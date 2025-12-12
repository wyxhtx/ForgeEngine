#pragma once



class Pipeline {
public:
	Pipeline(Device& Device) : Device{ Device } {
		createPipelineCache();
		InitBuffer.createPipelineLayout(procPipeline.pipelineLayout);
		createGraphicsPipeline();

		InitBuffer.drawUIText(0);	//不在createCommandBuffers前初始化就不能显示UIText
		InitBuffer.createCommandBuffer(InitBuffer.command.currentThreadId, procPipeline);

		if (debugInfo::LogSwitch) {
			Log::printTitle("————————Pipeline已创建");

			Log::delimiter();
		}
	}

	~Pipeline() {
		//等待地形线程结束，防止小窗点x退出时线程冲突
		while (threadCommInfo::plateLock);

		for (auto shaderModule : shaderModule) {
			vkDestroyShaderModule(Device.device, shaderModule, nullptr);
		}

		vkDestroyPipelineCache(Device.device, pipelineCache, nullptr);
		procPipeline.cleanupSwapChain(Device.device);

		if (debugInfo::LogSwitch) {
			Log::print("Pipeline已清除");
		}
	}

private:
	Device& Device;

	Plate InitPlate{};
	Buffer InitBuffer{ Device };

	VkPipelineCache pipelineCache;
	pipelineList::procPipelineNode procPipeline;
	std::vector<VkShaderModule> shaderModule;



	//创建管线缓存
	void createPipelineCache()
	{
		VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
		pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

		if (vkCreatePipelineCache(Device.device, &pipelineCacheCreateInfo, nullptr, &pipelineCache)) {
			Log::printError(__FILE__, __LINE__, "管线缓存创建失败");
			throw;
		}
	}

	//创建图形管线
	void createGraphicsPipeline() {
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};						//加载着色器
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};								//配置顶点输入
		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};								//配置输入装配
		VkViewport viewport{};																//设置视口
		VkRect2D scissor{};																	//设置剪裁
		VkPipelineViewportStateCreateInfo viewportState{};									//配置视口和剪裁
		VkPipelineRasterizationStateCreateInfo rasterizer{};								//配置光栅化
		VkPipelineMultisampleStateCreateInfo multisampling{};								//配置多重采样
		VkPipelineDepthStencilStateCreateInfo depthStencil{};								//配置深度/模板
		VkPipelineColorBlendAttachmentState colorBlendAttachment{};							//设置颜色混合附件
		VkPipelineColorBlendStateCreateInfo colorBlending{};								//配置颜色混合
		VkGraphicsPipelineCreateInfo pipelineInfo{};										//配置图形管线
		{
			vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

			inputAssembly.sType					 = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssembly.topology				 = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;		//图元装配方式，条带装配虽然减少内存，但是帧率2400，列表装配增加内存，但是帧率3700
			inputAssembly.primitiveRestartEnable = VK_FALSE;

			viewport.x		  = 0.0f;
			viewport.y		  = 0.0f;
			viewport.width    = (float)InitBuffer.getScExtent().width;
			viewport.height   = (float)InitBuffer.getScExtent().height;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;

			scissor.offset = { 0, 0 };
			scissor.extent = InitBuffer.getScExtent();

			viewportState.sType			= VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportState.viewportCount = 1;
			viewportState.pViewports	= &viewport;
			viewportState.scissorCount	= 1;
			viewportState.pScissors		= &scissor;

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

			multisampling.sType					= VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multisampling.sampleShadingEnable	= VK_FALSE;
			multisampling.rasterizationSamples	= InitBuffer.getMsaaSamples();
			multisampling.minSampleShading		= 1.0f;													
			multisampling.pSampleMask			= nullptr;													
			multisampling.alphaToCoverageEnable = VK_FALSE;											
			multisampling.alphaToOneEnable		= VK_FALSE;												

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

			colorBlendAttachment.colorWriteMask		 = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			colorBlendAttachment.blendEnable		 = VK_FALSE;
			colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;	
			colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
			colorBlendAttachment.colorBlendOp		 = VK_BLEND_OP_ADD;			
			colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;	
			colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			colorBlendAttachment.alphaBlendOp		 = VK_BLEND_OP_ADD;

			colorBlending.sType				= VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			colorBlending.logicOpEnable		= VK_FALSE;
			colorBlending.logicOp			= VK_LOGIC_OP_COPY;			
			colorBlending.attachmentCount	= 1;
			colorBlending.pAttachments		= &colorBlendAttachment;
			colorBlending.blendConstants[0] = 0.0f;				
			colorBlending.blendConstants[1] = 0.0f;				
			colorBlending.blendConstants[2] = 0.0f;				
			colorBlending.blendConstants[3] = 0.0f;				

			pipelineInfo.sType				 = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipelineInfo.stageCount			 = (uint32_t)shaderStages.size();
			pipelineInfo.pStages			 = shaderStages.data();
			pipelineInfo.pVertexInputState   = &vertexInputInfo;
			pipelineInfo.pInputAssemblyState = &inputAssembly;
			pipelineInfo.pViewportState		 = &viewportState;
			pipelineInfo.pRasterizationState = &rasterizer;
			pipelineInfo.pMultisampleState	 = &multisampling;
			pipelineInfo.pDepthStencilState  = &depthStencil;
			pipelineInfo.pColorBlendState	 = &colorBlending;
			pipelineInfo.pDynamicState		 = nullptr;					
			pipelineInfo.renderPass			 = InitBuffer.getRender(bufferEnum::procRenderId::osScence);
			pipelineInfo.subpass			 = 0;
			pipelineInfo.basePipelineHandle  = VK_NULL_HANDLE;		
			pipelineInfo.basePipelineIndex	 = -1;
		}

		//terrain
		{
			shaderStages[0] = loadShader(fileInfo::shader::terrainVert, VK_SHADER_STAGE_VERTEX_BIT);
			shaderStages[1] = loadShader(fileInfo::shader::terrainFrag, VK_SHADER_STAGE_FRAGMENT_BIT);

			vertexInputInfo.vertexBindingDescriptionCount	= static_cast<uint32_t>(terrainVertex::vertexNode::getBindingDescription().size());
			vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(terrainVertex::vertexNode::getAttributeDescriptions().size());
			vertexInputInfo.pVertexBindingDescriptions		= terrainVertex::vertexNode::getBindingDescription().data();
			vertexInputInfo.pVertexAttributeDescriptions	= terrainVertex::vertexNode::getAttributeDescriptions().data();

			pipelineInfo.layout = procPipeline.terrain.pipelineLayout;

			if (vkCreateGraphicsPipelines(Device.device, pipelineCache, 1, &pipelineInfo, nullptr, &procPipeline.terrain.pipeline) != VK_SUCCESS) {
				Log::printError(__FILE__, __LINE__, "terrain管线创建失败");
				throw;
			}
		}

		//model
		{
			shaderStages[0] = loadShader(fileInfo::shader::pbrVert, VK_SHADER_STAGE_VERTEX_BIT);
			shaderStages[1] = loadShader(fileInfo::shader::pbrFrag, VK_SHADER_STAGE_FRAGMENT_BIT);

			vertexInputInfo.vertexBindingDescriptionCount	= static_cast<uint32_t>(modelVertex::vertexNode::getBindingDescription().size());
			vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(modelVertex::vertexNode::getAttributeDescriptions().size());
			vertexInputInfo.pVertexBindingDescriptions		= modelVertex::vertexNode::getBindingDescription().data();
			vertexInputInfo.pVertexAttributeDescriptions	= modelVertex::vertexNode::getAttributeDescriptions().data();

			pipelineInfo.renderPass = InitBuffer.getRender(bufferEnum::procRenderId::osMirror);
			pipelineInfo.layout = procPipeline.model.pipelineLayout;

			if (vkCreateGraphicsPipelines(Device.device, pipelineCache, 1, &pipelineInfo, nullptr, &procPipeline.model.pipeline) != VK_SUCCESS) {
				Log::printError(__FILE__, __LINE__, "model管线创建失败");
				throw;
			}

			pipelineInfo.renderPass = InitBuffer.getRender(bufferEnum::procRenderId::osScence);
		}

		//skybox
		{
			shaderStages[0] = loadShader(fileInfo::shader::skyboxVert, VK_SHADER_STAGE_VERTEX_BIT);
			shaderStages[1] = loadShader(fileInfo::shader::skyboxFrag, VK_SHADER_STAGE_FRAGMENT_BIT);

			vertexInputInfo.vertexBindingDescriptionCount	= static_cast<uint32_t>(skyboxVertex::vertexNode::getBindingDescription().size());
			vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(skyboxVertex::vertexNode::getAttributeDescriptions().size());
			vertexInputInfo.pVertexBindingDescriptions		= skyboxVertex::vertexNode::getBindingDescription().data();
			vertexInputInfo.pVertexAttributeDescriptions	= skyboxVertex::vertexNode::getAttributeDescriptions().data();

			//实现只在空白处显示
			VkPipelineDepthStencilStateCreateInfo tempDepthStencil = depthStencil;
			tempDepthStencil.depthWriteEnable = VK_FALSE;
			tempDepthStencil.depthCompareOp   = VK_COMPARE_OP_LESS_OR_EQUAL;

			pipelineInfo.pDepthStencilState = &tempDepthStencil;
			pipelineInfo.layout				= procPipeline.skybox.pipelineLayout;

			if (vkCreateGraphicsPipelines(Device.device, pipelineCache, 1, &pipelineInfo, nullptr, &procPipeline.skybox.pipeline) != VK_SUCCESS) {
				Log::printError(__FILE__, __LINE__, "skybox管线创建失败");
				throw;
			}
			
			pipelineInfo.pDepthStencilState = &depthStencil;
		}

		//mine
		{
			shaderStages[0] = loadShader(fileInfo::shader::mineVert, VK_SHADER_STAGE_VERTEX_BIT);
			shaderStages[1] = loadShader(fileInfo::shader::mineFrag, VK_SHADER_STAGE_FRAGMENT_BIT);

			vertexInputInfo.vertexBindingDescriptionCount	= static_cast<uint32_t>(mineVertex::vertexNode::getBindingDescription().size());
			vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(mineVertex::vertexNode::getAttributeDescriptions().size());
			vertexInputInfo.pVertexBindingDescriptions		= mineVertex::vertexNode::getBindingDescription().data();
			vertexInputInfo.pVertexAttributeDescriptions	= mineVertex::vertexNode::getAttributeDescriptions().data();

			//选择框覆盖方块的颜色混合
			VkPipelineColorBlendAttachmentState tempColorBlendAttachment = colorBlendAttachment;
			tempColorBlendAttachment.blendEnable		 = VK_TRUE;
			tempColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			tempColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			tempColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

			//线段装配
			VkPipelineInputAssemblyStateCreateInfo tempInputAssembly = inputAssembly;
			tempInputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;

			//线填充
			VkPipelineRasterizationStateCreateInfo tempRasterizer = rasterizer;
			tempRasterizer.polygonMode = VK_POLYGON_MODE_LINE;
			tempRasterizer.lineWidth   = 1.0f;

			colorBlending.pAttachments		 = &tempColorBlendAttachment;
			pipelineInfo.pInputAssemblyState = &tempInputAssembly;
			pipelineInfo.pRasterizationState = &tempRasterizer;
			pipelineInfo.layout				 = procPipeline.mine.pipelineLayout;

			if (vkCreateGraphicsPipelines(Device.device, pipelineCache, 1, &pipelineInfo, nullptr, &procPipeline.mine.pipeline) != VK_SUCCESS) {
				Log::printError(__FILE__, __LINE__, "mine管线创建失败");
				throw;
			}
			
			colorBlending.pAttachments		 = &colorBlendAttachment;
			pipelineInfo.pInputAssemblyState = &inputAssembly;
			pipelineInfo.pRasterizationState = &rasterizer;
		}

		//osMirror
		{
			shaderStages[0] = loadShader(fileInfo::shader::osMirrorVert, VK_SHADER_STAGE_VERTEX_BIT);
			shaderStages[1] = loadShader(fileInfo::shader::osMirrorFrag, VK_SHADER_STAGE_FRAGMENT_BIT);

			vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(osMirrorVertex::vertexNode::getBindingDescription().size());
			vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(osMirrorVertex::vertexNode::getAttributeDescriptions().size());
			vertexInputInfo.pVertexBindingDescriptions = osMirrorVertex::vertexNode::getBindingDescription().data();
			vertexInputInfo.pVertexAttributeDescriptions = osMirrorVertex::vertexNode::getAttributeDescriptions().data();

			pipelineInfo.layout = procPipeline.osMirror.pipelineLayout;

			if (vkCreateGraphicsPipelines(Device.device, pipelineCache, 1, &pipelineInfo, nullptr, &procPipeline.osMirror.pipeline) != VK_SUCCESS) {
				Log::printError(__FILE__, __LINE__, "osMirror管线创建失败");
				throw;
			}
		}

		//osScence
		{
			shaderStages[0] = loadShader(fileInfo::shader::osScenceVert, VK_SHADER_STAGE_VERTEX_BIT);
			shaderStages[1] = loadShader(fileInfo::shader::osScenceFrag, VK_SHADER_STAGE_FRAGMENT_BIT);

			vertexInputInfo.vertexBindingDescriptionCount	= static_cast<uint32_t>(osScenceVertex::vertexNode::getBindingDescription().size());
			vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(osScenceVertex::vertexNode::getAttributeDescriptions().size());
			vertexInputInfo.pVertexBindingDescriptions		= osScenceVertex::vertexNode::getBindingDescription().data();
			vertexInputInfo.pVertexAttributeDescriptions	= osScenceVertex::vertexNode::getAttributeDescriptions().data();

			pipelineInfo.renderPass = InitBuffer.getRender(bufferEnum::procRenderId::scScence);
			pipelineInfo.layout = procPipeline.osScence.pipelineLayout;
			
			if (vkCreateGraphicsPipelines(Device.device, pipelineCache, 1, &pipelineInfo, nullptr, &procPipeline.osScence.pipeline) != VK_SUCCESS) {
				Log::printError(__FILE__, __LINE__, "osScence管线创建失败");
				throw;
			}

			pipelineInfo.renderPass = InitBuffer.getRender(bufferEnum::procRenderId::osScence);
		}

		//uiText
		{
			shaderStages[0] = loadShader(fileInfo::shader::uitextVert, VK_SHADER_STAGE_VERTEX_BIT);
			shaderStages[1] = loadShader(fileInfo::shader::uitextFrag, VK_SHADER_STAGE_FRAGMENT_BIT);

			vertexInputInfo.vertexBindingDescriptionCount	= static_cast<uint32_t>(uiTextVertex::vertexNode::getBindingDescription().size());
			vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(uiTextVertex::vertexNode::getAttributeDescriptions().size());
			vertexInputInfo.pVertexBindingDescriptions		= uiTextVertex::vertexNode::getBindingDescription().data();
			vertexInputInfo.pVertexAttributeDescriptions	= uiTextVertex::vertexNode::getAttributeDescriptions().data();

			//uiText和屏幕的颜色混合
			VkPipelineColorBlendAttachmentState tempColorBlendAttachment = colorBlendAttachment;
			tempColorBlendAttachment.blendEnable		 = VK_TRUE;
			tempColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			tempColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			tempColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

			//条带装配
			VkPipelineInputAssemblyStateCreateInfo tempInputAssembly = inputAssembly;
			tempInputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

			//顺时针正面
			VkPipelineRasterizationStateCreateInfo tempRasterizer = rasterizer;
			tempRasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

			//实现始终渲染在最上层
			VkPipelineDepthStencilStateCreateInfo tempDepthStencil = depthStencil;
			tempDepthStencil.depthTestEnable  = VK_FALSE;
			tempDepthStencil.depthWriteEnable = VK_FALSE;

			colorBlending.pAttachments		 = &tempColorBlendAttachment;
			pipelineInfo.pInputAssemblyState = &tempInputAssembly;
			pipelineInfo.pRasterizationState = &tempRasterizer;
			pipelineInfo.pDepthStencilState  = &tempDepthStencil;
			pipelineInfo.layout				 = procPipeline.uiText.pipelineLayout;

			if (vkCreateGraphicsPipelines(Device.device, pipelineCache, 1, &pipelineInfo, nullptr, &procPipeline.uiText.pipeline) != VK_SUCCESS) {
				Log::printError(__FILE__, __LINE__, "uiText管线创建失败");
				throw;
			}

			colorBlending.pAttachments		 = &colorBlendAttachment;
			pipelineInfo.pInputAssemblyState = &inputAssembly;
			pipelineInfo.pRasterizationState = &rasterizer;
			pipelineInfo.pDepthStencilState  = &depthStencil;
		}
	}



	VkPipelineShaderStageCreateInfo loadShader(std::string fileName, VkShaderStageFlagBits stage){
		shaderModule.push_back(createShaderModule(readFile(fileName)));

		VkPipelineShaderStageCreateInfo shaderStage = {};
		shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStage.stage = stage;
		shaderStage.module = shaderModule.back();
		shaderStage.pName = "main";
		return shaderStage;
	}

	VkShaderModule createShaderModule(const std::vector<char>& code) {
		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(Device.device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
			Log::printError(__FILE__, __LINE__, "着色器创建失败");
			throw;
		}

		return shaderModule;
	}

	static std::vector<char> readFile(const std::string& filename) {
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open()) {
			Log::printError(__FILE__, __LINE__, "着色器文件打开失败");
			throw;
		}

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);
		file.close();

		return buffer;
	}

public:
	void drawFrame(Camera& Camera) {
		static fpsStruct::tickNode uiTextTick;
		static fpsStruct::tickNode cubeTick;
		static playerStruct::selectBoxNode selectBox = InitPlate.getSelectBox(Camera.node);

		static int uiDeltaFrame = 0;                 //UI帧增量
		uiDeltaFrame++;



		//更新uiText和更新板块
		if (uiTextTick.isUpdate(fpsInfo::uiTextFPS)) {
			InitBuffer.drawUIText((int)(uiDeltaFrame / uiTextTick.dt));
			uiDeltaFrame = 0;

			//地形线程数为1且命令缓冲区未变更时更新板块
			if (InitBuffer.command.processingThreadSize == 1 && tempId == InitBuffer.command.currentThreadId) {
				static glm::dvec3 pos0 = glm::floor(playerInfo::initSpawnPos / (double)plateInfo::chunkLength) * (double)plateInfo::chunkLength;
				glm::dvec3 pos1 = glm::floor(Camera.node.pos / (double)plateInfo::chunkLength) * (double)plateInfo::chunkLength;
				
				if (pos0 != pos1) {
					//每一次更新板块都在下一帧实装，线程创建用时1ms
					threadCommInfo::plateLock = true;
					std::thread(&Pipeline::updatePlateThread, this, Camera.node.pos).detach();

					pos0 = pos1;
				}
			}
		}

		//破坏或放置方块后更新板块网格
		if (cubeTick.isUpdate(fpsInfo::cubeFPS)) {
			InitPlate.externalUpdateLock = true;
			
			if (!InitPlate.internalUpdateLock && tempId == InitBuffer.command.currentThreadId && !InitPlate.cubeChangeList.empty()) {
				InitPlate.updateDeltaCube();
				tempId = (InitBuffer.command.currentThreadId + 1) % 2;
			}

			InitPlate.externalUpdateLock = false;
		}

		//更换命令缓冲区后重建命令缓冲区（5~9ms）
		//r=1时，更新板块会把网格数组清空
		InitPlate.externalUpdateLock = true;
		if (!InitPlate.internalUpdateLock && tempId != InitBuffer.command.currentThreadId) {
			//原点迁移会导致板块网格重写，所以尽可能避免频繁迁移
			if (playerInfo::logicOriginPos != InitPlate.originPos) {
				playerInfo::logicOriginPos = InitPlate.originPos;
				Camera.updateView();

				if (debugInfo::LogSwitch) {
					Log::print("原点迁移");

					if (!InitPlate.isEntireRebuild) {
						Log::printError(__FILE__, __LINE__, "原点迁移时InitPlate.isEntireRebuild为假");
					}
				}
			}

			//目前重建命令缓冲区必定是updatePlate或updatePlatePartMesh触发的，通过二者真值表得出此处判断逻辑
			if (InitPlate.isEntireRebuild) {
				InitPlate.isEntireRebuild = false;

				//地形管线全部重建，R(8)N(34)M(15)
				InitBuffer.recreateBuffer(tempId, true);
			}
			else {
				//只重建Lod_0地形管线，R(8)N(34)M(4)
				InitBuffer.recreateBuffer(tempId, false);
			}

			InitBuffer.createCommandBuffer(tempId, procPipeline);

			InitBuffer.command.currentThreadId = tempId;
		}
		InitPlate.externalUpdateLock = false;

		selectBox = InitPlate.getSelectBox(Camera.node);	//当前帧获取的方块更新列表在下一帧更新
		InitBuffer.updateUniformBuffer(Camera.node, selectBox);

		if (InitBuffer.drawFrame()) {
			recreateSwapChain(); 
			Log::print("重建交换链");
		}
	}

private:
	int tempId = InitBuffer.command.currentThreadId;		//命令缓冲区id标识，用于解决线程冲突



	void updatePlateThread(glm::dvec3 playerPos) {
		InitBuffer.command.processingThreadSize++;
		timeStruct::timeNode time{};

		InitPlate.updatePlate(playerPos);
		tempId = (InitBuffer.command.currentThreadId + 1) % 2;

		Log::print("板块刷新用时:\t" + time.end());
		InitBuffer.command.processingThreadSize--;
		threadCommInfo::plateLock = false;
	}

	void recreateSwapChain() {
		vkDeviceWaitIdle(Device.device);

		InitBuffer.cleanupSwapChain(InitBuffer.command.currentThreadId);
		procPipeline.cleanupSwapChain(Device.device);

		InitBuffer.recreateSwapChain();
		InitBuffer.createPipelineLayout(procPipeline.pipelineLayout);
		createGraphicsPipeline();

		InitBuffer.createCommandBuffer(InitBuffer.command.currentThreadId, procPipeline);
	}
};