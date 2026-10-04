#pragma once



class Pipeline {
public:
	Pipeline(Device& tDevice) : Device{ tDevice } {
		startWorkers();

		pipeTerrain .create(fileInfo::image::cube, textureInfo::terrain::colorFormat, textureInfo::terrain::tileExtent, textureInfo::terrain::atlasExtent, cmd, Device.device, Device.graphicsQueue, Device.memAllocator, descFamily(pipeTerrain.pipeType), procSampler.node);
		pipeModel   .create(cmd, Device.device, Device.graphicsQueue, Device.memAllocator, descFamily(pipeModel.pipeType), procSampler.node);
		pipeSkybox  .create(cmd, Device.device, Device.graphicsQueue, Device.memAllocator, descFamily(pipeSkybox.pipeType));
		pipeMine    .create(cmd, Device.device, Device.graphicsQueue, Device.memAllocator, descFamily(pipeMine.pipeType));
		pipeUIText  .create(cmd, Device.device, Device.graphicsQueue, Device.memAllocator, descFamily(pipeUIText.pipeType), procSampler.node);
		pipeOSMirror.create(cmd, Device.device, Device.graphicsQueue, Device.memAllocator, descFamily(pipeOSMirror.pipeType), procSampler.node, frameView(pipeOSMirror.pipeType));
		pipeDSScence.create(Device.memAllocator, descFamily(pipeDSScence.pipeType), procSampler.node, { frameView(renderEnum::renderInstanceId::dsScence_ri, 0), frameView(renderEnum::renderInstanceId::dsScence_ri, 1), frameView(pipeDSScence.pipeType) }, procSampler.node[pipeShadow.samplerType], frameView(renderEnum::renderInstanceId::shadowMap_ri, 0), Device.device);
		pipeShadow	.create(pipeTerrain.isEnable, pipeModel.isEnable, cmd, Device.device, Device.graphicsQueue, Device.memAllocator, descFamily(pipeShadow.pipeType));
		pipeOSScence.create(descFamily(pipeOSScence.pipeType), procSampler.node, frameView(pipeOSScence.pipeType), Device.device);

		initUniformBuffer();	//初始化Uniform
		drawUIText(0);			//初始化uiText

		createPipeline();
		createCmd(cmd.thread.current.load());

		lockInfo::logPauseEnableLock.store(false);
		if (debugInfo::LogSwitch) {
			Log::printTitle("————————Pipeline已创建");

			Log::delimiter();
		}
	}

	~Pipeline() {
		shutdownWorkers();

		cmd.clear(Device.device);

		procRender  .clear(Device.device, Device.memAllocator);
		procSampler .clear(Device.device);
		procDesc    .clear(Device.device);
		procPipeline.clear(Device.device);

		if (pipeTerrain.isEnable)	 { pipeTerrain .clear(Device.device, Device.memAllocator); }
		if (pipeModel.isEnable)		 { pipeModel   .clear(Device.device, Device.memAllocator); }
		if (pipeSkybox.isEnable)	 { pipeSkybox  .clear(Device.device, Device.memAllocator); }
		if (pipeMine.isEnable)		 { pipeMine	   .clear(Device.device, Device.memAllocator); }
		if (pipeUIText.isEnable)	 { pipeUIText  .clear(Device.device, Device.memAllocator); }
		if (pipeOSMirror.isEnable)	 { pipeOSMirror.clear(Device.device, Device.memAllocator); }
		if (pipeOSScence.isEnable)	 { pipeOSScence.clear(Device.device, Device.memAllocator); }
		if (pipeDSScence.isEnable)	 { pipeDSScence.clear(Device.device, Device.memAllocator); }
		if (pipeShadow.isEnable)	 { pipeShadow  .clear(Device.device, Device.memAllocator); }

		lockInfo::logPauseEnableLock.store(false);
		if (debugInfo::LogSwitch) {
			Log::print("Pipeline已清除");
		}
	}



