#pragma once



//engine	列表空间
namespace engineList {
	//instance	层表
	inline const engineStruct::instanceLayerNode instanceLayerList[] = {
		{ "VK_LAYER_KHRONOS_validation", true },				//必需：校验层
	};

	//instance	扩展表
	inline const engineStruct::instanceExtensionNode instanceExtensionList[] = {
		{ VK_EXT_DEBUG_UTILS_EXTENSION_NAME, false, true },		//可选：调试，仅校验层开启时启用
	};

	//device	扩展表
	inline const engineStruct::deviceExtensionNode deviceExtensionList[] = {
		{ VK_KHR_SWAPCHAIN_EXTENSION_NAME,         true  },		//必需：呈现
		{ VK_EXT_MESH_SHADER_EXTENSION_NAME,       true  },		//必需：MeshShader
		{ VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME, false },		//可选：descriptor buffer
	};



	struct procTickNode {
		std::vector<engineStruct::tickNode> node{};



		procTickNode() : node(engineEnum::tickEnum::enumCount) {
			node[engineEnum::tickEnum::key]   .createTickNode(1024);
			node[engineEnum::tickEnum::uiText].createTickNode(4);
			node[engineEnum::tickEnum::plate] .createTickNode(4);
			node[engineEnum::tickEnum::cube]  .createTickNode(128);
		}
	};

	procTickNode procTick{};
}



//pipeline	列表空间
namespace pipelineList {
	struct procRenderNode {
		std::vector<renderStruct::renderTemplateNode> renderTemplateList{};
		std::vector<renderStruct::renderInstanceNode> renderInstanceList{};



		procRenderNode(VkFormat colorFormat, VkFormat depthFormat, const VkExtent2D& swapExtent, VkSampleCountFlagBits msaa, const VkDevice& device, const VmaAllocator& memAllocator) : renderTemplateList(renderEnum::renderTemplateId::enumCount), renderInstanceList(renderEnum::renderInstanceId::enumCount) {
			renderTemplateList[renderEnum::renderTemplateId::swapChain_rt].createRenderTemplateNode(renderEnum::renderTemplateId::swapChain_rt, colorFormat, depthFormat, msaa, device);
			renderTemplateList[renderEnum::renderTemplateId::offScreen_rt].createRenderTemplateNode(renderEnum::renderTemplateId::offScreen_rt, colorFormat, depthFormat, msaa, device);
			renderTemplateList[renderEnum::renderTemplateId::deferred_rt] .createRenderTemplateNode(renderEnum::renderTemplateId::deferred_rt,  colorFormat, depthFormat, msaa, device);
			renderTemplateList[renderEnum::renderTemplateId::shadowMap_rt].createRenderTemplateNode(renderEnum::renderTemplateId::shadowMap_rt, colorFormat, depthFormat, msaa, device);

			renderInstanceList[renderEnum::renderInstanceId::scScence_ri] .createRenderInstanceNode(renderTemplateList[renderEnum::renderTemplateId::swapChain_rt], colorFormat, depthFormat, swapExtent, msaa, device, memAllocator);
			renderInstanceList[renderEnum::renderInstanceId::osScence_ri] .createRenderInstanceNode(renderTemplateList[renderEnum::renderTemplateId::offScreen_rt], colorFormat, depthFormat, swapExtent, msaa, device, memAllocator);
			renderInstanceList[renderEnum::renderInstanceId::osMirror_ri] .createRenderInstanceNode(renderTemplateList[renderEnum::renderTemplateId::offScreen_rt], colorFormat, depthFormat, swapExtent, msaa, device, memAllocator);
			renderInstanceList[renderEnum::renderInstanceId::dsScence_ri] .createRenderInstanceNode(renderTemplateList[renderEnum::renderTemplateId::deferred_rt],  colorFormat, depthFormat, swapExtent, msaa, device, memAllocator);
			renderInstanceList[renderEnum::renderInstanceId::shadowMap_ri].createRenderInstanceNode(renderTemplateList[renderEnum::renderTemplateId::shadowMap_rt], colorFormat, depthFormat, renderInfo::shadowMapExtent, msaa, device, memAllocator);
		}

		procRenderNode() = delete;



		void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
			for (renderStruct::renderTemplateNode& renderTemplate : renderTemplateList) {
				renderTemplate.clear(device);
			}

			for (renderStruct::renderInstanceNode& renderInstance : renderInstanceList) {
				renderInstance.clear(device, memAllocator);
			}
		}

