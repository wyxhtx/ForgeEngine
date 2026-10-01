#pragma once



//engine	列表空间
namespace engineList {
	struct procTickNode {
		std::vector<engineStruct::tickNode> node{};



		procTickNode() : node(engineEnum::tickEnum::count) {
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



		procRenderNode(VkFormat colorFormat, VkFormat depthFormat, const VkExtent2D& swapExtent, VkSampleCountFlagBits msaa, const VkDevice& device, const VmaAllocator& memAllocator) : renderTemplateList(renderEnum::renderTemplateId::count), renderInstanceList(renderEnum::renderInstanceId::count) {
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

	struct procDescNode {
		VkDescriptorSetAllocateInfo		  allocInfo{};
		bufferEnum::procDescType::type    descType = bufferEnum::procDescType::count;
		std::vector<descStruct::descNode> node{};



		procDescNode(const VkDevice& device) : node(bufferEnum::procDescType::count) {
			allocInfo.sType				 = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.pNext				 = nullptr;
			allocInfo.descriptorSetCount = 1;
			
			node[bufferEnum::procDescType::scenceForward_d] .maxSet = pipelineInfo::scenceForwardCount;
			node[bufferEnum::procDescType::scenceDeferred_d].maxSet = pipelineInfo::scenceDeferredCount;
			node[bufferEnum::procDescType::uiText_d]		.maxSet = pipelineInfo::uiTextCount;
			node[bufferEnum::procDescType::dsScence_d]		.maxSet = pipelineInfo::dsScenceCount;
			node[bufferEnum::procDescType::shadowMap_d]		.maxSet = pipelineInfo::shadowCount;

			node[bufferEnum::procDescType::scenceDeferred_d].createReflectConfig(pipelineEnum::procPipelineId::terrain_p,    { fileInfo::shader::terrainVert,  fileInfo::shader::terrainFrag  });
			node[bufferEnum::procDescType::scenceDeferred_d].createReflectConfig(pipelineEnum::procPipelineId::model_p,      { fileInfo::shader::pbrVert,      fileInfo::shader::pbrFrag	  });
			node[bufferEnum::procDescType::scenceForward_d] .createReflectConfig(pipelineEnum::procPipelineId::skybox_p,     { fileInfo::shader::skyboxVert,   fileInfo::shader::skyboxFrag   });
			node[bufferEnum::procDescType::scenceForward_d] .createReflectConfig(pipelineEnum::procPipelineId::mine_p,       { fileInfo::shader::mineVert,     fileInfo::shader::mineFrag	  });
			node[bufferEnum::procDescType::scenceForward_d] .createReflectConfig(pipelineEnum::procPipelineId::osMirror_p,   { fileInfo::shader::osMirrorVert, fileInfo::shader::osMirrorFrag });
			node[bufferEnum::procDescType::scenceForward_d] .createReflectConfig(pipelineEnum::procPipelineId::osScence_p,   { fileInfo::shader::osScenceVert, fileInfo::shader::osScenceFrag });
			node[bufferEnum::procDescType::uiText_d]		.createReflectConfig(pipelineEnum::procPipelineId::uiText_p,     { fileInfo::shader::uiTextVert,   fileInfo::shader::uiTextFrag   });
			node[bufferEnum::procDescType::dsScence_d]		.createReflectConfig(pipelineEnum::procPipelineId::dsScence_p,   { fileInfo::shader::dsScenceVert, fileInfo::shader::dsScenceFrag });
			node[bufferEnum::procDescType::shadowMap_d]		.createReflectConfig(pipelineEnum::procPipelineId::shadowMap_p,  { fileInfo::shader::shadowVert,   fileInfo::shader::shadowFrag   });

			for (int i = 0; i < bufferEnum::procDescType::count; i++) {
				node[i].createReflectLayout(static_cast<bufferEnum::procDescType::type>(i), device);
			}
		}

		procDescNode() = delete;



		void clear(const VkDevice& device) {
			for (descStruct::descNode& desc : node) {
				desc.clear(device);
			}
		}

		void setAllocInfo(bufferEnum::procDescType::type tDescType) {
			allocInfo.descriptorPool = node[tDescType].pool;
			allocInfo.pSetLayouts    = &node[tDescType].layout;

			descType = tDescType;
		}

		const VkWriteDescriptorSet& getWrite(int writeId, const VkDescriptorSet& dstSet, const VkBuffer& buffer, VkDeviceSize range) {
			if (debugInfo::LogSwitch && descType >= bufferEnum::procDescType::count) { Log::printError(__FILE__, __LINE__, "getWrite 前必须先 setAllocInfo（当前家族未设置）"); }

			return node[descType].descConfig[writeId].setWrite(dstSet, buffer, range);
		}

		const VkWriteDescriptorSet& getWrite(int writeId, const VkDescriptorSet& dstSet, const VkSampler& sampler, const VkImageView& imageView) {
			if (debugInfo::LogSwitch && descType >= bufferEnum::procDescType::count) { Log::printError(__FILE__, __LINE__, "getWrite 前必须先 setAllocInfo（当前家族未设置）"); }

			return node[descType].descConfig[writeId].setWrite(dstSet, sampler, imageView);
		}
	};

	struct procSamplerNode {
		std::vector<VkSampler> node{};



		procSamplerNode() : node(bufferEnum::procSamplerId::count) {}



		void clear(const VkDevice& device) {
			for (const VkSampler& sampler : node) {
				vkDestroySampler(device, sampler, nullptr);
			}
		}

		void createTextureSampler(const VkDevice& device, uint32_t terrainMip, uint32_t sModelMip, uint32_t dModelMip) {
			VkSamplerCreateInfo samplerInfo{};
			{
				samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

				//过滤器即采用何种方式过滤图片
				samplerInfo.minFilter = VK_FILTER_LINEAR;

				//每个轴向使用的寻址模式全部采用镜像循环
				samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
				samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
				samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

				//各向异性过滤器
				samplerInfo.anisotropyEnable = VK_TRUE;
				samplerInfo.maxAnisotropy    = 16.0f;

				//采样范围超过图像时候返回的颜色
				samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

				//指定使用的坐标系统，用于访问图像的纹素
				samplerInfo.unnormalizedCoordinates = VK_FALSE;

				//开启比较功能，那么纹素首先和值进行比较，并且比较后的值用于过滤操作
				samplerInfo.compareEnable = VK_FALSE;
				samplerInfo.compareOp     = VK_COMPARE_OP_ALWAYS;

				samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
				samplerInfo.mipLodBias = 0.0f;
				samplerInfo.minLod     = 0.0f;
			}

			//terrain_s
			samplerInfo.magFilter = VK_FILTER_NEAREST;
			samplerInfo.maxLod    = static_cast<float>(terrainMip);
			if (vkCreateSampler(device, &samplerInfo, nullptr, &node[bufferEnum::procSamplerId::terrain_s]) != VK_SUCCESS) {
				Log::printError(__FILE__, __LINE__, "terrain采样器创建失败");
			}

			//staticModel_sn
			samplerInfo.magFilter = VK_FILTER_NEAREST;
			samplerInfo.maxLod    = static_cast<float>(sModelMip);
			if (vkCreateSampler(device, &samplerInfo, nullptr, &node[bufferEnum::procSamplerId::staticModel_sn]) != VK_SUCCESS) {
				Log::printError(__FILE__, __LINE__, "sModelN采样器创建失败");
			}

			//staticModel_sl
			samplerInfo.magFilter = VK_FILTER_LINEAR;
			if (vkCreateSampler(device, &samplerInfo, nullptr, &node[bufferEnum::procSamplerId::staticModel_sl]) != VK_SUCCESS) {
				Log::printError(__FILE__, __LINE__, "sModelL采样器创建失败");
			}

			//dynamicModel_sn
			samplerInfo.magFilter = VK_FILTER_NEAREST;
			samplerInfo.maxLod    = static_cast<float>(dModelMip);
			if (vkCreateSampler(device, &samplerInfo, nullptr, &node[bufferEnum::procSamplerId::dynamicModel_sn]) != VK_SUCCESS) {
				Log::printError(__FILE__, __LINE__, "dModelN采样器创建失败");
			}

			//dynamicModel_sl
			samplerInfo.magFilter = VK_FILTER_LINEAR;
			if (vkCreateSampler(device, &samplerInfo, nullptr, &node[bufferEnum::procSamplerId::dynamicModel_sl]) != VK_SUCCESS) {
				Log::printError(__FILE__, __LINE__, "dModelL采样器创建失败");
			}

			//uiText_s
			samplerInfo.magFilter     = VK_FILTER_LINEAR;
			samplerInfo.maxAnisotropy = 16.0f;
			//MSDF 图集：CLAMP 防跨字形出血（REPEAT 会在 uv=1 边缘采到另一侧）；无 mip → maxLod=0
			samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerInfo.maxLod       = 0.0f;
			if (vkCreateSampler(device, &samplerInfo, nullptr, &node[bufferEnum::procSamplerId::uiText_s]) != VK_SUCCESS) {
				Log::printError(__FILE__, __LINE__, "uiText采样器创建失败");
			}

			//offScreen_s
			samplerInfo.magFilter     = VK_FILTER_LINEAR;
			samplerInfo.maxLod        = 0.0f;
			samplerInfo.maxAnisotropy = 1.0f;
			if (vkCreateSampler(device, &samplerInfo, nullptr, &node[bufferEnum::procSamplerId::offScreen_s]) != VK_SUCCESS) {
				Log::printError(__FILE__, __LINE__, "offScreen采样器创建失败");
			}

			//shadowMap_s
			samplerInfo.magFilter        = VK_FILTER_NEAREST;
			samplerInfo.addressModeU     = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerInfo.addressModeV     = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerInfo.addressModeW     = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerInfo.anisotropyEnable = VK_FALSE;
			samplerInfo.maxAnisotropy    = 0;
			samplerInfo.mipmapMode       = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			samplerInfo.mipLodBias       = 0;
			samplerInfo.minLod           = 0.0f;
			samplerInfo.maxLod           = 0.0f;
			if (vkCreateSampler(device, &samplerInfo, nullptr, &node[bufferEnum::procSamplerId::shadowMap_s]) != VK_SUCCESS) {
				Log::printError(__FILE__, __LINE__, "shadow采样器创建失败");
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
			: nodeLayout(bufferEnum::procDescType::count), node(pipelineEnum::procPipelineId::count), renderTemplateList(tRenderList), descList(tDescList), msaa(tMsaa), device(tDevice) {
			for (int i = 0; i < bufferEnum::procDescType::count; i++) {
				nodeLayout[i].createPipeLayoutNode(device, tDescList[i].layout);
			}

			createPipelineNode(pipelineEnum::procPipelineId::terrain_p,   renderEnum::renderTemplateId::deferred_rt,  bufferEnum::procDescType::scenceDeferred_d, vertexStruct::terrainVertex::vertexNode::layout,   pipelineEnum::shaderFlowId::vert, pipelineStruct::pipeConfigParamNode{  });
			createPipelineNode(pipelineEnum::procPipelineId::model_p,	  renderEnum::renderTemplateId::deferred_rt,  bufferEnum::procDescType::scenceDeferred_d, vertexStruct::modelVertex::vertexNode::layout,     pipelineEnum::shaderFlowId::vert, pipelineStruct::pipeConfigParamNode{  });
			createPipelineNode(pipelineEnum::procPipelineId::skybox_p,	  renderEnum::renderTemplateId::deferred_rt,  bufferEnum::procDescType::scenceForward_d,  vertexStruct::skyboxVertex::vertexNode::layout,    pipelineEnum::shaderFlowId::vert, pipelineStruct::pipeConfigParamNode{ .depthType = pipelineEnum::depthId::test_noWrite_lessEqual });
			createPipelineNode(pipelineEnum::procPipelineId::mine_p,	  renderEnum::renderTemplateId::deferred_rt,  bufferEnum::procDescType::scenceForward_d,  vertexStruct::mineVertex::vertexNode::layout,      pipelineEnum::shaderFlowId::vert, pipelineStruct::pipeConfigParamNode{ .primitiveType = pipelineEnum::primitiveId::lineList, .rasterType = pipelineEnum::rasterId::line_back_ccw });
			createPipelineNode(pipelineEnum::procPipelineId::uiText_p,	  renderEnum::renderTemplateId::swapChain_rt, bufferEnum::procDescType::uiText_d,		  vertexStruct::uiTextVertex::vertexNode::layout,    pipelineEnum::shaderFlowId::vert, pipelineStruct::pipeConfigParamNode{ .primitiveType = pipelineEnum::primitiveId::triangleList, .rasterType = pipelineEnum::rasterId::fill_back_cw, .depthType = pipelineEnum::depthId::noDepth, .blendType = pipelineEnum::blendId::alphaBlend });
			createPipelineNode(pipelineEnum::procPipelineId::osMirror_p,  renderEnum::renderTemplateId::offScreen_rt, bufferEnum::procDescType::scenceForward_d,  vertexStruct::osMirrorVertex::vertexNode::layout,  pipelineEnum::shaderFlowId::vert, pipelineStruct::pipeConfigParamNode{  });
			createPipelineNode(pipelineEnum::procPipelineId::osScence_p,  renderEnum::renderTemplateId::swapChain_rt, bufferEnum::procDescType::scenceForward_d,  vertexStruct::offScreenVertex::vertexNode::layout, pipelineEnum::shaderFlowId::vert, pipelineStruct::pipeConfigParamNode{ .rasterType = pipelineEnum::rasterId::fill_front_ccw, .dynamicType = pipelineEnum::dynamicId::viewportScissor });
			createPipelineNode(pipelineEnum::procPipelineId::dsScence_p,  renderEnum::renderTemplateId::swapChain_rt, bufferEnum::procDescType::dsScence_d,		  vertexStruct::offScreenVertex::vertexNode::layout, pipelineEnum::shaderFlowId::vert, pipelineStruct::pipeConfigParamNode{ .rasterType = pipelineEnum::rasterId::fill_front_ccw, .dynamicType = pipelineEnum::dynamicId::viewportScissor });
			createPipelineNode(pipelineEnum::procPipelineId::shadowMap_p, renderEnum::renderTemplateId::shadowMap_rt, bufferEnum::procDescType::shadowMap_d,	  vertexStruct::shadowVertex::vertexNode::layout,	 pipelineEnum::shaderFlowId::vert, pipelineStruct::pipeConfigParamNode{ .depthType = pipelineEnum::depthId::test_write_lessEqual });

			if (debugInfo::LogSwitch) {
				checkPipeline();
			}

			for (pipelineStruct::pipelineNode& pipeNode : node) {
				pipeNode.isRecreate = true;
			}

			createPipeCache(device);
		}



		void createPipelineNode(pipelineEnum::procPipelineId::type tPipeType, renderEnum::renderTemplateId::type tRenderTemplateType, bufferEnum::procDescType::type tDescType, const vertexStruct::vertexLayoutNode& tVertexLayout, pipelineEnum::shaderFlowId::type tShaderFlowType, const pipelineStruct::pipeConfigParamNode& tParam) {
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
				if (node[0].descType >= bufferEnum::procDescType::count) { break; }

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