	void drawFrame(Camera& Camera) {
		static playerStruct::selectBoxNode selectBox = InitPlate.getSelectBox(playerInfo::logicOriginPos, Camera.node);
		static int uiDeltaFrame = 0;							//UI帧增量
		const bool cmdIsFlip    = cmd.isFlip();					//线程是否翻转



		//更新uiText
		uiDeltaFrame++;
		if (engineList::procTick.node[engineEnum::tickEnum::uiText].isUpdate()) {
			drawUIText(static_cast<int>(uiDeltaFrame / engineList::procTick.node[engineEnum::tickEnum::uiText].dt));
			uiDeltaFrame = 0;
		}

		//更新板块，因玩家移动后更新板块网格
		if (engineList::procTick.node[engineEnum::tickEnum::plate].isUpdate() && !cmdIsFlip) {
			static glm::dvec3 pos0 = glm::floor(playerInfo::initSpawnPos / static_cast<double>(plateInfo::chunkLength)) * static_cast<double>(plateInfo::chunkLength);
			glm::dvec3		  pos1 = glm::floor(Camera.node.pos / static_cast<double>(plateInfo::chunkLength)) * static_cast<double>(plateInfo::chunkLength);

			if (pos0 != pos1 && !lockInfo::updatePlateFuncLock.exchange(true)) {
				submitUpdatePlate(Camera.node.pos);
				pos0 = pos1;
			}
		}

		//更新方块，因放置/破坏方块后更新板块网格，命令缓冲区线程未变 & 方块变更列表非空 & 方块更新未入队
		if (engineList::procTick.node[engineEnum::tickEnum::cube].isUpdate() && !cmdIsFlip && InitPlate.isHandleChangeCubeList() && !lockInfo::updateCubeFuncLock.exchange(true)) {
			submitUpdateCube();
		}

		//更换命令缓冲区后重建命令缓冲区，r=1时更新板块会把网格数组清空
		if (cmdIsFlip && !lockInfo::recreateCmdFuncLock.exchange(true)) {
			submitRecreateCmd(Camera);
		}



		lockInfo::drawFuncLock.lock();
		const int currentThread = cmd.thread.current.load();								//必须在 drawFuncLock 内读，和cmd.thread.swap()互斥

		if (lockInfo::getLock(lockInfo::chunkMapDataLock)) {
			selectBox = InitPlate.getSelectBox(playerInfo::logicOriginPos, Camera.node);	//当前帧获取的方块更新列表在下一帧更新
			lockInfo::chunkMapDataLock.store(false);
		}

		updateUniform(playerInfo::logicOriginPos, Camera.node, selectBox);
		if (InitSwapchain.drawFrame(cmd.renderCmd[currentThread].list, cmd.renderCmd[currentThread].fence)) {
			lockInfo::drawFuncLock.unlock();
			recreateSwapChain(Camera);
			lockInfo::drawFuncLock.lock();
		}
		lockInfo::drawFuncLock.unlock();
	}

private:
	Device& Device;

	pipelineStruct::asyncWorker plateWorker{};		//板块更新专用 worker
	pipelineStruct::asyncWorker cubeWorker{};		//方块更新专用 worker
	pipelineStruct::asyncWorker cmdWorker{};		//命令重建专用 worker

	Plate	  InitPlate{};
	Swapchain InitSwapchain{ Device };
	cmdStruct::cmdNode cmd{ Device.device, Device.enableGpu->graphicsQueueFamily.index, uint32_t(swapStruct::swapImageList.size()) };

	pipelineList::procRenderNode   procRender  { InitSwapchain.colorFormat, InitSwapchain.depthFormat, InitSwapchain.swapExtent, InitSwapchain.msaa, Device.device, Device.memAllocator };
	pipelineList::procSamplerNode  procSampler { Device.device, Device.enableGpuInfo.limits };
	pipelineList::procDescNode     procDesc    { Device.device };
	pipelineList::procPipelineNode procPipeline{ procRender.renderTemplateList, procDesc.node, InitSwapchain.msaa, Device.device };

	bufferStruct::pipeTerrainNode  pipeTerrain { true,  pipelineEnum::procPipelineId::terrain_p,   samplerEnum::procSamplerId::terrain_s };
	bufferStruct::pipeModelNode	   pipeModel   { true,  pipelineEnum::procPipelineId::model_p	   };
	bufferStruct::pipeSkyboxNode   pipeSkybox  { true,	pipelineEnum::procPipelineId::skybox_p	   };
	bufferStruct::pipeMineNode	   pipeMine	   { true,  pipelineEnum::procPipelineId::mine_p	   };
	bufferStruct::pipeUITextNode   pipeUIText  { true,  pipelineEnum::procPipelineId::uiText_p,    samplerEnum::procSamplerId::uiText_s    };
	bufferStruct::pipeOSMirrorNode pipeOSMirror{ false, pipelineEnum::procPipelineId::osMirror_p,  samplerEnum::procSamplerId::offScreen_s };
	bufferStruct::pipeOSScenceNode pipeOSScence{ false, pipelineEnum::procPipelineId::osScence_p,  samplerEnum::procSamplerId::offScreen_s };
	bufferStruct::pipeDSScenceNode pipeDSScence{ true,	pipelineEnum::procPipelineId::dsScence_p,  samplerEnum::procSamplerId::offScreen_s };
	bufferStruct::pipeShadowNode   pipeShadow  { true,  pipelineEnum::procPipelineId::shadowMap_p, samplerEnum::procSamplerId::shadowMap_s };