		void recreateSwapChain(const VkExtent2D& swapExtent, VkFormat colorFormat, VkFormat depthFormat, VkSampleCountFlagBits msaa, const VkDevice& device, const VmaAllocator& memAllocator) {
			renderInstanceList[renderEnum::renderInstanceId::scScence_ri] .createRenderInstanceNode(renderTemplateList[renderEnum::renderTemplateId::swapChain_rt], colorFormat, depthFormat, swapExtent, msaa, device, memAllocator);
			renderInstanceList[renderEnum::renderInstanceId::osScence_ri] .createRenderInstanceNode(renderTemplateList[renderEnum::renderTemplateId::offScreen_rt], colorFormat, depthFormat, swapExtent, msaa, device, memAllocator);
			renderInstanceList[renderEnum::renderInstanceId::osMirror_ri] .createRenderInstanceNode(renderTemplateList[renderEnum::renderTemplateId::offScreen_rt], colorFormat, depthFormat, swapExtent, msaa, device, memAllocator);
			renderInstanceList[renderEnum::renderInstanceId::dsScence_ri] .createRenderInstanceNode(renderTemplateList[renderEnum::renderTemplateId::deferred_rt],  colorFormat, depthFormat, swapExtent, msaa, device, memAllocator);
			renderInstanceList[renderEnum::renderInstanceId::shadowMap_ri].createRenderInstanceNode(renderTemplateList[renderEnum::renderTemplateId::shadowMap_rt], colorFormat, depthFormat, renderInfo::shadowMapExtent, msaa, device, memAllocator);
		}

		void clearSwapChain(const VkDevice& device, const VmaAllocator& memAllocator) {
			for (renderStruct::renderInstanceNode& renderInstance : renderInstanceList) {
				renderInstance.clear(device, memAllocator);
			}
		}



		//获取 frameImage，为创建描述符服务
		const VkImageView& getFrameImage(renderEnum::renderInstanceId::type renderInstanceType, renderEnum::renderFrameId::type renderFrameType, int viewId) {
			return renderInstanceList[renderInstanceType].frameImageList[renderFrameType].imageList[viewId].view;
		}

		//获取 renderBegin
		const VkRenderPassBeginInfo& getRenderBegin(renderEnum::renderInstanceId::type renderInstanceType) {
			return renderInstanceList[renderInstanceType].frameBufferList[0].renderBegin;
		}

