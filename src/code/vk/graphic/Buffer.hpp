#pragma once



class Buffer {
public:
	Buffer(Device& tDevice) :Device{ tDevice } {
		pipeTerrain .create(fileInfo::image::cube, textureInfo::terrain::colorFormat, textureInfo::terrain::tileExtent, textureInfo::terrain::atlasExtent, cmd, Device.device, Device.graphicsQueue, Device.memAllocator);
		pipeModel   .create(cmd, Device.device, Device.graphicsQueue, Device.memAllocator);
		pipeSkybox  .create(cmd, Device.device, Device.graphicsQueue, Device.memAllocator);
		pipeMine    .create(cmd, Device.device, Device.graphicsQueue, Device.memAllocator);
		pipeUIText  .create(cmd, Device.device, Device.graphicsQueue, Device.memAllocator);
		pipeOSMirror.create(cmd, Device.device, Device.graphicsQueue, Device.memAllocator);
		pipeDSScence.create(Device.memAllocator);
		pipeShadow	.create(pipeTerrain.isEnable, pipeModel.isEnable, cmd, Device.device, Device.graphicsQueue, Device.memAllocator);



		auto [sModelMip, dModelMip] = pipeModel.getMaxModelMip();
		procSampler.createTextureSampler(Device.device, pipeTerrain.texture.mipLevel, sModelMip, dModelMip);
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

	bufferStruct::pipeTerrainNode	pipeTerrain		{ true,  pipelineEnum::procPipelineId::terrain_p,   bufferEnum::procSamplerId::terrain_s };
	bufferStruct::pipeModelNode		pipeModel		{ true,  pipelineEnum::procPipelineId::model_p };
	bufferStruct::pipeSkyboxNode	pipeSkybox		{ true,	 pipelineEnum::procPipelineId::skybox_p	    };
	bufferStruct::pipeMineNode		pipeMine		{ true,  pipelineEnum::procPipelineId::mine_p	    };
	bufferStruct::pipeUITextNode	pipeUIText		{ true,  pipelineEnum::procPipelineId::uiText_p,    bufferEnum::procSamplerId::uiText_s };
	bufferStruct::pipeOSMirrorNode	pipeOSMirror	{ false, pipelineEnum::procPipelineId::osMirror_p,  bufferEnum::procSamplerId::offScreen_s };
	bufferStruct::pipeOSScenceNode	pipeOSScence	{ false, pipelineEnum::procPipelineId::osScence_p,  bufferEnum::procSamplerId::offScreen_s };
	bufferStruct::pipeDSScenceNode	pipeDSScence	{ true,	 pipelineEnum::procPipelineId::dsScence_p,  bufferEnum::procSamplerId::offScreen_s };
	bufferStruct::pipeShadowNode	pipeShadow		{ true,  pipelineEnum::procPipelineId::shadowMap_p, bufferEnum::procSamplerId::shadowMap_s };



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
						procDescriptor.getWrite(2, pipeOSMirror.descriptorSet, procSampler.node[pipeOSMirror.samplerType], procRenderPass.getFrameImage(renderEnum::renderInstanceId::osMirror_ri, renderEnum::renderFrameId::conv_rf, 2))
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
						procDescriptor.getWrite(2, pipeOSScence.descriptorSet, procSampler.node[pipeOSScence.samplerType], procRenderPass.getFrameImage(renderEnum::renderInstanceId::osScence_ri, renderEnum::renderFrameId::conv_rf, 2))
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
							procDescriptor.getWrite(1, pipeModel.sNode[i].descriptorSet, procSampler.node[pipeModel.sNode[i].samplerType], pipeModel.sNode[i].texture.imageNode.view)
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
								procDescriptor.getWrite(1, pipeModel.dNode[i].node[j].descriptorSet, procSampler.node[pipeModel.dNode[i].samplerType], pipeModel.dNode[i].node[j].texture.imageNode.view)
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
						procDescriptor.getWrite(0, pipeUIText.descriptorSet, procSampler.node[pipeUIText.samplerType], pipeUIText.texture.imageNode.view)
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
					procDescriptor.getWrite(2, pipeDSScence.descriptorSet, procSampler.node[pipeDSScence.samplerType],   procRenderPass.getFrameImage(renderEnum::renderInstanceId::dsScence_ri,  renderEnum::renderFrameId::conv_rf, 0)),
					procDescriptor.getWrite(3, pipeDSScence.descriptorSet, procSampler.node[pipeDSScence.samplerType],   procRenderPass.getFrameImage(renderEnum::renderInstanceId::dsScence_ri,  renderEnum::renderFrameId::conv_rf, 1)),
					procDescriptor.getWrite(4, pipeDSScence.descriptorSet, procSampler.node[pipeDSScence.samplerType],   procRenderPass.getFrameImage(renderEnum::renderInstanceId::dsScence_ri,  renderEnum::renderFrameId::conv_rf, 2)),
					procDescriptor.getWrite(5, pipeDSScence.descriptorSet, procSampler.node[pipeShadow.samplerType], procRenderPass.getFrameImage(renderEnum::renderInstanceId::shadowMap_ri, renderEnum::renderFrameId::conv_rf, 0))
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
		glm::mat4 perspProj = glm::perspective(glm::radians(45.0f), static_cast<float>(InitSwapchain.swapExtent.width) / static_cast<float>(InitSwapchain.swapExtent.height), 0.1f, plateInfo::perspSightRange);
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

public:
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
									recorder.draw(pipelineEnum::cmdDrawId::indexed, procPipeline.getPipeline(pipeShadow.pipeType), procPipeline.getPipeLayout(pipeShadow.pipeType), pipeShadow.shadow_t.descriptorSet, pipeShadow.shadow_t.mesh[threadId][layer].vertex.buffer, pipeTerrain.mesh[threadId][layer].index.buffer, static_cast<uint32_t>(terrainVertex::indexs[layer].size()));
								}
							}
						}

						//model
						if (pipeModel.isEnable) {
							for (int nodeId = 0; nodeId < bufferEnum::procStaticModelId::count; nodeId++) {
								if (pipeModel.sNode[nodeId].isVisible) {
									recorder.draw(pipelineEnum::cmdDrawId::indexed, procPipeline.getPipeline(pipeShadow.pipeType), procPipeline.getPipeLayout(pipeShadow.pipeType), pipeShadow.shadow_ms[nodeId].descriptorSet, pipeShadow.shadow_ms[nodeId].mesh.vertex.buffer, pipeModel.sNode[nodeId].mesh.index.buffer, static_cast<uint32_t>(modelVertex::sIndexs[nodeId].size()));
								}
							}

							for (int nodeId = 0; nodeId < bufferEnum::procDynamicModelId::count; nodeId++) {
								if (pipeModel.dNode[nodeId].isVisible) {
									for (int subNode = 0; subNode < pipeShadow.shadow_md[nodeId].size(); subNode++) {
										recorder.draw(pipelineEnum::cmdDrawId::indexed, procPipeline.getPipeline(pipeShadow.pipeType), procPipeline.getPipeLayout(pipeShadow.pipeType), pipeShadow.shadow_md[nodeId][subNode].descriptorSet, pipeShadow.shadow_md[nodeId][subNode].mesh.vertex.buffer, pipeModel.dNode[nodeId].node[subNode].mesh.index.buffer, static_cast<uint32_t>(modelVertex::dIndexs[nodeId][subNode].size()));
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
								recorder.draw(pipelineEnum::cmdDrawId::indexed, procPipeline.getPipeline(pipeTerrain.pipeType), procPipeline.getPipeLayout(pipeTerrain.pipeType), pipeTerrain.descriptorSet, pipeTerrain.mesh[threadId][layer].vertex.buffer, pipeTerrain.mesh[threadId][layer].index.buffer, static_cast<uint32_t>(terrainVertex::indexs[layer].size()));
							}
						}
					}

					//model
					if (pipeModel.isEnable) {
						for (int nodeId = 0; nodeId < bufferEnum::procStaticModelId::count; nodeId++) {
							if (pipeModel.sNode[nodeId].isVisible) {
								recorder.draw(pipelineEnum::cmdDrawId::indexed, procPipeline.getPipeline(pipeModel.pipeType), procPipeline.getPipeLayout(pipeModel.pipeType), pipeModel.sNode[nodeId].descriptorSet, pipeModel.sNode[nodeId].mesh.vertex.buffer, pipeModel.sNode[nodeId].mesh.index.buffer, static_cast<uint32_t>(modelVertex::sIndexs[nodeId].size()));
							}
						}

						for (int nodeId = 0; nodeId < bufferEnum::procDynamicModelId::count; nodeId++) {
							if (pipeModel.dNode[nodeId].isVisible) {
								for (int subNode = 0; subNode < pipeModel.dNode[nodeId].node.size(); subNode++) {
									recorder.draw(pipelineEnum::cmdDrawId::indexed, procPipeline.getPipeline(pipeModel.pipeType), procPipeline.getPipeLayout(pipeModel.pipeType), pipeModel.dNode[nodeId].node[subNode].descriptorSet, pipeModel.dNode[nodeId].node[subNode].mesh.vertex.buffer, pipeModel.dNode[nodeId].node[subNode].mesh.index.buffer, static_cast<uint32_t>(modelVertex::dIndexs[nodeId][subNode].size()));
								}
							}
						}
					}

					//pipeSkybox
					if (pipeSkybox.isEnable) {
						recorder.draw(pipelineEnum::cmdDrawId::indexed, procPipeline.getPipeline(pipeSkybox.pipeType), procPipeline.getPipeLayout(pipeSkybox.pipeType), pipeSkybox.descriptorSet, pipeSkybox.mesh.vertex.buffer, pipeSkybox.mesh.index.buffer, static_cast<uint32_t>(skyboxVertex::indexs.size()));
					}

					//pipeMine
					if (pipeMine.isEnable) {
						recorder.draw(pipelineEnum::cmdDrawId::indexed, procPipeline.getPipeline(pipeMine.pipeType), procPipeline.getPipeLayout(pipeMine.pipeType), pipeMine.descriptorSet, pipeMine.mesh.vertex.buffer, pipeMine.mesh.index.buffer, static_cast<uint32_t>(mineVertex::indexs.size()));
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
						recorder.draw(pipelineEnum::cmdDrawId::indexed, procPipeline.getPipeline(pipeOSMirror.pipeType), procPipeline.getPipeLayout(pipeOSMirror.pipeType), pipeOSMirror.descriptorSet, pipeOSMirror.mesh.vertex.buffer, pipeOSMirror.mesh.index.buffer, static_cast<uint32_t>(osMirrorVertex::indexs.size()));
					}

					recorder.endRender();
				}

				/*scScence_ri*/
				{
					recorder.barrier();	//创建延迟渲染管线屏障，用于确保延迟渲染管线最后执行
					recorder.beginRender(&procRenderPass.getRenderBegin(renderEnum::renderInstanceId::scScence_ri, frameId));

					//pipeOSScence
					if (pipeOSScence.isEnable) {
						recorder.draw(pipelineEnum::cmdDrawId::fullscreen, procPipeline.getPipeline(pipeOSScence.pipeType), procPipeline.getPipeLayout(pipeOSScence.pipeType), pipeOSScence.descriptorSet, VK_NULL_HANDLE, VK_NULL_HANDLE, 0, InitSwapchain.swapExtent);
					}

					//pipeDSScence
					if (pipeDSScence.isEnable) {
						recorder.draw(pipelineEnum::cmdDrawId::fullscreen, procPipeline.getPipeline(pipeDSScence.pipeType), procPipeline.getPipeLayout(pipeDSScence.pipeType), pipeDSScence.descriptorSet, VK_NULL_HANDLE, VK_NULL_HANDLE, 0, InitSwapchain.swapExtent);

						//pipeUIText
						if (pipeUIText.isEnable) {
							recorder.draw(pipelineEnum::cmdDrawId::text, procPipeline.getPipeline(pipeUIText.pipeType), procPipeline.getPipeLayout(pipeUIText.pipeType), pipeUIText.descriptorSet, pipeUIText.vertex.buffer, VK_NULL_HANDLE, pipeUIText.msdfGlyphAtlas.glyphCount);
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
			pipeTerrain.mesh[threadId][layer].clear(Device.memAllocator);
			pipeTerrain.mesh[threadId][layer].create(terrainVertex::vertexs[layer], terrainVertex::indexs[layer], cmd, Device.device, Device.graphicsQueue, Device.memAllocator);

			//重建下一线程Lod_0地形阴影
			if (pipeShadow.isEnable) {
								shadowVertex::loadTerrainVertexs(layer);
				pipeShadow.shadow_t.mesh[threadId][layer].clear(Device.memAllocator);
				pipeShadow.shadow_t.mesh[threadId][layer].create(shadowVertex::tVertexs[layer], cmd, Device.device, Device.graphicsQueue, Device.memAllocator);
			}
		}

		//重建全部线程Lod_1地形管线，RM_0.4.6（8_32：20~50）
		if (isEntireRebuild.load()) {
			layer = plateEnum::terrainLayer::Layer_1;

			if (isNullTerrainMesh(layer)) {
				const std::vector<terrainVertex::vertexNode>& vertex = terrainVertex::vertexs[layer];
				const std::vector<uint32_t>&				  index  = terrainVertex::indexs[layer];
				//重建下一线程Lod_1地形管线
				pipeTerrain.mesh[threadId][layer].clear(Device.memAllocator);
				pipeTerrain.mesh[threadId][layer].create(vertex, index, cmd, Device.device, Device.graphicsQueue, Device.memAllocator);

				//重建下一线程Lod_1地形阴影
				if (pipeShadow.isEnable) {
										shadowVertex::loadTerrainVertexs(layer);
					pipeShadow.shadow_t.mesh[threadId][layer].clear(Device.memAllocator);
					pipeShadow.shadow_t.mesh[threadId][layer].create(shadowVertex::tVertexs[layer], cmd, Device.device, Device.graphicsQueue, Device.memAllocator);
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

				pipeTerrain.mesh[tempThreadId][layer].clear(Device.memAllocator);
				pipeTerrain.mesh[tempThreadId][layer].create(vertex, index, cmd, Device.device, Device.graphicsQueue, Device.memAllocator);

				//重建上轮当前线程Lod_1地形阴影
				if (pipeShadow.isEnable) {
										shadowVertex::loadTerrainVertexs(layer);
					pipeShadow.shadow_t.mesh[tempThreadId][layer].clear(Device.memAllocator);
					pipeShadow.shadow_t.mesh[tempThreadId][layer].create(shadowVertex::tVertexs[layer], cmd, Device.device, Device.graphicsQueue, Device.memAllocator);
				}


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
				procDescriptor.getWrite(2, pipeOSMirror.descriptorSet, procSampler.node[pipeOSMirror.samplerType], procRenderPass.getFrameImage(renderEnum::renderInstanceId::osMirror_ri, renderEnum::renderFrameId::conv_rf, 2))
			};
			vkUpdateDescriptorSets(Device.device, uint32_t(osMirrorWrite.size()), osMirrorWrite.data(), 0, nullptr);
		}

		//pipeOSScence
		if (pipeOSScence.isEnable) {
			std::array<VkWriteDescriptorSet, 1> osScenceWrite{
				procDescriptor.getWrite(2, pipeOSScence.descriptorSet, procSampler.node[pipeOSScence.samplerType], procRenderPass.getFrameImage(renderEnum::renderInstanceId::osScence_ri, renderEnum::renderFrameId::conv_rf, 2))
			};
			vkUpdateDescriptorSets(Device.device, uint32_t(osScenceWrite.size()), osScenceWrite.data(), 0, nullptr);
		}

		procDescriptor.setAllocInfo(bufferEnum::procDescriptorId::dsScence_d);

		//pipeDSScence
		if (pipeDSScence.isEnable) {
			std::array<VkWriteDescriptorSet, 6> tempWrite{
				procDescriptor.getWrite(0, pipeDSScence.descriptorSet, pipeDSScence.lightUniform.buffer,		   sizeof(pipeDSScence.lightUbo)),
				procDescriptor.getWrite(1, pipeDSScence.descriptorSet, pipeDSScence.shadowUniform.buffer,		   sizeof(pipeDSScence.shadowUbo)),
				procDescriptor.getWrite(2, pipeDSScence.descriptorSet, procSampler.node[pipeDSScence.samplerType],   procRenderPass.getFrameImage(renderEnum::renderInstanceId::dsScence_ri,  renderEnum::renderFrameId::conv_rf, 0)),
				procDescriptor.getWrite(3, pipeDSScence.descriptorSet, procSampler.node[pipeDSScence.samplerType],   procRenderPass.getFrameImage(renderEnum::renderInstanceId::dsScence_ri,  renderEnum::renderFrameId::conv_rf, 1)),
				procDescriptor.getWrite(4, pipeDSScence.descriptorSet, procSampler.node[pipeDSScence.samplerType],   procRenderPass.getFrameImage(renderEnum::renderInstanceId::dsScence_ri,  renderEnum::renderFrameId::conv_rf, 2)),
				procDescriptor.getWrite(5, pipeDSScence.descriptorSet, procSampler.node[pipeShadow.samplerType], procRenderPass.getFrameImage(renderEnum::renderInstanceId::shadowMap_ri, renderEnum::renderFrameId::conv_rf, 0))
			};

			vkUpdateDescriptorSets(Device.device, uint32_t(tempWrite.size()), tempWrite.data(), 0, nullptr);
		}
	}

	void clearSwapChain() {
		procRenderPass.clearSwapChain(Device.device, Device.memAllocator);
	}

	//更新Uniform
	void updateUniform(const glm::dvec3& logicOriginPos, const playerStruct::cameraNode& camera, const playerStruct::selectBoxNode& box) {
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

			pipeTerrain.uniform.createMemMap(&pipeTerrain.ubo, sizeof(pipeTerrain.ubo), Device.memAllocator);
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

					pipeModel.sNode[i].uniform.createMemMap(&pipeModel.sNode[i].ubo, sizeof(pipeModel.sNode[i].ubo), Device.memAllocator);

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

						pipeModel.dNode[i].node[j].uniform.createMemMap(&pipeModel.dNode[i].node[j].ubo, sizeof(pipeModel.dNode[i].node[j].ubo), Device.memAllocator);
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

			pipeSkybox.uniform.createMemMap(&pipeSkybox.ubo, sizeof(pipeSkybox.ubo), Device.memAllocator);
			pipeSkybox.lightUniform.createMemMap(&pipeSkybox.lightUbo, sizeof(pipeSkybox.lightUbo), Device.memAllocator);
		}

		//pipeMine
		if (pipeMine.isEnable) {
			pipeMine.ubo.model = glm::translate(glm::mat4(1), box.minepos);
			pipeMine.ubo.model *= glm::scale(glm::mat4(1), glm::vec3(1) * box.isVisible);
			pipeMine.ubo.view  = camera.view;

			pipeMine.uniform.createMemMap(&pipeMine.ubo, sizeof(pipeMine.ubo), Device.memAllocator);
		}

		//showUIText
		if (pipeUIText.isEnable) {
			showUIText.pos		= glm::floor(camera.pos);
			showUIText.chunkPos = glm::floor(camera.pos / static_cast<double>(plateInfo::chunkLength));
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

			pipeOSMirror.uniform.createMemMap(&pipeOSMirror.ubo, sizeof(pipeOSMirror.ubo), Device.memAllocator);
			pipeOSMirror.lightUniform.createMemMap(&pipeOSMirror.lightUbo, sizeof(pipeOSMirror.lightUbo), Device.memAllocator);
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
				pipeShadow.shadow_t.shadowMvpUniform.createMemMap(&pipeShadow.shadow_t.shadowMvpUbo, sizeof(pipeShadow.shadow_t.shadowMvpUbo), Device.memAllocator);
			}

			//model
			if (pipeModel.isEnable) {
				//shadow-sNode
				for (int i = 0; i < bufferEnum::procStaticModelId::count; i++) {
					if (pipeModel.sNode[i].isVisible) {
						pipeShadow.shadow_ms[i].shadowMvpUbo.mvp = pipeShadow.shadowVP * pipeModel.sNode[i].ubo.model;
						pipeShadow.shadow_ms[i].shadowMvpUniform.createMemMap(&pipeShadow.shadow_ms[i].shadowMvpUbo, sizeof(pipeShadow.shadow_ms[i].shadowMvpUbo), Device.memAllocator);
					}
				}

				//shadow-dNode
				for (int i = 0; i < bufferEnum::procDynamicModelId::count; i++) {
					if (pipeModel.dNode[i].isVisible) {
						for (int j = 0; j < pipeShadow.shadow_md[i].size(); j++) {
							pipeShadow.shadow_md[i][j].shadowMvpUbo.mvp = pipeShadow.shadowVP * pipeModel.dNode[i].node[j].ubo.model;
							pipeShadow.shadow_md[i][j].shadowMvpUniform.createMemMap(&pipeShadow.shadow_md[i][j].shadowMvpUbo, sizeof(pipeShadow.shadow_md[i][j].shadowMvpUbo), Device.memAllocator);
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

			pipeDSScence.lightUniform.createMemMap(&pipeDSScence.lightUbo, sizeof(pipeDSScence.lightUbo), Device.memAllocator);
			pipeDSScence.shadowUniform.createMemMap(&pipeDSScence.shadowUbo, sizeof(pipeDSScence.shadowUbo), Device.memAllocator);
		}
	}

	//绘制frame
	bool drawFrame(int threadId) {
		return InitSwapchain.drawFrame(cmd.renderCmd[threadId].list, cmd.renderCmd[threadId].fence);
	}

	const std::vector<bufferStruct::descriptorNode>& getDstList() {
		return procDescriptor.node;
	}

	const std::vector<renderStruct::renderTemplateNode>& getRenderList() {
		return procRenderPass.renderTemplateList;
	}

	const VkExtent2D& getSwapExtent() {
		return InitSwapchain.swapExtent;
	}

	VkSampleCountFlagBits getMsaa() {
		return InitSwapchain.msaa;
	}

private:
	uiStruct::uiTextInfoNode showUIText{};



	//地形网格是否为空
	bool isNullTerrainMesh(int layer) {
		return terrainVertex::vertexs[layer].size() && terrainVertex::indexs[layer].size();
	}
};