	uiStruct::uiTextInfoNode showUIText{};



	void startWorkers() {
		plateWorker.start();
		cubeWorker .start();
		cmdWorker  .start();
	}

	void shutdownWorkers() {
		plateWorker.shutdown();
		cubeWorker .shutdown();
		cmdWorker  .shutdown();
	}

	void initUniformBuffer() {
		//设置透视投影，参数依次是视野角度、宽高比、近裁剪面和远裁剪面，以及proj[1][1] *= -1轴向修正
		glm::mat4 perspProj = glm::perspective(glm::radians(45.0f), static_cast<float>(InitSwapchain.swapExtent.width) / static_cast<float>(InitSwapchain.swapExtent.height), 0.1f, plateInfo::perspSightRange);
		perspProj[1][1] *= -1;

		//设置正交投影
		glm::mat4 orthoProj = glm::ortho(-plateInfo::orthoSightRange, plateInfo::orthoSightRange, -plateInfo::orthoSightRange, plateInfo::orthoSightRange, 0.1f, plateInfo::perspSightRange);
		orthoProj[1][1] *= -1;



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
	
	void createPipeline() {
		const VkExtent2D& extent = InitSwapchain.swapExtent;

		//shadowMap_rt
		procPipeline.createPipeline(pipelineEnum::procPipelineId::shadowMap_p, renderInfo::shadowMapExtent);

		/*deferred_rt*/
		procPipeline.createPipeline(pipelineEnum::procPipelineId::terrain_p,   extent);
		procPipeline.createPipeline(pipelineEnum::procPipelineId::model_p,     extent);
		procPipeline.createPipeline(pipelineEnum::procPipelineId::skybox_p,    extent);
		procPipeline.createPipeline(pipelineEnum::procPipelineId::mine_p,      extent);
		procPipeline.createPipeline(pipelineEnum::procPipelineId::uiText_p,    extent);

		/*offScreen_rt*/
		procPipeline.createPipeline(pipelineEnum::procPipelineId::osMirror_p,  extent);

		/*swapChain_rt*/
		procPipeline.createPipeline(pipelineEnum::procPipelineId::dsScence_p,  extent);
		procPipeline.createPipeline(pipelineEnum::procPipelineId::osScence_p,  extent);
	}

	void createCmd(int threadId) {
		for (int frameId = 0; frameId < cmd.renderCmd[threadId].size(); frameId++) {
			cmd.recordCmd(threadId, frameId, [&](cmdStruct::cmdRecorderNode& recorder) {
				recordShadowCasters(recorder, threadId);
				recordDSScence(recorder, threadId);
				recordOSMirror(recorder);
				recordOSScence(recorder);
				recordScScence(recorder, frameId);
			});
		}
	}

	void recreateCmd(int threadId, Camera& Camera) {
		if (pipeTerrain.isEnable) {
			if (debugInfo::LogSwitch && (playerInfo::logicOriginPos != InitPlate.originPos) && !InitPlate.isEntireRebuild) {
				Log::printError(__FILE__, __LINE__, "原点迁移时isEntireRebuild为假");
			}

			int layer = plateEnum::terrainLayer::Layer_0;
			cmd.waitCmd(Device.device, threadId);

			rebuildTerrainMesh(threadId, layer);			//重建下一线程Lod_0地形管线，RM_0.4.6（8_32：10）
			rebuildAllCommands(threadId, layer, Camera);	//重建全部线程Lod_1地形管线，RM_0.4.6（8_32：20~50）

			switchCommandThread(threadId, Camera);			//重建下一线程命令缓冲区，并切换线程
		}
	}

	void rebuildTerrainMesh(int threadId, int layer) {
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
	}

	void rebuildAllCommands(int threadId, int layer, Camera& Camera) {
		if (InitPlate.isEntireRebuild.load()) {
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
				if (playerInfo::logicOriginPos != InitPlate.originPos) {
					playerInfo::logicOriginPos = InitPlate.originPos;
					Camera.updateView();

					Log::print("原点迁移");
				}

				lockInfo::queueThreadLock.lock();
				cmd.renderCmd[threadId].recreateCmdList(Device.device);
				createCmd(threadId);
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

				InitPlate.isEntireRebuild.store(false);
				return;
			}

			InitPlate.isEntireRebuild.store(false);
		}
	}

