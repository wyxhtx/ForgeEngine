#pragma once



class Pipeline {
public:
	Pipeline(Device& tDevice) : Device{ tDevice } {
		createPipeline();
		InitBuffer.createCmd(InitBuffer.cmd.thread.current.load(), procPipeline);

		plateWorker.start();
		cubeWorker .start();
		cmdWorker  .start();
		lockInfo::logPauseEnableLock.store(false);

		if (debugInfo::LogSwitch) {
			Log::printTitle("————————Pipeline已创建");

			Log::delimiter();
		}
	}

	~Pipeline() {
		plateWorker.shutdown();		//排空板块队列并退出
		cubeWorker .shutdown();		//排空方块队列并退出
		cmdWorker  .shutdown();		//排空命令队列并退出

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

	pipelineList::procPipelineNode procPipeline{ Device.device, InitBuffer.getDstList(), InitBuffer.getRenderList(), InitBuffer.getMsaa() };

	pipelineStruct::asyncWorker plateWorker{};		//板块更新专用 worker
	pipelineStruct::asyncWorker cubeWorker{};		//方块更新专用 worker
	pipelineStruct::asyncWorker cmdWorker{};		//命令重建专用 worker



	//创建图形管线
	void createPipeline() {
		const VkExtent2D& extent = InitBuffer.getSwapExtent();

		//shadowMap_rt
		procPipeline.createPipeline<shadowVertex::vertexNode>(Device.device, pipelineEnum::procPipelineId::shadowMap_p, renderInfo::shadowMapExtent);

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
		static int uiDeltaFrame = 0;							//UI帧增量
		const bool cmdIsFlip    = InitBuffer.cmd.isFlip();		//线程是否翻转



		//更新uiText
		uiDeltaFrame++;
		if (engineList::procTick.node[engineEnum::tickEnum::uiText].isUpdate()) {
			InitBuffer.drawUIText(static_cast<int>(uiDeltaFrame / engineList::procTick.node[engineEnum::tickEnum::uiText].dt));
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
		const int currentThread = InitBuffer.cmd.thread.current.load();						//必须在 drawFuncLock 内读，和cmd.thread.swap()互斥

		if (lockInfo::getLock(lockInfo::chunkMapDataLock)) {
			selectBox = InitPlate.getSelectBox(playerInfo::logicOriginPos, Camera.node);	//当前帧获取的方块更新列表在下一帧更新
			lockInfo::chunkMapDataLock.store(false);
		}

		InitBuffer.updateUniform(playerInfo::logicOriginPos, Camera.node, selectBox);
		if (InitBuffer.drawFrame(currentThread)) {
			lockInfo::drawFuncLock.unlock();
			recreateSwapChain(Camera);
			lockInfo::drawFuncLock.lock();
		}
		lockInfo::drawFuncLock.unlock();
	}

private:
	void recreateSwapChain(Camera& Camera) {
		plateWorker.waitIdle();					//等板块任务排空
		cubeWorker .waitIdle();					//等方块任务排空
		cmdWorker  .waitIdle();					//等命令任务排空

		lockInfo::queueThreadLock.lock();		//recreateSwapChain里队列命令操作太多了挨个隔离太麻烦，干脆全部隔离
		vkDeviceWaitIdle(Device.device);

		InitBuffer  .clearSwapChain();
		procPipeline.clearSwapChain(Device.device);

		InitBuffer.recreateSwapChain();
		createPipeline();
		lockInfo::queueThreadLock.unlock();

		InitBuffer.cmd.flip();
		InitBuffer.recreateCmd(InitBuffer.cmd.thread.next.load(), InitPlate.isEntireRebuild, InitPlate.originPos, Camera, procPipeline);
		Log::print("重建交换链");
	}

	//提交板块更新任务，plateWorker执行
	void submitUpdatePlate(const glm::dvec3& playerPos) {
		plateWorker.submit([this, playerPos] {
			engineStruct::timeNode time{};

			InitPlate.updatePlate(playerInfo::logicOriginPos, playerPos);
			InitBuffer.cmd.flip();

			Log::print("板块刷新用时:\t" + time.end());
			lockInfo::updatePlateFuncLock.store(false);
		});
	}

	//提交方块更新任务，cubeWorker执行
	void submitUpdateCube() {
		cubeWorker.submit([this] {
			if (lockInfo::getLock(lockInfo::plateMeshDataLock)) {
				if (lockInfo::getLock(lockInfo::chunkMapDataLock)) {
					InitPlate.handleChangeCubeList();
					InitBuffer.cmd.flip();
					lockInfo::chunkMapDataLock.store(false);
				}
				lockInfo::plateMeshDataLock.store(false);
			}

			lockInfo::updateCubeFuncLock.store(false);
		});
	}

	//提交命令缓冲区重建任务，cmdWorker执行
	void submitRecreateCmd(Camera& Camera) {
		cmdWorker.submit([this, &Camera] {
			if (lockInfo::getLock(lockInfo::plateMeshDataLock)) {
				InitBuffer.recreateCmd(InitBuffer.cmd.thread.next.load(), InitPlate.isEntireRebuild, InitPlate.originPos, Camera, procPipeline);
				lockInfo::plateMeshDataLock.store(false);
			}

			lockInfo::recreateCmdFuncLock.store(false);
		});
	}
};