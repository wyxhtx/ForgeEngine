#pragma once



class Pipeline {
public:
	Pipeline(Device& Device) : Device{ Device } {
		procPipeline.createPipeLayout(Device.device, InitBuffer.getDst().node);
		createPipeline();

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

		procPipeline.clear(Device.device);

		if (debugInfo::LogSwitch) {
			Log::print("Pipeline已清除");
		}
	}

private:
	Device& Device;

	Plate InitPlate{};
	Buffer InitBuffer{ Device };

	pipelineList::procPipelineNode	procPipeline;

	

	//创建图形管线
	void createPipeline() {
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStage{};						//加载着色器
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};								//配置顶点输入
		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};								//配置输入装配
		VkViewport viewport{};																//设置视口
		VkRect2D scissor{};																	//设置剪裁
		VkPipelineViewportStateCreateInfo viewportState{};									//配置视口和剪裁
		VkPipelineRasterizationStateCreateInfo rasterizer{};								//配置光栅化
		VkPipelineMultisampleStateCreateInfo multisampling{};								//配置多重采样
		VkPipelineDepthStencilStateCreateInfo depthStencil{};								//配置深度/模板
		std::array<VkPipelineColorBlendAttachmentState, 1> colorAttachmentForward{};		//设置颜色前向混合附件
		std::array<VkPipelineColorBlendAttachmentState, 3> colorAttachmentDeferred{};		//设置颜色延迟混合附件
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

			colorAttachmentForward[0].colorWriteMask		= VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			colorAttachmentForward[0].blendEnable			= VK_FALSE;
			colorAttachmentForward[0].srcColorBlendFactor	= VK_BLEND_FACTOR_ONE;	
			colorAttachmentForward[0].dstColorBlendFactor	= VK_BLEND_FACTOR_ZERO;
			colorAttachmentForward[0].colorBlendOp			= VK_BLEND_OP_ADD;			
			colorAttachmentForward[0].srcAlphaBlendFactor	= VK_BLEND_FACTOR_ONE;	
			colorAttachmentForward[0].dstAlphaBlendFactor	= VK_BLEND_FACTOR_ZERO;
			colorAttachmentForward[0].alphaBlendOp			= VK_BLEND_OP_ADD;

			colorAttachmentDeferred[0] = colorAttachmentForward[0];
			colorAttachmentDeferred[1] = colorAttachmentForward[0];
			colorAttachmentDeferred[2] = colorAttachmentForward[0];

			colorBlending.sType				= VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			colorBlending.logicOpEnable		= VK_FALSE;
			colorBlending.logicOp			= VK_LOGIC_OP_COPY;			
			colorBlending.attachmentCount	= static_cast<uint32_t>(colorAttachmentForward.size());
			colorBlending.pAttachments		= colorAttachmentForward.data();
			colorBlending.blendConstants[0] = 0.0f;				
			colorBlending.blendConstants[1] = 0.0f;				
			colorBlending.blendConstants[2] = 0.0f;				
			colorBlending.blendConstants[3] = 0.0f;				

			pipelineInfo.sType				 = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipelineInfo.stageCount			 = (uint32_t)shaderStage.size();
			pipelineInfo.pStages			 = shaderStage.data();
			pipelineInfo.pVertexInputState   = &vertexInputInfo;
			pipelineInfo.pInputAssemblyState = &inputAssembly;
			pipelineInfo.pViewportState		 = &viewportState;
			pipelineInfo.pRasterizationState = &rasterizer;
			pipelineInfo.pMultisampleState	 = &multisampling;
			pipelineInfo.pDepthStencilState  = &depthStencil;
			pipelineInfo.pColorBlendState	 = &colorBlending;
			pipelineInfo.pDynamicState		 = nullptr;					
			pipelineInfo.subpass			 = 0;
			pipelineInfo.basePipelineHandle  = VK_NULL_HANDLE;		
			pipelineInfo.basePipelineIndex	 = -1;
		}

		/*ds_f*/
		{
			pipelineInfo.renderPass = InitBuffer.getRender(bufferEnum::frameNodeId::ds_f);

			multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

			colorBlending.attachmentCount = static_cast<uint32_t>(colorAttachmentDeferred.size());
			colorBlending.pAttachments = colorAttachmentDeferred.data();

			//terrain_p
			procPipeline.createPipeline<terrainVertex::vertexNode>(Device.device, pipelineEnum::procPipelineId::terrain_p, shaderStage, vertexInputInfo, pipelineInfo);

			//model_p
			procPipeline.createPipeline<modelVertex::vertexNode>(Device.device, pipelineEnum::procPipelineId::model_p, shaderStage, vertexInputInfo, pipelineInfo);

			//skybox_p
			{
				//实现只在空白处显示
				VkPipelineDepthStencilStateCreateInfo tempDepthStencil = depthStencil;
				tempDepthStencil.depthWriteEnable = VK_FALSE;
				tempDepthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

				pipelineInfo.pDepthStencilState = &tempDepthStencil;

				procPipeline.createPipeline<skyboxVertex::vertexNode>(Device.device, pipelineEnum::procPipelineId::skybox_p, shaderStage, vertexInputInfo, pipelineInfo);

				pipelineInfo.pDepthStencilState = &depthStencil;
			}

			//mine_p
			{
				//线段装配
				VkPipelineInputAssemblyStateCreateInfo tempInputAssembly = inputAssembly;
				tempInputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;

				//线填充
				VkPipelineRasterizationStateCreateInfo tempRasterizer = rasterizer;
				tempRasterizer.polygonMode = VK_POLYGON_MODE_LINE;
				tempRasterizer.lineWidth = 1.0f;

				pipelineInfo.pInputAssemblyState = &tempInputAssembly;
				pipelineInfo.pRasterizationState = &tempRasterizer;

				procPipeline.createPipeline<mineVertex::vertexNode>(Device.device, pipelineEnum::procPipelineId::mine_p, shaderStage, vertexInputInfo, pipelineInfo);

				pipelineInfo.pInputAssemblyState = &inputAssembly;
				pipelineInfo.pRasterizationState = &rasterizer;
			}

			//uiText_p
			{
				//条带装配
				VkPipelineInputAssemblyStateCreateInfo tempInputAssembly = inputAssembly;
				tempInputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

				//顺时针正面
				VkPipelineRasterizationStateCreateInfo tempRasterizer = rasterizer;
				tempRasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

				//实现始终渲染在最上层
				VkPipelineDepthStencilStateCreateInfo tempDepthStencil = depthStencil;
				tempDepthStencil.depthTestEnable = VK_FALSE;
				tempDepthStencil.depthWriteEnable = VK_FALSE;

				pipelineInfo.pInputAssemblyState = &tempInputAssembly;
				pipelineInfo.pRasterizationState = &tempRasterizer;
				pipelineInfo.pDepthStencilState = &tempDepthStencil;

				procPipeline.createPipeline<uiTextVertex::vertexNode>(Device.device, pipelineEnum::procPipelineId::uiText_p, shaderStage, vertexInputInfo, pipelineInfo);

				pipelineInfo.pInputAssemblyState = &inputAssembly;
				pipelineInfo.pRasterizationState = &rasterizer;
				pipelineInfo.pDepthStencilState = &depthStencil;
			}

			multisampling.rasterizationSamples = InitBuffer.getMsaaSamples();
			colorBlending.attachmentCount = static_cast<uint32_t>(colorAttachmentForward.size());
			colorBlending.pAttachments = colorAttachmentForward.data();
		}

		/*os_f*/
		{
			pipelineInfo.renderPass = InitBuffer.getRender(bufferEnum::frameNodeId::os_f);

			//osMirror_p
			{
				procPipeline.createPipeline<osMirrorVertex::vertexNode>(Device.device, pipelineEnum::procPipelineId::osMirror_p, shaderStage, vertexInputInfo, pipelineInfo);
			}
		}

		/*sc_f*/
		{
			pipelineInfo.renderPass = InitBuffer.getRender(bufferEnum::frameNodeId::sc_f);

			std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR };
			VkPipelineDynamicStateCreateInfo dynamicState{};
			dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
			dynamicState.pDynamicStates = dynamicStates.data();
			pipelineInfo.pDynamicState = &dynamicState;

			rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;

			//dsScence_p
			procPipeline.createPipeline<offScreenVertex::vertexNode>(Device.device, pipelineEnum::procPipelineId::dsScence_p, shaderStage, vertexInputInfo, pipelineInfo);

			//osScence_p
			procPipeline.createPipeline<offScreenVertex::vertexNode>(Device.device, pipelineEnum::procPipelineId::osScence_p, shaderStage, vertexInputInfo, pipelineInfo);

			pipelineInfo.pDynamicState = nullptr;
			rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		}
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
		createPipeline();

		InitBuffer.createCommandBuffer(InitBuffer.command.currentThreadId, procPipeline);
	}
};