#pragma once



class Buffer {
public:
	Buffer(Device& Device) :Device{ Device } {
		createMeshBuffer();
		createUniformBuffer();
		createTexture();
		pipeModel.getMaxModelMip();
		procSampler.createTextureSampler(Device.device, pipeTerrain.texture.mipLevel, pipeModel.sModelMip, pipeModel.dModelMip);
		createDescriptorSet();

		initUniformBuffer();	//初始化Uniform
		drawUIText(0);			//初始化uiText

		if (debugInfo::LogSwitch) {
			Log::printTitle("————————Buffer已创建");

			Log::delimiter();
		}
	}

	~Buffer() {
		procRenderPass.clear(Device.device, Device.memAllocator);
		procDescriptor.clear(Device.device);
		procSampler	  .clear(Device.device);

		cmd.clear(Device.device);

		if (pipeTerrain.isEnable)	 { pipeTerrain	 .clear(Device.device, Device.memAllocator); }
		if (pipeModel.isEnable)  { pipeModel .clear(Device.device, Device.memAllocator); }
		if (pipeSkybox.isEnable)	 { pipeSkybox	 .clear(Device.device, Device.memAllocator); }
		if (pipeMine.isEnable)		 { pipeMine		 .clear(Device.device, Device.memAllocator); }
		if (pipeUIText.isEnable)	 { pipeUIText	 .clear(Device.device, Device.memAllocator); }
		if (pipeOSMirror.isEnable)	 { pipeOSMirror	 .clear(Device.device, Device.memAllocator); }
		if (pipeOSScence.isEnable)	 { pipeOSScence	 .clear(Device.device, Device.memAllocator); }
		if (pipeDSScence.isEnable)	 { pipeDSScence	 .clear(Device.device, Device.memAllocator); }
		if (pipeShadow.isEnable) { pipeShadow.clear(Device.device, Device.memAllocator); }
		
		if (debugInfo::LogSwitch) {
			Log::print("Buffer已清除");
		}
	}



	//todo 先这样凑合，等后面全放管线下
	Device& Device;

	Swapchain InitSwapchain{ Device };

	cmdStruct::cmdNode cmd{ Device.device, Device.enableGpu->graphicsQueueFamily.index, uint32_t(swapStruct::swapImageList.size()) };

private:
	bufferList::procRenderNode		procRenderPass{ InitSwapchain.colorFormat, InitSwapchain.depthFormat, InitSwapchain.swapExtent, InitSwapchain.msaa, Device.device, Device.memAllocator };
	bufferList::procDescriptorNode	procDescriptor{ Device.device };
	bufferList::procSamplerNode		procSampler{};

	bufferStruct::pipeTerrainNode	pipeTerrain		{ true,	 pipelineEnum::procPipelineId::terrain_p,   bufferEnum::procSamplerId::terrain_s, fileInfo::image::cube, textureInfo::terrain::colorFormat, textureInfo::terrain::tileExtent, textureInfo::terrain::atlasExtent, cmd, Device.device, Device.graphicsQueue, Device.memAllocator};
	bufferStruct::pipeModelNode		pipeModel		{ true,  pipelineEnum::procPipelineId::model_p	    };
	bufferStruct::pipeSkyboxNode	pipeSkybox		{ true,	 pipelineEnum::procPipelineId::skybox_p	    };
	bufferStruct::pipeMineNode		pipeMine		{ true,  pipelineEnum::procPipelineId::mine_p	    };
	bufferStruct::pipeUITextNode	pipeUIText		{ true,  pipelineEnum::procPipelineId::uiText_p,    bufferEnum::procSamplerId::uiText_s   , cmd, Device.device, Device.graphicsQueue, Device.memAllocator };
	bufferStruct::pipeOSMirrorNode	pipeOSMirror	{ false, pipelineEnum::procPipelineId::osMirror_p,  bufferEnum::procSamplerId::offScreen_s };
	bufferStruct::pipeOSScenceNode	pipeOSScence	{ false, pipelineEnum::procPipelineId::osScence_p,  bufferEnum::procSamplerId::offScreen_s };
	bufferStruct::pipeDSScenceNode	pipeDSScence	{ true,	 pipelineEnum::procPipelineId::dsScence_p,  bufferEnum::procSamplerId::offScreen_s };
	bufferStruct::pipeShadowNode	pipeShadow		{ true,  pipelineEnum::procPipelineId::shadowMap_p, bufferEnum::procSamplerId::shadowMap_s };



	void createMeshBuffer() {
		//pipeTerrain
		if (pipeTerrain.isEnable) {
			for (int thread = 0; thread < bufferInfo::commandThreadCount; thread++) {
				for (int layer = 0; layer < plateEnum::terrainLayer::count; layer++) {
					createVertexBuffer(terrainVertex::vertexs[layer], pipeTerrain.vertex[thread][layer]);
					createIndexBuffer(terrainVertex::indexs[layer], pipeTerrain.index[thread][layer]);
				}
			}
		}

		//model
		if (pipeModel.isEnable) {
			//model-sNode
			for (int i = 0; i < bufferEnum::procStaticModelId::count; i++) {
				loadStaticModel(pipeModel.sNode[i].modelFile, modelVertex::sVertexs[i], modelVertex::sIndexs[i], shadowVertex::sVertexs[i]);

				createVertexBuffer(modelVertex::sVertexs[i], pipeModel.sNode[i].vertex);
				createIndexBuffer(modelVertex::sIndexs[i], pipeModel.sNode[i].index);
			}

			//model-dNode
			for (int i = 0; i < bufferEnum::procDynamicModelId::count; i++) {
				shadowVertex::dVertexs[i].resize(modelVertex::dIndexs[i].size());
				pipeShadow.shadow_md[i].resize(modelVertex::dIndexs[i].size());

				loadDynamicModel(pipeModel.dNode[i].modelFile, modelVertex::dVertexs[i], modelVertex::dIndexs[i], shadowVertex::dVertexs[i]);

				for (int j = 0; j < pipeModel.dNode[i].node.size(); j++) {
					createVertexBuffer(modelVertex::dVertexs[i][j], pipeModel.dNode[i].node[j].vertex);
					createIndexBuffer(modelVertex::dIndexs[i][j], pipeModel.dNode[i].node[j].index);
				}
			}
		}

		//pipeSkybox
		if (pipeSkybox.isEnable) {
			createVertexBuffer(skyboxVertex::vertexs, pipeSkybox.vertex);
			createIndexBuffer(skyboxVertex::indexs,   pipeSkybox.index);
		}

		//pipeMine
		if (pipeMine.isEnable) {
			createVertexBuffer(mineVertex::vertexs, pipeMine.vertex);
			createIndexBuffer(mineVertex::indexs, pipeMine.index);
		}

		//pipeUIText
		if (pipeUIText.isEnable) {
			createBuffer(2048 * sizeof(glm::vec4), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, pipeUIText.vertex);
		}

		//pipeOSMirror
		if (pipeOSMirror.isEnable) {
			createVertexBuffer(osMirrorVertex::vertexs, pipeOSMirror.vertex);
			createIndexBuffer(osMirrorVertex::indexs,	pipeOSMirror.index);
		}

		//shadow
		if (pipeShadow.isEnable) {
			//pipeTerrain
			if (pipeTerrain.isEnable) {
				//shadow-pipeTerrain
				for (int layer = 0; layer < plateEnum::terrainLayer::count; layer++) {
					shadowVertex::loadTerrainVertexs(layer);

					for (int thread = 0; thread < bufferInfo::commandThreadCount; thread++) {
						createVertexBuffer(shadowVertex::tVertexs[layer], pipeShadow.shadow_t.vertex[thread][layer]);
					}
				}
			}

			//model
			if (pipeModel.isEnable) {
				//shadow-sNode
				for (int i = 0; i < bufferEnum::procStaticModelId::count; i++) {
					createVertexBuffer(shadowVertex::sVertexs[i], pipeShadow.shadow_ms[i].vertex);
				}

				//shadow-dNode
				for (int i = 0; i < bufferEnum::procDynamicModelId::count; i++) {
					for (int j = 0; j < pipeShadow.shadow_md[i].size(); j++) {
						createVertexBuffer(shadowVertex::dVertexs[i][j], pipeShadow.shadow_md[i][j].vertex);
					}
				}
			}
		}
	}

