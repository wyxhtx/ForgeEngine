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
	//管线着色器结点：一条管线用哪套描述符布局、挂在哪个渲染模板下、采样数、用哪些着色器
	struct pipelineShaderNode {
		pipelineEnum::procPipelineId::type  pipeType{};          //管线 id（本行的键，表顺序无关）
		bufferEnum::procDescriptorId::type  descriptor{};        //描述符布局
		renderEnum::renderTemplateId::type  renderTemplate{};    //渲染模板
		bool                                isMsaa{};            //是否用引擎的 MSAA 采样数，否则强制单采样
		std::vector<std::string>            fileList{};          //着色器文件，索引与 pipelineEnum::shaderId::vert 一致



		//采样数：本行跟随引擎 MSAA 就取引擎采样数，否则强制单采样
		VkSampleCountFlagBits getMsaa(VkSampleCountFlagBits msaa) const {
			return isMsaa ? msaa : VK_SAMPLE_COUNT_1_BIT;
		}
	};

	//管线 id → 着色器（每行自带 pipeType，表顺序无关）
	//唯一来源：引擎据此建管线；反射布局据此推导"每个描述符布局用到哪些着色器"
	const std::array<pipelineShaderNode, pipelineEnum::procPipelineId::count> procPipelineShader{ {
		{ pipelineEnum::procPipelineId::terrain_p,   bufferEnum::procDescriptorId::scenceDeferred_d, renderEnum::renderTemplateId::deferred_rt,  false, { fileInfo::shader::terrainVert,  fileInfo::shader::terrainFrag  } },
		{ pipelineEnum::procPipelineId::model_p,     bufferEnum::procDescriptorId::scenceDeferred_d, renderEnum::renderTemplateId::deferred_rt,  false, { fileInfo::shader::pbrVert,      fileInfo::shader::pbrFrag      } },
		{ pipelineEnum::procPipelineId::skybox_p,    bufferEnum::procDescriptorId::scenceForward_d,  renderEnum::renderTemplateId::deferred_rt,  false, { fileInfo::shader::skyboxVert,   fileInfo::shader::skyboxFrag   } },
		{ pipelineEnum::procPipelineId::mine_p,      bufferEnum::procDescriptorId::scenceForward_d,  renderEnum::renderTemplateId::deferred_rt,  false, { fileInfo::shader::mineVert,     fileInfo::shader::mineFrag     } },
		{ pipelineEnum::procPipelineId::uiText_p,    bufferEnum::procDescriptorId::uiText_d,         renderEnum::renderTemplateId::swapChain_rt, true,  { fileInfo::shader::uiTextVert,   fileInfo::shader::uiTextFrag   } },
		{ pipelineEnum::procPipelineId::osMirror_p,  bufferEnum::procDescriptorId::scenceForward_d,  renderEnum::renderTemplateId::offScreen_rt, true,  { fileInfo::shader::osMirrorVert, fileInfo::shader::osMirrorFrag } },
		{ pipelineEnum::procPipelineId::osScence_p,  bufferEnum::procDescriptorId::scenceForward_d,  renderEnum::renderTemplateId::swapChain_rt, true,  { fileInfo::shader::osScenceVert, fileInfo::shader::osScenceFrag } },
		{ pipelineEnum::procPipelineId::dsScence_p,  bufferEnum::procDescriptorId::dsScence_d,       renderEnum::renderTemplateId::swapChain_rt, true,  { fileInfo::shader::dsScenceVert, fileInfo::shader::dsScenceFrag } },
		{ pipelineEnum::procPipelineId::shadowMap_p, bufferEnum::procDescriptorId::shadowMap_d,      renderEnum::renderTemplateId::shadowMap_rt, false, { fileInfo::shader::shadowVert,   fileInfo::shader::shadowFrag   } },
	} };



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
			for (int i = 0; i < renderEnum::renderTemplateId::count; i++) {
				renderTemplateList[i].clear(device);
			}

			for (int i = 0; i < renderEnum::renderInstanceId::count; i++) {
				renderInstanceList[i].clear(device, memAllocator);
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
			for (int i = 0; i < renderEnum::renderInstanceId::count; i++) {
				renderInstanceList[i].clear(device, memAllocator);
			}
		}

		//获取frameImage，为创建描述符服务
		const VkImageView& getFrameImage(renderEnum::renderInstanceId::type renderInstanceType, renderEnum::renderFrameId::type renderFrameType, int viewId) {
			return renderInstanceList[renderInstanceType].frameImageList[renderFrameType].imageList[viewId].view;
		}

		//获取renderBegin
		const VkRenderPassBeginInfo& getRenderBegin(renderEnum::renderInstanceId::type renderInstanceType) {
			return renderInstanceList[renderInstanceType].frameBufferList[0].renderBegin;
		}

		//获取renderBegin，swap版
		const VkRenderPassBeginInfo& getRenderBegin(renderEnum::renderInstanceId::type renderInstanceType, int frameId) {
			return renderInstanceList[renderInstanceType].frameBufferList[frameId].renderBegin;
		}
	};

	struct procDescriptorNode {
		VkDescriptorSetAllocateInfo allocInfo{};
		bufferEnum::procDescriptorId::type nodeId;

		std::vector<bufferStruct::descriptorNode> node{};



		procDescriptorNode(const VkDevice& device) : node(bufferEnum::procDescriptorId::count) {
			//allocInfo
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.pNext = nullptr;
			allocInfo.descriptorSetCount = 1;

			//描述符布局：运行期反射 spv 现算，天然与着色器一致
			reflectLayout();

			//描述符池上限属应用侧决策，spv 里没有，仍在此手写
			node[bufferEnum::procDescriptorId::scenceForward_d] .maxSet = bufferInfo::scenceForwardCount;
			node[bufferEnum::procDescriptorId::scenceDeferred_d].maxSet = bufferInfo::scenceDeferredCount;
			node[bufferEnum::procDescriptorId::uiText_d]		.maxSet = bufferInfo::uiTextCount;
			node[bufferEnum::procDescriptorId::dsScence_d]		.maxSet = bufferInfo::dsScenceCount;
			node[bufferEnum::procDescriptorId::shadowMap_d]		.maxSet = bufferInfo::shadowCount;

			for (int i = 0; i < bufferEnum::procDescriptorId::count; i++) {
				node[i].createDescriptorNode(device);
			}
		}



		void clear(const VkDevice& device) {
			for (int i = 0; i < bufferEnum::procDescriptorId::count; i++) {
				node[i].clear(device);
			}
		}



		//反射 spv 现算描述符布局：布局不再由构建期生成，改在启动时按磁盘上的 spv 现算
		void reflectLayout() {
			//着色器索引 → 阶段（索引与 pipelineEnum::shaderId::vert 一致：0 顶点、1 片段）
			const std::array<VkShaderStageFlagBits, pipelineEnum::shaderId::vert::count> vertStageList{ VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT };

			//按描述符汇总着色器：同一布局常被多条管线共用，需合并去重
			std::array<std::vector<spirvTool::reflectShaderNode>, bufferEnum::procDescriptorId::count> descriptorShader{};

			//按表逐管线汇总；行自带 pipeType，与表顺序无关
			for (const pipelineList::pipelineShaderNode& pipeline : pipelineList::procPipelineShader) {
				if (debugInfo::LogSwitch && pipeline.fileList.size() != vertStageList.size()) {
					Log::printError(__FILE__, __LINE__, pipelineInfo::procPipelineName[pipeline.pipeType] + " 着色器数量不是 " + std::to_string(vertStageList.size()) + " 个（当前只支持顶点加片段）");
				}

				std::vector<spirvTool::reflectShaderNode>& shaderList = descriptorShader[pipeline.descriptor];

				for (size_t i = 0; i < pipeline.fileList.size(); i++) {
					//去重：多条管线共用同一着色器时只反射一次
					bool isExist = false;

					for (const spirvTool::reflectShaderNode& exist : shaderList) {
						if (exist.file == pipeline.fileList[i]) { isExist = true; break; }
					}

					if (!isExist) {
						shaderList.push_back(spirvTool::reflectShaderNode{ .file = pipeline.fileList[i], .stage = vertStageList[i] });
					}
				}
			}

			//逐描述符反射并合并：同一绑定出现在多个阶段则阶段求并集，类型与数组长度必须一致
			for (int descriptorId = 0; descriptorId < bufferEnum::procDescriptorId::count; descriptorId++) {
				std::vector<bufferStruct::descriptorConfigNode> configList{};
				std::string bindingText{};

				for (const spirvTool::reflectShaderNode& shader : descriptorShader[descriptorId]) {
					for (const spirvTool::reflectBindingNode& reflect : spirvTool::reflectFile(shader.file, shader.stage)) {
						//当前布局只支持单集合（GLSL 无 set 语法时反射出的集合号恒为 0）
						if (debugInfo::LogSwitch && reflect.set != 0) {
							Log::printError(__FILE__, __LINE__, pipelineInfo::procDescriptorName[descriptorId] + " 运行时 spv 的描述符集号非 0：" + reflect.name);
						}

						bool isMerge = false;

						for (bufferStruct::descriptorConfigNode& merge : configList) {
							if (merge.layout.binding != reflect.binding) { continue; }

							if (debugInfo::LogSwitch && merge.layout.descriptorType != reflect.type) {
								Log::printError(__FILE__, __LINE__, pipelineInfo::procDescriptorName[descriptorId] + " 绑定 " + std::to_string(reflect.binding) + " 在不同着色器里类型不一致");
							}
							if (debugInfo::LogSwitch && merge.layout.descriptorCount != reflect.count) {
								Log::printError(__FILE__, __LINE__, pipelineInfo::procDescriptorName[descriptorId] + " 绑定 " + std::to_string(reflect.binding) + " 在不同着色器里数组长度不一致");
							}

							merge.layout.stageFlags = static_cast<VkShaderStageFlags>(merge.layout.stageFlags | reflect.stage);
							isMerge = true;
							break;
						}

						if (!isMerge) {
							configList.push_back(bufferStruct::descriptorConfigNode{ reflect.binding, reflect.type, static_cast<VkShaderStageFlags>(reflect.stage), reflect.count });

							//绑定之间用 → 衔接（首项不加）
							bindingText += (bindingText.empty() ? std::string{} : std::string{ " → " }) + "b" + std::to_string(reflect.binding) + " " + reflect.name;
						}
					}
				}

				node[descriptorId].config = configList;

				//名字按 tab 位补齐（tab 宽 8，补到第 16 列）：让各描述符的反射日志列对齐
				Log::print(Log::tabPad(pipelineInfo::procDescriptorName[descriptorId], 16) + "描述符布局反射完成：" + std::to_string(configList.size()) + " 项 → " + bindingText);
			}
		}

		void setAllocInfo(bufferEnum::procDescriptorId::type id) {
			allocInfo.descriptorPool = node[id].pool;
			allocInfo.pSetLayouts    = &node[id].layout;

			nodeId = id;
		}

		const VkWriteDescriptorSet& getWrite(int writeId, const VkDescriptorSet& dstSet, const VkBuffer& buffer, VkDeviceSize range) {
			return node[nodeId].config[writeId].setWrite(dstSet, buffer, range);
		}

		const VkWriteDescriptorSet& getWrite(int writeId, const VkDescriptorSet& dstSet, const VkSampler& sampler, const VkImageView& imageView) {
			return node[nodeId].config[writeId].setWrite(dstSet, sampler, imageView);
		}
	};

	struct procSamplerNode {
		std::vector<VkSampler> node{};



		procSamplerNode() : node(bufferEnum::procSamplerId::count) {}



		void clear(const VkDevice& device) {
			for (int i = 0; i < bufferEnum::procSamplerId::count; i++) {
				vkDestroySampler(device, node[i], nullptr);
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



		procPipelineNode(const VkDevice& device, const std::vector<bufferStruct::descriptorNode>& dstList, const std::vector<renderStruct::renderTemplateNode>& renderList, VkSampleCountFlagBits msaa) : nodeLayout(bufferEnum::procDescriptorId::count), node(pipelineEnum::procPipelineId::count) {
			/*配置nodeLayout*/
			for (size_t i = 0; i < bufferEnum::procDescriptorId::count; i++) {
				nodeLayout[i].createPipeLayoutNode(device, dstList[i].layout);
			}

			/*配置node：着色器、渲染模板、描述符布局全部由 pipelineList::procPipelineShader 表驱动；建到 node[pipeType] 上，与表顺序无关*/
			//node[pipelineEnum::procPipelineId::terrain_p] .createMeshPipelineNode(device, extent, msaa, bufferEnum::procDescriptorId::scenceDeferred_d, fileInfo::shader::terrainTask, fileInfo::shader::terrainMesh, fileInfo::shader::terrainFrag);
			for (const pipelineList::pipelineShaderNode& shader : pipelineList::procPipelineShader) {
				node[shader.pipeType].createPipelineNode(shader.getMsaa(msaa), renderList[shader.renderTemplate], nodeLayout[shader.descriptor], shader.fileList, device);
			}

			/*开发期自检：pipeType 是表的键，重复会让后一行静默覆盖前一行（表顺序无关的代价）*/
			if (debugInfo::LogSwitch) {
				std::array<bool, pipelineEnum::procPipelineId::count> isUsed{};

				for (const pipelineList::pipelineShaderNode& shader : pipelineList::procPipelineShader) {
					if (isUsed[shader.pipeType]) {
						Log::printError(__FILE__, __LINE__, pipelineInfo::procPipelineName[shader.pipeType] + " 在管线着色器表里重复，后一行会覆盖前一行");
					}

					isUsed[shader.pipeType] = true;
				}
			}

			/*配置node.pipeConfig*/
			//depthStencil 实现只在空白处显示
			node[pipelineEnum::procPipelineId::skybox_p].pipeConfig.depthStencil.depthWriteEnable = VK_FALSE;
			node[pipelineEnum::procPipelineId::skybox_p].pipeConfig.depthStencil.depthCompareOp   = VK_COMPARE_OP_LESS_OR_EQUAL;

			//primitiveState 线段图元，rasterizer 线填充
			node[pipelineEnum::procPipelineId::mine_p].pipeConfig.primitiveState.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
			node[pipelineEnum::procPipelineId::mine_p].pipeConfig.rasterizer.polygonMode  = VK_POLYGON_MODE_LINE;
			node[pipelineEnum::procPipelineId::mine_p].pipeConfig.rasterizer.lineWidth    = 1.0f;

			//primitiveState 条带图元，rasterizer 顺时针，depthStencil 实现始终渲染在最上层
			node[pipelineEnum::procPipelineId::uiText_p].pipeConfig.primitiveState.topology       = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
			node[pipelineEnum::procPipelineId::uiText_p].pipeConfig.rasterizer.frontFace          = VK_FRONT_FACE_CLOCKWISE;
			node[pipelineEnum::procPipelineId::uiText_p].pipeConfig.depthStencil.depthTestEnable  = VK_FALSE;
			node[pipelineEnum::procPipelineId::uiText_p].pipeConfig.depthStencil.depthWriteEnable = VK_FALSE;

			//颜色附件混合：MSDF 半透明平滑（frag 输出 white*alpha，需 src_alpha 混合）
			for (auto& blendAtt : node[pipelineEnum::procPipelineId::uiText_p].pipeConfig.colorAttachmentList) {
				blendAtt.blendEnable         = VK_TRUE;
				blendAtt.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
				blendAtt.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
				blendAtt.colorBlendOp        = VK_BLEND_OP_ADD;
				blendAtt.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
				blendAtt.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
				blendAtt.alphaBlendOp        = VK_BLEND_OP_ADD;
			}

			//pipeDynamicStageState 动态视口与动态裁剪，rasterizer 剔除正面
			node[pipelineEnum::procPipelineId::osScence_p].pipeConfig.setPipeDynamicStage({ VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR });
			node[pipelineEnum::procPipelineId::osScence_p].pipeConfig.rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;

			//pipeDynamicStageState 动态视口与动态裁剪，rasterizer 剔除正面
			node[pipelineEnum::procPipelineId::dsScence_p].pipeConfig.setPipeDynamicStage({ VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR });
			node[pipelineEnum::procPipelineId::dsScence_p].pipeConfig.rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;

			//depthStencil 防止照射面被误判为阴影
			node[pipelineEnum::procPipelineId::shadowMap_p].pipeConfig.depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

			/*创建pipeCache*/
			createPipelineCache(device);
		}

		procPipelineNode() = delete;



		void clear(const VkDevice& device) {
			for (size_t i = 0; i < bufferEnum::procDescriptorId::count; i++) {
				nodeLayout[i].clear(device);
			}

			for (int i = 0; i < pipelineEnum::procPipelineId::count; i++) {
				node[i].clear(device);
			}

			vkDestroyPipelineCache(device, pipeCache, nullptr);
		}



		//创建管线
		template<typename VertexType>
		void createPipeline(const VkDevice& device, pipelineEnum::procPipelineId::type id, const VkExtent2D& extent) {
			//配置vertInputStage
			auto binding   = VertexType::getBindingDescription();
			auto attribute = VertexType::getAttributeDescriptions();

			VkPipelineVertexInputStateCreateInfo vertInputStage{};
			vertInputStage.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertInputStage.vertexBindingDescriptionCount   = uint32_t(binding.size());
			vertInputStage.vertexAttributeDescriptionCount = uint32_t(attribute.size());
			vertInputStage.pVertexBindingDescriptions      = binding.data();
			vertInputStage.pVertexAttributeDescriptions    = attribute.data();

			//配置pipeConfig
			pipelineStruct::pipeConfigNode& pipeConfig = node[id].pipeConfig;
			pipeConfig.viewport.width  = static_cast<float>(extent.width);
			pipeConfig.viewport.height = static_cast<float>(extent.height);
			pipeConfig.scissor.extent  = extent;
			pipeConfig.pipeInfo.pVertexInputState = &vertInputStage;

			//创建pipeline
			if (vkCreateGraphicsPipelines(device, pipeCache, 1, &pipeConfig.pipeInfo, nullptr, &node[id].pipeline) != VK_SUCCESS) {
				Log::printError(__FILE__, __LINE__, "管线创建失败");
			}
		}

		void clearSwapChain(const VkDevice& device) {
			for (int i = 0; i < pipelineEnum::procPipelineId::count; i++) {
				node[i].clearSwapChain(device);
			}
		}

	private:
		VkPipelineCache pipeCache;



		//创建pipeCache
		void createPipelineCache(const VkDevice& device){
			VkPipelineCacheCreateInfo pipeCacheInfo{};
			pipeCacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

			if (vkCreatePipelineCache(device, &pipeCacheInfo, nullptr, &pipeCache)) {
				Log::printError(__FILE__, __LINE__, "管线缓存创建失败");
			}
		}

	public:
		VkPipelineLayout& getPipeLayout(pipelineEnum::procPipelineId::type id) {
			return node[id].pipeConfig.pipeInfo.layout;
		}

		VkPipeline& getPipeline(pipelineEnum::procPipelineId::type id) {
			return node[id].pipeline;
		}
	};
}