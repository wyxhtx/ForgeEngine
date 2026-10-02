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
		procSampler.createSamplerList(Device.device, Device.enableGpuInfo.limits, pipeTerrain.texture.mipLevel, sModelMip, dModelMip);
		createDescSet();

		initUniformBuffer();	//初始化Uniform
		drawUIText(0);			//初始化uiText

		if (debugInfo::LogSwitch) {
			Log::printTitle("————————Buffer已创建");

			Log::delimiter();
		}
	}

	~Buffer() {
		procRender .clear(Device.device, Device.memAllocator);
		procDesc   .clear(Device.device);
		procSampler.clear(Device.device);

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
	pipelineList::procRenderNode   procRender{ InitSwapchain.colorFormat, InitSwapchain.depthFormat, InitSwapchain.swapExtent, InitSwapchain.msaa, Device.device, Device.memAllocator };
	pipelineList::procDescNode     procDesc{ Device.device };
	pipelineList::procSamplerNode  procSampler{};

	bufferStruct::pipeTerrainNode  pipeTerrain	{ true,  pipelineEnum::procPipelineId::terrain_p,   samplerEnum::procSamplerId::terrain_s };
	bufferStruct::pipeModelNode	   pipeModel	{ true,  pipelineEnum::procPipelineId::model_p		};
	bufferStruct::pipeSkyboxNode   pipeSkybox	{ true,	 pipelineEnum::procPipelineId::skybox_p	    };
	bufferStruct::pipeMineNode	   pipeMine		{ true,  pipelineEnum::procPipelineId::mine_p	    };
	bufferStruct::pipeUITextNode   pipeUIText   { true,  pipelineEnum::procPipelineId::uiText_p,    samplerEnum::procSamplerId::uiText_s };
	bufferStruct::pipeOSMirrorNode pipeOSMirror	{ false, pipelineEnum::procPipelineId::osMirror_p,  samplerEnum::procSamplerId::offScreen_s };
	bufferStruct::pipeOSScenceNode pipeOSScence	{ false, pipelineEnum::procPipelineId::osScence_p,  samplerEnum::procSamplerId::offScreen_s };
	bufferStruct::pipeDSScenceNode pipeDSScence	{ true,	 pipelineEnum::procPipelineId::dsScence_p,  samplerEnum::procSamplerId::offScreen_s };
	bufferStruct::pipeShadowNode   pipeShadow   { true,  pipelineEnum::procPipelineId::shadowMap_p, samplerEnum::procSamplerId::shadowMap_s };



	void createDescSet() {
		/*scenceForward_d*/
		{
			procDesc.setAllocInfo(descEnum::descType::scenceForward_d);

			//pipeSkybox
			if (pipeSkybox.isEnable) {
				if (vkAllocateDescriptorSets(Device.device, &procDesc.allocInfo, &pipeSkybox.descriptorSet) == VK_SUCCESS) {
					std::array<VkWriteDescriptorSet, 2> tempWrite{
						procDesc.getWrite(0, pipeSkybox.descriptorSet, pipeSkybox.uniform.buffer, sizeof(pipeSkybox.ubo)),
						procDesc.getWrite(1, pipeSkybox.descriptorSet, pipeSkybox.lightUniform.buffer, sizeof(pipeSkybox.lightUbo))
					};

					vkUpdateDescriptorSets(Device.device, uint32_t(tempWrite.size()), tempWrite.data(), 0, nullptr);
				}
				else {
					Log::printError(__FILE__, __LINE__, "skybox分配描述符集失败");
				}
			}

			//pipeMine
			if (pipeMine.isEnable) {
				if (vkAllocateDescriptorSets(Device.device, &procDesc.allocInfo, &pipeMine.descriptorSet) == VK_SUCCESS) {
					std::array<VkWriteDescriptorSet, 1> tempWrite{
						procDesc.getWrite(0, pipeMine.descriptorSet, pipeMine.uniform.buffer, sizeof(pipeMine.ubo)),
					};

					vkUpdateDescriptorSets(Device.device, uint32_t(tempWrite.size()), tempWrite.data(), 0, nullptr);
				}
				else {
					Log::printError(__FILE__, __LINE__, "mine分配描述符集失败");
				}
			}

			//pipeOSMirror
			if (pipeOSMirror.isEnable) {
				if (vkAllocateDescriptorSets(Device.device, &procDesc.allocInfo, &pipeOSMirror.descriptorSet) == VK_SUCCESS) {
					std::array<VkWriteDescriptorSet, 3> tempWrite{
						procDesc.getWrite(0, pipeOSMirror.descriptorSet, pipeOSMirror.uniform.buffer,				 sizeof(pipeOSMirror.ubo)),
						procDesc.getWrite(1, pipeOSMirror.descriptorSet, pipeOSMirror.lightUniform.buffer,			 sizeof(pipeOSMirror.lightUbo)),
						procDesc.getWrite(2, pipeOSMirror.descriptorSet, procSampler.node[pipeOSMirror.samplerType], procRender.getFrameImage(renderEnum::renderInstanceId::osMirror_ri, renderEnum::renderFrameId::conv_rf, 2))
					};

					vkUpdateDescriptorSets(Device.device, uint32_t(tempWrite.size()), tempWrite.data(), 0, nullptr);
				}
				else {
					Log::printError(__FILE__, __LINE__, "osMirror分配描述符集失败");
				}
			}

			//pipeOSScence
			if (pipeOSScence.isEnable) {
				if (vkAllocateDescriptorSets(Device.device, &procDesc.allocInfo, &pipeOSScence.descriptorSet) == VK_SUCCESS) {
					std::array<VkWriteDescriptorSet, 1> tempWrite{
						procDesc.getWrite(2, pipeOSScence.descriptorSet, procSampler.node[pipeOSScence.samplerType], procRender.getFrameImage(renderEnum::renderInstanceId::osScence_ri, renderEnum::renderFrameId::conv_rf, 2))
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
			procDesc.setAllocInfo(descEnum::descType::scenceDeferred_d);

			//pipeTerrain
			pipeTerrain.createDescSet(procDesc.node[descEnum::descType::scenceDeferred_d], procSampler.node, Device.device);

			//model
			if (pipeModel.isEnable) {
				//model-sNode
				for (int i = 0; i < pipeModel.sNode.size(); i++) {
					if (vkAllocateDescriptorSets(Device.device, &procDesc.allocInfo, &pipeModel.sNode[i].descriptorSet) == VK_SUCCESS) {
						std::array<VkWriteDescriptorSet, 2> tempWrite{
							procDesc.getWrite(0, pipeModel.sNode[i].descriptorSet, pipeModel.sNode[i].uniform.buffer, sizeof(pipeModel.sNode[i].ubo)),
							procDesc.getWrite(1, pipeModel.sNode[i].descriptorSet, procSampler.node[pipeModel.sNode[i].samplerType], pipeModel.sNode[i].texture.imageNode.view)
						};

						vkUpdateDescriptorSets(Device.device, uint32_t(tempWrite.size()), tempWrite.data(), 0, nullptr);
					}
					else {
						Log::printError(__FILE__, __LINE__, "model-sNode_" + std::to_string(i) + ":\t分配描述符集失败");
					}
				}

				//model-dNode
				for (int i = 0; i < pipeModel.dNode.size(); i++) {
					for (int j = 0; j < pipeModel.dNode[i].node.size(); j++) {
						if (vkAllocateDescriptorSets(Device.device, &procDesc.allocInfo, &pipeModel.dNode[i].node[j].descriptorSet) == VK_SUCCESS) {
							std::array<VkWriteDescriptorSet, 2> tempWrite{
								procDesc.getWrite(0, pipeModel.dNode[i].node[j].descriptorSet, pipeModel.dNode[i].node[j].uniform.buffer, sizeof(pipeModel.dNode[i].node[j].ubo)),
								procDesc.getWrite(1, pipeModel.dNode[i].node[j].descriptorSet, procSampler.node[pipeModel.dNode[i].samplerType], pipeModel.dNode[i].node[j].texture.imageNode.view)
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
			procDesc.setAllocInfo(descEnum::descType::uiText_d);

			//pipeUIText
			if (pipeUIText.isEnable) {
				if (vkAllocateDescriptorSets(Device.device, &procDesc.allocInfo, &pipeUIText.descriptorSet) == VK_SUCCESS) {
					std::array<VkWriteDescriptorSet, 1> tempWrite{
						procDesc.getWrite(0, pipeUIText.descriptorSet, procSampler.node[pipeUIText.samplerType], pipeUIText.texture.imageNode.view)
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
			procDesc.setAllocInfo(descEnum::descType::dsScence_d);

			//pipeDSScence
			if (vkAllocateDescriptorSets(Device.device, &procDesc.allocInfo, &pipeDSScence.descriptorSet) == VK_SUCCESS) {
				std::array<VkWriteDescriptorSet, 6> tempWrite{
					procDesc.getWrite(0, pipeDSScence.descriptorSet, pipeDSScence.lightUniform.buffer,			 sizeof(pipeDSScence.lightUbo)),
					procDesc.getWrite(1, pipeDSScence.descriptorSet, pipeDSScence.shadowUniform.buffer,			 sizeof(pipeDSScence.shadowUbo)),
					procDesc.getWrite(2, pipeDSScence.descriptorSet, procSampler.node[pipeDSScence.samplerType], procRender.getFrameImage(renderEnum::renderInstanceId::dsScence_ri,  renderEnum::renderFrameId::conv_rf, 0)),
					procDesc.getWrite(3, pipeDSScence.descriptorSet, procSampler.node[pipeDSScence.samplerType], procRender.getFrameImage(renderEnum::renderInstanceId::dsScence_ri,  renderEnum::renderFrameId::conv_rf, 1)),
					procDesc.getWrite(4, pipeDSScence.descriptorSet, procSampler.node[pipeDSScence.samplerType], procRender.getFrameImage(renderEnum::renderInstanceId::dsScence_ri,  renderEnum::renderFrameId::conv_rf, 2)),
					procDesc.getWrite(5, pipeDSScence.descriptorSet, procSampler.node[pipeShadow.samplerType],   procRender.getFrameImage(renderEnum::renderInstanceId::shadowMap_ri, renderEnum::renderFrameId::conv_rf, 0))
				};

				vkUpdateDescriptorSets(Device.device, uint32_t(tempWrite.size()), tempWrite.data(), 0, nullptr);
			}
			else {
				Log::printError(__FILE__, __LINE__, "dsScence分配描述符集失败");
			}
		}

		/*shadowMap_d*/
		if (pipeShadow.isEnable) {
			procDesc.setAllocInfo(descEnum::descType::shadowMap_d);

			//pipeTerrain
			if (pipeTerrain.isEnable) {
				//shadow-pipeTerrain
				if (vkAllocateDescriptorSets(Device.device, &procDesc.allocInfo, &pipeShadow.shadow_t.descriptorSet) == VK_SUCCESS) {
					std::array<VkWriteDescriptorSet, 1> tempWrite{
						procDesc.getWrite(0, pipeShadow.shadow_t.descriptorSet, pipeShadow.shadow_t.shadowMvpUniform.buffer, sizeof(pipeShadow.shadow_t.shadowMvpUbo)),
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
				for (bufferStruct::shadowModelNode& shadowNode : pipeShadow.shadow_ms) {
					if (vkAllocateDescriptorSets(Device.device, &procDesc.allocInfo, &shadowNode.descriptorSet) == VK_SUCCESS) {
						std::array<VkWriteDescriptorSet, 1> tempWrite{
							procDesc.getWrite(0, shadowNode.descriptorSet, shadowNode.shadowMvpUniform.buffer, sizeof(shadowNode.shadowMvpUbo)),
						};

						vkUpdateDescriptorSets(Device.device, uint32_t(tempWrite.size()), tempWrite.data(), 0, nullptr);
					}
					else {
						Log::printError(__FILE__, __LINE__, "shadow-sNode描述符集分配失败");
					}
				}

				//shadow-dNode
				for (std::vector<bufferStruct::shadowModelNode>& shadowNodeList : pipeShadow.shadow_md) {
					for (bufferStruct::shadowModelNode& shadowNode : shadowNodeList) {
						if (vkAllocateDescriptorSets(Device.device, &procDesc.allocInfo, &shadowNode.descriptorSet) == VK_SUCCESS) {
							std::array<VkWriteDescriptorSet, 1> tempWrite{
								procDesc.getWrite(0, shadowNode.descriptorSet, shadowNode.shadowMvpUniform.buffer, sizeof(shadowNode.shadowMvpUbo)),
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
			for (bufferStruct::modelStaticNode& sNode : pipeModel.sNode) {
				sNode.ubo.proj = perspProj;
			}

			//model-dNode
			for (bufferStruct::modelDynamicNode& dNode : pipeModel.dNode) {
				for (bufferStruct::modelDynamicSubNode& modelNode : dNode.node) {
					modelNode.ubo.proj = perspProj;
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
	//绘制 uiText
	void drawUIText(int fps) {
		if (pipeUIText.isEnable) {
			pipeUIText.msdfGlyphAtlas.begin();
			pipeUIText.msdfGlyphAtlas.addVec3 (uiEnum::uiTextAnchorEnum::topLeft,  "世界坐标",   showUIText.pos);
			pipeUIText.msdfGlyphAtlas.addVec3 (uiEnum::uiTextAnchorEnum::topLeft,  "区块坐标",   showUIText.chunkPos);
			pipeUIText.msdfGlyphAtlas.addVec3 (uiEnum::uiTextAnchorEnum::topLeft,  "准星方块",   showUIText.minepos);
			pipeUIText.msdfGlyphAtlas.addValue(uiEnum::uiTextAnchorEnum::topRight, "FPS",		 std::to_string(fps), 8);
			pipeUIText.msdfGlyphAtlas.build(pipeUIText.vertex.bufferMem, Device.memAllocator);
		}
	}



	//创建 cmd
	void createCmd(int threadId, pipelineList::procPipelineNode& procPipeline) {
		for (int frameId = 0; frameId < cmd.renderCmd[threadId].size(); frameId++) {
			cmd.recordCmd(threadId, frameId, [&](cmdStruct::cmdRecorderNode& recorder) {
				/*shadowMap_ri*/
				{
					recorder.beginRender(&procRender.getRenderBegin(renderEnum::renderInstanceId::shadowMap_ri));

					if (pipeShadow.isEnable) {
						//pipeTerrain
						if (pipeTerrain.isEnable) {
							for (int layer = 0; layer < plateEnum::terrainLayer::enumCount; layer++) {
								if (isNullTerrainMesh(layer)) {
									recorder.draw(pipelineEnum::cmdDrawId::indexed, procPipeline.getPipeline(pipeShadow.pipeType), procPipeline.getPipeLayout(pipeShadow.pipeType), pipeShadow.shadow_t.descriptorSet, pipeShadow.shadow_t.mesh[threadId][layer].vertex.buffer, pipeTerrain.mesh[threadId][layer].index.buffer, static_cast<uint32_t>(vertexStruct::terrainVertex::mesh[layer].indexs.size()));
								}
							}
						}

						//model
						if (pipeModel.isEnable) {
							for (int nodeId = 0; nodeId < pipeModel.sNode.size(); nodeId++) {
								if (pipeModel.sNode[nodeId].isVisible) {
									recorder.draw(pipelineEnum::cmdDrawId::indexed, procPipeline.getPipeline(pipeShadow.pipeType), procPipeline.getPipeLayout(pipeShadow.pipeType), pipeShadow.shadow_ms[nodeId].descriptorSet, pipeShadow.shadow_ms[nodeId].mesh.vertex.buffer, pipeModel.sNode[nodeId].mesh.index.buffer, static_cast<uint32_t>(vertexStruct::modelVertex::sMesh[nodeId].indexs.size()));
								}
							}

							for (int nodeId = 0; nodeId < pipeModel.dNode.size(); nodeId++) {
								if (pipeModel.dNode[nodeId].isVisible) {
									for (int subNode = 0; subNode < pipeShadow.shadow_md[nodeId].size(); subNode++) {
										recorder.draw(pipelineEnum::cmdDrawId::indexed, procPipeline.getPipeline(pipeShadow.pipeType), procPipeline.getPipeLayout(pipeShadow.pipeType), pipeShadow.shadow_md[nodeId][subNode].descriptorSet, pipeShadow.shadow_md[nodeId][subNode].mesh.vertex.buffer, pipeModel.dNode[nodeId].node[subNode].mesh.index.buffer, static_cast<uint32_t>(vertexStruct::modelVertex::dMesh[nodeId][subNode].indexs.size()));
									}
								}
							}
						}
					}

					recorder.endRender();
				}

				/*dsScence_ri*/
				{
					recorder.beginRender(&procRender.getRenderBegin(renderEnum::renderInstanceId::dsScence_ri));

					//pipeTerrain
					if (pipeTerrain.isEnable) {
						for (int layer = 0; layer < plateEnum::terrainLayer::enumCount; layer++) {
							if (isNullTerrainMesh(layer)) {
								recorder.draw(pipelineEnum::cmdDrawId::indexed, procPipeline.getPipeline(pipeTerrain.pipeType), procPipeline.getPipeLayout(pipeTerrain.pipeType), pipeTerrain.descriptorSet, pipeTerrain.mesh[threadId][layer].vertex.buffer, pipeTerrain.mesh[threadId][layer].index.buffer, static_cast<uint32_t>(vertexStruct::terrainVertex::mesh[layer].indexs.size()));
							}
						}
					}

					//model
					if (pipeModel.isEnable) {
						for (int nodeId = 0; nodeId < pipeModel.sNode.size(); nodeId++) {
							if (pipeModel.sNode[nodeId].isVisible) {
								recorder.draw(pipelineEnum::cmdDrawId::indexed, procPipeline.getPipeline(pipeModel.pipeType), procPipeline.getPipeLayout(pipeModel.pipeType), pipeModel.sNode[nodeId].descriptorSet, pipeModel.sNode[nodeId].mesh.vertex.buffer, pipeModel.sNode[nodeId].mesh.index.buffer, static_cast<uint32_t>(vertexStruct::modelVertex::sMesh[nodeId].indexs.size()));
							}
						}

						for (int nodeId = 0; nodeId < pipeModel.dNode.size(); nodeId++) {
							if (pipeModel.dNode[nodeId].isVisible) {
								for (int subNode = 0; subNode < pipeModel.dNode[nodeId].node.size(); subNode++) {
									recorder.draw(pipelineEnum::cmdDrawId::indexed, procPipeline.getPipeline(pipeModel.pipeType), procPipeline.getPipeLayout(pipeModel.pipeType), pipeModel.dNode[nodeId].node[subNode].descriptorSet, pipeModel.dNode[nodeId].node[subNode].mesh.vertex.buffer, pipeModel.dNode[nodeId].node[subNode].mesh.index.buffer, static_cast<uint32_t>(vertexStruct::modelVertex::dMesh[nodeId][subNode].indexs.size()));
								}
							}
						}
					}

					//pipeSkybox
					if (pipeSkybox.isEnable) {
						recorder.draw(pipelineEnum::cmdDrawId::indexed, procPipeline.getPipeline(pipeSkybox.pipeType), procPipeline.getPipeLayout(pipeSkybox.pipeType), pipeSkybox.descriptorSet, pipeSkybox.mesh.vertex.buffer, pipeSkybox.mesh.index.buffer, static_cast<uint32_t>(vertexStruct::skyboxVertex::indexs.size()));
					}

					//pipeMine
					if (pipeMine.isEnable) {
						recorder.draw(pipelineEnum::cmdDrawId::indexed, procPipeline.getPipeline(pipeMine.pipeType), procPipeline.getPipeLayout(pipeMine.pipeType), pipeMine.descriptorSet, pipeMine.mesh.vertex.buffer, pipeMine.mesh.index.buffer, static_cast<uint32_t>(vertexStruct::mineVertex::indexs.size()));
					}

					recorder.endRender();
				}

				/*osMirror_ri*/
				if (pipeOSMirror.isEnable) {
					recorder.beginRender(&procRender.getRenderBegin(renderEnum::renderInstanceId::osMirror_ri));

					recorder.endRender();
				}

				/*osScence_ri*/
				if (pipeOSScence.isEnable) {
					recorder.beginRender(&procRender.getRenderBegin(renderEnum::renderInstanceId::osScence_ri));

					//pipeOSMirror
					if (pipeOSMirror.isEnable) {
						recorder.draw(pipelineEnum::cmdDrawId::indexed, procPipeline.getPipeline(pipeOSMirror.pipeType), procPipeline.getPipeLayout(pipeOSMirror.pipeType), pipeOSMirror.descriptorSet, pipeOSMirror.mesh.vertex.buffer, pipeOSMirror.mesh.index.buffer, static_cast<uint32_t>(vertexStruct::osMirrorVertex::indexs.size()));
					}

					recorder.endRender();
				}

				/*scScence_ri*/
				{
					recorder.barrier();	//创建延迟渲染管线屏障，用于确保延迟渲染管线最后执行
					recorder.beginRender(&procRender.getRenderBegin(renderEnum::renderInstanceId::scScence_ri, frameId));

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

	//重建 cmd
	void recreateCmd(int threadId, std::atomic<bool>& isEntireRebuild, const glm::dvec3& plateOriginPos, Camera& Camera, pipelineList::procPipelineNode& procPipeline) {
		if (pipeTerrain.isEnable) {
			if (debugInfo::LogSwitch && (playerInfo::logicOriginPos != plateOriginPos) && !isEntireRebuild) {
				Log::printError(__FILE__, __LINE__, "原点迁移时isEntireRebuild为假");
			}



			int layer = plateEnum::terrainLayer::Layer_0;
			cmd.waitCmd(Device.device, threadId);

			//重建下一线程Lod_0地形管线，RM_0.4.6（8_32：10）
			if (isNullTerrainMesh(layer)) {
				pipeTerrain.mesh[threadId][layer].clear(Device.memAllocator);
				pipeTerrain.mesh[threadId][layer].create(vertexStruct::terrainVertex::mesh[layer].vertexs, vertexStruct::terrainVertex::mesh[layer].indexs, cmd, Device.device, Device.graphicsQueue, Device.memAllocator);

				//重建下一线程Lod_0地形阴影
				if (pipeShadow.isEnable) {
					vertexStruct::shadowVertex::loadTerrainVertexs(layer);
					pipeShadow.shadow_t.mesh[threadId][layer].clear(Device.memAllocator);
					pipeShadow.shadow_t.mesh[threadId][layer].create(vertexStruct::shadowVertex::tMesh[layer].vertexs, cmd, Device.device, Device.graphicsQueue, Device.memAllocator);
				}
			}

			//重建全部线程Lod_1地形管线，RM_0.4.6（8_32：20~50）
			if (isEntireRebuild.load()) {
				layer = plateEnum::terrainLayer::Layer_1;

				if (isNullTerrainMesh(layer)) {
					const std::vector<vertexStruct::terrainVertex::vertexNode>& vertex = vertexStruct::terrainVertex::mesh[layer].vertexs;
					const std::vector<uint32_t>& index  = vertexStruct::terrainVertex::mesh[layer].indexs;
					//重建下一线程Lod_1地形管线
					pipeTerrain.mesh[threadId][layer].clear(Device.memAllocator);
					pipeTerrain.mesh[threadId][layer].create(vertex, index, cmd, Device.device, Device.graphicsQueue, Device.memAllocator);

					//重建下一线程Lod_1地形阴影
					if (pipeShadow.isEnable) {
						vertexStruct::shadowVertex::loadTerrainVertexs(layer);
						pipeShadow.shadow_t.mesh[threadId][layer].clear(Device.memAllocator);
						pipeShadow.shadow_t.mesh[threadId][layer].create(vertexStruct::shadowVertex::tMesh[layer].vertexs, cmd, Device.device, Device.graphicsQueue, Device.memAllocator);
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
						vertexStruct::shadowVertex::loadTerrainVertexs(layer);
						pipeShadow.shadow_t.mesh[tempThreadId][layer].clear(Device.memAllocator);
						pipeShadow.shadow_t.mesh[tempThreadId][layer].create(vertexStruct::shadowVertex::tMesh[layer].vertexs, cmd, Device.device, Device.graphicsQueue, Device.memAllocator);
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
	}

	void recreateSwapChain() {
		InitSwapchain.recreateSwapChain();
		procRender.recreateSwapChain(InitSwapchain.swapExtent, InitSwapchain.colorFormat, InitSwapchain.depthFormat, InitSwapchain.msaa, Device.device, Device.memAllocator);



		//重建离屏描述符，因为交换链重建后osView也得重建
		procDesc.setAllocInfo(descEnum::descType::scenceForward_d);

		//pipeOSMirror
		if (pipeOSMirror.isEnable) {
			std::array<VkWriteDescriptorSet, 3> osMirrorWrite{
				procDesc.getWrite(0, pipeOSMirror.descriptorSet, pipeOSMirror.uniform.buffer,				 sizeof(pipeOSMirror.ubo)),
				procDesc.getWrite(1, pipeOSMirror.descriptorSet, pipeOSMirror.lightUniform.buffer,			 sizeof(pipeOSMirror.lightUbo)),
				procDesc.getWrite(2, pipeOSMirror.descriptorSet, procSampler.node[pipeOSMirror.samplerType], procRender.getFrameImage(renderEnum::renderInstanceId::osMirror_ri, renderEnum::renderFrameId::conv_rf, 2))
			};
			vkUpdateDescriptorSets(Device.device, uint32_t(osMirrorWrite.size()), osMirrorWrite.data(), 0, nullptr);
		}

		//pipeOSScence
		if (pipeOSScence.isEnable) {
			std::array<VkWriteDescriptorSet, 1> osScenceWrite{
				procDesc.getWrite(2, pipeOSScence.descriptorSet, procSampler.node[pipeOSScence.samplerType], procRender.getFrameImage(renderEnum::renderInstanceId::osScence_ri, renderEnum::renderFrameId::conv_rf, 2))
			};
			vkUpdateDescriptorSets(Device.device, uint32_t(osScenceWrite.size()), osScenceWrite.data(), 0, nullptr);
		}

		procDesc.setAllocInfo(descEnum::descType::dsScence_d);

		//pipeDSScence
		if (pipeDSScence.isEnable) {
			std::array<VkWriteDescriptorSet, 6> tempWrite{
				procDesc.getWrite(0, pipeDSScence.descriptorSet, pipeDSScence.lightUniform.buffer,		     sizeof(pipeDSScence.lightUbo)),
				procDesc.getWrite(1, pipeDSScence.descriptorSet, pipeDSScence.shadowUniform.buffer,		     sizeof(pipeDSScence.shadowUbo)),
				procDesc.getWrite(2, pipeDSScence.descriptorSet, procSampler.node[pipeDSScence.samplerType], procRender.getFrameImage(renderEnum::renderInstanceId::dsScence_ri,  renderEnum::renderFrameId::conv_rf, 0)),
				procDesc.getWrite(3, pipeDSScence.descriptorSet, procSampler.node[pipeDSScence.samplerType], procRender.getFrameImage(renderEnum::renderInstanceId::dsScence_ri,  renderEnum::renderFrameId::conv_rf, 1)),
				procDesc.getWrite(4, pipeDSScence.descriptorSet, procSampler.node[pipeDSScence.samplerType], procRender.getFrameImage(renderEnum::renderInstanceId::dsScence_ri,  renderEnum::renderFrameId::conv_rf, 2)),
				procDesc.getWrite(5, pipeDSScence.descriptorSet, procSampler.node[pipeShadow.samplerType],	 procRender.getFrameImage(renderEnum::renderInstanceId::shadowMap_ri, renderEnum::renderFrameId::conv_rf, 0))
			};

			vkUpdateDescriptorSets(Device.device, uint32_t(tempWrite.size()), tempWrite.data(), 0, nullptr);
		}
	}

	void clearSwapChain() {
		procRender.clearSwapChain(Device.device, Device.memAllocator);
	}

	//更新 Uniform
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
			for (bufferStruct::modelStaticNode& sNode : pipeModel.sNode) {
				sNode.currentWorldPos = camera.pos;

				if (glm::distance(sNode.modelWorldPos, sNode.currentWorldPos) < plateInfo::perspSightRange) {
					sNode.ubo.model = glm::translate(glm::mat4(1), glm::vec3(sNode.modelWorldPos - logicOriginPos));	//位移
					sNode.ubo.model *= glm::scale(glm::mat4(1), glm::vec3(0.1f));												//缩放
					//sNode.ubo.model	*= glm::rotate(glm::mat4(1), gameTime * glm::radians(60.f), cameraInfo::worldUp);							//旋转
					sNode.ubo.modelTI = glm::transpose(glm::inverse(sNode.ubo.model));
					sNode.ubo.view = camera.view;

					sNode.uniform.createMemMap(&sNode.ubo, sizeof(sNode.ubo), Device.memAllocator);

					sNode.isVisible = true;
				}
				else {
					sNode.isVisible = false;
				}
			}

			//model-dNode
			for (bufferStruct::modelDynamicNode& dNode : pipeModel.dNode) {
				dNode.currentWorldPos = camera.pos;

				if (glm::distance(dNode.modelWorldPos, dNode.currentWorldPos) < plateInfo::perspSightRange) {
					for (bufferStruct::modelDynamicSubNode& modelNode : dNode.node) {
						modelNode.ubo.model = glm::translate(glm::mat4(1), glm::vec3(dNode.modelWorldPos - logicOriginPos));	//位移
						modelNode.ubo.model *= glm::scale(glm::mat4(1), glm::vec3(0.1f));												//缩放
						modelNode.ubo.modelTI = glm::transpose(glm::inverse(modelNode.ubo.model));
						modelNode.ubo.view = camera.view;

						modelNode.uniform.createMemMap(&modelNode.ubo, sizeof(modelNode.ubo), Device.memAllocator);
					}

					dNode.isVisible = true;
				}
				else {
					dNode.isVisible = false;
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
				for (int i = 0; i < pipeModel.sNode.size(); i++) {
					if (pipeModel.sNode[i].isVisible) {
						pipeShadow.shadow_ms[i].shadowMvpUbo.mvp = pipeShadow.shadowVP * pipeModel.sNode[i].ubo.model;
						pipeShadow.shadow_ms[i].shadowMvpUniform.createMemMap(&pipeShadow.shadow_ms[i].shadowMvpUbo, sizeof(pipeShadow.shadow_ms[i].shadowMvpUbo), Device.memAllocator);
					}
				}

				//shadow-dNode
				for (int i = 0; i < pipeModel.dNode.size(); i++) {
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

	//绘制 frame
	bool drawFrame(int threadId) {
		return InitSwapchain.drawFrame(cmd.renderCmd[threadId].list, cmd.renderCmd[threadId].fence);
	}



	const std::vector<renderStruct::renderTemplateNode>& getRenderList() {
		return procRender.renderTemplateList;
	}

	const std::vector<descStruct::descNode>& getDesc() {
		return procDesc.node;
	}

	VkSampleCountFlagBits getMsaa() {
		return InitSwapchain.msaa;
	}

	const VkExtent2D& getSwapExtent() {
		return InitSwapchain.swapExtent;
	}

private:
	uiStruct::uiTextInfoNode showUIText{};



	//地形网格是否为空
	bool isNullTerrainMesh(int layer) {
		return vertexStruct::terrainVertex::mesh[layer].vertexs.size() && vertexStruct::terrainVertex::mesh[layer].indexs.size();
	}
};