	void switchCommandThread(int threadId, Camera& Camera) {
		lockInfo::drawFuncLock.lock();
		if (playerInfo::logicOriginPos != InitPlate.originPos) {
			playerInfo::logicOriginPos = InitPlate.originPos;
			Camera.updateView();

			Log::print("原点迁移");
		}

		lockInfo::queueThreadLock.lock();
		cmd.renderCmd[threadId].recreateCmdList(Device.device);
		createCmd(threadId);
		cmd.swap();
		lockInfo::queueThreadLock.unlock();
		lockInfo::drawFuncLock.unlock();
	}

	void recreateSwapChain(Camera& Camera) {
		plateWorker.waitIdle();					//等板块任务排空
		cubeWorker .waitIdle();					//等方块任务排空
		cmdWorker  .waitIdle();					//等命令任务排空

		lockInfo::queueThreadLock.lock();		//recreateSwapChain里队列命令操作太多了挨个隔离太麻烦，干脆全部隔离
		vkDeviceWaitIdle(Device.device);
		clearSwapChain();
		procPipeline.clearSwapChain(Device.device);

		InitSwapchain.recreateSwapChain();
		procRender.recreateSwapChain(InitSwapchain.swapExtent, InitSwapchain.colorFormat, InitSwapchain.depthFormat, InitSwapchain.msaa, Device.device, Device.memAllocator);

		pipeOSMirror.updateFrameDesc(descFamily(pipeOSMirror.pipeType), procSampler.node, frameView(pipeOSMirror.pipeType), Device.device);
		pipeOSScence.updateFrameDesc(descFamily(pipeOSScence.pipeType), procSampler.node, frameView(pipeOSScence.pipeType), Device.device);
		pipeDSScence.updateFrameDesc(descFamily(pipeDSScence.pipeType), procSampler.node, { frameView(renderEnum::renderInstanceId::dsScence_ri, 0), frameView(renderEnum::renderInstanceId::dsScence_ri, 1), frameView(pipeDSScence.pipeType) }, procSampler.node[pipeShadow.samplerType], frameView(renderEnum::renderInstanceId::shadowMap_ri, 0), Device.device);
		
		createPipeline();
		lockInfo::queueThreadLock.unlock();

		cmd.flip();
		recreateCmd(cmd.thread.next.load(), Camera);
		Log::print("重建交换链");
	}

	void clearSwapChain() {
		procRender.clearSwapChain(Device.device, Device.memAllocator);
	}


	
	void submitUpdatePlate(const glm::dvec3& playerPos) {
		plateWorker.submit([this, playerPos] {
			engineStruct::timeNode time{};

			InitPlate.updatePlate(playerInfo::logicOriginPos, playerPos);
			cmd.flip();

			Log::print("板块刷新用时:\t" + time.end());
			lockInfo::updatePlateFuncLock.store(false);
		});
	}

	void submitUpdateCube() {
		cubeWorker.submit([this] {
			if (lockInfo::getLock(lockInfo::plateMeshDataLock)) {
				if (lockInfo::getLock(lockInfo::chunkMapDataLock)) {
					InitPlate.handleChangeCubeList();
					cmd.flip();
					lockInfo::chunkMapDataLock.store(false);
				}
				lockInfo::plateMeshDataLock.store(false);
			}

			lockInfo::updateCubeFuncLock.store(false);
		});
	}

	void submitRecreateCmd(Camera& Camera) {
		cmdWorker.submit([this, &Camera] {
			if (lockInfo::getLock(lockInfo::plateMeshDataLock)) {
				recreateCmd(cmd.thread.next.load(), Camera);
				lockInfo::plateMeshDataLock.store(false);
			}

			lockInfo::recreateCmdFuncLock.store(false);
		});
	}
	
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