	void createUniformBuffer() {
		//pipeTerrain
		if (pipeTerrain.isEnable) {
			createBuffer(sizeof(pipeTerrain.ubo), pipeTerrain.uniform);
		}

		//model
		if (pipeModel.isEnable) {
			//model-sNode
			for (int i = 0; i < bufferEnum::procStaticModelId::count; i++) {
				createBuffer(sizeof(pipeModel.sNode[i].ubo), pipeModel.sNode[i].uniform);
			}

			//model-dNode
			for (int i = 0; i < bufferEnum::procDynamicModelId::count; i++) {
				for (int j = 0; j < pipeModel.dNode[i].node.size(); j++) {
					createBuffer(sizeof(pipeModel.dNode[i].node[j].ubo), pipeModel.dNode[i].node[j].uniform);
				}
			}
		}

		//pipeSkybox
		if (pipeSkybox.isEnable) {
			createBuffer(sizeof(pipeSkybox.ubo), pipeSkybox.uniform);
			createBuffer(sizeof(pipeSkybox.lightUbo), pipeSkybox.lightUniform);
		}

		//pipeMine
		if (pipeMine.isEnable) {
			createBuffer(sizeof(pipeMine.ubo), pipeMine.uniform);
		}

		//pipeOSMirror
		if (pipeOSMirror.isEnable) {
			createBuffer(sizeof(pipeOSMirror.ubo),		pipeOSMirror.uniform);
			createBuffer(sizeof(pipeOSMirror.lightUbo), pipeOSMirror.lightUniform);
		}

		//pipeDSScence
		if (pipeDSScence.isEnable) {
			createBuffer(sizeof(pipeDSScence.lightUbo), pipeDSScence.lightUniform);
			createBuffer(sizeof(pipeDSScence.shadowUbo), pipeDSScence.shadowUniform);
		}

		//shadow
		if (pipeShadow.isEnable) {
			//pipeTerrain
			if (pipeTerrain.isEnable) {
				//shadow-pipeTerrain
				createBuffer(sizeof(pipeShadow.shadow_t.shadowMvpUbo), pipeShadow.shadow_t.shadowMvpUniform);
			}

			//model
			if (pipeModel.isEnable) {
				//shadow-sNode
				for (int i = 0; i < bufferEnum::procStaticModelId::count; i++) {
					createBuffer(sizeof(pipeShadow.shadow_ms[i].shadowMvpUbo), pipeShadow.shadow_ms[i].shadowMvpUniform);
				}

				//shadow-dNode
				for (int i = 0; i < bufferEnum::procDynamicModelId::count; i++) {
					for (int j = 0; j < pipeShadow.shadow_md[i].size(); j++) {
						createBuffer(sizeof(pipeShadow.shadow_md[i][j].shadowMvpUbo), pipeShadow.shadow_md[i][j].shadowMvpUniform);
					}
				}
			}
		}
	}

	void createTexture() {
		//model
		if (pipeModel.isEnable) {
			//model-sNode
			for (int i = 0; i < bufferEnum::procStaticModelId::count; i++) {
				pipeModel.sNode[i].texture.createKtx(pipeModel.sNode[i].textureFile, cmd, Device.device, Device.graphicsQueue, Device.memAllocator);
			}

			//model-dNode
			for (int i = 0; i < bufferEnum::procDynamicModelId::count; i++) {
				for (int j = 0; j < pipeModel.dNode[i].node.size(); j++) {
					pipeModel.dNode[i].node[j].texture.createKtx(pipeModel.dNode[i].node[j].textureFile, cmd, Device.device, Device.graphicsQueue, Device.memAllocator);
				}
			}
		}
	}

	void createDescriptorSet() {
		/*scenceForward_d*/
		{
			procDescriptor.setAllocInfo(bufferEnum::procDescriptorId::scenceForward_d);

			//pipeSkybox
			if (pipeSkybox.isEnable) {
				if (vkAllocateDescriptorSets(Device.device, &procDescriptor.allocInfo, &pipeSkybox.descriptorSet) == VK_SUCCESS) {
					std::array<VkWriteDescriptorSet, 2> tempWrite{
						procDescriptor.getWrite(0, pipeSkybox.descriptorSet, pipeSkybox.uniform.buffer, sizeof(pipeSkybox.ubo)),
						procDescriptor.getWrite(1, pipeSkybox.descriptorSet, pipeSkybox.lightUniform.buffer, sizeof(pipeSkybox.lightUbo))
					};

					vkUpdateDescriptorSets(Device.device, uint32_t(tempWrite.size()), tempWrite.data(), 0, nullptr);
				}
				else {
					Log::printError(__FILE__, __LINE__, "skybox分配描述符集失败");
				}
			}

			//pipeMine
			if (pipeMine.isEnable) {
				if (vkAllocateDescriptorSets(Device.device, &procDescriptor.allocInfo, &pipeMine.descriptorSet) == VK_SUCCESS) {
					std::array<VkWriteDescriptorSet, 1> tempWrite{
						procDescriptor.getWrite(0, pipeMine.descriptorSet, pipeMine.uniform.buffer, sizeof(pipeMine.ubo)),
					};

					vkUpdateDescriptorSets(Device.device, uint32_t(tempWrite.size()), tempWrite.data(), 0, nullptr);
				}
				else {
					Log::printError(__FILE__, __LINE__, "mine分配描述符集失败");
				}
			}

			//pipeOSMirror
			if (pipeOSMirror.isEnable) {
				if (vkAllocateDescriptorSets(Device.device, &procDescriptor.allocInfo, &pipeOSMirror.descriptorSet) == VK_SUCCESS) {
					std::array<VkWriteDescriptorSet, 3> tempWrite{
						procDescriptor.getWrite(0, pipeOSMirror.descriptorSet, pipeOSMirror.uniform.buffer,				 sizeof(pipeOSMirror.ubo)),
						procDescriptor.getWrite(1, pipeOSMirror.descriptorSet, pipeOSMirror.lightUniform.buffer,		 sizeof(pipeOSMirror.lightUbo)),
						procDescriptor.getWrite(2, pipeOSMirror.descriptorSet, procSampler.node[pipeOSMirror.samplerId], procRenderPass.getFrameImage(renderEnum::renderInstanceId::osMirror_ri, renderEnum::renderFrameId::conv_rf, 2))
					};

					vkUpdateDescriptorSets(Device.device, uint32_t(tempWrite.size()), tempWrite.data(), 0, nullptr);
				}
				else {
					Log::printError(__FILE__, __LINE__, "osMirror分配描述符集失败");
				}
			}

			//pipeOSScence
			if (pipeOSScence.isEnable) {
				if (vkAllocateDescriptorSets(Device.device, &procDescriptor.allocInfo, &pipeOSScence.descriptorSet) == VK_SUCCESS) {
					std::array<VkWriteDescriptorSet, 1> tempWrite{
						procDescriptor.getWrite(2, pipeOSScence.descriptorSet, procSampler.node[pipeOSScence.samplerId], procRenderPass.getFrameImage(renderEnum::renderInstanceId::osScence_ri, renderEnum::renderFrameId::conv_rf, 2))
					};

					vkUpdateDescriptorSets(Device.device, uint32_t(tempWrite.size()), tempWrite.data(), 0, nullptr);
				}
				else {
					Log::printError(__FILE__, __LINE__, "osScence分配描述符集失败");
				}
			}
		}

		/*scenceDeferred_d*/
		{
			procDescriptor.setAllocInfo(bufferEnum::procDescriptorId::scenceDeferred_d);

			//pipeTerrain
			if (pipeTerrain.isEnable) {
				if (vkAllocateDescriptorSets(Device.device, &procDescriptor.allocInfo, &pipeTerrain.descriptorSet) == VK_SUCCESS) {
					std::array<VkWriteDescriptorSet, 2> tempWrite{
						procDescriptor.getWrite(0, pipeTerrain.descriptorSet, pipeTerrain.uniform.buffer, sizeof(pipeTerrain.ubo)),
						procDescriptor.getWrite(1, pipeTerrain.descriptorSet, procSampler.node[pipeTerrain.samplerType], pipeTerrain.texture.imageNode.view),
					};

					vkUpdateDescriptorSets(Device.device, uint32_t(tempWrite.size()), tempWrite.data(), 0, nullptr);
				}
				else {
					Log::printError(__FILE__, __LINE__, "terrain分配描述符集失败");
				}
			}

			//model
			if (pipeModel.isEnable) {
				//model-sNode
				for (int i = 0; i < bufferEnum::procStaticModelId::count; i++) {
					if (vkAllocateDescriptorSets(Device.device, &procDescriptor.allocInfo, &pipeModel.sNode[i].descriptorSet) == VK_SUCCESS) {
						std::array<VkWriteDescriptorSet, 2> tempWrite{
							procDescriptor.getWrite(0, pipeModel.sNode[i].descriptorSet, pipeModel.sNode[i].uniform.buffer, sizeof(pipeModel.sNode[i].ubo)),
							procDescriptor.getWrite(1, pipeModel.sNode[i].descriptorSet, procSampler.node[pipeModel.sNode[i].samplerId], pipeModel.sNode[i].texture.imageNode.view)
						};

						vkUpdateDescriptorSets(Device.device, uint32_t(tempWrite.size()), tempWrite.data(), 0, nullptr);
					}
					else {
						Log::printError(__FILE__, __LINE__, "model-sNode_" + std::to_string(i) + ":\t分配描述符集失败");
					}
				}

				//model-dNode
				for (int i = 0; i < bufferEnum::procDynamicModelId::count; i++) {
					for (int j = 0; j < pipeModel.dNode[i].node.size(); j++) {
						if (vkAllocateDescriptorSets(Device.device, &procDescriptor.allocInfo, &pipeModel.dNode[i].node[j].descriptorSet) == VK_SUCCESS) {
							std::array<VkWriteDescriptorSet, 2> tempWrite{
								procDescriptor.getWrite(0, pipeModel.dNode[i].node[j].descriptorSet, pipeModel.dNode[i].node[j].uniform.buffer, sizeof(pipeModel.dNode[i].node[j].ubo)),
								procDescriptor.getWrite(1, pipeModel.dNode[i].node[j].descriptorSet, procSampler.node[pipeModel.dNode[i].samplerId], pipeModel.dNode[i].node[j].texture.imageNode.view)
							};

							vkUpdateDescriptorSets(Device.device, uint32_t(tempWrite.size()), tempWrite.data(), 0, nullptr);
						}
						else {
							Log::printError(__FILE__, __LINE__, "model-dNode_" + std::to_string(i) + "_" + std::to_string(j) + ":\t分配描述符集失败");
						}
					}
				}
			}
		}

		/*uiText_d*/
		{
			procDescriptor.setAllocInfo(bufferEnum::procDescriptorId::uiText_d);

			//pipeUIText
			if (pipeUIText.isEnable) {
				if (vkAllocateDescriptorSets(Device.device, &procDescriptor.allocInfo, &pipeUIText.descriptorSet) == VK_SUCCESS) {
					std::array<VkWriteDescriptorSet, 1> tempWrite{
						procDescriptor.getWrite(0, pipeUIText.descriptorSet, procSampler.node[pipeUIText.samplerId], pipeUIText.texture.imageNode.view)
					};

					vkUpdateDescriptorSets(Device.device, uint32_t(tempWrite.size()), tempWrite.data(), 0, nullptr);
				}
				else {
					Log::printError(__FILE__, __LINE__, "uiText分配描述符集失败");
				}
			}
		}

		/*dsScence_d*/
		if (pipeDSScence.isEnable) {
			procDescriptor.setAllocInfo(bufferEnum::procDescriptorId::dsScence_d);

			//pipeDSScence
			if (vkAllocateDescriptorSets(Device.device, &procDescriptor.allocInfo, &pipeDSScence.descriptorSet) == VK_SUCCESS) {
				std::array<VkWriteDescriptorSet, 6> tempWrite{
					procDescriptor.getWrite(0, pipeDSScence.descriptorSet, pipeDSScence.lightUniform.buffer,		   sizeof(pipeDSScence.lightUbo)),
					procDescriptor.getWrite(1, pipeDSScence.descriptorSet, pipeDSScence.shadowUniform.buffer,		   sizeof(pipeDSScence.shadowUbo)),
					procDescriptor.getWrite(2, pipeDSScence.descriptorSet, procSampler.node[pipeDSScence.samplerId],   procRenderPass.getFrameImage(renderEnum::renderInstanceId::dsScence_ri,  renderEnum::renderFrameId::conv_rf, 0)),
					procDescriptor.getWrite(3, pipeDSScence.descriptorSet, procSampler.node[pipeDSScence.samplerId],   procRenderPass.getFrameImage(renderEnum::renderInstanceId::dsScence_ri,  renderEnum::renderFrameId::conv_rf, 1)),
					procDescriptor.getWrite(4, pipeDSScence.descriptorSet, procSampler.node[pipeDSScence.samplerId],   procRenderPass.getFrameImage(renderEnum::renderInstanceId::dsScence_ri,  renderEnum::renderFrameId::conv_rf, 2)),
					procDescriptor.getWrite(5, pipeDSScence.descriptorSet, procSampler.node[pipeShadow.samplerId], procRenderPass.getFrameImage(renderEnum::renderInstanceId::shadowMap_ri, renderEnum::renderFrameId::conv_rf, 0))
				};

				vkUpdateDescriptorSets(Device.device, uint32_t(tempWrite.size()), tempWrite.data(), 0, nullptr);
			}
			else {
				Log::printError(__FILE__, __LINE__, "dsScence分配描述符集失败");
			}
		}

		/*shadowMap_d*/
		if (pipeShadow.isEnable) {
			procDescriptor.setAllocInfo(bufferEnum::procDescriptorId::shadowMap_d);

			//pipeTerrain
			if (pipeTerrain.isEnable) {
				//shadow-pipeTerrain
				if (vkAllocateDescriptorSets(Device.device, &procDescriptor.allocInfo, &pipeShadow.shadow_t.descriptorSet) == VK_SUCCESS) {
					std::array<VkWriteDescriptorSet, 1> tempWrite{
						procDescriptor.getWrite(0, pipeShadow.shadow_t.descriptorSet, pipeShadow.shadow_t.shadowMvpUniform.buffer, sizeof(pipeShadow.shadow_t.shadowMvpUbo)),
					};

					vkUpdateDescriptorSets(Device.device, uint32_t(tempWrite.size()), tempWrite.data(), 0, nullptr);
				}
				else {
					Log::printError(__FILE__, __LINE__, "shadow-terrain描述符集分配失败");
				}
			}

			//model
			if (pipeModel.isEnable) {
				//shadow-sNode
				for (int i = 0; i < bufferEnum::procStaticModelId::count; i++) {
					if (vkAllocateDescriptorSets(Device.device, &procDescriptor.allocInfo, &pipeShadow.shadow_ms[i].descriptorSet) == VK_SUCCESS) {
						std::array<VkWriteDescriptorSet, 1> tempWrite{
							procDescriptor.getWrite(0, pipeShadow.shadow_ms[i].descriptorSet, pipeShadow.shadow_ms[i].shadowMvpUniform.buffer, sizeof(pipeShadow.shadow_ms[i].shadowMvpUbo)),
						};

						vkUpdateDescriptorSets(Device.device, uint32_t(tempWrite.size()), tempWrite.data(), 0, nullptr);
					}
					else {
						Log::printError(__FILE__, __LINE__, "shadow-sNode描述符集分配失败");
					}
				}

				//shadow-dNode
				for (int i = 0; i < bufferEnum::procDynamicModelId::count; i++) {
					for (int j = 0; j < pipeShadow.shadow_md[i].size(); j++) {
						if (vkAllocateDescriptorSets(Device.device, &procDescriptor.allocInfo, &pipeShadow.shadow_md[i][j].descriptorSet) == VK_SUCCESS) {
							std::array<VkWriteDescriptorSet, 1> tempWrite{
								procDescriptor.getWrite(0, pipeShadow.shadow_md[i][j].descriptorSet, pipeShadow.shadow_md[i][j].shadowMvpUniform.buffer, sizeof(pipeShadow.shadow_md[i][j].shadowMvpUbo)),
							};

							vkUpdateDescriptorSets(Device.device, uint32_t(tempWrite.size()), tempWrite.data(), 0, nullptr);
						}
						else {
							Log::printError(__FILE__, __LINE__, "shadow-dNode描述符集分配失败");
						}
					}
				}
			}
		}
	}

