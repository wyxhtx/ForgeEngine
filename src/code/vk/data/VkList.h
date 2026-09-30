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

	struct procDescriptorNode {
		VkDescriptorSetAllocateInfo				allocInfo{};
		bufferEnum::procDescriptorId::type		descriptorType = bufferEnum::procDescriptorId::count;
		std::vector<descStruct::descriptorNode> node{};



		procDescriptorNode(const VkDevice& device) : node(bufferEnum::procDescriptorId::count) {
			allocInfo.sType				 = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.pNext				 = nullptr;
			allocInfo.descriptorSetCount = 1;
			
			reflectLayout(bufferEnum::procDescriptorId::scenceDeferred_d, bufferInfo::scenceDeferredCount, std::vector<std::string>{ fileInfo::shader::terrainVert,  fileInfo::shader::terrainFrag, fileInfo::shader::pbrVert,  fileInfo::shader::pbrFrag }, device);
			reflectLayout(bufferEnum::procDescriptorId::scenceForward_d,  bufferInfo::scenceForwardCount,  std::vector<std::string>{ fileInfo::shader::skyboxVert,   fileInfo::shader::skyboxFrag,  fileInfo::shader::mineVert, fileInfo::shader::mineFrag, fileInfo::shader::osMirrorVert, fileInfo::shader::osMirrorFrag, fileInfo::shader::osScenceVert, fileInfo::shader::osScenceFrag }, device);
			reflectLayout(bufferEnum::procDescriptorId::uiText_d,		  bufferInfo::uiTextCount,		   std::vector<std::string>{ fileInfo::shader::uiTextVert,   fileInfo::shader::uiTextFrag   }, device);
			reflectLayout(bufferEnum::procDescriptorId::dsScence_d,		  bufferInfo::dsScenceCount,	   std::vector<std::string>{ fileInfo::shader::dsScenceVert, fileInfo::shader::dsScenceFrag }, device);
			reflectLayout(bufferEnum::procDescriptorId::shadowMap_d,	  bufferInfo::shadowCount,		   std::vector<std::string>{ fileInfo::shader::shadowVert,   fileInfo::shader::shadowFrag   }, device);
		}

		procDescriptorNode() = delete;



		void clear(const VkDevice& device) {
			for (descStruct::descriptorNode& descriptorNode : node) {
				descriptorNode.clear(device);
			}
		}



		//反射 SPIR-V 提取绑定信息，创建描述符布局
		void reflectLayout(bufferEnum::procDescriptorId::type tDescriptorType, uint32_t tMaxSet, std::span<const std::string> shaderFileList, const VkDevice& device) {
			node[tDescriptorType].maxSet = tMaxSet;

			for (const std::string& file : shaderFileList) {
				for (const descStruct::reflectBindingNode& reflect : descStruct::reflectFile(file)) {
					//当前布局只支持单集合（GLSL 无 set 语法时反射出的集合号恒为 0）
					if (debugInfo::LogSwitch && reflect.set != 0) {
						Log::printError(__FILE__, __LINE__, pipelineInfo::procDescriptorName[tDescriptorType] + " 运行时 spv 的描述符集号非 0：" + reflect.name);
					}

					bool isMerge = false;
					for (descStruct::descriptorConfigNode& merge : node[tDescriptorType].config) {
						if (merge.layout.binding != reflect.binding) { continue; }

						if (debugInfo::LogSwitch && merge.layout.descriptorType != reflect.type) {
							Log::printError(__FILE__, __LINE__, pipelineInfo::procDescriptorName[tDescriptorType] + " 绑定 " + std::to_string(reflect.binding) + " 在不同着色器里类型不一致");
						}

						if (debugInfo::LogSwitch && merge.layout.descriptorCount != reflect.count) {
							Log::printError(__FILE__, __LINE__, pipelineInfo::procDescriptorName[tDescriptorType] + " 绑定 " + std::to_string(reflect.binding) + " 在不同着色器里数组长度不一致");
						}

						merge.layout.stageFlags = static_cast<VkShaderStageFlags>(merge.layout.stageFlags | reflect.stage);
						isMerge = true;
						break;
					}

					if (!isMerge) {
						node[tDescriptorType].config.push_back(descStruct::descriptorConfigNode{ reflect.binding, reflect.type, static_cast<VkShaderStageFlags>(reflect.stage), reflect.count });
					}
				}
			}

			node[tDescriptorType].createDescriptorNode(device);
		}

		void setAllocInfo(bufferEnum::procDescriptorId::type tDescriptorType) {
			allocInfo.descriptorPool = node[tDescriptorType].pool;
			allocInfo.pSetLayouts    = &node[tDescriptorType].layout;

			descriptorType = tDescriptorType;
		}

		const VkWriteDescriptorSet& getWrite(int writeId, const VkDescriptorSet& dstSet, const VkBuffer& buffer, VkDeviceSize range) {
			if (debugInfo::LogSwitch && descriptorType >= bufferEnum::procDescriptorId::count) { Log::printError(__FILE__, __LINE__, "getWrite 前必须先 setAllocInfo（当前家族未设置）"); }

			return node[descriptorType].config[writeId].setWrite(dstSet, buffer, range);
		}

		const VkWriteDescriptorSet& getWrite(int writeId, const VkDescriptorSet& dstSet, const VkSampler& sampler, const VkImageView& imageView) {
			if (debugInfo::LogSwitch && descriptorType >= bufferEnum::procDescriptorId::count) { Log::printError(__FILE__, __LINE__, "getWrite 前必须先 setAllocInfo（当前家族未设置）"); }

			return node[descriptorType].config[writeId].setWrite(dstSet, sampler, imageView);
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
		std::vector<pipelineStruct::pipeLayoutNode> nodeLayout{};
		std::vector<pipelineStruct::pipelineNode>   node{};

		VkDevice device{};



		procPipelineNode(const VkDevice& tDevice, const std::vector<renderStruct::renderTemplateNode>& tRenderList, const std::vector<descStruct::descriptorNode>& dstList, VkSampleCountFlagBits tMsaa)
			: nodeLayout(bufferEnum::procDescriptorId::count), node(pipelineEnum::procPipelineId::count), device(tDevice) {
			for (int i = 0; i < bufferEnum::procDescriptorId::count; i++) {
				nodeLayout[i].createPipeLayoutNode(device, dstList[i].layout);
			}

			node[pipelineEnum::procPipelineId::terrain_p]  .createPipelineNode(bufferEnum::procDescriptorId::scenceDeferred_d, vertexStruct::terrainVertex::vertexNode::layout,   renderEnum::renderTemplateId::deferred_rt,  pipelineEnum::shaderFlowId::vert, pipelineStruct::pipeConfigParamNode{  }, { fileInfo::shader::terrainVert,  fileInfo::shader::terrainFrag }, tMsaa, tRenderList[renderEnum::renderTemplateId::deferred_rt], nodeLayout[bufferEnum::procDescriptorId::scenceDeferred_d], device);
			node[pipelineEnum::procPipelineId::model_p]	   .createPipelineNode(bufferEnum::procDescriptorId::scenceDeferred_d, vertexStruct::modelVertex::vertexNode::layout,     renderEnum::renderTemplateId::deferred_rt,  pipelineEnum::shaderFlowId::vert, pipelineStruct::pipeConfigParamNode{  }, { fileInfo::shader::pbrVert,      fileInfo::shader::pbrFrag }, tMsaa, tRenderList[renderEnum::renderTemplateId::deferred_rt], nodeLayout[bufferEnum::procDescriptorId::scenceDeferred_d], device);
			node[pipelineEnum::procPipelineId::skybox_p]   .createPipelineNode(bufferEnum::procDescriptorId::scenceForward_d,  vertexStruct::skyboxVertex::vertexNode::layout,    renderEnum::renderTemplateId::deferred_rt,  pipelineEnum::shaderFlowId::vert, pipelineStruct::pipeConfigParamNode{ .depthType = pipelineEnum::depthId::test_noWrite_lessEqual }, { fileInfo::shader::skyboxVert,   fileInfo::shader::skyboxFrag }, tMsaa, tRenderList[renderEnum::renderTemplateId::deferred_rt], nodeLayout[bufferEnum::procDescriptorId::scenceForward_d], device);
			node[pipelineEnum::procPipelineId::mine_p]	   .createPipelineNode(bufferEnum::procDescriptorId::scenceForward_d,  vertexStruct::mineVertex::vertexNode::layout,      renderEnum::renderTemplateId::deferred_rt,  pipelineEnum::shaderFlowId::vert, pipelineStruct::pipeConfigParamNode{ .primitiveType = pipelineEnum::primitiveId::lineList, .rasterType = pipelineEnum::rasterId::line_back_ccw }, { fileInfo::shader::mineVert,     fileInfo::shader::mineFrag }, tMsaa, tRenderList[renderEnum::renderTemplateId::deferred_rt], nodeLayout[bufferEnum::procDescriptorId::scenceForward_d], device);
			node[pipelineEnum::procPipelineId::uiText_p]   .createPipelineNode(bufferEnum::procDescriptorId::uiText_d,         vertexStruct::uiTextVertex::vertexNode::layout,    renderEnum::renderTemplateId::swapChain_rt, pipelineEnum::shaderFlowId::vert, pipelineStruct::pipeConfigParamNode{ .primitiveType = pipelineEnum::primitiveId::triangleList, .rasterType = pipelineEnum::rasterId::fill_back_cw, .depthType = pipelineEnum::depthId::noDepth, .blendType = pipelineEnum::blendId::alphaBlend }, { fileInfo::shader::uiTextVert,   fileInfo::shader::uiTextFrag }, tMsaa, tRenderList[renderEnum::renderTemplateId::swapChain_rt], nodeLayout[bufferEnum::procDescriptorId::uiText_d], device);
			node[pipelineEnum::procPipelineId::osMirror_p] .createPipelineNode(bufferEnum::procDescriptorId::scenceForward_d,  vertexStruct::osMirrorVertex::vertexNode::layout,  renderEnum::renderTemplateId::offScreen_rt, pipelineEnum::shaderFlowId::vert, pipelineStruct::pipeConfigParamNode{  }, { fileInfo::shader::osMirrorVert, fileInfo::shader::osMirrorFrag }, tMsaa, tRenderList[renderEnum::renderTemplateId::offScreen_rt], nodeLayout[bufferEnum::procDescriptorId::scenceForward_d], device);
			node[pipelineEnum::procPipelineId::osScence_p] .createPipelineNode(bufferEnum::procDescriptorId::scenceForward_d,  vertexStruct::offScreenVertex::vertexNode::layout, renderEnum::renderTemplateId::swapChain_rt, pipelineEnum::shaderFlowId::vert, pipelineStruct::pipeConfigParamNode{ .rasterType = pipelineEnum::rasterId::fill_front_ccw, .dynamicType = pipelineEnum::dynamicId::viewportScissor }, { fileInfo::shader::osScenceVert, fileInfo::shader::osScenceFrag }, tMsaa, tRenderList[renderEnum::renderTemplateId::swapChain_rt], nodeLayout[bufferEnum::procDescriptorId::scenceForward_d], device);
			node[pipelineEnum::procPipelineId::dsScence_p] .createPipelineNode(bufferEnum::procDescriptorId::dsScence_d,       vertexStruct::offScreenVertex::vertexNode::layout, renderEnum::renderTemplateId::swapChain_rt, pipelineEnum::shaderFlowId::vert, pipelineStruct::pipeConfigParamNode{ .rasterType = pipelineEnum::rasterId::fill_front_ccw, .dynamicType = pipelineEnum::dynamicId::viewportScissor }, { fileInfo::shader::dsScenceVert, fileInfo::shader::dsScenceFrag }, tMsaa, tRenderList[renderEnum::renderTemplateId::swapChain_rt], nodeLayout[bufferEnum::procDescriptorId::dsScence_d], device);
			node[pipelineEnum::procPipelineId::shadowMap_p].createPipelineNode(bufferEnum::procDescriptorId::shadowMap_d,      vertexStruct::shadowVertex::vertexNode::layout,    renderEnum::renderTemplateId::shadowMap_rt, pipelineEnum::shaderFlowId::vert, pipelineStruct::pipeConfigParamNode{ .depthType = pipelineEnum::depthId::test_write_lessEqual }, { fileInfo::shader::shadowVert,   fileInfo::shader::shadowFrag }, tMsaa, tRenderList[renderEnum::renderTemplateId::shadowMap_rt], nodeLayout[bufferEnum::procDescriptorId::shadowMap_d], device);

			if (debugInfo::LogSwitch) {
				checkPipeline();
			}

			for (pipelineStruct::pipelineNode& pipeNode : node) {
				pipeNode.isRecreate = true;
			}

			createPipeCache(device);
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
		
			for (int d = 0; d < nodeLayout.size(); d++) {
				if (node[0].descriptorType >= bufferEnum::procDescriptorId::count) { break; }

				bool used = false;
				for (const pipelineStruct::pipelineNode& pipeNode : node) {
					if (static_cast<int>(pipeNode.descriptorType) == d) {
						used = true; break;
					}
				}

				if (!used) { 
					Log::printError(__FILE__, __LINE__, "描述符 " + std::to_string(d) + " 未被管线使用"); 
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
			return nodeLayout[node[pipeType].descriptorType].pipeLayout;
		}

		VkPipeline& getPipeline(pipelineEnum::procPipelineId::type pipeType) {
			return node[pipeType].pipeline;
		}
	};
}