		//获取 renderBegin，swap版
		const VkRenderPassBeginInfo& getRenderBegin(renderEnum::renderInstanceId::type renderInstanceType, int frameId) {
			return renderInstanceList[renderInstanceType].frameBufferList[frameId].renderBegin;
		}
	};

	struct procSamplerNode {
		std::vector<samplerStruct::samplerConfigNode> config{};
		std::vector<VkSampler>                        node{};



		procSamplerNode(const VkDevice& device, const VkPhysicalDeviceLimits& limits)
			: config(samplerEnum::procSamplerId::enumCount), node(samplerEnum::procSamplerId::enumCount) {
			createSamplerList(device, limits);
		}



		//创建 全部采样器
		void createSamplerList(const VkDevice& device, const VkPhysicalDeviceLimits& limits) {
			if (limits.maxSamplerAnisotropy < 1.0f) {
				Log::printError(__FILE__, __LINE__, "设备限制非法：maxSamplerAnisotropy = " + std::to_string(limits.maxSamplerAnisotropy) + "（应 ≥1；多半是 enableGpuInfo 未被正确填充）");
			}

			const std::vector<samplerStruct::samplerConfigParamNode> presetList{
				/* terrain_s        */ { .magType = samplerEnum::magId::nearest, .lodSourceType = samplerEnum::lodSourceId::clampNone },
				/* staticModel_sn   */ { .magType = samplerEnum::magId::nearest, .lodSourceType = samplerEnum::lodSourceId::clampNone },
				/* staticModel_sl   */ { .lodSourceType = samplerEnum::lodSourceId::clampNone },
				/* dynamicModel_sn  */ { .magType = samplerEnum::magId::nearest, .lodSourceType = samplerEnum::lodSourceId::clampNone },
				/* dynamicModel_sl  */ { .lodSourceType = samplerEnum::lodSourceId::clampNone },
				/* uiText_s         */ { .addressType = samplerEnum::addressId::clampEdge, .anisoType = samplerEnum::anisoId::off },	//无 mip ⇒ 各向异性无意义，显式关掉
				/* offScreen_s      */ { .addressType = samplerEnum::addressId::clampEdge, .anisoType = samplerEnum::anisoId::x1 },
				/* shadowMap_s      */ { .magType = samplerEnum::magId::nearest, .mipmapType = samplerEnum::mipmapId::linear, .addressType = samplerEnum::addressId::clampEdge, .anisoType = samplerEnum::anisoId::off , .compareType = samplerEnum::compareId::lessOrEqual, .borderColorType = samplerEnum::borderColorId::floatOpaqueWhite },
			};

			//dev 期自检：预设表与 procSamplerId
			if (debugInfo::LogSwitch && presetList.size() != static_cast<size_t>(samplerEnum::procSamplerId::enumCount)) {
				Log::printError(__FILE__, __LINE__, "采样器预设表数量与 procSamplerId 不一致");
			}

			for (uint32_t i = 0; i < static_cast<uint32_t>(presetList.size()); i++) {
				config[i].createSamplerConfigNode(presetList[i]);
				createSampler(device, limits, static_cast<samplerEnum::procSamplerId::type>(i), node[i], config[i].samplerInfo);
			}
		}

		//创建 单个采样器
		void createSampler(const VkDevice& device, const VkPhysicalDeviceLimits& limits, const samplerEnum::procSamplerId::type& samplerId, VkSampler& sampler, const VkSamplerCreateInfo& configInfo) {
			VkSamplerCreateInfo samplerInfo = configInfo;
			//设备限制夹取（先把上下界规整：std::clamp 在 lo > hi 时是 UB，Rel 的 -O2 会把它放大成任意结果）
			const float maxAniso      = (limits.maxSamplerAnisotropy >= 1.0f)	 ? limits.maxSamplerAnisotropy : 1.0f;
			const float lodBiasLimit  = (limits.maxSamplerLodBias > 0.0f)		 ? limits.maxSamplerLodBias  : 0.0f;
			samplerInfo.maxAnisotropy = (samplerInfo.maxAnisotropy < maxAniso)	 ? samplerInfo.maxAnisotropy : maxAniso;
			samplerInfo.mipLodBias    = (samplerInfo.mipLodBias < -lodBiasLimit) ? -lodBiasLimit : ((samplerInfo.mipLodBias > lodBiasLimit) ? lodBiasLimit : samplerInfo.mipLodBias);

			if (vkCreateSampler(device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
				Log::printError(__FILE__, __LINE__, "采样器创建失败，id " + std::to_string(static_cast<int>(samplerId)));
			}
		}

		void clear(const VkDevice& device) {
			for (VkSampler& sampler : node) {
				if (sampler != VK_NULL_HANDLE) {
					vkDestroySampler(device, sampler, nullptr);
					sampler = VK_NULL_HANDLE;
				}
			}
		}
	};

	struct procDescNode {
		std::vector<descStruct::descNode> node{};



		procDescNode(cmdStruct::cmdNode& cmd, const VkDevice& device, const VkPhysicalDevice& gpu, const VkQueue& graphicsQueue, const VmaAllocator& memAllocator, PFN_vkGetDescriptorSetLayoutSizeEXT tPfnSize, PFN_vkGetDescriptorSetLayoutBindingOffsetEXT tPfnOffset, PFN_vkGetDescriptorEXT tPfnGet) : node(descEnum::descType::enumCount) {
			for (descStruct::descNode& n : node) {
				n.descBuffer.pfnGetSetLayoutSize = tPfnSize;
				n.descBuffer.pfnGetBindingOffset = tPfnOffset;
				n.descBuffer.pfnGetDescriptor    = tPfnGet;
			}

			node[descEnum::descType::scenceForward_d] .maxSet = pipelineInfo::scenceForwardCount;
			node[descEnum::descType::scenceDeferred_d].maxSet = pipelineInfo::scenceDeferredCount;
			node[descEnum::descType::uiText_d]		  .maxSet = pipelineInfo::uiTextCount;
			node[descEnum::descType::dsScence_d]	  .maxSet = pipelineInfo::dsScenceCount;
			node[descEnum::descType::shadowMap_d]	  .maxSet = pipelineInfo::shadowCount;

			node[descEnum::descType::scenceDeferred_d].createReflectConfig(pipelineEnum::procPipelineId::terrain_p,    { fileInfo::shader::terrainVert,  fileInfo::shader::terrainFrag  });
			node[descEnum::descType::scenceDeferred_d].createReflectConfig(pipelineEnum::procPipelineId::model_p,      { fileInfo::shader::pbrVert,      fileInfo::shader::pbrFrag	  });
			node[descEnum::descType::scenceForward_d] .createReflectConfig(pipelineEnum::procPipelineId::skybox_p,     { fileInfo::shader::skyboxVert,   fileInfo::shader::skyboxFrag   });
			node[descEnum::descType::scenceForward_d] .createReflectConfig(pipelineEnum::procPipelineId::mine_p,       { fileInfo::shader::mineVert,     fileInfo::shader::mineFrag	  });
			node[descEnum::descType::scenceForward_d] .createReflectConfig(pipelineEnum::procPipelineId::osMirror_p,   { fileInfo::shader::osMirrorVert, fileInfo::shader::osMirrorFrag });
			node[descEnum::descType::scenceForward_d] .createReflectConfig(pipelineEnum::procPipelineId::osScence_p,   { fileInfo::shader::osScenceVert, fileInfo::shader::osScenceFrag });
			node[descEnum::descType::uiText_d]		  .createReflectConfig(pipelineEnum::procPipelineId::uiText_p,     { fileInfo::shader::uiTextVert,   fileInfo::shader::uiTextFrag   });
			node[descEnum::descType::dsScence_d]	  .createReflectConfig(pipelineEnum::procPipelineId::dsScence_p,   { fileInfo::shader::dsScenceVert, fileInfo::shader::dsScenceFrag });
			node[descEnum::descType::shadowMap_d]	  .createReflectConfig(pipelineEnum::procPipelineId::shadowMap_p,  { fileInfo::shader::shadowVert,   fileInfo::shader::shadowFrag   });

			for (int i = 0; i < descEnum::descType::enumCount; i++) {
				node[i].createReflectLayout(static_cast<descEnum::descType::type>(i), device);
			}
		}

		procDescNode() = delete;



		void clear(const VkDevice& device) {
			for (descStruct::descNode& desc : node) {
				desc.clear(device);
			}
		}
	};



	struct procPipelineNode {
		const std::vector<renderStruct::renderTemplateNode>& renderTemplateList;
		const std::vector<descStruct::descNode>&			 descList;
		VkSampleCountFlagBits                                msaa{};
		VkDevice											 device{};

		std::vector<pipelineStruct::pipeLayoutNode>			 nodeLayout{};
		std::vector<pipelineStruct::pipelineNode>			 node{};



		procPipelineNode(const std::vector<renderStruct::renderTemplateNode>& tRenderList, const std::vector<descStruct::descNode>& tDescList, VkSampleCountFlagBits tMsaa, const VkDevice& tDevice)
			: nodeLayout(descEnum::descType::enumCount), node(pipelineEnum::procPipelineId::enumCount), renderTemplateList(tRenderList), descList(tDescList), msaa(tMsaa), device(tDevice) {
			for (int i = 0; i < descEnum::descType::enumCount; i++) {
				nodeLayout[i].createPipeLayoutNode(device, tDescList[i].layout);
			}

			createPipelineNode(pipelineEnum::procPipelineId::terrain_p,   renderEnum::renderTemplateId::deferred_rt,  descEnum::descType::scenceDeferred_d, vertexStruct::terrainVertex::vertexNode::layout,   pipelineEnum::shaderFlowId::vert, pipelineStruct::pipeConfigParamNode{  });
			createPipelineNode(pipelineEnum::procPipelineId::model_p,	  renderEnum::renderTemplateId::deferred_rt,  descEnum::descType::scenceDeferred_d, vertexStruct::modelVertex::vertexNode::layout,     pipelineEnum::shaderFlowId::vert, pipelineStruct::pipeConfigParamNode{  });
			createPipelineNode(pipelineEnum::procPipelineId::skybox_p,	  renderEnum::renderTemplateId::deferred_rt,  descEnum::descType::scenceForward_d,  vertexStruct::skyboxVertex::vertexNode::layout,    pipelineEnum::shaderFlowId::vert, pipelineStruct::pipeConfigParamNode{ .depthType = pipelineEnum::depthId::test_noWrite_lessEqual });
			createPipelineNode(pipelineEnum::procPipelineId::mine_p,	  renderEnum::renderTemplateId::deferred_rt,  descEnum::descType::scenceForward_d,  vertexStruct::mineVertex::vertexNode::layout,      pipelineEnum::shaderFlowId::vert, pipelineStruct::pipeConfigParamNode{ .primitiveType = pipelineEnum::primitiveId::lineList, .rasterType = pipelineEnum::rasterId::line_back_ccw });
			createPipelineNode(pipelineEnum::procPipelineId::uiText_p,	  renderEnum::renderTemplateId::swapChain_rt, descEnum::descType::uiText_d,			vertexStruct::uiTextVertex::vertexNode::layout,    pipelineEnum::shaderFlowId::vert, pipelineStruct::pipeConfigParamNode{ .primitiveType = pipelineEnum::primitiveId::triangleList, .rasterType = pipelineEnum::rasterId::fill_back_cw, .depthType = pipelineEnum::depthId::noDepth, .blendType = pipelineEnum::blendId::alphaBlend });
			createPipelineNode(pipelineEnum::procPipelineId::osMirror_p,  renderEnum::renderTemplateId::offScreen_rt, descEnum::descType::scenceForward_d,  vertexStruct::osMirrorVertex::vertexNode::layout,  pipelineEnum::shaderFlowId::vert, pipelineStruct::pipeConfigParamNode{  });
			createPipelineNode(pipelineEnum::procPipelineId::osScence_p,  renderEnum::renderTemplateId::swapChain_rt, descEnum::descType::scenceForward_d,  vertexStruct::offScreenVertex::vertexNode::layout, pipelineEnum::shaderFlowId::vert, pipelineStruct::pipeConfigParamNode{ .rasterType = pipelineEnum::rasterId::fill_front_ccw, .dynamicType = pipelineEnum::dynamicId::viewportScissor });
			createPipelineNode(pipelineEnum::procPipelineId::dsScence_p,  renderEnum::renderTemplateId::swapChain_rt, descEnum::descType::dsScence_d,		vertexStruct::offScreenVertex::vertexNode::layout, pipelineEnum::shaderFlowId::vert, pipelineStruct::pipeConfigParamNode{ .rasterType = pipelineEnum::rasterId::fill_front_ccw, .dynamicType = pipelineEnum::dynamicId::viewportScissor });
			createPipelineNode(pipelineEnum::procPipelineId::shadowMap_p, renderEnum::renderTemplateId::shadowMap_rt, descEnum::descType::shadowMap_d,		vertexStruct::shadowVertex::vertexNode::layout,	   pipelineEnum::shaderFlowId::vert, pipelineStruct::pipeConfigParamNode{ .depthType = pipelineEnum::depthId::test_write_lessEqual });

			if (debugInfo::LogSwitch) {
				checkPipeline();
			}

			for (pipelineStruct::pipelineNode& pipeNode : node) {
				pipeNode.isRecreate = true;
			}

			createPipeCache(device);
		}



		void createPipelineNode(pipelineEnum::procPipelineId::type tPipeType, renderEnum::renderTemplateId::type tRenderTemplateType, descEnum::descType::type tDescType, const vertexStruct::vertexLayoutNode& tVertexLayout, pipelineEnum::shaderFlowId::type tShaderFlowType, const pipelineStruct::pipeConfigParamNode& tParam) {
			node[tPipeType].createPipelineNode(tShaderFlowType, descList[tDescType].getShaderFileList(tPipeType), tVertexLayout, tRenderTemplateType, tDescType, msaa, renderTemplateList[tRenderTemplateType], nodeLayout[tDescType], tParam, device);
		}

		void createPipeline(pipelineEnum::procPipelineId::type tPipeType, const VkExtent2D& extent) {
			if (!node[tPipeType].isRecreate) {
				return;
			}

			//配置 pipeConfig
			pipelineStruct::pipeConfigNode& pipeConfig = node[tPipeType].pipeConfig;
			pipeConfig.viewport.width  = static_cast<float>(extent.width);
			pipeConfig.viewport.height = static_cast<float>(extent.height);
			pipeConfig.scissor.extent  = extent;

			//创建 pipeline
			if (vkCreateGraphicsPipelines(device, pipeCache, 1, &pipeConfig.pipeInfo, nullptr, &node[tPipeType].pipeline) != VK_SUCCESS) {
				Log::printError(__FILE__, __LINE__, "管线创建失败");
			}

			node[tPipeType].isRecreate = false;
		}

		void clear(const VkDevice& device) {
			for (pipelineStruct::pipelineNode& pipeNode : node) {
				pipeNode.clear(device);
			}

			for (pipelineStruct::pipeLayoutNode& layoutNode : nodeLayout) {
				layoutNode.clear(device); 
			}

			savePipeCache(device);
			vkDestroyPipelineCache(device, pipeCache, nullptr);
		}

		void clearSwapChain(const VkDevice& device) {
			for (pipelineStruct::pipelineNode& pipeNode : node) {
				if (pipeNode.renderTemplateType != renderEnum::renderTemplateId::shadowMap_rt && !isViewportDynamic(pipeNode.dynamicType)) {
					pipeNode.isRecreate = true;
					pipeNode.clearSwapChain(device);
				}
			}
		}

	private:
		VkPipelineCache pipeCache{};



		void createPipeCache(const VkDevice& device){
			std::vector<char> cacheData{};
			std::ifstream	  cacheFile(fileInfo::path::pipeCacheFile, std::ios::ate | std::ios::binary);

			if (cacheFile.is_open()) {
				const size_t cacheSize = static_cast<size_t>(cacheFile.tellg());

				if (cacheSize > 0) {
					cacheData.resize(cacheSize);
					cacheFile.seekg(0);
					cacheFile.read(cacheData.data(), cacheSize);
				}

				cacheFile.close();
			}

			VkPipelineCacheCreateInfo pipeCacheInfo{};
			pipeCacheInfo.sType           = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
			pipeCacheInfo.initialDataSize = cacheData.size();
			pipeCacheInfo.pInitialData    = cacheData.empty() ? nullptr : cacheData.data();

			if (vkCreatePipelineCache(device, &pipeCacheInfo, nullptr, &pipeCache) != VK_SUCCESS) {
				pipeCache = VK_NULL_HANDLE;
				Log::printWarning(__FILE__, __LINE__, "管线缓存创建失败，本帧起无缓存继续");
			}
		}

		void savePipeCache(const VkDevice& device) {
			if (pipeCache == VK_NULL_HANDLE) {
				return;
			}

			size_t cacheSize = 0;
			if (vkGetPipelineCacheData(device, pipeCache, &cacheSize, nullptr) != VK_SUCCESS || cacheSize == 0) {
				return;
			}

			std::vector<char> cacheData(cacheSize);
			if (vkGetPipelineCacheData(device, pipeCache, &cacheSize, cacheData.data()) != VK_SUCCESS) {
				return;
			}

			std::error_code dirError{};
			std::filesystem::create_directories(std::filesystem::path(fileInfo::path::pipeCacheFile).parent_path(), dirError);
			std::ofstream cacheFile(fileInfo::path::pipeCacheFile, std::ios::binary | std::ios::trunc);
			if (cacheFile.is_open()) {
				cacheFile.write(cacheData.data(), static_cast<std::streamsize>(cacheSize));
				cacheFile.close();
			}
		}
		
		void checkPipeline() {
			for (int i = 0; i < node.size(); i++) {
				if (node[i].shaderFileList.empty()) { 
					Log::printError(__FILE__, __LINE__, "管线 " + std::to_string(i) + " 未配置着色器文件"); 
				}
			}
		
			for (int descId = 0; descId < nodeLayout.size(); descId++) {
				if (node[0].descType == descEnum::descType::enumNull) { break; }

				bool used = false;
				for (const pipelineStruct::pipelineNode& pipeNode : node) {
					if (static_cast<int>(pipeNode.descType) == descId) {
						used = true; break;
					}
				}

				if (!used) { 
					Log::printError(__FILE__, __LINE__, "描述符 " + std::to_string(descId) + " 未被管线使用"); 
				}
			}
		}
		
		bool isViewportDynamic(pipelineEnum::dynamicId::type dynamicType) {
			switch (dynamicType) {
			case pipelineEnum::dynamicId::none:							return false;
			case pipelineEnum::dynamicId::viewportScissor:				return true;
			case pipelineEnum::dynamicId::viewportScissorLineWidth:		return true;
			case pipelineEnum::dynamicId::viewportScissorDepthBias:		return true;
			case pipelineEnum::dynamicId::viewportScissorBounds:		return true;
			case pipelineEnum::dynamicId::viewportScissorRasterDepth:	return true;
			case pipelineEnum::dynamicId::viewportScissorAll:			return true;
			}

			return false;
		}

	public:
		VkPipelineLayout& getPipeLayout(pipelineEnum::procPipelineId::type pipeType) {
			return nodeLayout[node[pipeType].descType].pipeLayout;
		}

		VkPipeline& getPipeline(pipelineEnum::procPipelineId::type pipeType) {
			return node[pipeType].pipeline;
		}
	};
}