	void initUniformBuffer() {
		//设置透视投影，参数依次是视野角度、宽高比、近裁剪面和远裁剪面，以及proj[1][1] *= -1轴向修正
		glm::mat4 perspProj = glm::perspective(glm::radians(45.0f), (float)InitSwapchain.swapExtent.width / (float)InitSwapchain.swapExtent.height, 0.1f, plateInfo::perspSightRange);
		perspProj[1][1]		*= -1;

		//设置正交投影
		glm::mat4 orthoProj = glm::ortho(-plateInfo::orthoSightRange, plateInfo::orthoSightRange, -plateInfo::orthoSightRange, plateInfo::orthoSightRange, 0.1f, plateInfo::perspSightRange);
		orthoProj[1][1]		*= -1;



		//pipeTerrain
		if (pipeTerrain.isEnable) {
			pipeTerrain.ubo.model	= glm::translate(glm::mat4(1), glm::vec3(0));
			pipeTerrain.ubo.modelTI	= glm::transpose(glm::inverse(pipeTerrain.ubo.model));
			pipeTerrain.ubo.proj	= perspProj;
		}

		//model
		if (pipeModel.isEnable) {
			//model-sNode
			for (int i = 0; i < bufferEnum::procStaticModelId::count; i++) {
				pipeModel.sNode[i].ubo.proj = perspProj;
			}

			//model-dNode
			for (int i = 0; i < bufferEnum::procDynamicModelId::count; i++) {
				for (int j = 0; j < pipeModel.dNode[i].node.size(); j++) {
					pipeModel.dNode[i].node[j].ubo.proj = perspProj;
				}
			}
		}

		//pipeSkybox
		if (pipeSkybox.isEnable) {
			pipeSkybox.ubo.proj = perspProj;
		}

		//pipeMine
		if (pipeMine.isEnable) {
			pipeMine.ubo.proj = perspProj;
		}

		//pipeOSMirror
		if (pipeOSMirror.isEnable) {
			pipeOSMirror.ubo.proj = perspProj;
			pipeOSMirror.lightUbo.gLightColor	= lightInfo::sunLightColor;
			pipeOSMirror.lightUbo.diffuseScale  = lightInfo::diffuseScale;
			pipeOSMirror.lightUbo.gLightDir		= lightInfo::sunLightDir;
			pipeOSMirror.lightUbo.specularScale = lightInfo::specularScale;
		}

		//pipeDSScence
		if (pipeDSScence.isEnable) {
			pipeDSScence.lightUbo.ambientScale	= lightInfo::ambientScale;
			pipeDSScence.lightUbo.diffuseScale	= lightInfo::diffuseScale;
			pipeDSScence.lightUbo.specularScale = lightInfo::specularScale;
		}

		//shadow
		if (pipeShadow.isEnable) {
			pipeShadow.shadowProj = orthoProj;
		}
	}



	//将vertexNode类中的顶点映射到buffer类里对应的VkBuffer
	void createVertexBuffer(auto& node, bufferStruct::baseBufferNode& vertexBuffer) {
		VkDeviceSize bufferSize = sizeof(node[0]) * node.size();

		bufferStruct::baseBufferNode cpuBuffer{ node.data(), bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, Device.memAllocator };
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer);
		cmd.copyBufferToBuffer(Device.device, Device.graphicsQueue, cpuBuffer.buffer, vertexBuffer.buffer, bufferSize);

