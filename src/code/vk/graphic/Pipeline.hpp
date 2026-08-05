#pragma once



class Pipeline {
public:
	Pipeline(Device& Device) : Device{ Device } {
		createPipeline();

		InitBuffer.drawUIText(0);	//不在createCommandBuffers前初始化就不能显示UIText
		InitBuffer.createCommandBuffer(InitBuffer.cmd.currentThreadId, procPipeline);

		lockInfo::logPauseEnableLock.store(true);

		if (debugInfo::LogSwitch) {
			Log::printTitle("————————Pipeline已创建");

			Log::delimiter();
		}
	}

	~Pipeline() {
		while (lockInfo::updatePlateFuncLock.load()) { std::this_thread::yield(); }
		while (lockInfo::recreateCmdFuncLock.load()) { std::this_thread::yield(); }

		procPipeline.clear(Device.device);

		lockInfo::logPauseEnableLock.store(false);

		if (debugInfo::LogSwitch) {
			Log::print("Pipeline已清除");
		}
	}

private:
	Device& Device;

	Plate  InitPlate{};
	Buffer InitBuffer{ Device };

	pipelineList::procPipelineNode procPipeline{ Device.device, InitBuffer.getDstList(), InitBuffer.getRenderList(), InitBuffer.getMsaaSamples() };



	//创建图形管线
	void createPipeline() {
		const VkExtent2D& extent = InitBuffer.getScExtent();

		//shadowMap_rt
		procPipeline.createPipeline<shadowVertex::vertexNode>(Device.device, pipelineEnum::procPipelineId::shadowMap_p, VkExtent2D{ renderInfo::shadowMapLength, renderInfo::shadowMapLength });

		/*deferred_rt*/
		procPipeline.createPipeline<terrainVertex::vertexNode>(Device.device, pipelineEnum::procPipelineId::terrain_p, extent);
		procPipeline.createPipeline<modelVertex::vertexNode>  (Device.device, pipelineEnum::procPipelineId::model_p,   extent);
		procPipeline.createPipeline<skyboxVertex::vertexNode> (Device.device, pipelineEnum::procPipelineId::skybox_p,  extent);
		procPipeline.createPipeline<mineVertex::vertexNode>   (Device.device, pipelineEnum::procPipelineId::mine_p,    extent);
		procPipeline.createPipeline<uiTextVertex::vertexNode> (Device.device, pipelineEnum::procPipelineId::uiText_p,  extent);

		/*offScreen_rt*/
		procPipeline.createPipeline<osMirrorVertex::vertexNode>(Device.device, pipelineEnum::procPipelineId::osMirror_p, extent);

		/*swapChain_rt*/
		procPipeline.createPipeline<offScreenVertex::vertexNode>(Device.device, pipelineEnum::procPipelineId::dsScence_p, extent);
		procPipeline.createPipeline<offScreenVertex::vertexNode>(Device.device, pipelineEnum::procPipelineId::osScence_p, extent);
	}

public:
	void drawFrame(Camera& Camera) {
		static playerStruct::selectBoxNode selectBox = InitPlate.getSelectBox(playerInfo::logicOriginPos, Camera.node);
		static int uiDeltaFrame = 0;                 //UI帧增量



		//更新uiText
		uiDeltaFrame++;
		if (engineList::procTick.node[engineEnum::tickEnum::uiText].isUpdate()) {
			InitBuffer.drawUIText((int)(uiDeltaFrame / engineList::procTick.node[engineEnum::tickEnum::uiText].dt));
			uiDeltaFrame = 0;
		}

		//更新板块，因玩家移动后更新板块网格
		if (engineList::procTick.node[engineEnum::tickEnum::plate].isUpdate() && !InitBuffer.cmd.isUpdate()) {
			static glm::dvec3 pos0 = glm::floor(playerInfo::initSpawnPos / (double)plateInfo::chunkLength) * (double)plateInfo::chunkLength;
			glm::dvec3 pos1		   = glm::floor(Camera.node.pos / (double)plateInfo::chunkLength) * (double)plateInfo::chunkLength;

			if (pos0 != pos1 && !lockInfo::updatePlateFuncLock.exchange(true)) {
				std::thread(&Pipeline::atomicUpdatePlate, this, Camera.node.pos).detach();
				pos0 = pos1;
			}
		}

		//更新方块，因放置/破坏方块后更新板块网格，命令缓冲区线程未变 & 方块变更列表非空 & 板块网格未锁
		if (engineList::procTick.node[engineEnum::tickEnum::cube].isUpdate() && !InitBuffer.cmd.isUpdate() && InitPlate.isHandleChangeCubeList()) {
			if (lockInfo::getLock(lockInfo::plateMeshDataLock)) {
				if (lockInfo::getLock(lockInfo::chunkMapDataLock)) {
					InitPlate.handleChangeCubeList();
					InitBuffer.cmd.setNextThreadId();
					lockInfo::chunkMapDataLock.store(false);
				}
				lockInfo::plateMeshDataLock.store(false);
			}
		}

		//更换命令缓冲区后重建命令缓冲区，r=1时更新板块会把网格数组清空
		if (InitBuffer.cmd.isUpdate() && !lockInfo::recreateCmdFuncLock.exchange(true)) {
			std::thread(&Pipeline::atomicRecreateCommandBuffer, this, std::ref(Camera)).detach();
		}



		lockInfo::drawFuncLock.lock();
		if (lockInfo::getLock(lockInfo::chunkMapDataLock)) {
			selectBox = InitPlate.getSelectBox(playerInfo::logicOriginPos, Camera.node);		//当前帧获取的方块更新列表在下一帧更新
			lockInfo::chunkMapDataLock.store(false);
		}

		InitBuffer.updateUniformBuffer(playerInfo::logicOriginPos, Camera.node, selectBox);
		if (InitBuffer.drawFrame()) {
			lockInfo::drawFuncLock.unlock();
			recreateSwapChain(Camera);
			lockInfo::drawFuncLock.lock();
		}
		lockInfo::drawFuncLock.unlock();
	}

private:
	//原子线程更新板块
	void atomicUpdatePlate(glm::dvec3 playerPos) {
		engineStruct::timeNode time{};

		InitPlate.updatePlate(playerInfo::logicOriginPos, playerPos);
		InitBuffer.cmd.setNextThreadId();

		Log::print("板块刷新用时:\t" + time.end());
		lockInfo::updatePlateFuncLock.store(false);
	}