		updateTerrainUniform (camera);											//pipeTerrain
		updateModelUniform	 (camera, gameTime, logicOriginPos);				//model
		updateSkyboxUniform	 (camera, currentHour);								//pipeSkybox
		updateMineUniform	 (camera, box);										//pipeMine
		updateUITextUniform  (camera, box);										//showUIText
		updateOSMirrorUniform(offsetPos, camera, currentHour, logicOriginPos);	//pipeOSMirror
		updateShadowUniform  (playerPos, currentHour);							//shadow
		updateDSScenceUniform(offsetPos, camera, currentHour, logicOriginPos);	//pipeDSScence
	}



	descStruct::descNode& descFamily(pipelineEnum::procPipelineId::type pipeType) {
		return procDesc.node[procPipeline.node[pipeType].descType];
	}

	VkImageView frameView(renderEnum::renderInstanceId::type instanceId, uint32_t viewId) {
		return procRender.getFrameImage(instanceId, renderEnum::renderFrameId::conv_rf, viewId);
	}

	VkImageView frameView(pipelineEnum::procPipelineId::type pipeType) {
		renderEnum::renderInstanceId::type instanceId = renderEnum::renderInstanceId::enumNull;		//帧图实例：仅采样离屏帧图的管线需要

		switch (pipeType) {
		case pipelineEnum::procPipelineId::osMirror_p: { instanceId = renderEnum::renderInstanceId::osMirror_ri; break; }
		case pipelineEnum::procPipelineId::osScence_p: { instanceId = renderEnum::renderInstanceId::osScence_ri; break; }
		case pipelineEnum::procPipelineId::dsScence_p: { instanceId = renderEnum::renderInstanceId::dsScence_ri; break; }
		default: { break; }}

		return procRender.getFrameImage(instanceId, renderEnum::renderFrameId::conv_rf, 2);
	}

	bool isNullTerrainMesh(int layer) {
		return vertexStruct::terrainVertex::mesh[layer].vertexs.size() && vertexStruct::terrainVertex::mesh[layer].indexs.size();
	}

	void updateTerrainUniform(const playerStruct::cameraNode& camera) {
		if (pipeTerrain.isEnable) {
			pipeTerrain.ubo.view = camera.view;

			pipeTerrain.uniform.createMemMap(&pipeTerrain.ubo, sizeof(pipeTerrain.ubo), Device.memAllocator);
		}
	}

	void updateModelUniform(const playerStruct::cameraNode& camera, float gameTime, const glm::dvec3& logicOriginPos) {
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
	}

	void updateSkyboxUniform(const playerStruct::cameraNode& camera, float currentHour) {
		if (pipeSkybox.isEnable) {
			pipeSkybox.ubo.view				= camera.view;
			pipeSkybox.lightUbo.currentHour = currentHour;

			pipeSkybox.uniform.createMemMap(&pipeSkybox.ubo, sizeof(pipeSkybox.ubo), Device.memAllocator);
			pipeSkybox.lightUniform.createMemMap(&pipeSkybox.lightUbo, sizeof(pipeSkybox.lightUbo), Device.memAllocator);
		}
	}

	void updateMineUniform(const playerStruct::cameraNode& camera, const playerStruct::selectBoxNode& box) {
		if (pipeMine.isEnable) {
			pipeMine.ubo.model = glm::translate(glm::mat4(1), box.minepos);
			pipeMine.ubo.model *= glm::scale(glm::mat4(1), glm::vec3(1) * box.isVisible);
			pipeMine.ubo.view  = camera.view;

			pipeMine.uniform.createMemMap(&pipeMine.ubo, sizeof(pipeMine.ubo), Device.memAllocator);
		}
	}

	void updateUITextUniform(const playerStruct::cameraNode& camera, const playerStruct::selectBoxNode& box) {
		if (pipeUIText.isEnable) {
			showUIText.pos		= glm::floor(camera.pos);
			showUIText.chunkPos = glm::floor(camera.pos / static_cast<double>(plateInfo::chunkLength));
			showUIText.minepos  = box.minepos;

			//方块数组真实坐标
			//showUIText.minepos.x = static_cast<int>(fmod(box.minepos.x, plateInfo::chunkLength));
			//showUIText.minepos.y = static_cast<int>(fmod(box.minepos.y, plateInfo::chunkLength));
			//showUIText.minepos.z = static_cast<int>(fmod(box.minepos.z, plateInfo::chunkLength));
		}
	}

	void updateOSMirrorUniform(const glm::vec3& offsetPos, const playerStruct::cameraNode& camera, float currentHour, const glm::dvec3& logicOriginPos) {
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
	}

	void updateShadowUniform(const glm::vec3& playerPos, float currentHour) {
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
	}

	void updateDSScenceUniform(const glm::vec3& offsetPos, const playerStruct::cameraNode& camera, float currentHour, const glm::dvec3& logicOriginPos) {
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

	void recordMine(cmdStruct::cmdRecorderNode& recorder) {
		if (pipeMine.isEnable) {
			recorder.draw(pipelineEnum::cmdDrawId::indexed, procPipeline.getPipeline(pipeMine.pipeType), procPipeline.getPipeLayout(pipeMine.pipeType), pipeMine.descriptorSet, pipeMine.mesh.vertex.buffer, pipeMine.mesh.index.buffer, static_cast<uint32_t>(vertexStruct::mineVertex::indexs.size()));
		}
	}

	void recordSkybox(cmdStruct::cmdRecorderNode& recorder) {
		if (pipeSkybox.isEnable) {
			recorder.draw(pipelineEnum::cmdDrawId::indexed, procPipeline.getPipeline(pipeSkybox.pipeType), procPipeline.getPipeLayout(pipeSkybox.pipeType), pipeSkybox.descriptorSet, pipeSkybox.mesh.vertex.buffer, pipeSkybox.mesh.index.buffer, static_cast<uint32_t>(vertexStruct::skyboxVertex::indexs.size()));
		}
	}

	void recordUIText(cmdStruct::cmdRecorderNode& recorder) {
		if (pipeUIText.isEnable) {
			recorder.draw(pipelineEnum::cmdDrawId::text, procPipeline.getPipeline(pipeUIText.pipeType), procPipeline.getPipeLayout(pipeUIText.pipeType), pipeUIText.descriptorSet, pipeUIText.vertex.buffer, VK_NULL_HANDLE, pipeUIText.msdfGlyphAtlas.glyphCount);
		}
	}

	void recordModel(cmdStruct::cmdRecorderNode& recorder) {
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
	}

	void recordTerrain(cmdStruct::cmdRecorderNode& recorder, int threadId) {
		if (pipeTerrain.isEnable) {
			for (int layer = 0; layer < plateEnum::terrainLayer::enumCount; layer++) {
				if (isNullTerrainMesh(layer)) {
					recorder.draw(pipelineEnum::cmdDrawId::indexed, procPipeline.getPipeline(pipeTerrain.pipeType), procPipeline.getPipeLayout(pipeTerrain.pipeType), pipeTerrain.descriptorSet, pipeTerrain.mesh[threadId][layer].vertex.buffer, pipeTerrain.mesh[threadId][layer].index.buffer, static_cast<uint32_t>(vertexStruct::terrainVertex::mesh[layer].indexs.size()));
				}
			}
		}
	}

	void recordScScence(cmdStruct::cmdRecorderNode& recorder, int frameId) {
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
				recordUIText(recorder);
			}

			recorder.endRender();
		}
	}

	void recordOSScence(cmdStruct::cmdRecorderNode& recorder) {
		/*osScence_ri*/
		if (pipeOSScence.isEnable) {
			recorder.beginRender(&procRender.getRenderBegin(renderEnum::renderInstanceId::osScence_ri));

			//pipeOSMirror
			if (pipeOSMirror.isEnable) {
				recorder.draw(pipelineEnum::cmdDrawId::indexed, procPipeline.getPipeline(pipeOSMirror.pipeType), procPipeline.getPipeLayout(pipeOSMirror.pipeType), pipeOSMirror.descriptorSet, pipeOSMirror.mesh.vertex.buffer, pipeOSMirror.mesh.index.buffer, static_cast<uint32_t>(vertexStruct::osMirrorVertex::indexs.size()));
			}

			recorder.endRender();
		}
	}

	void recordOSMirror(cmdStruct::cmdRecorderNode& recorder) {
		/*osMirror_ri*/
		if (pipeOSMirror.isEnable) {
			recorder.beginRender(&procRender.getRenderBegin(renderEnum::renderInstanceId::osMirror_ri));

			recorder.endRender();
		}
	}

	void recordDSScence(cmdStruct::cmdRecorderNode& recorder, int threadId) {
		/*dsScence_ri*/
		recorder.beginRender(&procRender.getRenderBegin(renderEnum::renderInstanceId::dsScence_ri));
		recordTerrain(recorder, threadId);	//pipeTerrain
		recordModel(recorder);				//model
		recordSkybox(recorder);				//pipeSkybox
		recordMine(recorder);				//pipeMine
		recorder.endRender();
	}

	//记录阴影投射者（阴影趟：地形 + 模型，写阴影贴图 ✓）
	void recordShadowCasters(cmdStruct::cmdRecorderNode& recorder, int threadId) {
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
};