		cpuBuffer.clear(Device.memAllocator);
	}

	//将vertexNode类中的索引映射到buffer类里对应的VkBuffer
	void createIndexBuffer(std::vector<uint32_t>& node, bufferStruct::baseBufferNode& indexBuffer) {
		VkDeviceSize bufferSize = sizeof(node[0]) * node.size();

		bufferStruct::baseBufferNode cpuBuffer{ node.data(), bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, Device.memAllocator };
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer);
		cmd.copyBufferToBuffer(Device.device, Device.graphicsQueue, cpuBuffer.buffer, indexBuffer.buffer, bufferSize);

		cpuBuffer.clear(Device.memAllocator);
	}

	//加载静态模型
	void loadStaticModel(const std::string& filename, std::vector<modelVertex::vertexNode>& vertexs, std::vector<uint32_t>& indexs, std::vector<shadowVertex::vertexNode>& shadowVertexs) {
		tinygltf::Model model;
		tinygltf::TinyGLTF loader;
		std::string err, warn;
		std::ifstream file(filename);
		bool ret;



		//检查文件是否存在  
		if (!file.good()) {
			Log::printError(__FILE__, __LINE__, "模型文件不存在: \t" + filename);
		}
		file.close();

		//加载glTF文件（支持二进制和ASCII格式）  
		if (filename.find(".glb") != std::string::npos) {
			ret = loader.LoadBinaryFromFile(&model, &err, &warn, filename);
		}
		else {
			ret = loader.LoadASCIIFromFile(&model, &err, &warn, filename);
		}

		/*if (!warn.empty()) {
			Log::print("glTF警告: " + warn);	//因为使用KTX管理贴图，故屏蔽gltf缺失贴图的报错
		}*/
		if (!ret) {
			Log::printError(__FILE__, __LINE__, "glTF加载失败:  \t" + err);
		}

		// 验证基本结构  
		if (model.meshes.empty()) {
			Log::printError(__FILE__, __LINE__, "glTF文件不包含任何mesh");
		}

		if (model.buffers.empty()) {
			Log::printError(__FILE__, __LINE__, "glTF文件不包含任何buffer数据");
		}



		//遍历所有mesh和primitive
		std::unordered_map<modelVertex::vertexNode, uint32_t> uniqueVertices{};

		for (const tinygltf::Mesh& mesh : model.meshes) {
			loadGltfModel(model, mesh, vertexs, indexs, shadowVertexs, uniqueVertices);
		}

		if (debugInfo::LogSwitch) {
			if (vertexs.empty()) {
				Log::printError(__FILE__, __LINE__, "加载的模型不包含任何顶点");
			}

			if (indexs.empty()) {
				Log::printError(__FILE__, __LINE__, "加载的模型不包含任何索引");
			}
		}
	}

	//加载动态模型
	void loadDynamicModel(const std::string& filename, std::vector<std::vector<modelVertex::vertexNode>>& vertexsList, std::vector<std::vector<uint32_t>>& indexsList, std::vector<std::vector<shadowVertex::vertexNode>>& shadowVertexsList) {
		tinygltf::Model model;
		tinygltf::TinyGLTF loader;
		std::string err, warn;
		std::ifstream file(filename);
		bool ret;



		//检查文件是否存在  
		if (!file.good()) {
			Log::printError(__FILE__, __LINE__, "模型文件不存在:  \t" + filename);
		}
		file.close();

		//加载glTF文件（支持二进制和ASCII格式）  
		if (filename.find(".glb") != std::string::npos) {
			ret = loader.LoadBinaryFromFile(&model, &err, &warn, filename);
		}
		else {
			ret = loader.LoadASCIIFromFile(&model, &err, &warn, filename);
		}

		/*if (!warn.empty()) {
			Log::print("glTF警告: " + warn);	//因为使用KTX管理贴图，故屏蔽gltf缺失贴图的报错
		}*/
		if (!ret) {
			Log::printError(__FILE__, __LINE__, "glTF加载失败: \t" + err);
		}

		if (model.meshes.empty()) {
			Log::printError(__FILE__, __LINE__, "glTF文件不包含任何mesh");
		}

		if (model.buffers.empty()) {
			Log::printError(__FILE__, __LINE__, "glTF文件不包含任何buffer数据");
		}



		//遍历所有mesh和primitive
		std::unordered_map<modelVertex::vertexNode, uint32_t> uniqueVertices{};

		for (int i = 0; i < model.meshes.size(); i++) {
			loadGltfModel(model, model.meshes[i], vertexsList[i], indexsList[i], shadowVertexsList[i], uniqueVertices);

			if (debugInfo::LogSwitch) {
				if (vertexsList[i].empty()) {
					Log::printError(__FILE__, __LINE__, "加载的模型不包含任何顶点");
				}

				if (indexsList[i].empty()) {
					Log::printError(__FILE__, __LINE__, "加载的模型不包含任何索引");
				}
			}
		}
	}
	
	void loadGltfModel(const tinygltf::Model& model, const tinygltf::Mesh& mesh, std::vector<modelVertex::vertexNode>& vertexs, std::vector<uint32_t>& indexs, std::vector<shadowVertex::vertexNode>& shadowVertexs, std::unordered_map<modelVertex::vertexNode, uint32_t>& uniqueVertices) {
		for (const auto& primitive : mesh.primitives) {
			//获取顶点数据  
			const float* positions = nullptr;
			const float* texCoords = nullptr;
			const float* normals   = nullptr;
			size_t vertexCount = 0;

			//获取POSITION属性  
			if (primitive.attributes.find("POSITION") != primitive.attributes.end()) {
				const auto& accessor = model.accessors[primitive.attributes.at("POSITION")];
				const auto& bufferView = model.bufferViews[accessor.bufferView];
				const auto& buffer = model.buffers[bufferView.buffer];

				positions = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
				vertexCount = accessor.count;
			}
			else {
				Log::printWarning(__FILE__, __LINE__, "primitive缺少POSITION属性");
				continue;
			}

			//获取TEXCOORD_0属性  
			if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
				const auto& accessor = model.accessors[primitive.attributes.at("TEXCOORD_0")];
				const auto& bufferView = model.bufferViews[accessor.bufferView];
				const auto& buffer = model.buffers[bufferView.buffer];

				texCoords = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
			}
			else {
				Log::printWarning(__FILE__, __LINE__, "primitive缺少TEXCOORD_0属性");
				texCoords = nullptr;
			}

			//获取NORMAL属性 - 添加法线加载
			if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
				const auto& accessor = model.accessors[primitive.attributes.at("NORMAL")];
				const auto& bufferView = model.bufferViews[accessor.bufferView];
				const auto& buffer = model.buffers[bufferView.buffer];

				normals = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
			}
			else {
				Log::printWarning(__FILE__, __LINE__, "primitive缺少NORMAL属性，将使用默认法线");
				normals = nullptr;
			}

			//存储当前primitive的顶点映射  
			std::vector<uint32_t> primitiveVertexIndices;

			for (size_t i = 0; i < vertexCount; i++) {
				modelVertex::vertexNode vertex{};

				//坐标系转换：从glTF的Y向上转换为本引擎的Z向上
				vertex.position = {
					positions[i * 3 + 0],     //X保持不变  
					positions[i * 3 + 2],     //Z -> Y (交换Y和Z)  
					-positions[i * 3 + 1]     //-Y -> Z (翻转Y轴并赋给Z)  
				};

				//UV坐标处理
				if (texCoords) {
					vertex.texCoord = {
						texCoords[i * 2 + 0],
						texCoords[i * 2 + 1]      //glTF标准UV，不翻转  
					};
				}
				else {
					vertex.texCoord = { 0.0f, 0.0f }; //默认UV坐标
				}

				//法线处理 - 添加法线加载
				if (normals) {
					//法线也需要进行坐标系转换
					vertex.normal = {
						normals[i * 3 + 0],      //X保持不变
						normals[i * 3 + 2],      //Z -> Y (交换Y和Z)
						-normals[i * 3 + 1]      //-Y -> Z (翻转Y轴并赋给Z)
					};

					//标准化法线
					float length = std::sqrt(
						vertex.normal.x * vertex.normal.x +
						vertex.normal.y * vertex.normal.y +
						vertex.normal.z * vertex.normal.z
					);

					if (length > 0.0f) {
						vertex.normal.x /= length;
						vertex.normal.y /= length;
						vertex.normal.z /= length;
					}
				}
				else {
					//如果没有法线数据，设置为默认上方向（根据坐标系转换）
					vertex.normal = { 0.0f, 1.0f, 0.0f }; //Y轴向上
				}

				//使用顶点去重
				if (uniqueVertices.count(vertex) == 0) {
					uniqueVertices[vertex] = static_cast<uint32_t>(vertexs.size());

					vertexs.push_back(vertex);
					shadowVertexs.push_back(shadowVertex::vertexNode{ vertex.position });
				}

				//记录原始顶点到去重后顶点的映射
				primitiveVertexIndices.push_back(uniqueVertices[vertex]);
			}

			//处理索引数据  
			if (primitive.indices >= 0) {
				const auto& accessor = model.accessors[primitive.indices];
				const auto& bufferView = model.bufferViews[accessor.bufferView];
				const auto& buffer = model.buffers[bufferView.buffer];

				size_t indexCount = accessor.count;

				if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
					const uint16_t* indices = reinterpret_cast<const uint16_t*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);

					for (size_t i = 0; i < indexCount; i++) {
						uint32_t originalVertexIndex = indices[i];
						if (originalVertexIndex < primitiveVertexIndices.size()) {
							indexs.push_back(primitiveVertexIndices[originalVertexIndex]);
						}
					}
				}
				else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
					const uint32_t* indices = reinterpret_cast<const uint32_t*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);

					for (size_t i = 0; i < indexCount; i++) {
						uint32_t originalVertexIndex = indices[i];
						if (originalVertexIndex < primitiveVertexIndices.size()) {
							indexs.push_back(primitiveVertexIndices[originalVertexIndex]);
						}
					}
				}
				else {
					Log::printWarning(__FILE__, __LINE__, "不支持的索引类型");
				}
			}
			else {
				//如果没有索引，生成三角形索引
				for (size_t i = 0; i < vertexCount; i += 3) {
					indexs.push_back(primitiveVertexIndices[i]);
					indexs.push_back(primitiveVertexIndices[i + 1]);
					indexs.push_back(primitiveVertexIndices[i + 2]);
				}
			}
		}
	}




	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, bufferStruct::baseBufferNode& baseBufferNode) {
		VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferInfo.size = size;
		bufferInfo.usage = usage;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_UNKNOWN; //使用 VMA_MEMORY_USAGE_UNKNOWN 将内存属性交由属性参数控制
		allocInfo.requiredFlags = properties;		//设置所需的内存属性

		vmaCreateBuffer(Device.memAllocator, &bufferInfo, &allocInfo, &baseBufferNode.buffer, &baseBufferNode.bufferMem, nullptr);
	}

	//为createUniformBuffer服务的简略版
	void createBuffer(VkDeviceSize size, bufferStruct::baseBufferNode& baseBufferNode) {
		VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferInfo.size = size;
		bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_UNKNOWN;																//使用 VMA_MEMORY_USAGE_UNKNOWN 将内存属性交由属性参数控制
		allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;	//设置所需的内存属性

		vmaCreateBuffer(Device.memAllocator, &bufferInfo, &allocInfo, &baseBufferNode.buffer, &baseBufferNode.bufferMem, nullptr);
	}

	void createMemMap(VmaAllocation tempMemory, const void* sourceData, size_t dataSize) {
		//内存映射
		void* data;
		vmaMapMemory(Device.memAllocator, tempMemory, &data);
		memcpy(data, sourceData, dataSize);
		vmaUnmapMemory(Device.memAllocator, tempMemory);
	}



	//todo mip与array 倒置，待删除
	void createImage(uint32_t width, uint32_t height, uint32_t arrayLayer, uint32_t mipLevel, VkFormat format, VkSampleCountFlagBits numSamples, VkImageUsageFlags usage, VkImage& image, VmaAllocation& allocation) {
		VkImageCreateInfo imageInfo = {};
		imageInfo.sType			= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType		= VK_IMAGE_TYPE_2D;
		imageInfo.format		= format;
		imageInfo.extent.width	= width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth	= 1;
		imageInfo.mipLevels		= mipLevel;
		imageInfo.arrayLayers	= arrayLayer;
		imageInfo.samples		= numSamples;
		imageInfo.tiling		= VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage			= usage;
		imageInfo.sharingMode	= VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage			= VMA_MEMORY_USAGE_GPU_ONLY;
		allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		if (vmaCreateImage(Device.memAllocator, &imageInfo, &allocInfo, &image, &allocation, nullptr) != VK_SUCCESS) {
			Log::printError(__FILE__, __LINE__, "图像创建失败");
		}
	}

	//todo mip与array 倒置，待删除
	void createImageView(const VkImage& image, VkImageView& view, uint32_t arrayLayer, uint32_t mipLevel, VkImageViewType viewType, VkFormat format, VkImageAspectFlags aspectFlags) {
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType							 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image							 = image;
		viewInfo.viewType						 = viewType;
		viewInfo.format							 = format;
		viewInfo.subresourceRange.aspectMask	 = aspectFlags;
		viewInfo.subresourceRange.baseMipLevel	 = 0;
		viewInfo.subresourceRange.levelCount	 = mipLevel;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount	 = arrayLayer;

		if (vkCreateImageView(Device.device, &viewInfo, nullptr, &view) != VK_SUCCESS) {
			Log::printError(__FILE__, __LINE__, "图像视图创建失败");
		}
	}