	//原子线程更新命令缓冲区
	void atomicRecreateCommandBuffer(Camera& Camera) {
		if (lockInfo::getLock(lockInfo::plateMeshDataLock)) {
			InitBuffer.recreateCommandBuffer(InitBuffer.cmd.nextThreadId, InitPlate.isEntireRebuild, InitPlate.originPos, Camera, procPipeline);
			lockInfo::plateMeshDataLock.store(false);
		}

		lockInfo::recreateCmdFuncLock.store(false);
	}

	void recreateSwapChain(Camera& Camera) {
		while (lockInfo::updatePlateFuncLock.load()) { std::this_thread::yield(); }
		while (lockInfo::recreateCmdFuncLock.load()) { std::this_thread::yield(); }

		lockInfo::queueThreadLock.lock();		//recreateSwapChain里队列命令操作太多了挨个隔离太麻烦，干脆全部隔离
		vkDeviceWaitIdle(Device.device);

		InitBuffer.clearSwapChain();
		procPipeline.clearSwapChain(Device.device);

		InitBuffer.recreateSwapChain();
		createPipeline();
		lockInfo::queueThreadLock.unlock();

		InitBuffer.cmd.setNextThreadId();
		InitBuffer.recreateCommandBuffer(InitBuffer.cmd.nextThreadId, InitPlate.isEntireRebuild, InitPlate.originPos, Camera, procPipeline);
		Log::print("重建交换链");

		lockInfo::recreateCmdFuncLock.store(false);
		lockInfo::updatePlateFuncLock.store(false);
	}
};