public:
	//创建cmd
	void createCmd(int threadId, pipelineList::procPipelineNode& procPipeline) {
		for (int frameId = 0; frameId < cmd.renderCmd[threadId].size(); frameId++) {
			cmd.recordCmd(threadId, frameId, [&](cmdStruct::cmdRecorderNode& recorder) {
				/*shadowMap_ri*/
				{
					recorder.beginRender(&procRenderPass.getRenderBegin(renderEnum::renderInstanceId::shadowMap_ri));

					if (pipeShadow.isEnable) {
						//pipeTerrain
						if (pipeTerrain.isEnable) {
							for (int layer = 0; layer < plateEnum::terrainLayer::count; layer++) {
								if (isNullTerrainMesh(layer)) {
									recorder.draw(pipelineEnum::cmdDrawId::indexed, procPipeline.getPipeline(pipeShadow.pipeId), procPipeline.getPipeLayout(pipeShadow.pipeId), pipeShadow.shadow_t.descriptorSet, pipeShadow.shadow_t.vertex[threadId][layer].buffer, pipeTerrain.index[threadId][layer].buffer, (uint32_t)terrainVertex::indexs[layer].size());
								}
							}
						}

						//model
						if (pipeModel.isEnable) {
							for (int nodeId = 0; nodeId < bufferEnum::procStaticModelId::count; nodeId++) {
								if (pipeModel.sNode[nodeId].isVisible) {
									recorder.draw(pipelineEnum::cmdDrawId::indexed, procPipeline.getPipeline(pipeShadow.pipeId), procPipeline.getPipeLayout(pipeShadow.pipeId), pipeShadow.shadow_ms[nodeId].descriptorSet, pipeShadow.shadow_ms[nodeId].vertex.buffer, pipeModel.sNode[nodeId].index.buffer, (uint32_t)modelVertex::sIndexs[nodeId].size());
								}
							}

							for (int nodeId = 0; nodeId < bufferEnum::procDynamicModelId::count; nodeId++) {
								if (pipeModel.dNode[nodeId].isVisible) {
									for (int subNode = 0; subNode < pipeShadow.shadow_md[nodeId].size(); subNode++) {
										recorder.draw(pipelineEnum::cmdDrawId::indexed, procPipeline.getPipeline(pipeShadow.pipeId), procPipeline.getPipeLayout(pipeShadow.pipeId), pipeShadow.shadow_md[nodeId][subNode].descriptorSet, pipeShadow.shadow_md[nodeId][subNode].vertex.buffer, pipeModel.dNode[nodeId].node[subNode].index.buffer, (uint32_t)modelVertex::dIndexs[nodeId][subNode].size());
									}
								}
							}
						}
					}

					recorder.endRender();
				}

				/*dsScence_ri*/
				{
					recorder.beginRender(&procRenderPass.getRenderBegin(renderEnum::renderInstanceId::dsScence_ri));

					//pipeTerrain
					if (pipeTerrain.isEnable) {
						for (int layer = 0; layer < plateEnum::terrainLayer::count; layer++) {
							if (isNullTerrainMesh(layer)) {
								recorder.draw(pipelineEnum::cmdDrawId::indexed, procPipeline.getPipeline(pipeTerrain.pipeType), procPipeline.getPipeLayout(pipeTerrain.pipeType), pipeTerrain.descriptorSet, pipeTerrain.vertex[threadId][layer].buffer, pipeTerrain.index[threadId][layer].buffer, (uint32_t)terrainVertex::indexs[layer].size());
							}
						}
					}

					//model
					if (pipeModel.isEnable) {
						for (int nodeId = 0; nodeId < bufferEnum::procStaticModelId::count; nodeId++) {
							if (pipeModel.sNode[nodeId].isVisible) {
								recorder.draw(pipelineEnum::cmdDrawId::indexed, procPipeline.getPipeline(pipeModel.pipeId), procPipeline.getPipeLayout(pipeModel.pipeId), pipeModel.sNode[nodeId].descriptorSet, pipeModel.sNode[nodeId].vertex.buffer, pipeModel.sNode[nodeId].index.buffer, (uint32_t)modelVertex::sIndexs[nodeId].size());
							}
						}

						for (int nodeId = 0; nodeId < bufferEnum::procDynamicModelId::count; nodeId++) {
							if (pipeModel.dNode[nodeId].isVisible) {
								for (int subNode = 0; subNode < pipeModel.dNode[nodeId].node.size(); subNode++) {
									recorder.draw(pipelineEnum::cmdDrawId::indexed, procPipeline.getPipeline(pipeModel.pipeId), procPipeline.getPipeLayout(pipeModel.pipeId), pipeModel.dNode[nodeId].node[subNode].descriptorSet, pipeModel.dNode[nodeId].node[subNode].vertex.buffer, pipeModel.dNode[nodeId].node[subNode].index.buffer, (uint32_t)modelVertex::dIndexs[nodeId][subNode].size());
								}
							}
						}
					}

					//pipeSkybox
					if (pipeSkybox.isEnable) {
						recorder.draw(pipelineEnum::cmdDrawId::indexed, procPipeline.getPipeline(pipeSkybox.pipeId), procPipeline.getPipeLayout(pipeSkybox.pipeId), pipeSkybox.descriptorSet, pipeSkybox.vertex.buffer, pipeSkybox.index.buffer, (uint32_t)skyboxVertex::indexs.size());
					}

					//pipeMine
					if (pipeMine.isEnable) {
						recorder.draw(pipelineEnum::cmdDrawId::indexed, procPipeline.getPipeline(pipeMine.pipeId), procPipeline.getPipeLayout(pipeMine.pipeId), pipeMine.descriptorSet, pipeMine.vertex.buffer, pipeMine.index.buffer, (uint32_t)mineVertex::indexs.size());
					}

					recorder.endRender();
				}

				/*osMirror_ri*/
				if (pipeOSMirror.isEnable) {
					recorder.beginRender(&procRenderPass.getRenderBegin(renderEnum::renderInstanceId::osMirror_ri));

					recorder.endRender();
				}

				/*osScence_ri*/
				if (pipeOSScence.isEnable) {
					recorder.beginRender(&procRenderPass.getRenderBegin(renderEnum::renderInstanceId::osScence_ri));

					//pipeOSMirror
					if (pipeOSMirror.isEnable) {
						recorder.draw(pipelineEnum::cmdDrawId::indexed, procPipeline.getPipeline(pipeOSMirror.pipeId), procPipeline.getPipeLayout(pipeOSMirror.pipeId), pipeOSMirror.descriptorSet, pipeOSMirror.vertex.buffer, pipeOSMirror.index.buffer, (uint32_t)osMirrorVertex::indexs.size());
					}

					recorder.endRender();
				}

				/*scScence_ri*/
				{
					recorder.barrier();	//创建延迟渲染管线屏障，用于确保延迟渲染管线最后执行
					recorder.beginRender(&procRenderPass.getRenderBegin(renderEnum::renderInstanceId::scScence_ri, frameId));

					//pipeOSScence
					if (pipeOSScence.isEnable) {
						recorder.draw(pipelineEnum::cmdDrawId::fullscreen, procPipeline.getPipeline(pipeOSScence.pipeId), procPipeline.getPipeLayout(pipeOSScence.pipeId), pipeOSScence.descriptorSet, VK_NULL_HANDLE, VK_NULL_HANDLE, 0, InitSwapchain.swapExtent);
					}

					//pipeDSScence
					if (pipeDSScence.isEnable) {
						recorder.draw(pipelineEnum::cmdDrawId::fullscreen, procPipeline.getPipeline(pipeDSScence.pipeId), procPipeline.getPipeLayout(pipeDSScence.pipeId), pipeDSScence.descriptorSet, VK_NULL_HANDLE, VK_NULL_HANDLE, 0, InitSwapchain.swapExtent);

						//pipeUIText
						if (pipeUIText.isEnable) {
							recorder.draw(pipelineEnum::cmdDrawId::text, procPipeline.getPipeline(pipeUIText.pipeId), procPipeline.getPipeLayout(pipeUIText.pipeId), pipeUIText.descriptorSet, pipeUIText.vertex.buffer, VK_NULL_HANDLE, pipeUIText.msdfGlyphAtlas.glyphCount);
						}
					}

					recorder.endRender();
				}
			});
		}
	}

	//重建cmd
	void recreateCmd(int threadId, std::atomic<bool>& isEntireRebuild, const glm::dvec3& plateOriginPos, Camera& Camera, pipelineList::procPipelineNode& procPipeline) {
		if (!pipeTerrain.isEnable) { return; }
		if (debugInfo::LogSwitch && (playerInfo::logicOriginPos != plateOriginPos) && !isEntireRebuild) {
			Log::printError(__FILE__, __LINE__, "原点迁移时isEntireRebuild为假");
		}



		int layer = plateEnum::terrainLayer::Layer_0;
		cmd.waitCmd(Device.device, threadId);

		//重建下一线程Lod_0地形管线，RM_0.4.6（8_32：10）
		if (isNullTerrainMesh(layer)) {
			vmaDestroyBuffer(Device.memAllocator, pipeTerrain.vertex[threadId][layer].buffer, pipeTerrain.vertex[threadId][layer].bufferMem);
			vmaDestroyBuffer(Device.memAllocator, pipeTerrain.index[threadId][layer].buffer,  pipeTerrain.index[threadId][layer].bufferMem);

			createVertexBuffer(terrainVertex::vertexs[layer], pipeTerrain.vertex[threadId][layer]);
			createIndexBuffer (terrainVertex::indexs[layer],  pipeTerrain.index[threadId][layer]);

			//重建下一线程Lod_0地形阴影
			if (pipeShadow.isEnable) {
				vmaDestroyBuffer(Device.memAllocator, pipeShadow.shadow_t.vertex[threadId][layer].buffer, pipeShadow.shadow_t.vertex[threadId][layer].bufferMem);

				shadowVertex::loadTerrainVertexs(layer);
				createVertexBuffer(shadowVertex::tVertexs[layer], pipeShadow.shadow_t.vertex[threadId][layer]);
			}
		}

		//重建全部线程Lod_1地形管线，RM_0.4.6（8_32：20~50）
		if (isEntireRebuild.load()) {
			layer = plateEnum::terrainLayer::Layer_1;

			if (isNullTerrainMesh(layer)) {
				const std::vector<terrainVertex::vertexNode>& vertex = terrainVertex::vertexs[layer];
				const std::vector<uint32_t>&				  index  = terrainVertex::indexs[layer];
				VkDeviceSize vertexSize = sizeof(vertex[0]) * vertex.size();
				VkDeviceSize indexSize  = sizeof(index[0])  * index.size();
				bufferStruct::baseBufferNode vertexBuffer{ vertex.data(), vertexSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, Device.memAllocator };
				bufferStruct::baseBufferNode indexBuffer { index.data(),  indexSize,  VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, Device.memAllocator };

				//重建下一线程Lod_1地形管线
				vmaDestroyBuffer(Device.memAllocator, pipeTerrain.vertex[threadId][layer].buffer, pipeTerrain.vertex[threadId][layer].bufferMem);
				vmaDestroyBuffer(Device.memAllocator, pipeTerrain.index[threadId][layer].buffer,  pipeTerrain.index[threadId][layer].bufferMem);
				createBuffer(vertexSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, pipeTerrain.vertex[threadId][layer]);
				createBuffer(indexSize,  VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, pipeTerrain.index[threadId][layer]);
				cmd.copyBufferToBuffer(Device.device, Device.graphicsQueue, vertexBuffer.buffer, pipeTerrain.vertex[threadId][layer].buffer, vertexSize);
				cmd.copyBufferToBuffer(Device.device, Device.graphicsQueue, indexBuffer.buffer,  pipeTerrain.index[threadId][layer].buffer,  indexSize);

				//重建下一线程Lod_1地形阴影
				if (pipeShadow.isEnable) {
					vmaDestroyBuffer(Device.memAllocator, pipeShadow.shadow_t.vertex[threadId][layer].buffer, pipeShadow.shadow_t.vertex[threadId][layer].bufferMem);

					shadowVertex::loadTerrainVertexs(layer);
					createVertexBuffer(shadowVertex::tVertexs[layer], pipeShadow.shadow_t.vertex[threadId][layer]);
				}

				//重建下一线程命令缓冲区，并切换线程
				lockInfo::drawFuncLock.lock();
				if (playerInfo::logicOriginPos != plateOriginPos) {
					playerInfo::logicOriginPos = plateOriginPos;
					Camera.updateView();

					Log::print("原点迁移");
				}

				lockInfo::queueThreadLock.lock();
				cmd.renderCmd[threadId].recreateCmdList(Device.device);
				createCmd(threadId, procPipeline);
				cmd.swap();
				lockInfo::queueThreadLock.unlock();
				lockInfo::drawFuncLock.unlock();



				//重建上轮当前线程Lod_1地形管线
				int tempThreadId = (threadId + 1) % 2;
				cmd.waitCmd(Device.device, tempThreadId);

				vmaDestroyBuffer(Device.memAllocator, pipeTerrain.vertex[tempThreadId][layer].buffer, pipeTerrain.vertex[tempThreadId][layer].bufferMem);
				vmaDestroyBuffer(Device.memAllocator, pipeTerrain.index[tempThreadId][layer].buffer,  pipeTerrain.index[tempThreadId][layer].bufferMem);
				createBuffer(vertexSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, pipeTerrain.vertex[tempThreadId][layer]);
				createBuffer(indexSize,  VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, pipeTerrain.index[tempThreadId][layer]);
				cmd.copyBufferToBuffer(Device.device, Device.graphicsQueue, vertexBuffer.buffer, pipeTerrain.vertex[tempThreadId][layer].buffer, vertexSize);
				cmd.copyBufferToBuffer(Device.device, Device.graphicsQueue, indexBuffer.buffer,  pipeTerrain.index[tempThreadId][layer].buffer,  indexSize);

				//重建上轮当前线程Lod_1地形阴影
				if (pipeShadow.isEnable) {
					vmaDestroyBuffer(Device.memAllocator, pipeShadow.shadow_t.vertex[tempThreadId][layer].buffer, pipeShadow.shadow_t.vertex[tempThreadId][layer].bufferMem);

					shadowVertex::loadTerrainVertexs(layer);
					createVertexBuffer(shadowVertex::tVertexs[layer], pipeShadow.shadow_t.vertex[tempThreadId][layer]);
				}

				vertexBuffer.clear(Device.memAllocator);
				indexBuffer .clear(Device.memAllocator);

				isEntireRebuild.store(false);
				return;
			}

			isEntireRebuild.store(false);
		}



		//重建下一线程命令缓冲区，并切换线程
		lockInfo::drawFuncLock.lock();
		if (playerInfo::logicOriginPos != plateOriginPos) {
			playerInfo::logicOriginPos = plateOriginPos;
			Camera.updateView();

			Log::print("原点迁移");
		}

		lockInfo::queueThreadLock.lock();
		cmd.renderCmd[threadId].recreateCmdList(Device.device);
		createCmd(threadId, procPipeline);
		cmd.swap();
		lockInfo::queueThreadLock.unlock();
		lockInfo::drawFuncLock.unlock();
	}

	//更新Uniform
	void updateUniform(const glm::dvec3 logicOriginPos, const playerStruct::cameraNode& camera, const playerStruct::selectBoxNode& box) {
		static std::chrono::steady_clock::time_point time0 = std::chrono::steady_clock::now();	//游戏起始时刻
		static std::chrono::steady_clock::time_point time1;										//游戏当前时刻
		static float gameTime;																	//游戏运行时长，最多支持连续运行194天（现实时间）
		static float currentHour;																//当前小时，取值范围[0,24)，含小数部分
		glm::vec3 offsetPos = playerInfo::initSpawnPos - logicOriginPos;
		glm::vec3 playerPos = camera.pos - logicOriginPos;

		time1		= std::chrono::steady_clock::now();
		gameTime	= std::chrono::duration_cast<std::chrono::milliseconds>(time1 - time0).count() / 1000.0f;
		currentHour = std::fmod(gameTime, 24.0f);



		//pipeTerrain
		if (pipeTerrain.isEnable) {
			pipeTerrain.ubo.view = camera.view;

			createMemMap(pipeTerrain.uniform.bufferMem, &pipeTerrain.ubo, sizeof(pipeTerrain.ubo));
		}

		//model
		if (pipeModel.isEnable) {
			//model-sNode
			for (int i = 0; i < bufferEnum::procStaticModelId::count; i++) {
				pipeModel.sNode[i].currentWorldPos = camera.pos;

				if (glm::distance(pipeModel.sNode[i].modelWorldPos, pipeModel.sNode[i].currentWorldPos) < plateInfo::perspSightRange) {
					pipeModel.sNode[i].ubo.model = glm::translate(glm::mat4(1), glm::vec3(pipeModel.sNode[i].modelWorldPos - logicOriginPos));	//位移
					pipeModel.sNode[i].ubo.model *= glm::scale(glm::mat4(1), glm::vec3(0.1f));												//缩放
					//pipeModel.sNode[i].ubo.model	*= glm::rotate(glm::mat4(1), gameTime * glm::radians(60.f), cameraInfo::worldUp);							//旋转
					pipeModel.sNode[i].ubo.modelTI = glm::transpose(glm::inverse(pipeModel.sNode[i].ubo.model));
					pipeModel.sNode[i].ubo.view = camera.view;

					createMemMap(pipeModel.sNode[i].uniform.bufferMem, &pipeModel.sNode[i].ubo, sizeof(pipeModel.sNode[i].ubo));

					pipeModel.sNode[i].isVisible = true;
				}
				else {
					pipeModel.sNode[i].isVisible = false;
				}
			}

			//model-dNode
			for (int i = 0; i < bufferEnum::procDynamicModelId::count; i++) {
				pipeModel.dNode[i].currentWorldPos = camera.pos;

				if (glm::distance(pipeModel.dNode[i].modelWorldPos, pipeModel.dNode[i].currentWorldPos) < plateInfo::perspSightRange) {
					for (int j = 0; j < pipeModel.dNode[i].node.size(); j++) {
						pipeModel.dNode[i].node[j].ubo.model = glm::translate(glm::mat4(1), glm::vec3(pipeModel.dNode[i].modelWorldPos - logicOriginPos));	//位移
						pipeModel.dNode[i].node[j].ubo.model *= glm::scale(glm::mat4(1), glm::vec3(0.1f));												//缩放
						pipeModel.dNode[i].node[j].ubo.modelTI = glm::transpose(glm::inverse(pipeModel.dNode[i].node[j].ubo.model));
						pipeModel.dNode[i].node[j].ubo.view = camera.view;

						createMemMap(pipeModel.dNode[i].node[j].uniform.bufferMem, &pipeModel.dNode[i].node[j].ubo, sizeof(pipeModel.dNode[i].node[j].ubo));
					}

					pipeModel.dNode[i].isVisible = true;
				}
				else {
					pipeModel.dNode[i].isVisible = false;
				}
			}
		}

		//pipeSkybox
		if (pipeSkybox.isEnable) {
			pipeSkybox.ubo.view				= camera.view;
			pipeSkybox.lightUbo.currentHour = currentHour;

			createMemMap(pipeSkybox.uniform.bufferMem,	  &pipeSkybox.ubo,		sizeof(pipeSkybox.ubo));
			createMemMap(pipeSkybox.lightUniform.bufferMem, &pipeSkybox.lightUbo, sizeof(pipeSkybox.lightUbo));
		}

		//pipeMine
		if (pipeMine.isEnable) {
			pipeMine.ubo.model = glm::translate(glm::mat4(1), box.minepos);
			pipeMine.ubo.model *= glm::scale(glm::mat4(1), glm::vec3(1) * box.isVisible);
			pipeMine.ubo.view  = camera.view;

			createMemMap(pipeMine.uniform.bufferMem, &pipeMine.ubo, sizeof(pipeMine.ubo));
		}

		//showUIText
		if (pipeUIText.isEnable) {
			showUIText.pos		= glm::floor(camera.pos);
			showUIText.chunkPos = glm::floor(camera.pos / (double)plateInfo::chunkLength);
			showUIText.minepos  = box.minepos;

			//方块数组真实坐标
			//showUIText.minepos.x = static_cast<int>(fmod(box.minepos.x, plateInfo::chunkLength));
			//showUIText.minepos.y = static_cast<int>(fmod(box.minepos.y, plateInfo::chunkLength));
			//showUIText.minepos.z = static_cast<int>(fmod(box.minepos.z, plateInfo::chunkLength));
		}

		//pipeOSMirror
		if (pipeOSMirror.isEnable) {
			pipeOSMirror.ubo.model				= glm::translate(glm::mat4(1), glm::vec3(-16.0f, 24.0f, -16.0f));
			pipeOSMirror.ubo.model				*= glm::scale(glm::mat4(1), glm::vec3(32.0f));
			pipeOSMirror.ubo.modelTI			= glm::transpose(glm::inverse(pipeOSMirror.ubo.model));
			pipeOSMirror.ubo.view				= camera.view;
			pipeOSMirror.lightUbo.viewPos		= glm::vec3{ camera.pos - logicOriginPos };
			pipeOSMirror.lightUbo.currentHour	= currentHour;
			pipeOSMirror.lightUbo.ambientScale	= lightInfo::ambientScale;
			pipeOSMirror.lightUbo.gLightPos		= offsetPos;
			pipeOSMirror.lightUbo.flashPos		= glm::vec3{ camera.pos - logicOriginPos };
			pipeOSMirror.lightUbo.flashDir		= camera.front;
			pipeOSMirror.lightUbo.flashCutOff	= glm::cos(glm::radians(12.5f));
			pipeOSMirror.lightUbo.outerCutOff	= glm::cos(glm::radians(14.5f));

			createMemMap(pipeOSMirror.uniform.bufferMem, &pipeOSMirror.ubo, sizeof(pipeOSMirror.ubo));
			createMemMap(pipeOSMirror.lightUniform.bufferMem, &pipeOSMirror.lightUbo, sizeof(pipeOSMirror.lightUbo));
		}

		//shadow
		if (pipeShadow.isEnable) {
			float	   hourRad = (currentHour / 24.0f) * 2.0f * glm::pi<float>();															//0~2π
			float	   radius = 32.0f;																										//旋转半径
			glm::vec3  lightPos = glm::vec3(radius * cos(hourRad), 64.0f, radius * sin(hourRad));											//光源绕Y轴旋转的坐标，无需减逻辑原点偏移
			pipeShadow.shadowVP = pipeShadow.shadowProj * glm::lookAt(playerPos + lightPos/*摄像机坐标*/, playerPos/*被观测坐标*/, cameraInfo::worldUp);	//为实现阴影跟随玩家故加上玩家坐标

			//pipeTerrain
			if (pipeTerrain.isEnable) {
				//shadow-pipeTerrain
				pipeShadow.shadow_t.shadowMvpUbo.mvp = pipeShadow.shadowVP * pipeTerrain.ubo.model;
				createMemMap(pipeShadow.shadow_t.shadowMvpUniform.bufferMem, &pipeShadow.shadow_t.shadowMvpUbo, sizeof(pipeShadow.shadow_t.shadowMvpUbo));
			}

			//model
			if (pipeModel.isEnable) {
				//shadow-sNode
				for (int i = 0; i < bufferEnum::procStaticModelId::count; i++) {
					if (pipeModel.sNode[i].isVisible) {
						pipeShadow.shadow_ms[i].shadowMvpUbo.mvp = pipeShadow.shadowVP * pipeModel.sNode[i].ubo.model;
						createMemMap(pipeShadow.shadow_ms[i].shadowMvpUniform.bufferMem, &pipeShadow.shadow_ms[i].shadowMvpUbo, sizeof(pipeShadow.shadow_ms[i].shadowMvpUbo));
					}
				}

				//shadow-dNode
				for (int i = 0; i < bufferEnum::procDynamicModelId::count; i++) {
					if (pipeModel.dNode[i].isVisible) {
						for (int j = 0; j < pipeShadow.shadow_md[i].size(); j++) {
							pipeShadow.shadow_md[i][j].shadowMvpUbo.mvp = pipeShadow.shadowVP * pipeModel.dNode[i].node[j].ubo.model;
							createMemMap(pipeShadow.shadow_md[i][j].shadowMvpUniform.bufferMem, &pipeShadow.shadow_md[i][j].shadowMvpUbo, sizeof(pipeShadow.shadow_md[i][j].shadowMvpUbo));
						}
					}
				}
			}
		}

		//pipeDSScence
		if (pipeDSScence.isEnable) {
			pipeDSScence.lightUbo.viewPos	  = glm::vec3{ camera.pos - logicOriginPos };		//位置
			pipeDSScence.lightUbo.currentHour = currentHour;								//时间
			
			//点光源
			pipeDSScence.lightUbo.pointLightList[0].position = offsetPos;					//位置
			pipeDSScence.lightUbo.pointLightList[0].radius	 = 16.0f;						//半径
			pipeDSScence.lightUbo.pointLightList[0].color	 = glm::vec3(1.0f);				//颜色
		
			//平行光
			pipeDSScence.lightUbo.dirLightList[0].direction = lightInfo::sunLightDir;		//方向
			pipeDSScence.lightUbo.dirLightList[0].scale		= 0.5f;							//强度
			pipeDSScence.lightUbo.dirLightList[0].color		= lightInfo::sunLightColor;		//颜色
		
			//聚光灯
			pipeDSScence.lightUbo.spotLightList[0].position  = glm::vec3{ camera.pos - logicOriginPos };		//位置
			pipeDSScence.lightUbo.spotLightList[0].inner	 = glm::cos(glm::radians(12.5f));			//内圆锥
			pipeDSScence.lightUbo.spotLightList[0].color	 = glm::vec3(1.0f);							//颜色
			pipeDSScence.lightUbo.spotLightList[0].outer	 = glm::cos(glm::radians(14.5f));			//外圆锥
			pipeDSScence.lightUbo.spotLightList[0].direction = camera.front;							//方向

			pipeDSScence.shadowUbo.shadowMvp    = pipeShadow.shadowVP;
			pipeDSScence.shadowUbo.shadowExtent = glm::vec2{ renderInfo::shadowMapExtent.width, renderInfo::shadowMapExtent.height };

			createMemMap(pipeDSScence.lightUniform.bufferMem,  &pipeDSScence.lightUbo,  sizeof(pipeDSScence.lightUbo));
			createMemMap(pipeDSScence.shadowUniform.bufferMem, &pipeDSScence.shadowUbo, sizeof(pipeDSScence.shadowUbo));
		}
	}
	
	//绘制uiText
	void drawUIText(int fps) {
		if (pipeUIText.isEnable) {
			pipeUIText.msdfGlyphAtlas.begin();
			pipeUIText.msdfGlyphAtlas.addVec3 (uiEnum::uiTextAnchorEnum::topLeft,  "worldpos",   showUIText.pos);
			pipeUIText.msdfGlyphAtlas.addVec3 (uiEnum::uiTextAnchorEnum::topLeft,  "chunkpos",   showUIText.chunkPos);
			pipeUIText.msdfGlyphAtlas.addVec3 (uiEnum::uiTextAnchorEnum::topLeft,  "mineboxpos", showUIText.minepos);
			pipeUIText.msdfGlyphAtlas.addValue(uiEnum::uiTextAnchorEnum::topRight, "fps",		 std::to_string(fps), 8);
			pipeUIText.msdfGlyphAtlas.build(pipeUIText.vertex.bufferMem, Device.memAllocator);
		}
	}

	//绘制frame
	bool drawFrame(int threadId) {
		return InitSwapchain.drawFrame(cmd.renderCmd[threadId].list, cmd.renderCmd[threadId].fence);
	}



	void clearSwapChain() {
		procRenderPass.clearSwapChain(Device.device, Device.memAllocator);
	}

	void recreateSwapChain() {
		InitSwapchain.recreateSwapChain();
		procRenderPass.recreateSwapChain(InitSwapchain.swapExtent, InitSwapchain.colorFormat, InitSwapchain.depthFormat, InitSwapchain.msaa, Device.device, Device.memAllocator);



		//重建离屏描述符，因为交换链重建后osView也得重建
		procDescriptor.setAllocInfo(bufferEnum::procDescriptorId::scenceForward_d);

		//pipeOSMirror
		if (pipeOSMirror.isEnable) {
			std::array<VkWriteDescriptorSet, 3> osMirrorWrite{
				procDescriptor.getWrite(0, pipeOSMirror.descriptorSet, pipeOSMirror.uniform.buffer,				 sizeof(pipeOSMirror.ubo)),
				procDescriptor.getWrite(1, pipeOSMirror.descriptorSet, pipeOSMirror.lightUniform.buffer,		 sizeof(pipeOSMirror.lightUbo)),
				procDescriptor.getWrite(2, pipeOSMirror.descriptorSet, procSampler.node[pipeOSMirror.samplerId], procRenderPass.getFrameImage(renderEnum::renderInstanceId::osMirror_ri, renderEnum::renderFrameId::conv_rf, 2))
			};
			vkUpdateDescriptorSets(Device.device, uint32_t(osMirrorWrite.size()), osMirrorWrite.data(), 0, nullptr);
		}

		//pipeOSScence
		if (pipeOSScence.isEnable) {
			std::array<VkWriteDescriptorSet, 1> osScenceWrite{
				procDescriptor.getWrite(2, pipeOSScence.descriptorSet, procSampler.node[pipeOSScence.samplerId], procRenderPass.getFrameImage(renderEnum::renderInstanceId::osScence_ri, renderEnum::renderFrameId::conv_rf, 2))
			};
			vkUpdateDescriptorSets(Device.device, uint32_t(osScenceWrite.size()), osScenceWrite.data(), 0, nullptr);
		}

		procDescriptor.setAllocInfo(bufferEnum::procDescriptorId::dsScence_d);

		//pipeDSScence
		if (pipeDSScence.isEnable) {
			std::array<VkWriteDescriptorSet, 6> tempWrite{
				procDescriptor.getWrite(0, pipeDSScence.descriptorSet, pipeDSScence.lightUniform.buffer,		   sizeof(pipeDSScence.lightUbo)),
				procDescriptor.getWrite(1, pipeDSScence.descriptorSet, pipeDSScence.shadowUniform.buffer,		   sizeof(pipeDSScence.shadowUbo)),
				procDescriptor.getWrite(2, pipeDSScence.descriptorSet, procSampler.node[pipeDSScence.samplerId],   procRenderPass.getFrameImage(renderEnum::renderInstanceId::dsScence_ri,  renderEnum::renderFrameId::conv_rf, 0)),
				procDescriptor.getWrite(3, pipeDSScence.descriptorSet, procSampler.node[pipeDSScence.samplerId],   procRenderPass.getFrameImage(renderEnum::renderInstanceId::dsScence_ri,  renderEnum::renderFrameId::conv_rf, 1)),
				procDescriptor.getWrite(4, pipeDSScence.descriptorSet, procSampler.node[pipeDSScence.samplerId],   procRenderPass.getFrameImage(renderEnum::renderInstanceId::dsScence_ri,  renderEnum::renderFrameId::conv_rf, 2)),
				procDescriptor.getWrite(5, pipeDSScence.descriptorSet, procSampler.node[pipeShadow.samplerId], procRenderPass.getFrameImage(renderEnum::renderInstanceId::shadowMap_ri, renderEnum::renderFrameId::conv_rf, 0))
			};

			vkUpdateDescriptorSets(Device.device, uint32_t(tempWrite.size()), tempWrite.data(), 0, nullptr);
		}
	}

private:
	uiStruct::uiTextInfoNode showUIText{};



	//地形网格是否为空
	bool isNullTerrainMesh(int layer) {
		return terrainVertex::vertexs[layer].size() && terrainVertex::indexs[layer].size();
	}
	
public:
	const std::vector<bufferStruct::descriptorNode>& getDstList() {
		return procDescriptor.node;
	}

	const std::vector<renderStruct::renderTemplateNode>& getRenderList() {
		return procRenderPass.renderTemplateList;
	}

	const VkExtent2D& getSwapExtent() {
		return InitSwapchain.swapExtent;
	}

	const VkSampleCountFlagBits& getMsaa() {
		return InitSwapchain.msaa;
	}
};