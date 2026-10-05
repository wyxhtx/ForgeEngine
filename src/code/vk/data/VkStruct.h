#pragma once



//engine    结构空间
namespace engineStruct {
	//instance	层结点
	struct instanceLayerNode {
		const char* name;
		bool        enable;
	};

	//instance	扩展结点
	struct instanceExtensionNode {
		const char* name;
		bool        enable;
		bool        layerSwitch;	//仅当 debugInfo::LayerSwitch 开启时才启用
	};

	//device	扩展结点
	struct deviceExtensionNode {
		const char* name;
		bool        enable;
	};



	struct tickNode {
		double time0 = glfwGetTime();
		double time1 = 0.0;
		double dt    = 0.0;
		double fps   = 0.0;



		void createTickNode(double tempFps) {
			fps = tempFps;
		}



		//是否更新
		bool isUpdate() {
			time1 = glfwGetTime();

			if ((dt = time1 - time0) > 1.0 / fps) {
				time0 = time1;

				return true;
			}

			return false;
		}
	};

	struct timeNode {
		std::chrono::high_resolution_clock::time_point start{};
		std::chrono::duration<double, std::milli> duration{};



		timeNode() {
			start = std::chrono::high_resolution_clock::now();
		}



		std::string end() {
			duration = std::chrono::high_resolution_clock::now() - start;
			return std::to_string(duration.count());
		}
	};
}

//device    结构空间
namespace deviceStruct {
	//队列族结点
	struct queueFamilyNode {
		uint32_t index = VK_QUEUE_FAMILY_IGNORED;               //队列族内队列索引，初始值为忽略
		uint32_t count = 0;                                     //队列族内队列数量，初始值为0
		float priority = 1.0f;                                  //队列族内队列优先级，暂时无用
	};

	//物理设备结点
	struct gpuNode {
		VkPhysicalDevice gpu;                                   //物理设备，一般是显卡
		deviceStruct::queueFamilyNode graphicsQueueFamily{};    //图形队列族
	};
}



//cmd       结构空间
namespace cmdStruct {
	//cmd	线程结点
	struct cmdThreadNode {
		std::mutex lock{};                  //线程锁：保护 flip/swap 的读改写
		std::atomic<int> current{ 0 };      //当前线程
		std::atomic<int> next{ 0 };         //下一线程



		//判断更新（无锁读，安全）
		bool isFlip() const {
			return next.load() != current.load();
		}

		//进行更新：next切到另一线程
		void flip() {
			std::lock_guard<std::mutex> guard(lock);
			next.store((current.load() + 1) % 2);
		}

		//完成更新：当前线程切到next
		void swap() {
			std::lock_guard<std::mutex> guard(lock);
			current.store(next.load());
		}
	};

	//cmd	配置结点
	struct cmdConfigNode {
		VkPipeline       pipe        = VK_NULL_HANDLE;
		VkPipelineLayout layout      = VK_NULL_HANDLE;
		VkDescriptorSet  desc        = VK_NULL_HANDLE;
		VkBuffer         vertBuffer  = VK_NULL_HANDLE;
		VkBuffer         indexBuffer = VK_NULL_HANDLE;



		//重置
		void reset() {
			pipe        = VK_NULL_HANDLE;
			layout      = VK_NULL_HANDLE;
			desc        = VK_NULL_HANDLE;
			vertBuffer  = VK_NULL_HANDLE;
			indexBuffer = VK_NULL_HANDLE;
		}
	};

	//cmd	录制结点
	struct cmdRecorderNode {
		VkCommandBuffer           cmd = VK_NULL_HANDLE;     //命令
		VkCommandBufferBeginInfo  cmdBegin{};               //命令开始
		cmdStruct::cmdConfigNode  cmdConfig{};              //命令配置

		VkViewport                viewport{};               //视口
		VkRect2D                  scissor{};                //裁剪

		std::vector<VkDeviceSize> vertOffset{ 0 };          //顶点偏移
		VkMemoryBarrier           memBarrier{};             //内存屏障



		cmdRecorderNode() {
			cmdBegin.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			cmdBegin.flags            = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
			cmdBegin.pInheritanceInfo = nullptr;

			viewport.x        = 0.0f;
			viewport.y        = 0.0f;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;
			scissor.offset = { 0, 0 };

			memBarrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
			memBarrier.pNext         = nullptr;
			memBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			memBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		}



		//绑定cmd
		void bind(const VkCommandBuffer& tCmd) {
			cmd = tCmd;
			cmdConfig.reset();
		}

		//重置cmd
		void reset() {
			if (vkResetCommandBuffer(cmd, 0) != VK_SUCCESS) {
				Log::printError(__FILE__, __LINE__, "cmd重置失败");
			}
		}

		//开始cmd
		void begin() {
			if (vkBeginCommandBuffer(cmd, &cmdBegin) != VK_SUCCESS) {
				Log::printError(__FILE__, __LINE__, "cmd开始失败");
			}
		}

		//结束cmd
		void end() {
			vertOffset = std::vector<VkDeviceSize>{ 0 };

			if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
				Log::printError(__FILE__, __LINE__, "cmd结束失败");
			}
		}

		//开始渲染
		void beginRender(const VkRenderPassBeginInfo* info) {
			cmdConfig.reset();
			vkCmdBeginRenderPass(cmd, info, VK_SUBPASS_CONTENTS_INLINE);
		}

		//结束渲染
		void endRender() {
			vkCmdEndRenderPass(cmd);
		}

		//绘制
		void draw(pipelineEnum::cmdDrawId::type cmdDrawType, const VkPipeline& tPipe, const VkPipelineLayout& tLayout, const VkDescriptorSet& tDesc, const VkBuffer& tVertBuffer = VK_NULL_HANDLE, const VkBuffer& tIndexBuffer = VK_NULL_HANDLE, uint32_t tCount = 0, const VkExtent2D& tExtent = {}) {
			switch (cmdDrawType) {
			case pipelineEnum::cmdDrawId::indexed: {
				bindPipe(tPipe);
				bindLayoutDesc(tLayout, tDesc);
				bindVert(tVertBuffer);
				bindIndex(tIndexBuffer);

				vkCmdDrawIndexed(cmd, tCount, 1, 0, 0, 0);
				break;
			}
			case pipelineEnum::cmdDrawId::fullscreen: {
				viewport.width  = static_cast<float>(tExtent.width);
				viewport.height = static_cast<float>(tExtent.height);
				scissor.extent  = tExtent;

				bindPipe(tPipe);
				bindLayoutDesc(tLayout, tDesc);

				vkCmdSetViewport(cmd, 0, 1, &viewport);
				vkCmdSetScissor(cmd, 0, 1, &scissor);
				vkCmdDraw(cmd, 3, 1, 0, 0);
				break;
			}
			case pipelineEnum::cmdDrawId::text: {
				bindPipe(tPipe);
				bindLayoutDesc(tLayout, tDesc);
				bindVert(tVertBuffer);

				vkCmdBindVertexBuffers(cmd, 1, 1, &tVertBuffer, vertOffset.data());
				vkCmdDraw(cmd, 6 * tCount, 1, 0, 0);
				break;
			}
			}
		}

		//管线屏障
		void barrier() {
			vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 1, &memBarrier, 0, nullptr, 0, nullptr);
		}

	private:
		//绑定pipe
		void bindPipe(const VkPipeline& tPipe) {
			if (cmdConfig.pipe != tPipe) {
				cmdConfig.pipe = tPipe;
				vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, cmdConfig.pipe);
			}
		}

		//绑定layout、desc（API 要求同时给）
		void bindLayoutDesc(const VkPipelineLayout& tLayout, const VkDescriptorSet& tDesc) {
			if (cmdConfig.layout != tLayout || cmdConfig.desc != tDesc) {
				cmdConfig.layout = tLayout;
				cmdConfig.desc   = tDesc;
				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, cmdConfig.layout, 0, 1, &cmdConfig.desc, 0, nullptr);
			}
		}

		//绑定vert
		void bindVert(const VkBuffer& tVertBuffer) {
			if (cmdConfig.vertBuffer != tVertBuffer) {
				cmdConfig.vertBuffer = tVertBuffer;
				vkCmdBindVertexBuffers(cmd, 0, 1, &cmdConfig.vertBuffer, vertOffset.data());
			}
		}

		//绑定index
		void bindIndex(const VkBuffer& tIndexBuffer) {
			if (cmdConfig.indexBuffer != tIndexBuffer) {
				cmdConfig.indexBuffer = tIndexBuffer;
				vkCmdBindIndexBuffer(cmd, cmdConfig.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
			}
		}
	};
	
	//cmd	缓冲结点
	struct cmdBufferNode {
		VkCommandPool                pool  = VK_NULL_HANDLE;
		VkFence                      fence = VK_NULL_HANDLE;    //本线程命令执行完成信号
		VkCommandBufferAllocateInfo  info{};
		std::vector<VkCommandBuffer> list{};



		//创建cmdPool、info、fence
		cmdBufferNode(const VkDevice& device, uint32_t queueFamily, uint32_t frameCount) {
			VkCommandPoolCreateInfo poolInfo{};
			poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			poolInfo.queueFamilyIndex = queueFamily;
	
			if (vkCreateCommandPool(device, &poolInfo, nullptr, &pool) != VK_SUCCESS) {
				Log::printError(__FILE__, __LINE__, "cmdPool创建失败");
			}
	
			info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			info.commandPool        = pool;
			info.commandBufferCount = frameCount;
	
			list.resize(frameCount);
			createCmdList(device);
	
			VkFenceCreateInfo fenceInfo{};
			fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	
			if (vkCreateFence(device, &fenceInfo, nullptr, &fence) != VK_SUCCESS) {
				Log::printError(__FILE__, __LINE__, "fence创建失败");
			}
		}

		cmdBufferNode() = delete;



		//创建cmdList
		void createCmdList(const VkDevice& device) {
			if (vkAllocateCommandBuffers(device, &info, list.data()) != VK_SUCCESS) {
				Log::printError(__FILE__, __LINE__, "cmdList创建失败");
			}
		}

		//重建cmdList
		void recreateCmdList(const VkDevice& device) {
			clearCmdList(device);
			createCmdList(device);
		}

		//清理cmdPool
		void clearCmdPool(const VkDevice& device) {
			vkDestroyFence(device, fence, nullptr);
			vkDestroyCommandPool(device, pool, nullptr);

			pool  = VK_NULL_HANDLE;
			fence = VK_NULL_HANDLE;
		}

		//清理cmdList
		void clearCmdList(const VkDevice& device) {
			vkFreeCommandBuffers(device, pool, static_cast<uint32_t>(list.size()), list.data());
		}

		//重载
		VkCommandBuffer& operator[](int frameId) { return list[frameId]; }

		size_t           size()            const { return list.size(); }
	};



	//cmd	结点
	struct cmdNode {
		cmdStruct::cmdThreadNode              thread{};         //线程管理器
		cmdStruct::cmdRecorderNode            recorder{};       //录制管理器

		std::vector<cmdStruct::cmdBufferNode> renderCmd{};      //渲染命令，双缓冲
		cmdStruct::cmdBufferNode              transientCmd;     //临时命令，单缓冲



		cmdNode(const VkDevice& device, uint32_t queueFamily, uint32_t frameCount) : renderCmd{ cmdStruct::cmdBufferNode(device, queueFamily, frameCount), cmdStruct::cmdBufferNode(device, queueFamily, frameCount) }, transientCmd{ device, queueFamily, 1 } {}

		cmdNode() = delete;



		void clear(const VkDevice& device) {
			for (auto& node : renderCmd) {
				node.clearCmdPool(device);
			}

			transientCmd.clearCmdPool(device);
		}



		//拷贝 buffer 到 buffer
		void copyBufferToBuffer(const VkDevice& device, const VkQueue& graphicsQueue, const VkBuffer& srcBuffer, const VkBuffer& dstBuffer, VkDeviceSize size) {
			recordTransientCmd();

			VkBufferCopy copyRegion{};
			copyRegion.srcOffset = 0;
			copyRegion.dstOffset = 0;
			copyRegion.size      = size;
			vkCmdCopyBuffer(transientCmd[0], srcBuffer, dstBuffer, 1, &copyRegion);

			submitTransientCmd(device, graphicsQueue);
		}

		//拷贝 buffer 到 image，转换 → 拷贝 → 转换
		void copyBufferToImage(const VkDevice& device, const VkQueue& graphicsQueue, const VkBuffer& buffer, const VkImage& image, const std::vector<VkBufferImageCopy>& regions, uint32_t mipLevel, uint32_t arrayLayer, VkFormat format) {
			recordTransientCmd();

			recordImageLayoutBarrier(image, mipLevel, arrayLayer, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			vkCmdCopyBufferToImage(transientCmd[0], buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(regions.size()), regions.data());
			recordImageLayoutBarrier(image, mipLevel, arrayLayer, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

			submitTransientCmd(device, graphicsQueue);
		}

		//录制 renderCmd 本线程命令
		template<typename F>
		void recordCmd(int threadId, int frameId, F&& func) {
			recorder.bind(renderCmd[threadId][frameId]);
			recorder.reset();
			recorder.begin();
			func(recorder);
			recorder.end();
		}

		//等待 renderCmd 本线程命令
		void waitCmd(const VkDevice& device, int threadId) {
			vkWaitForFences(device, 1, &renderCmd[threadId].fence, VK_TRUE, UINT64_MAX);
		}

	private:
		//录制图像布局转换屏障
		void recordImageLayoutBarrier(const VkImage& image, uint32_t mipLevel, uint32_t arrayLayer, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
			VkImageMemoryBarrier memBarrier{};
			memBarrier.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			memBarrier.oldLayout            = oldLayout;
			memBarrier.newLayout            = newLayout;
			memBarrier.srcQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
			memBarrier.dstQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
			memBarrier.image                = image;
			memBarrier.subresourceRange.baseMipLevel   = 0;
			memBarrier.subresourceRange.levelCount     = mipLevel;
			memBarrier.subresourceRange.baseArrayLayer = 0;
			memBarrier.subresourceRange.layerCount     = arrayLayer;

			if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
				memBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

				if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT) {
					memBarrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
				}
			}
			else {
				memBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			}

			VkPipelineStageFlags srcPipeStage{};
			VkPipelineStageFlags dstPipeStage{};
			if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
				memBarrier.srcAccessMask = 0;
				memBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				srcPipeStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				dstPipeStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			}
			else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
				memBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				memBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				srcPipeStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				dstPipeStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			}
			else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
				memBarrier.srcAccessMask = 0;
				memBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				srcPipeStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				dstPipeStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			}
			else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
				memBarrier.srcAccessMask = 0;
				memBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				srcPipeStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				dstPipeStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			}
			else if (debugInfo::LogSwitch) {
				Log::printError(__FILE__, __LINE__, "无支持的布局转换");
			}

			vkCmdPipelineBarrier(transientCmd[0], srcPipeStage, dstPipeStage, 0, 0, nullptr, 0, nullptr, 1, &memBarrier);
		}

		//录制 transientCmd
		void recordTransientCmd() {
			vkResetCommandBuffer(transientCmd[0], 0);

			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			if (vkBeginCommandBuffer(transientCmd[0], &beginInfo) != VK_SUCCESS) {
				Log::printError(__FILE__, __LINE__, "transientCmd开始失败");
			}
		}

		//提交 transientCmd
		void submitTransientCmd(const VkDevice& device, const VkQueue& graphicsQueue) {
			vkEndCommandBuffer(transientCmd[0]);

			VkSubmitInfo submitInfo{};
			submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers    = &transientCmd[0];

			{
				std::lock_guard<std::mutex> queueGuard(lockInfo::queueThreadLock);
				vkResetFences(device, 1, &transientCmd.fence);

				if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, transientCmd.fence) != VK_SUCCESS) {
					Log::printError(__FILE__, __LINE__, "transientCmd提交失败");
				}
			}

			vkWaitForFences(device, 1, &transientCmd.fence, VK_TRUE, UINT64_MAX);
			vkResetFences  (device, 1, &transientCmd.fence);
		}

	public:
		bool isFlip() const { return thread.isFlip(); }

		void swap()         { thread.swap(); }

		void flip()         { thread.flip(); }
	};
}

//uiStruct  结构空间
namespace uiStruct {
	//uiText布局行结点
	struct uiTextLayoutRowNode {
		uiEnum::uiTextAnchorEnum::type anchor{};    //锚点（块归属）
		std::string label{};                        //行标签（不含冒号，渲染自动补 ":"）
		bool        isVecRow = false;               //true=坐标三值行（x/y/z cell），false=单值行
		glm::dvec3  vecValue{};                     //坐标三值（ivec3 由重载内转 double）
		std::string value{};                        //单值文本（已格式化，仅 isVecRow=false 有效）
	};

	//uiText段结点（排版产出，x/y 为像素，y 为基线）
	struct uiTextSegmentNode {
		float       x;
		float       y;
		std::string text;
	};

	//uiText布局结点（每帧重建；行/段缓冲跨帧复用容量）
	struct uiTextLayoutNode {
		std::vector<uiTextLayoutRowNode> row{};       //本帧待排版的行（调用顺序即行序）
		std::vector<uiTextSegmentNode>   segment{};   //排版产出（容量跨帧保留，避免每帧分配）
		size_t prevHash = ~0ull;                      //上次顶点内容 hash（~0ull 保证首帧必写）
	};

	//uiText信息结点
	struct uiTextInfoNode {
		glm::dvec3 pos{};
		glm::dvec3 chunkPos{};
		glm::ivec3 minepos{};
	};



	//字形度量结点
	struct msdfGlyphNode {
		glm::vec4 planeBounds{};        //显示盒，文本行左上为原点，y 向下为正
		glm::vec4 uvBounds{};           //uv盒，含内边距
		float     advance = 0.0f;       //占位宽度
	};

	//字形度量图集结点
	struct msdfGlyphAtlasNode {
		//烘焙产物
		std::unordered_map<uint32_t, msdfGlyphNode> map;    //图集Map：码点 → 字形度量
		std::vector<uint8_t> data{};                        //图集数据（RGBA8；上传成功后可按需清空）
		float emSize = 32.0f;                               //图集尺度
		int   width  = 0;                                   //图集宽度
		int   height = 0;                                   //图集高度

		//烘焙参数（改这些等于改图集 ⇒ 缓存指纹随之变化，旧缓存自动失效）
		double cornerAngle = 3.0;                           //边缘着色夹角阈值（度），小于该值视为直角
		double pixelRange  = 2.0;                           //SDF 边缘带宽
		double miterLimit  = 1.0;                           //尖角斜接上限

		//HUD 默认参数
		float defaultFontSizePx      = 14.0f;               //默认屏幕字号
		float defaultLineHeightRatio = 1.3f;                //默认行距系数
		float defaultMarginPx        = 8.0f;                //默认页边距

		//暂存排版，每帧重建，段缓冲容量跨帧保留
		uiStruct::uiTextLayoutNode uiTextLayout;

		glm::vec4* vertCursor = nullptr;    //本帧顶点写游标，写入过程中前进
		uint32_t   glyphCount = 0;          //本帧已写入字形数



		msdfGlyphAtlasNode(const std::string& fontPath, const std::string& charsetPath) {
			//1、字集表：读原文（缺失/读空回退纯 ASCII），原文兼作缓存指纹
			const std::string charsetText = readCharsetText(charsetPath);

			//2、缓存指纹：字体 size/mtime + 字集表字节 + 烘焙参数（配魔数末位格式版本一起判缓存是否可用）
			const uint64_t fingerprint = createFingerprint(fontPath, charsetText);

			//3、命中缓存：直接读回像素与字形度量，连 FreeType 都不必初始化
			engineStruct::timeNode cacheTime;
			if (loadCache(fontPath, fingerprint)) {
				if (debugInfo::LogSwitch) { 
					Log::print("uiText 图集：" + getAtlasText() + "（缓存命中 " + cacheTime.end() + " ms）"); 
				}

				return;
			}

			//4、初始化 FreeType 库，该库的单例操作不能多线程
			msdfgen::FreetypeHandle* ft = msdfgen::initializeFreetype();
			if (!ft) {
				Log::printError(__FILE__, __LINE__, "freetype库初始化失败");
			}

			//5、加载字体
			msdfgen::FontHandle* font = msdfgen::loadFont(ft, fontPath.c_str());
			if (!font) {
				msdfgen::deinitializeFreetype(ft);
				Log::printError(__FILE__, __LINE__, "字体加载失败");
			}

			//6、取字符集：ASCII + 字集表（中文），生成字形几何列表
			engineStruct::timeNode bakeTime;
			std::vector<msdf_atlas::GlyphGeometry> glyphs;
			msdf_atlas::FontGeometry fontGeometry(&glyphs);
			fontGeometry.loadCharset(font, 1.0, createCharset(charsetText, charsetPath));

			//7、边缘着色：让直角/尖角保持锐利，不被圆角化
			//cornerAngle = 3.0°，夹角小于该值视为直角
			for (msdf_atlas::GlyphGeometry& glyph : glyphs) {
				glyph.edgeColoring(&msdfgen::edgeColoringInkTrap, cornerAngle, 0);
			}



			//8、打包：把全部字形塞进一张图集
			//POWER_OF_TWO_RECTANGLE：尺寸取 2 的幂矩形，便于采样/mip
			//setScale(emSize)         ：烘焙尺度，像素/em
			//setPixelRange(pixelRange)：SDF 边缘带宽 → 渲染端需用 pixelRange × scale
			//setMiterLimit(miterLimit)：尖角斜接上限
			msdf_atlas::TightAtlasPacker packer;
			packer.setDimensionsConstraint(msdf_atlas::DimensionsConstraint::POWER_OF_TWO_RECTANGLE);
			packer.setScale(emSize);
			packer.setPixelRange(pixelRange);
			packer.setMiterLimit(miterLimit);
			if (packer.pack(glyphs.data(), static_cast<int>(glyphs.size())) != 0) {
				msdfgen::destroyFont(font);
				msdfgen::deinitializeFreetype(ft);
				Log::printError(__FILE__, __LINE__, "msdf图集打包失败");
			}
			packer.getDimensions(width, height);

			//9、生成 MTSDF 位图，4 通道：RGB 三向距离 + A 真实距离，线程 4 只用于栅格化，与 FreeType 生命周期无关
			msdf_atlas::ImmediateAtlasGenerator<float, 4, msdf_atlas::mtsdfGenerator, msdf_atlas::BitmapAtlasStorage<unsigned char, 4>> generator(width, height);
			msdf_atlas::GeneratorAttributes attributes;
			generator.setAttributes(attributes);
			generator.setThreadCount(4);
			generator.generate(glyphs.data(), static_cast<int>(glyphs.size()));

			//10、取位图只读视图，逐行翻转拷贝成RGBA8，因为原点在左下，所以下面翻成左上，原因：FreeType/msdfgen 位图原点在左下，Vulkan 纹理上传按左上
			msdfgen::BitmapConstRef<unsigned char, 4> bitmap = generator.atlasStorage();
			data.resize(static_cast<size_t>(width) * height * 4);
			for (int rowDown = 0; rowDown < height; ++rowDown) {
				const int rowSource = height - 1 - rowDown;

				for (int column = 0; column < width; ++column) {
					const unsigned char* pixel = bitmap(column, rowSource);
					size_t offset = (static_cast<size_t>(rowDown) * width + column) * 4;

					data[offset] = pixel[0];
					data[offset + 1] = pixel[1];
					data[offset + 2] = pixel[2];
					data[offset + 3] = pixel[3];
				}
			}

			//11、逐字形填度量表
			//planeBounds：平面边界（em 单位）→ × emSize 得像素；y 取负 = 屏幕 y 向下
			//uvBounds   ：图集 UV，按像素归一化；y 用 (高 - top) 做上下翻转
			//advance    ：步进（em → 像素）
			for (const msdf_atlas::GlyphGeometry& glyph : glyphs) {
				const uint32_t codepoint = glyph.getCodepoint();
				if (!codepoint) { continue; }                 // 跳过 .notdef（码点 0）

				double left = 0, bottom = 0, right = 0, top = 0;
				glyph.getQuadPlaneBounds(left, bottom, right, top);

				double atlasLeft = 0, atlasBottom = 0, atlasRight = 0, atlasTop = 0;
				glyph.getQuadAtlasBounds(atlasLeft, atlasBottom, atlasRight, atlasTop);

				msdfGlyphNode node;
				node.planeBounds.x = static_cast<float>(left)      * emSize;
				node.planeBounds.z = static_cast<float>(right)     * emSize;
				node.planeBounds.y = static_cast<float>(-top)      * emSize;
				node.planeBounds.w = static_cast<float>(-bottom)   * emSize;

				node.uvBounds.x = static_cast<float>(atlasLeft)  / static_cast<float>(width);
				node.uvBounds.z = static_cast<float>(atlasRight) / static_cast<float>(width);
				node.uvBounds.y = static_cast<float>(height - atlasTop   ) / static_cast<float>(height);
				node.uvBounds.w = static_cast<float>(height - atlasBottom) / static_cast<float>(height);

				node.advance = static_cast<float>(glyph.getAdvance()) * emSize;

				map[codepoint] = node;
			}

			//清理
			msdfgen::destroyFont(font);
			msdfgen::deinitializeFreetype(ft);

			//12、落盘缓存：下次启动直接命中
			const std::string bakeMs  = bakeTime.end();
			const bool        isSaved = saveCache(fontPath, fingerprint);
			if (debugInfo::LogSwitch) { 
				Log::print("uiText 图集：" + getAtlasText() + "（烘焙 " + bakeMs + " ms，" + (isSaved ? "已写缓存" : "缓存写入失败") + "）"); 
			}
		}

		msdfGlyphAtlasNode() = delete;



		//开始排版
		void begin() {
			uiTextLayout.row.clear();
		}

		//添加 vec3，ivec3型
		void addVec3(uiEnum::uiTextAnchorEnum::type anchor, const std::string& label, const glm::ivec3& valueVec) {
			addVec3(anchor, label, glm::dvec3{ static_cast<double>(valueVec.x), static_cast<double>(valueVec.y), static_cast<double>(valueVec.z) });
		}

		//添加 vec3，dvec3型
		void addVec3(uiEnum::uiTextAnchorEnum::type anchor, const std::string& label, const glm::dvec3& valueVec) {
			uiTextLayoutRowNode row;
			row.anchor   = anchor;
			row.label    = label;
			row.isVecRow = true;
			row.vecValue = valueVec;
			uiTextLayout.row.push_back(row);
		}

		//添加 单值
		void addValue(uiEnum::uiTextAnchorEnum::type anchor, const std::string& label, const std::string& value, size_t width) {
			uiTextLayoutRowNode row;
			row.anchor   = anchor;
			row.label    = label;
			row.isVecRow = false;
			row.value    = padFixedWidth(value, width);     //定长width
			uiTextLayout.row.push_back(row);
		}

		//构建排版
		void build(const VmaAllocation& bufferMem, const VmaAllocator& memAllocator) {
			std::vector<uiTextSegmentNode>& outSegments = uiTextLayout.segment;
			outSegments.clear();

			const float viewportWidthPx = static_cast<float>(windowInfo::width);
			const float fontSizePx      = defaultFontSizePx;
			const float lineHeight      = getLineHeight();
			const float marginPx        = defaultMarginPx;

			//让 x/y/z 三格横向等宽、纵向逐行下移
			const float tagPx     = std::max(getStringWidthPx("x:"), std::max(getStringWidthPx("y:"), getStringWidthPx("z:")));
			const float cellPx    = tagPx + getStringWidthPx("-9,999,999");
			const float spacePx   = getCharWidthPx(static_cast<uint32_t>(' '));
			const float baseline0 = marginPx + fontSizePx;
			float       topLeftY  = baseline0;
			float       topRightY = baseline0;



			//第一遍：仅 topLeft 块需块级值列起点，块内最大标签宽
			float maxLabelWidthPx = 0.0f;
			for (const uiTextLayoutRowNode& row : uiTextLayout.row) {
				if (row.anchor == uiEnum::uiTextAnchorEnum::topLeft) {
					maxLabelWidthPx = std::max(maxLabelWidthPx, getStringWidthPx(row.label + ": "));
				}
			}

			const float colLabelX   = marginPx;
			const float valueStartX = colLabelX + maxLabelWidthPx;

			//产出坐标行 x:/y:/z:
			auto emitPosCells = [&](float zoneX, float rowY, const uiTextLayoutRowNode& row) {
				const double values[3] = { row.vecValue.x, row.vecValue.y, row.vecValue.z };

				for (int k = 0; k < 3; ++k) {
					const float cellX = zoneX + static_cast<float>(k) * cellPx;
					outSegments.push_back({ cellX, rowY, std::string(1, "xyz"[k]) + ":" });
					outSegments.push_back({ cellX + tagPx, rowY, padFixedWidth(formatThousand(values[k]), 10) });
				}
			};



			//第二遍：逐行定 x/y，锚点内调用顺序即行序
			for (const uiTextLayoutRowNode& row : uiTextLayout.row) {
				if (row.anchor == uiEnum::uiTextAnchorEnum::topLeft) {
					const float rowY = topLeftY;
					topLeftY += lineHeight;

					//标签列左端齐 margin，值区左端齐 valueStartX，跨行列对齐
					outSegments.push_back({ colLabelX, rowY, row.label + ":" });

					if (row.isVecRow) {
						emitPosCells(valueStartX, rowY, row);
					}
					else {
						outSegments.push_back({ valueStartX, rowY, row.value });
					}
				}
				else if (row.anchor == uiEnum::uiTextAnchorEnum::topRight) {
					const float rowY = topRightY;
					topRightY += lineHeight;

					//整行内容右端齐视口右缘，标签与值区粘合排布
					const float labelWidthPx = getStringWidthPx(row.label + ":");
					const float zoneWidthPx  = row.isVecRow ? 3.0f * cellPx : getStringWidthPx(row.value);
					const float rowWidthPx   = labelWidthPx + spacePx + zoneWidthPx;
					const float rowX         = viewportWidthPx - marginPx - rowWidthPx;
					const float zoneX        = rowX + labelWidthPx + spacePx;

					outSegments.push_back({ rowX, rowY, row.label + ":" });

					if (row.isVecRow) {
						emitPosCells(zoneX, rowY, row);
					}
					else {
						outSegments.push_back({ zoneX, rowY, row.value });
					}
				}
			}

			//内容 hash：与上次比对，判定是否需要重写顶点
			size_t hash = 0;
			for (const auto& segment : outSegments) {
				hash ^= std::hash<std::string>{}(segment.text);
				hash ^= static_cast<size_t>(segment.x * 4.0f) + 0x9e3779b9u + (hash << 6u) + (hash >> 2u);
			}

			if (hash != uiTextLayout.prevHash) {
				vmaMapMemory(memAllocator, bufferMem, reinterpret_cast<void**>(&vertCursor));
				glyphCount = 0;

				for (const auto& segment : uiTextLayout.segment) {
					setText(vertCursor, glyphCount, segment.text, segment.x, segment.y);
				}

				vmaUnmapMemory(memAllocator, bufferMem);
				vertCursor = nullptr;

				//字形数达顶点缓冲上限：已截断，此处统一告警一次
				if (glyphCount >= pipelineInfo::uiTextGlyphCount) {
					Log::printWarning(__FILE__, __LINE__, "uiText 字形数达顶点缓冲上限，已截断");
				}

				uiTextLayout.prevHash = hash;
			}
		}

	private:
		//读取 字集表原文（缺失时告警并返回空串 ⇒ 调用方回退纯 ASCII）
		std::string readCharsetText(const std::string& charsetPath) const {
			std::ifstream file(charsetPath);
			if (!file.is_open()) {
				Log::printWarning(__FILE__, __LINE__, "字集表打开失败，回退 ASCII：" + charsetPath);
				return {};
			}

			std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
			file.close();

			return text;
		}

		//创建 缓存指纹：字体 size/mtime + 字集表字节 + 烘焙参数（FNV-1a 64，逐字节确定性）
		uint64_t createFingerprint(const std::string& fontPath, const std::string& charsetText) const {
			uint64_t hash = 0xcbf29ce484222325ull;

			auto mix = [&hash](const void* bytes, const size_t size) {
				const uint8_t* pointer = static_cast<const uint8_t*>(bytes);

				for (size_t i = 0; i < size; ++i) {
					hash ^= pointer[i];
					hash *= 0x100000001b3ull;
				}
			};

			std::error_code errorCode{};
			const uintmax_t fontSize = std::filesystem::file_size(fontPath, errorCode);
			const int64_t   fontTime = static_cast<int64_t>(std::filesystem::last_write_time(fontPath, errorCode).time_since_epoch().count());

			mix(&fontSize, sizeof(fontSize));
			mix(&fontTime, sizeof(fontTime));
			mix(charsetText.data(), charsetText.size());
			mix(&emSize, sizeof(emSize));
			mix(&cornerAngle, sizeof(cornerAngle));
			mix(&pixelRange, sizeof(pixelRange));
			mix(&miterLimit, sizeof(miterLimit));

			return hash;
		}

		//读取 图集缓存（魔数/指纹不符或内容不完整一律当未命中；失败不抛，交给调用方烘焙）
		bool loadCache(const std::string& fontPath, const uint64_t fingerprint) {
			if (!engineInfo::msdfCacheSwitch) { return false; }

			const std::string cacheFile = getCacheFile(fontPath);
			std::ifstream file(cacheFile, std::ios::binary);
			if (!file.is_open()) { return false; }

			char     magic[8]{};
			uint64_t fileFingerprint = 0;
			float    fileEmSize      = 0.0f;
			uint32_t fileWidth       = 0;
			uint32_t fileHeight      = 0;
			uint32_t fileGlyphCount  = 0;
			uint32_t filePixelBytes  = 0;

			file.read(magic, 8);
			readValue(file, fileFingerprint);
			readValue(file, fileEmSize);
			readValue(file, fileWidth);
			readValue(file, fileHeight);
			readValue(file, fileGlyphCount);
			readValue(file, filePixelBytes);

			//魔数（含格式版本）/指纹/尺寸自检
			const bool isValid = file && std::memcmp(magic, getCacheMagic(), 8) == 0 && fileFingerprint == fingerprint && fileEmSize == emSize && fileWidth && fileHeight && fileGlyphCount && filePixelBytes == fileWidth * fileHeight * 4;
			if (!isValid) {
				if (debugInfo::LogSwitch) { 
					Log::print("uiText 图集缓存已失效，重新烘焙：" + cacheFile);
				}
				return false;
			}

			std::unordered_map<uint32_t, msdfGlyphNode> fileMap;
			std::vector<uint8_t>                        fileData(filePixelBytes);

			for (uint32_t i = 0; i < fileGlyphCount; ++i) {
				uint32_t  codepoint = 0;
				glm::vec4 planeBounds{};
				glm::vec4 uvBounds{};
				float     advance = 0.0f;

				readValue(file, codepoint);
				readValue(file, planeBounds);
				readValue(file, uvBounds);
				readValue(file, advance);
				if (!file) {
					Log::printWarning(__FILE__, __LINE__, "图集缓存内容不完整，重新烘焙：" + cacheFile);
					return false;
				}

				msdfGlyphNode node;
				node.planeBounds   = planeBounds;
				node.uvBounds      = uvBounds;
				node.advance       = advance;
				fileMap[codepoint] = node;
			}

			file.read(reinterpret_cast<char*>(fileData.data()), static_cast<std::streamsize>(filePixelBytes));
			if (!file || file.gcount() != static_cast<std::streamsize>(filePixelBytes)) {
				Log::printWarning(__FILE__, __LINE__, "图集缓存像素不完整，重新烘焙：" + cacheFile);
				return false;
			}
			file.close();

			emSize = fileEmSize;
			width  = static_cast<int>(fileWidth);
			height = static_cast<int>(fileHeight);
			map    = std::move(fileMap);
			data   = std::move(fileData);

			return true;
		}

		//写入 图集缓存（先写临时件再改名，避免半截缓存；失败只告警、不影响本次运行）
		bool saveCache(const std::string& fontPath, const uint64_t fingerprint) const {
			if (!engineInfo::msdfCacheSwitch) { return false; }

			const std::string cacheFile = getCacheFile(fontPath);
			const std::string tempFile  = cacheFile + ".tmp";

			std::error_code errorCode{};
			std::filesystem::create_directories(fileInfo::path::msdfCachePath, errorCode);

			std::ofstream file(tempFile, std::ios::binary | std::ios::trunc);
			if (!file.is_open()) {
				Log::printWarning(__FILE__, __LINE__, "图集缓存打开失败（不影响本次运行）：" + tempFile);
				return false;
			}

			file.write(getCacheMagic(), 8);
			writeValue(file, fingerprint);
			writeValue(file, emSize);
			writeValue(file, static_cast<uint32_t>(width));
			writeValue(file, static_cast<uint32_t>(height));
			writeValue(file, static_cast<uint32_t>(map.size()));
			writeValue(file, static_cast<uint32_t>(data.size()));

			for (const auto& [codepoint, glyph] : map) {
				writeValue(file, codepoint);
				writeValue(file, glyph.planeBounds);
				writeValue(file, glyph.uvBounds);
				writeValue(file, glyph.advance);
			}

			file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
			const bool isWritten = static_cast<bool>(file);
			file.close();

			if (!isWritten) {
				Log::printWarning(__FILE__, __LINE__, "图集缓存写入失败（不影响本次运行）：" + tempFile);
				std::filesystem::remove(tempFile, errorCode);
				return false;
			}

			std::filesystem::remove(cacheFile, errorCode);          //Windows 改名不覆盖已存在目标，先删旧件
			std::filesystem::rename(tempFile, cacheFile, errorCode);
			if (errorCode) {
				Log::printWarning(__FILE__, __LINE__, "图集缓存改名失败（不影响本次运行）：" + cacheFile);
				return false;
			}

			return true;
		}

		//获取 图集描述文本（日志用）
		std::string getAtlasText() const {
			return std::to_string(width) + "×" + std::to_string(height) + " RGBA8，" + std::to_string(static_cast<size_t>(width) * height * 4 / 1024 / 1024) + " MB";
		}

		//创建 图集字集：ASCII + 字集表（每行汉字，读取跳过空白与控制字符）
		msdf_atlas::Charset createCharset(const std::string& charsetText, const std::string& charsetPath) const {
			msdf_atlas::Charset charset = msdf_atlas::Charset::ASCII;

			const size_t beforeCount = charset.size();
			for (uint32_t codepoint : utf8ToCodepoints(charsetText)) {
				if (codepoint < 0x21) { continue; }

				charset.add(codepoint);
			}

			const size_t addCount = charset.size() - beforeCount;
			if (!addCount) {
				Log::printWarning(__FILE__, __LINE__, "字集表无有效字符：" + charsetPath);
			}
			else {
				Log::print("uiText 字集：" + std::to_string(addCount) + " 字");
			}

			return charset;
		}

		//固定宽度，填充空格
		std::string padFixedWidth(const std::string& text, size_t width) {
			std::string result = text;

			if (result.size() < width) {
				result.append(width - result.size(), ' ');
			}

			return result;
		}

		//获取 lineHeight
		float getLineHeight() const {
			return defaultFontSizePx * defaultLineHeightRatio;
		}

		//获取 字符串宽度像素，默认字号
		float getStringWidthPx(const std::string& text) const {
			return getStringWidthPx(text, defaultFontSizePx);
		}

		//获取 字符串宽度像素，指定字号
		float getStringWidthPx(const std::string& text, float fontSizePx) const {
			const float scale = fontSizePx / emSize;
			float       width = 0.0f;

			for (uint32_t codepoint : utf8ToCodepoints(text)) {
				const msdfGlyphNode* glyph = getGlyph(codepoint);

				if (glyph) {
					width += glyph->advance * scale;
				}
			}

			return width;
		}

		//获取 单字符宽度像素，默认字号
		float getCharWidthPx(uint32_t codepoint) const {
			return getCharWidthPx(codepoint, defaultFontSizePx);
		}

		//获取 单字符宽度像素，指定字号
		float getCharWidthPx(uint32_t codepoint, float fontSizePx) const {
			const msdfGlyphNode* glyph = getGlyph(codepoint);

			return glyph ? glyph->advance * (fontSizePx / emSize) : 0.0f;
		}

		//整数千分位，保留负号
		std::string formatThousand(double value) {
			long long  number   = static_cast<long long>(value);
			const bool negative = number < 0;
			unsigned long long absolute = negative ? static_cast<unsigned long long>(-(number + 1)) + 1ull : static_cast<unsigned long long>(number);

			std::string digits = std::to_string(absolute);
			std::string output;
			int groupCount = 0;

			for (int i = static_cast<int>(digits.size()) - 1; i >= 0; --i) {
				output.insert(output.begin(), digits[i]);

				if (++groupCount == 3 && i > 0) {
					output.insert(output.begin(), ',');
					groupCount = 0;
				}
			}

			if (negative) {
				output.insert(output.begin(), '-');
			}

			return output;
		}

		//设置 文本，默认布局
		void setText(glm::vec4*& mapped, uint32_t& letterCount, const std::string& text, float positionX, float positionY) const {
			setText(mapped, letterCount, text, positionX, positionY, bufferEnum::textAlignId::left, defaultFontSizePx);
		}

		//设置 文本，指定布局
		void setText(glm::vec4*& mapped, uint32_t& letterCount, const std::string& text, float positionX, float positionY, bufferEnum::textAlignId::type alignType, float fontSizePx) const {
			if (map.empty() || !mapped) {
				return;
			}

			const float scale   = fontSizePx / emSize;
			float       originX = positionX;

			//对齐
			switch (alignType) {
			case bufferEnum::textAlignId::right:
				originX -= getStringWidthPx(text, fontSizePx);
				break;
			case bufferEnum::textAlignId::center:
				originX -= getStringWidthPx(text, fontSizePx) * 0.5f;
				break;
			case bufferEnum::textAlignId::left:
				break;
			}

			//逐字几何
			float cursorX = originX;
			for (uint32_t codepoint : utf8ToCodepoints(text)) {
				const msdfGlyphNode* glyph = getGlyph(codepoint);

				if (!glyph) {
					continue;
				}

				//顶点缓冲上限：字形数写满即停（越界会写坏显存），由 build 统一告警
				if (letterCount >= pipelineInfo::uiTextGlyphCount) {
					break;
				}

				setGlyph(mapped, letterCount, *glyph, cursorX, positionY, scale);
			}
		}



		//获取 缓存文件路径：storage/local/cache/msdf/<字体名>.msdf.bin
		std::string getCacheFile(const std::string& fontPath) const {
			return fileInfo::path::msdfCachePath + std::filesystem::path(fontPath).stem().string() + ".msdf.bin";
		}

		//写入 缓存字段（POD 原样落盘，同机同版本读回）
		void writeValue(std::ofstream& file, const auto& value) const {
			file.write(reinterpret_cast<const char*>(&value), sizeof(value));
		}

		//读取 缓存字段
		void readValue(std::ifstream& file, auto& value) const {
			file.read(reinterpret_cast<char*>(&value), sizeof(value));
		}

		//获取 缓存魔数（末位 = 缓存格式版本：改磁盘布局就 +1，旧缓存自动失效）
		const char* getCacheMagic() const {
			return "FEATLAS1";
		}

		//UTF-8 → 码点流
		std::vector<uint32_t> utf8ToCodepoints(const std::string& text) const {
			std::vector<uint32_t> codepoints;
			size_t index = 0;

			while (index < text.size()) {
				uint8_t  byte      = static_cast<uint8_t>(text[index]);
				uint32_t codepoint = 0;
				size_t   length    = 0;

				if (byte < 0x80) {
					codepoint = byte;
					length    = 1;
				}
				else if ((byte >> 5) == 0x6) {
					codepoint = byte & 0x1F;
					length    = 2;
				}
				else if ((byte >> 4) == 0xE) {
					codepoint = byte & 0x0F;
					length    = 3;
				}
				else if ((byte >> 3) == 0x1E) {
					codepoint = byte & 0x07;
					length    = 4;
				}
				else {
					++index;
					continue;
				}

				if (index + length > text.size()) {
					break;
				}

				bool valid = true;
				for (size_t k = 1; k < length; ++k) {
					uint8_t continuation = static_cast<uint8_t>(text[index + k]);

					if ((continuation & 0xC0) != 0x80) {
						valid = false;
						break;
					}

					codepoint = (codepoint << 6) | (continuation & 0x3F);
				}

				if (!valid) {
					++index;
					continue;
				}

				codepoints.push_back(codepoint);
				index += length;
			}

			return codepoints;
		}

		//获取 msdfGlyphNode
		const msdfGlyphNode* getGlyph(uint32_t codepoint) const {
			const auto it = map.find(codepoint);

			return (it == map.end()) ? nullptr : &it->second;
		}

		//设置 msdfGlyphNode，单个字形 → 顶点，写六顶点（两三角形）、推进光标、计数
		void setGlyph(glm::vec4*& mapped, uint32_t& letterCount, const msdfGlyphNode& glyph, float& cursorX, float baselineY, float scale) const {
			//像素 → NDC（左上(-1,-1)，y 向下为正）
			const float ndcX   = 2.0f / windowInfo::width;
			const float ndcY   = 2.0f / windowInfo::height;
			const float left   = (cursorX   + glyph.planeBounds.x * scale) * ndcX - 1.0f;
			const float right  = (cursorX   + glyph.planeBounds.z * scale) * ndcX - 1.0f;
			const float top    = (baselineY + glyph.planeBounds.y * scale) * ndcY - 1.0f;
			const float bottom = (baselineY + glyph.planeBounds.w * scale) * ndcY - 1.0f;
			const float u0     = glyph.uvBounds.x;
			const float v0     = glyph.uvBounds.y;
			const float u1     = glyph.uvBounds.z;
			const float v1     = glyph.uvBounds.w;

			//两三角形（绕序一致，配合 fill_back_cw）：左上→右上→左下、左下→右上→右下
			mapped->x = left;  mapped->y = top;    mapped->z = u0; mapped->w = v0; mapped++;
			mapped->x = right; mapped->y = top;    mapped->z = u1; mapped->w = v0; mapped++;
			mapped->x = left;  mapped->y = bottom; mapped->z = u0; mapped->w = v1; mapped++;
			mapped->x = left;  mapped->y = bottom; mapped->z = u0; mapped->w = v1; mapped++;
			mapped->x = right; mapped->y = top;    mapped->z = u1; mapped->w = v0; mapped++;
			mapped->x = right; mapped->y = bottom; mapped->z = u1; mapped->w = v1; mapped++;

			cursorX += glyph.advance * scale;
			letterCount++;
		}
	};
}



//buffer    结构空间
namespace bufferStruct {
	//baseBuffer		结点
	struct baseBufferNode {
		VkBuffer      buffer{};
		VmaAllocation bufferMem{};



		//创建 cpuBuffer
		baseBufferNode(const void* bufferData, VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags memProperty, const VmaAllocator& memAllocator) {
			createBuffer(bufferSize, bufferUsage, memProperty, memAllocator);
			createMemMap(bufferData, bufferSize, memAllocator);
		}

		//创建 buffer
		baseBufferNode(VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags memProperty, const VmaAllocator& memAllocator) {
			createBuffer(bufferSize, bufferUsage, memProperty, memAllocator);
		}

		//创建 uniformBuffer
		baseBufferNode(VkDeviceSize bufferSize, const VmaAllocator& memAllocator) {
			createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, memAllocator);
		}

		//todo
		//baseBufferNode() = delete;
		baseBufferNode() {}



		//写入缓冲数据
		void createMemMap(const void* bufferData, VkDeviceSize bufferSize, const VmaAllocator& memAllocator) {
			void* memMap{};

			vmaMapMemory(memAllocator, bufferMem, &memMap);
			memcpy(memMap, bufferData, bufferSize);
			vmaUnmapMemory(memAllocator, bufferMem);
		}

		//清理buffer
		void clear(const VmaAllocator& memAllocator) {
			vmaDestroyBuffer(memAllocator, buffer, bufferMem);
		}

	private:
		//创建buffer
		void createBuffer(VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags memProperty, const VmaAllocator& memAllocator) {
			VkBufferCreateInfo bufferInfo{};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size  = bufferSize;
			bufferInfo.usage = bufferUsage;

			VmaAllocationCreateInfo memInfo{};
			memInfo.usage         = VMA_MEMORY_USAGE_UNKNOWN;       //使用 VMA_MEMORY_USAGE_UNKNOWN 将内存属性交由属性参数控制
			memInfo.requiredFlags = memProperty;		            //设置所需的内存属性

			vmaCreateBuffer(memAllocator, &bufferInfo, &memInfo, &buffer, &bufferMem, nullptr);
		}
	};

	//meshBuffer		结点
	struct meshBufferNode {
		baseBufferNode vertex{};        //顶点缓冲
		baseBufferNode index{};         //索引缓冲



		//创建网格缓冲：顶点容器 + 索引表 → 暂存 cpuBuffer → deviceLocal
		void create(const auto& vertexNode, const std::vector<uint32_t>& indexList, cmdStruct::cmdNode& cmd, const VkDevice& device, const VkQueue& graphicsQueue, const VmaAllocator& memAllocator) {
			VkDeviceSize vertexSize = sizeof(vertexNode[0]) * vertexNode.size();
			VkDeviceSize indexSize  = sizeof(indexList[0])  * indexList.size();

			baseBufferNode cpuVertex{ vertexNode.data(), vertexSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, memAllocator };
			baseBufferNode cpuIndex { indexList.data(),  indexSize,  VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, memAllocator };

			vertex = baseBufferNode{ vertexSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memAllocator };
			index  = baseBufferNode{ indexSize,  VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memAllocator };

			cmd.copyBufferToBuffer(device, graphicsQueue, cpuVertex.buffer, vertex.buffer, vertexSize);
			cmd.copyBufferToBuffer(device, graphicsQueue, cpuIndex.buffer,  index.buffer,  indexSize);

			cpuVertex.clear(memAllocator);
			cpuIndex.clear(memAllocator);
		}

		//创建仅顶点网格缓冲（阴影等，索引复用源网格）
		void create(const auto& vertexNode, cmdStruct::cmdNode& cmd, const VkDevice& device, const VkQueue& graphicsQueue, const VmaAllocator& memAllocator) {
			VkDeviceSize vertexSize = sizeof(vertexNode[0]) * vertexNode.size();

			baseBufferNode cpuVertex{ vertexNode.data(), vertexSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, memAllocator };

			vertex = baseBufferNode{ vertexSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memAllocator };

			cmd.copyBufferToBuffer(device, graphicsQueue, cpuVertex.buffer, vertex.buffer, vertexSize);

			cpuVertex.clear(memAllocator);
		}

		//清理网格缓冲
		void clear(const VmaAllocator& memAllocator) {
			vertex.clear(memAllocator);
			index.clear(memAllocator);
		}
	};

	//descBuffer		结点
	struct descBufferNode {
		VkDeviceSize              setSize{};			//每 set 字节数，布局反射得出
		std::vector<VkDeviceSize> bindingOffset{};		//各 binding 在 set 内偏移
		std::vector<VkDeviceSize> bindingStride{};		//各 binding 数组步长
		baseBufferNode            descGpuBuffer{};		//GPU 缓冲
		baseBufferNode            descCpuBuffer{};		//CPU 缓冲
		VkDeviceAddress           descAddress{};		//GPU 地址

		//函数指针
		PFN_vkGetDescriptorSetLayoutSizeEXT          pfnGetSetLayoutSize{};
		PFN_vkGetDescriptorSetLayoutBindingOffsetEXT pfnGetBindingOffset{};
		PFN_vkGetDescriptorEXT                       pfnGetDescriptor{};



		//创建 descGpuBuffer、descCpuBuffer
		void createBuffer(const VkDevice& tDevice, const VmaAllocator& tAllocator, uint32_t tMaxSet) {
			const VkDeviceSize size = tMaxSet * setSize;

			descGpuBuffer = baseBufferNode{ size, VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, tAllocator };
			descCpuBuffer = baseBufferNode{ size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, tAllocator };

			VkBufferDeviceAddressInfo addrInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = descGpuBuffer.buffer };
			descAddress = vkGetBufferDeviceAddress(tDevice, &addrInfo);
		}

		//上传 CPU 缓冲 -> GPU 缓冲
		void upload(cmdStruct::cmdNode& tCmd, const VkDevice& tDevice, const VkQueue& tQueue, uint32_t tMaxSet) {
			tCmd.copyBufferToBuffer(tDevice, tQueue, descCpuBuffer.buffer, descGpuBuffer.buffer, tMaxSet * setSize);
		}
	};



	//imageBuffer		结点
	struct imageBufferNode {
		VkImage       image{};          //在renderEnum::renderFrameId::swap_rf类型下无用
		VmaAllocation imageMem{};
		VkImageView   view{};



		imageBufferNode(VkImageType imageType, VkFormat format, const VkExtent2D& imageExtent, uint32_t mipLevel, uint32_t arrayLayer, VkSampleCountFlagBits msaa, VkImageUsageFlags usage, VkImageAspectFlags aspectFlags, const VkDevice& device, const VmaAllocator& memAllocator) {
			create(imageType, format, imageExtent, mipLevel, arrayLayer, msaa, usage, aspectFlags, device, memAllocator);
		}

		imageBufferNode() {}



		void create(VkImageType imageType, VkFormat format, const VkExtent2D& imageExtent, uint32_t mipLevel, uint32_t arrayLayer, VkSampleCountFlagBits msaa, VkImageUsageFlags usage, VkImageAspectFlags aspectFlags, const VkDevice& device, const VmaAllocator& memAllocator) {
			VkImageCreateInfo imageInfo{};
			imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageInfo.imageType     = imageType;
			imageInfo.format        = format;
			imageInfo.extent.width  = imageExtent.width;
			imageInfo.extent.height = imageExtent.height;
			imageInfo.extent.depth  = 1;
			imageInfo.mipLevels     = mipLevel;
			imageInfo.arrayLayers   = arrayLayer;
			imageInfo.samples       = msaa;
			imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
			imageInfo.usage         = usage;
			imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

			VmaAllocationCreateInfo allocInfo{};
			allocInfo.usage         = VMA_MEMORY_USAGE_GPU_ONLY;
			allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

			if (vmaCreateImage(memAllocator, &imageInfo, &allocInfo, &image, &imageMem, nullptr) != VK_SUCCESS) {
				Log::printError(__FILE__, __LINE__, "image创建失败");
			}

			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image    = image;
			viewInfo.viewType = (arrayLayer > 1) ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format   = format;
			viewInfo.subresourceRange.aspectMask     = aspectFlags;
			viewInfo.subresourceRange.baseMipLevel   = 0;
			viewInfo.subresourceRange.levelCount     = mipLevel;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount     = arrayLayer;

			if (vkCreateImageView(device, &viewInfo, nullptr, &view) != VK_SUCCESS) {
				Log::printError(__FILE__, __LINE__, "view创建失败");
			}
		}

		void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
			vkDestroyImageView(device, view, nullptr);
			vmaDestroyImage(memAllocator, image, imageMem);
		}



		//用于重建swap时清理view
		void clearView(const VkDevice& device) {
			vkDestroyImageView(device, view, nullptr);
		}
	};

	//imageBufferList	结点
	struct imageBufferListNode {
		std::vector<bufferStruct::imageBufferNode> imageList{};



		//创建swap图像列表
		void createSwapImageList(const std::vector<VkImage>& swapImageList, VkFormat format, VkImageAspectFlags aspectFlags, const VkDevice& device) {
			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format   = format;
			viewInfo.subresourceRange.aspectMask     = aspectFlags;
			viewInfo.subresourceRange.baseMipLevel   = 0;
			viewInfo.subresourceRange.levelCount     = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount     = 1;

			imageList.clear();
			imageList.resize(swapImageList.size());

			for (int i = 0; i < swapImageList.size(); i++) {
				viewInfo.image = swapImageList[i];

				if (vkCreateImageView(device, &viewInfo, nullptr, &imageList[i].view) != VK_SUCCESS) {
					Log::printError(__FILE__, __LINE__, "view创建失败");
				}
			}
		}

		void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
			for (bufferStruct::imageBufferNode& node : imageList) {
				node.clear(device, memAllocator);
			}
		}

		//用于重建swap时清理view列表
		void clearView(const VkDevice& device) {
			for (bufferStruct::imageBufferNode& node : imageList) {
				node.clearView(device);
			}
		}



		//获取view列表
		std::vector<VkImageView> getViewList() const {
			std::vector<VkImageView> viewList{};

			for (const auto& node : imageList) {
				viewList.push_back(node.view);
			}

			return viewList;
		}
	};

	//ktx				加载结点
	struct ktxLoadNode {
		ktxTexture2*		  ktxTexture{};
		bool                  needWriteCache{ false };
		std::filesystem::path cacheFile{};
		std::string           error{};
	};

	//textureBuffer		结点
	struct textureBufferNode {
		uint32_t mipLevel   = 1;                    //mip级别
		uint32_t arrayLayer = 1;                    //纹理数组层数
		std::string textureFile;                    //纹理文件路径

		bufferStruct::imageBufferNode imageNode{};  //图像缓冲结点



		//创建 KTX 图集，无损压缩
		void createKtxArray(VkFormat format, const VkExtent2D& tileExtent, const VkExtent2D& atlasExtent, cmdStruct::cmdNode& cmd, const VkDevice& device, const VkQueue& graphicsQueue, const VmaAllocator& memAllocator) {
			if (textureFile.empty() || !std::filesystem::exists(textureFile)) {
				Log::printError(__FILE__, __LINE__, "贴图文件不存在");
				return;
			}

			ktxTexture2* ktxTexture{};
			ktxResult result = ktxTexture2_CreateFromNamedFile(textureFile.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture);
			if (result != KTX_SUCCESS) {
				Log::printError(__FILE__, __LINE__, "KTX贴图加载失败");
				return;
			}



			mipLevel   = ktxTexture->numLevels;
			arrayLayer = atlasExtent.width * atlasExtent.height;
			imageNode.create(VK_IMAGE_TYPE_2D, format, tileExtent, mipLevel, arrayLayer, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, device, memAllocator);

			baseBufferNode cpuBuffer{ ktxTexture->pData, ktxTexture->dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, memAllocator };
			cmd.copyBufferToImage(device, graphicsQueue, cpuBuffer.buffer, imageNode.image, makeImageCopyRegion(tileExtent.width, tileExtent.height, atlasExtent.width, atlasExtent.height, ktxTexture->baseWidth, ktxTexture->baseHeight, arrayLayer), mipLevel, arrayLayer, format);
			cpuBuffer.clear(memAllocator);
			ktxTexture2_Destroy(ktxTexture);
		}

		//创建 KTX 纹理，微损压缩 BC7 + mip
		void createKtx(cmdStruct::cmdNode& cmd, const VkDevice& device, const VkQueue& graphicsQueue, const VmaAllocator& memAllocator) {
			ktxLoadNode load = loadKtx();
			if (!load.error.empty()) { 
				Log::printError(__FILE__, __LINE__, load.error); 
				return; 
			}

			uploadKtx(load, cmd, device, graphicsQueue, memAllocator);
		}

		//创建 文本纹理
		void createText(const void* data, VkDeviceSize size, uint32_t width, uint32_t height, cmdStruct::cmdNode& cmd, const VkDevice& device, const VkQueue& graphicsQueue, const VmaAllocator& memAllocator) {
			if (!data || !size || !width || !height) {
				Log::printError(__FILE__, __LINE__, "纹理数据无效");
				return;
			}

			baseBufferNode cpuBuffer{ data, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, memAllocator };
			imageNode.create(VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, VkExtent2D{ width, height }, mipLevel, arrayLayer, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, device, memAllocator);
			
			cmd.copyBufferToImage(device, graphicsQueue, cpuBuffer.buffer, imageNode.image, makeImageCopyRegion(width, height), mipLevel, arrayLayer, VK_FORMAT_R8G8B8A8_UNORM);
			cpuBuffer.clear(memAllocator);
		}

		void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
			imageNode.clear(device, memAllocator);
			textureFile.clear();
		}



		ktxLoadNode loadKtx() {
			ktxLoadNode load{};
			if (textureFile.empty() || !std::filesystem::exists(textureFile)) { 
				load.error = "贴图文件不存在"; 
				return load; 
			}

			std::string cacheName = textureFile;
			for (char& ch : cacheName) { 
				if (ch == '/' || ch == '\\' || ch == ':') { 
					ch = '_'; 
				} 
			}

			if (cacheName.size() > 5 && cacheName.compare(cacheName.size() - 5, 5, ".ktx2") == 0) { 
				cacheName.erase(cacheName.size() - 5); 
			}

			load.cacheFile = std::filesystem::path(fileInfo::path::ktxCachePath) / (cacheName + ".bc7.ktx2");



			ktxResult result = KTX_SUCCESS;

			//有缓存：直接读缓存
			if (engineInfo::ktxCacheSwitch && std::filesystem::exists(load.cacheFile)) {		
				result = ktxTexture2_CreateFromNamedFile(load.cacheFile.string().c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &load.ktxTexture);

				//缓存自校验：仍需转码当未命中
				if (result == KTX_SUCCESS && ktxTexture2_NeedsTranscoding(load.ktxTexture)) {	
					ktxTexture2_Destroy(load.ktxTexture);
					load.ktxTexture = nullptr;
					Log::printWarning(__FILE__, __LINE__, "KTX 缓存仍需转码，回退源贴图：" + load.cacheFile.string());
				}
			}

			//无缓存（或缓存不可用）：解码源 → 转码 BC7
			if (load.ktxTexture == nullptr) {				
				result = ktxTexture2_CreateFromNamedFile(textureFile.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &load.ktxTexture);
				if (result != KTX_SUCCESS) { load.error = "KTX贴图加载失败"; return load; }

				result = ktxTexture2_TranscodeBasis(load.ktxTexture, KTX_TTF_BC7_RGBA, 0);
				if (result != KTX_SUCCESS) { ktxTexture2_Destroy(load.ktxTexture); load.ktxTexture = nullptr; load.error = "KTX贴图转码失败"; return load; }

				load.needWriteCache = engineInfo::ktxCacheSwitch;
			}

			return load;
		}

		void uploadKtx(ktxLoadNode& load, cmdStruct::cmdNode& cmd, const VkDevice& device, const VkQueue& graphicsQueue, const VmaAllocator& memAllocator) {
			mipLevel   = load.ktxTexture->numLevels;
			arrayLayer = 1;
			imageNode.create(VK_IMAGE_TYPE_2D, VK_FORMAT_BC7_UNORM_BLOCK, VkExtent2D{ load.ktxTexture->baseWidth, load.ktxTexture->baseHeight }, mipLevel, arrayLayer, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, device, memAllocator);

			baseBufferNode cpuBuffer{ load.ktxTexture->pData, load.ktxTexture->dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, memAllocator };
			cmd.copyBufferToImage(device, graphicsQueue, cpuBuffer.buffer, imageNode.image, makeImageCopyRegion(load.ktxTexture->baseWidth, load.ktxTexture->baseHeight, mipLevel, load.ktxTexture), mipLevel, arrayLayer, VK_FORMAT_BC7_UNORM_BLOCK);
			cpuBuffer.clear(memAllocator);

			//上传完成后：ZSTD 超压缩写缓存
			if (load.needWriteCache) {				
				std::filesystem::create_directories(load.cacheFile.parent_path());
				ktxTexture2_DeflateZstd(load.ktxTexture, 3);

				if (ktxTexture2_WriteToNamedFile(load.ktxTexture, load.cacheFile.string().c_str()) != KTX_SUCCESS) {
					Log::printWarning(__FILE__, __LINE__, "KTX 缓存写入失败（不影响本次运行）：" + load.cacheFile.string());
				}
			}

			ktxTexture2_Destroy(load.ktxTexture);
		}

	private:
		//构建图像拷贝区域，KTX 图集
		std::vector<VkBufferImageCopy> makeImageCopyRegion(uint32_t tileWidth, uint32_t tileHeight, uint32_t atlasWidth, uint32_t atlasHeight, uint32_t texWidth, uint32_t texHeight, uint32_t arrayLayer) {
			std::vector<VkBufferImageCopy> regions{ arrayLayer };
			uint32_t texturePixel = 4;
			uint32_t wOffset      = tileWidth  * texturePixel;
			uint32_t hOffset      = atlasWidth * tileWidth * tileHeight * texturePixel;

			for (uint32_t w = 0; w < atlasWidth; w++) {
				for (uint32_t h = 0; h < atlasHeight; h++) {
					uint32_t id     = w + h * atlasWidth;
					uint32_t offset = w * wOffset + h * hOffset;

					regions[id].bufferOffset                    = offset;
					regions[id].bufferRowLength                 = texWidth;
					regions[id].bufferImageHeight               = texHeight;
					regions[id].imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
					regions[id].imageSubresource.mipLevel       = 0;
					regions[id].imageSubresource.baseArrayLayer = id;
					regions[id].imageSubresource.layerCount     = 1;
					regions[id].imageOffset                     = { 0, 0, 0 };
					regions[id].imageExtent                     = { tileWidth, tileHeight, 1 };
				}
			}

			return regions;
		}

		//构建图像拷贝区域，KTX 纹理
		std::vector<VkBufferImageCopy> makeImageCopyRegion(uint32_t width, uint32_t height, uint32_t mipLevels, ktxTexture2* ktxTexture) {
			std::vector<VkBufferImageCopy> regions{ mipLevels };

			for (uint32_t i = 0; i < mipLevels; i++) {
				ktx_size_t offset;
				ktxTexture2_GetImageOffset(ktxTexture, i, 0, 0, &offset);

				regions[i].bufferOffset                    = offset;
				regions[i].bufferRowLength                 = 0;
				regions[i].bufferImageHeight               = 0;
				regions[i].imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
				regions[i].imageSubresource.mipLevel       = i;
				regions[i].imageSubresource.baseArrayLayer = 0;
				regions[i].imageSubresource.layerCount     = 1;
				regions[i].imageOffset                     = { 0, 0, 0 };
				regions[i].imageExtent                     = { width >> i, height >> i, 1 };
			}

			return regions;
		}

		//构建图像拷贝区域，单区域
		std::vector<VkBufferImageCopy> makeImageCopyRegion(uint32_t width, uint32_t height) {
			VkBufferImageCopy region{};
			region.bufferOffset                    = 0;
			region.bufferRowLength                 = 0;
			region.bufferImageHeight               = 0;
			region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.mipLevel       = 0;
			region.imageSubresource.baseArrayLayer = 0;
			region.imageSubresource.layerCount     = 1;
			region.imageOffset                     = { 0, 0, 0 };
			region.imageExtent                     = { width, height, 1 };

			return { region };
		}
	};
}

//swapchain 结构空间
namespace swapStruct {
	struct swapSupportNode {
		VkSurfaceCapabilitiesKHR        capabilitie{};
		std::vector<VkSurfaceFormatKHR> swapFormat{};
		std::vector<VkPresentModeKHR>   presentMode{};

		VkSwapchainCreateInfoKHR swapCreateInfo{};



		swapSupportNode() {
			swapCreateInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			swapCreateInfo.imageArrayLayers = 1;
			swapCreateInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			swapCreateInfo.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
			swapCreateInfo.clipped          = VK_TRUE;
			swapCreateInfo.oldSwapchain     = VK_NULL_HANDLE;
		}



		//设置交换链支持结点
		void createSwapSupport(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& presentSurface) {
			//表面功能查询
			if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, presentSurface, &capabilitie)) {
				Log::printError(__FILE__, __LINE__, "表面功能获取失败");
			}

			//表面格式查询
			uint32_t formatCount;
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, presentSurface, &formatCount, nullptr);
			if (formatCount == 0) {
				Log::printError(__FILE__, __LINE__, "表面格式获取失败");
			}
			swapFormat.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, presentSurface, &formatCount, swapFormat.data());

			//呈现模式查询
			uint32_t presentModeCount;
			vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, presentSurface, &presentModeCount, nullptr);
			if (presentModeCount == 0) {
				Log::printError(__FILE__, __LINE__, "呈现模式获取失败");
			}
			presentMode.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, presentSurface, &presentModeCount, presentMode.data());



			if (debugInfo::LogSwitch) {
				Log::printTitle("交换链配置");

				Log::print("最小图像数量:\t" + std::to_string(capabilitie.minImageCount));
				Log::print("最大图像数量:\t" + std::to_string(capabilitie.maxImageCount));
				Log::print("");

				Log::print("支持格式:");
				for (auto format : swapFormat) {
					std::cout << "\t" << "format: " << format.format << ", colorSpace: " << format.colorSpace << std::endl;
				}
				Log::print("");

				Log::print("呈现模式:");
				for (auto mode : presentMode) {
					std::cout << "\t" << "mode: " << mode << std::endl;
				}

				Log::delimiter();
			}
		}



		//设置swapCreateInfo
		void createSwapCreateInfo(const VkSurfaceKHR& presentSurface) {
			VkSurfaceFormatKHR SwapFormat = getSwapFormat();
			swapCreateInfo.surface                = presentSurface;
			swapCreateInfo.minImageCount          = capabilitie.minImageCount + (capabilitie.maxImageCount > capabilitie.minImageCount);
			swapCreateInfo.imageFormat            = SwapFormat.format;
			swapCreateInfo.imageColorSpace        = SwapFormat.colorSpace;
			swapCreateInfo.imageExtent            = getSwapExtent();
			swapCreateInfo.imageSharingMode       = VK_SHARING_MODE_EXCLUSIVE;
			swapCreateInfo.queueFamilyIndexCount  = 0;
			swapCreateInfo.pQueueFamilyIndices    = nullptr;
			swapCreateInfo.preTransform           = capabilitie.currentTransform;
			swapCreateInfo.presentMode            = getSwapPresentMode();
			swapCreateInfo.oldSwapchain           = VK_NULL_HANDLE;
		}

		//重建swapExtent
		void recreateSwapCreateInfo(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& presentSurface, const VkSwapchainKHR& swapchain) {
			if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, presentSurface, &capabilitie)) {
				Log::printError(__FILE__, __LINE__, "重建交换链时表面功能获取失败");
			}

			swapCreateInfo.imageExtent = getSwapExtent();
			swapCreateInfo.oldSwapchain = swapchain;
		}

	private:
		//获取交换链绘制范围
		VkExtent2D getSwapExtent() {
			//Vulkan特殊约定，如果currentExtent.width为最大值，表示尺寸由应用程序决定，反之使用系统指定的尺寸
			if (capabilitie.currentExtent.width == std::numeric_limits<uint32_t>::max()) {
				VkExtent2D actualExtent = { windowInfo::width, windowInfo::height };

				actualExtent.width = std::max(capabilitie.minImageExtent.width, std::min(capabilitie.maxImageExtent.width, actualExtent.width));
				actualExtent.height = std::max(capabilitie.minImageExtent.height, std::min(capabilitie.maxImageExtent.height, actualExtent.height));

				return actualExtent;
			}
			else {
				return capabilitie.currentExtent;
			}
		}

		//获取交换链格式
		VkSurfaceFormatKHR getSwapFormat() {
			//只有一个未定义格式时，返回指定格式
			if (swapFormat.size() == 1 && swapFormat[0].format == VK_FORMAT_UNDEFINED) {
				return { VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
			}

			//含有多个格式支持时，从中挑选返回
			for (VkSurfaceFormatKHR availableFormat : swapFormat) {
				if (availableFormat.format == VK_FORMAT_R8G8B8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
					return availableFormat;
				}
			}

			return swapFormat[0];
		}

		//获取交换链合适的呈现模式
		VkPresentModeKHR getSwapPresentMode() {
			for (auto availablePresentMode : presentMode) {
				if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {

					if (debugInfo::LogSwitch) {
						Log::print("交换链选择三缓存呈现模式");

						Log::delimiter();
					}

					return availablePresentMode;
				}
				else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
					if (debugInfo::LogSwitch) {
						Log::print("交换链选择立即呈现模式");

						Log::delimiter();
					}

					return availablePresentMode;
				}
			}

			if (debugInfo::LogSwitch) {
				Log::print("交换链选择垂直中断呈现模式");

				Log::delimiter();
			}

			return VK_PRESENT_MODE_FIFO_KHR;
		}

	public:
		//获取深度格式
		VkFormat getDepthFormat(const VkPhysicalDevice& physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
			for (VkFormat format : candidates) {
				//获取当前格式的属性信息
				VkFormatProperties props;
				vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

				//检查当前格式是否支持所需的特性和平铺方式
				if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
					//如果当前格式支持最佳平铺并且满足特性要求，则返回该格式
					return format;
				}
				else if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
					//如果当前格式支持线性平铺并且满足特性要求，则返回该格式
					return format;
				}
			}

			Log::printError(__FILE__, __LINE__, "深度格式获取失败");
			return VK_FORMAT_UNDEFINED;
		}

		//获取多重采样
		VkSampleCountFlagBits getMsaa(const VkPhysicalDevice& physicalDevice) {
			VkPhysicalDeviceProperties physicalDeviceProperties;
			vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

			VkSampleCountFlags count = std::min(physicalDeviceProperties.limits.framebufferColorSampleCounts, physicalDeviceProperties.limits.framebufferDepthSampleCounts);
			/*if (count & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
			if (count & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
			if (count & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
			if (count & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }*/
			if (count & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
			if (count & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

			return VK_SAMPLE_COUNT_1_BIT;
		}
	};

	//交换链图像列表，服务于renderFrameNode
	std::vector<VkImage> swapImageList{};
}

//render    结构空间
namespace renderStruct {
	//render附件结点
	struct renderAttachmentNode {
		std::vector<VkAttachmentDescription>              attDescList{};    //渲染附件描述列表
		std::array<std::vector<VkAttachmentReference>, 3> attRefList{};     //渲染附件引用列表，0 颜色、1 深度、2 解析



		renderAttachmentNode(renderEnum::renderAttId::type renderAttType, VkFormat colorFormat, VkFormat depthFormat, VkSampleCountFlagBits msaa = VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM, VkImageLayout finalLayout = VK_IMAGE_LAYOUT_MAX_ENUM) {
			switch (renderAttType) {
			case renderEnum::renderAttId::forward_ra: {
				//前向渲染附件列表：颜色、深度、解析
				attDescList = {
					createAttachmentDesc(colorFormat, msaa,                  VK_ATTACHMENT_LOAD_OP_CLEAR,     VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
					createAttachmentDesc(depthFormat, msaa,                  VK_ATTACHMENT_LOAD_OP_CLEAR,     VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL),
					createAttachmentDesc(colorFormat, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE,     finalLayout),
				};

				attRefList = {{
					{createAttachmentRef(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)},
					{createAttachmentRef(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)},
					{createAttachmentRef(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)},
				}};

				break;
			}
			case renderEnum::renderAttId::deferred_ra: {
				//延迟渲染附件列表：坐标颜色、法线颜色、漫反射颜色、阴影深度
				attDescList = {
					createAttachmentDesc(renderInfo::dsPosColorFormat, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
					createAttachmentDesc(colorFormat, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
					createAttachmentDesc(colorFormat, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
					createAttachmentDesc(depthFormat, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL),
				};

				attRefList = {{
					{createAttachmentRef(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL), createAttachmentRef(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL), createAttachmentRef(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)},
					{createAttachmentRef(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)},
					{},
				}};

				break;
			}
			case renderEnum::renderAttId::shadowMap_ra: {
				//阴影映射附件列表：阴影深度
				attDescList = {
					createAttachmentDesc(depthFormat, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
				};

				attRefList = {{
					{},
					{createAttachmentRef(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)},
					{},
				}};

				break;
			}
			}
		}

		renderAttachmentNode() = delete;

	private:
		uint32_t refIndex = 0;      //refList的索引



		//创建渲染附件描述
		VkAttachmentDescription createAttachmentDesc(VkFormat format, VkSampleCountFlagBits msaa, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp, VkImageLayout finalLayout) {
			return {
				.format         = format,                            //附件的图像格式
				.samples        = msaa,                              //多重采样的数量
				.loadOp         = loadOp,                            //渲染前颜色与深度的加载操作
				.storeOp        = storeOp,                           //渲染后颜色与深度的存储操作
				.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,   //渲染前模板的加载操作
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,  //渲染后模板的存储操作
				.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,         //渲染前图像的布局
				.finalLayout    = finalLayout,                       //渲染后图像的布局
			};
		}

		//创建渲染附件引用
		VkAttachmentReference createAttachmentRef(VkImageLayout layout) {
			return{
				.attachment = refIndex++,   //附件的索引
				.layout     = layout,       //附件的图像布局
			};
		}
	};

	//render子通道结点
	struct renderSubpassNode {
		/*
		示例链：上阶段    → 本阶段
		依赖 0：外部      → subpass 0     （开始渲染）
		依赖 1：subpass 0 → subpass 1     （子通道间同步）
		依赖 2：subpass 1 → subpass 2     （子通道间同步）
		依赖 3：subpass 2 → 外部          （结束渲染）

		当前链：上阶段    → 本阶段
		依赖 0：外部      → subpass 0     （开始渲染）
		依赖 1：subpass 0 → 外部          （结束渲染）
		*/

		std::vector<VkSubpassDescription> subpassList{};        //渲染子通道列表
		std::vector<VkSubpassDependency>  dependencyList{};     //渲染子通道依赖列表，比subpassList多一个，具体参考上述示例



		renderSubpassNode(renderEnum::renderTemplateId::type renderTemplateType, VkPipelineBindPoint pipeType, const std::array<std::vector<VkAttachmentReference>, 3>& attRefList) {
			subpassList = {
				createSubpass(pipeType, attRefList),
			};

			switch (renderTemplateType) {
			case renderEnum::renderTemplateId::swapChain_rt: {
				dependencyList = {
					//依赖 0：外部 → subpass 0，等上一帧颜色和深度写完才能开始读写
					createSubpassDependency(
						VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
						0,                   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,                                             VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
					),
					//依赖 1：subpass 0 → 外部，确保本帧颜色和深度写完才被下一帧读取
					createSubpassDependency(
						0,                   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,                                              VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
						VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,                                                  VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
					),
				};

				break;
			}
			case renderEnum::renderTemplateId::offScreen_rt: {
				dependencyList = {
					//依赖 0：外部 → subpass 0，等上一帧片段着色器结束，开始颜色深度读写
					createSubpassDependency(
						VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,                                                                                                  VK_ACCESS_NONE,
						0,                   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
					),
					//依赖 1：subpass 0 → 外部，等颜色深度写完，片段着色器才能读
					createSubpassDependency(
						0,                   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
						VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,                                                                                                  VK_ACCESS_SHADER_READ_BIT
					),
				};

				break;
			}
			case renderEnum::renderTemplateId::deferred_rt: {
				dependencyList = {
					//依赖 0：外部 → subpass 0，等上一帧颜色写完，本帧开始读写颜色附件
					createSubpassDependency(
						VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
						0,                   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
					),
					//依赖 1：subpass 0 → 外部，等本帧颜色写完，片段着色器才能读（供光照阶段采样）
					createSubpassDependency(
						0,                   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
						VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,         VK_ACCESS_SHADER_READ_BIT
					),
				};

				break;
			}
			case renderEnum::renderTemplateId::shadowMap_rt: {
				dependencyList = {
					//依赖 0：外部 → subpass 0，等上一帧深度写完，本帧开始深度读写
					createSubpassDependency(
						VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
						0,                   VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
					),
					//依赖 1：subpass 0 → 外部，等深度写完，片段着色器才能读阴影贴图
					createSubpassDependency(
						0,                   VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
						VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,                                                  VK_ACCESS_SHADER_READ_BIT
					),
				};

				break;
			}
			}
		}

		renderSubpassNode() = delete;

	private:
		//创建subpass
		VkSubpassDescription createSubpass(VkPipelineBindPoint pipeType, const std::array<std::vector<VkAttachmentReference>, 3>& attRefList) {
			return{
				.pipelineBindPoint       = pipeType,                        //管线类型（图形、计算、光追等等）
				.colorAttachmentCount    = static_cast<uint32_t>(attRefList[0].size()),  //颜色附件数量
				.pColorAttachments       = attRefList[0].data(),            //颜色附件引用
				.pResolveAttachments     = attRefList[2].data(),            //解析附件引用
				.pDepthStencilAttachment = attRefList[1].data(),            //深度附件引用
			};
		}

		//创建subpass依赖
		VkSubpassDependency createSubpassDependency(uint32_t srcSubpass, VkPipelineStageFlags srcStageMask, VkAccessFlags srcAccessMask, uint32_t dstSubpass, VkPipelineStageFlags dstStageMask, VkAccessFlags dstAccessMask) {
			return {
				.srcSubpass      = srcSubpass,                      //上阶段已完成的子通道，（VK_SUBPASS_EXTERNAL：外部、0/1/2：第n个子通道）
				.dstSubpass      = dstSubpass,                      //本阶段要执行的子通道，（VK_SUBPASS_EXTERNAL：外部、0/1/2：第n个子通道）
				.srcStageMask    = srcStageMask,                    //上阶段已完成的管线阶段，（颜色、深度、着色器、全部）
				.dstStageMask    = dstStageMask,                    //本阶段要执行的管线阶段，（颜色、深度、着色器、全部）
				.srcAccessMask   = srcAccessMask,                   //上阶段对附件的内存权限，（颜色读写、深度读写、着色器读写、内存读写、无权限）
				.dstAccessMask   = dstAccessMask,                   //本阶段对附件的内存权限，（颜色读写、深度读写、着色器读写、内存读写、无权限）
				.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,     //依赖类型，（同一区域的像素串行，不同区域的像素可并行）
			};
		}
	};
	
	//render帧结点
	struct renderFrameNode {
		VkFramebuffer         frameBuffer{};    //帧缓冲
		VkRenderPassBeginInfo renderBegin{};    //渲染通道开始信息



		renderFrameNode(const VkRenderPass& render, const std::vector<VkImageView>& frameAttList, const std::vector<VkClearValue>& clearValueList, const VkExtent2D& extent, const VkDevice& device) {
			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass      = render;
			framebufferInfo.attachmentCount = static_cast<uint32_t>(frameAttList.size());
			framebufferInfo.pAttachments    = frameAttList.data();
			framebufferInfo.width           = extent.width;
			framebufferInfo.height          = extent.height;
			framebufferInfo.layers          = 1;

			if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &frameBuffer) != VK_SUCCESS) {
				Log::printError(__FILE__, __LINE__, "frameBuffer创建失败");
			}

			renderBegin.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderBegin.renderPass        = render;
			renderBegin.framebuffer       = frameBuffer;
			renderBegin.renderArea.offset = { 0, 0 };
			renderBegin.renderArea.extent = extent;
			renderBegin.clearValueCount   = static_cast<uint32_t>(clearValueList.size());
			renderBegin.pClearValues      = clearValueList.data();
		}

		renderFrameNode() = delete;



		void clear(const VkDevice& device) {
			vkDestroyFramebuffer(device, frameBuffer, nullptr);
		}
	};



	//render模板结点
	struct renderTemplateNode {
		renderEnum::renderAttId::type      renderAttType      = renderEnum::renderAttId::enumNull;			//未指定附件
		renderEnum::renderTemplateId::type renderTemplateType = renderEnum::renderTemplateId::enumNull;		//未指定模板

		VkRenderPass render{};      //渲染通道



		void createRenderTemplateNode(renderEnum::renderTemplateId::type tRenderTemplateType, VkFormat colorFormat, VkFormat depthFormat, VkSampleCountFlagBits msaa, const VkDevice& device) {
			switch (tRenderTemplateType) {
			case renderEnum::renderTemplateId::swapChain_rt: {
				renderAttType = renderEnum::renderAttId::forward_ra;

				renderStruct::renderAttachmentNode renderAttachment{ renderAttType, colorFormat, depthFormat, msaa, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR };
				renderStruct::renderSubpassNode    renderSubpass   { tRenderTemplateType, VK_PIPELINE_BIND_POINT_GRAPHICS, renderAttachment.attRefList };

				createRender(renderAttachment.attDescList, renderSubpass.subpassList, renderSubpass.dependencyList, device, "交换链");

				break;
			}
			case renderEnum::renderTemplateId::offScreen_rt: {
				renderAttType = renderEnum::renderAttId::forward_ra;

				renderStruct::renderAttachmentNode renderAttachment{ renderAttType, colorFormat, depthFormat, msaa, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
				renderStruct::renderSubpassNode    renderSubpass   { tRenderTemplateType, VK_PIPELINE_BIND_POINT_GRAPHICS, renderAttachment.attRefList };

				createRender(renderAttachment.attDescList, renderSubpass.subpassList, renderSubpass.dependencyList, device, "离屏");

				break;
			}
			case renderEnum::renderTemplateId::deferred_rt: {
				renderAttType = renderEnum::renderAttId::deferred_ra;

				renderStruct::renderAttachmentNode renderAttachment{ renderAttType, colorFormat, depthFormat };
				renderStruct::renderSubpassNode    renderSubpass   { tRenderTemplateType, VK_PIPELINE_BIND_POINT_GRAPHICS, renderAttachment.attRefList };

				createRender(renderAttachment.attDescList, renderSubpass.subpassList, renderSubpass.dependencyList, device, "延迟着色");

				break;
			}
			case renderEnum::renderTemplateId::shadowMap_rt: {
				renderAttType = renderEnum::renderAttId::shadowMap_ra;

				renderStruct::renderAttachmentNode renderAttachment{ renderAttType, VK_FORMAT_MAX_ENUM, depthFormat };
				renderStruct::renderSubpassNode    renderSubpass   { tRenderTemplateType, VK_PIPELINE_BIND_POINT_GRAPHICS, renderAttachment.attRefList };

				createRender(renderAttachment.attDescList, renderSubpass.subpassList, renderSubpass.dependencyList, device, "阴影映射");

				break;
			}
			}

			renderTemplateType = tRenderTemplateType;
		}

		void clear(const VkDevice& device) {
			vkDestroyRenderPass(device, render, nullptr);
		}

	private:
		void createRender(const std::vector<VkAttachmentDescription>& attDescList, const std::vector<VkSubpassDescription>& subpassList, const std::vector<VkSubpassDependency>& dependencyList, const VkDevice& device, const char* name) {
			VkRenderPassCreateInfo createInfo{};
			createInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			createInfo.attachmentCount = static_cast<uint32_t>(attDescList.size());
			createInfo.pAttachments    = attDescList.data();
			createInfo.subpassCount    = static_cast<uint32_t>(subpassList.size());
			createInfo.pSubpasses      = subpassList.data();
			createInfo.dependencyCount = static_cast<uint32_t>(dependencyList.size());
			createInfo.pDependencies   = dependencyList.data();

			if (vkCreateRenderPass(device, &createInfo, nullptr, &render) != VK_SUCCESS) {
				Log::printError(__FILE__, __LINE__, std::string(name) + "渲染通道创建失败");
			}
		}
	};

	//render实例结点
	struct renderInstanceNode {
		std::vector<bufferStruct::imageBufferListNode> frameImageList{};        //帧图像列表
		std::vector<renderStruct::renderFrameNode>     frameBufferList{};       //帧缓冲列表



		void createRenderInstanceNode(const renderStruct::renderTemplateNode& renderTemplate, VkFormat colorFormat, VkFormat depthFormat, const VkExtent2D& imageExtent, VkSampleCountFlagBits msaa, const VkDevice& device, const VmaAllocator& memAllocator) {
			switch (renderTemplate.renderAttType) {
			case renderEnum::renderAttId::forward_ra: {
				clearValueList = { renderInfo::colorClearValue, renderInfo::depthClearValue };
				
				break;
			}
			case renderEnum::renderAttId::deferred_ra: {
				clearValueList = { renderInfo::colorClearValue, renderInfo::colorClearValue, renderInfo::colorClearValue, renderInfo::depthClearValue };

				break;
			}
			case renderEnum::renderAttId::shadowMap_ra: {
				clearValueList = { renderInfo::depthClearValue };

				break;
			}
			}

			switch (renderTemplate.renderTemplateType) {
			case renderEnum::renderTemplateId::swapChain_rt: {
				frameImageList.resize(renderEnum::renderFrameId::enumCount);

				//color、depth
				frameImageList[renderEnum::renderFrameId::conv_rf].imageList = {
					{VK_IMAGE_TYPE_2D, colorFormat, imageExtent, 1, 1, msaa, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_ASPECT_COLOR_BIT, device, memAllocator},
					{VK_IMAGE_TYPE_2D, depthFormat, imageExtent, 1, 1, msaa, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,                                   VK_IMAGE_ASPECT_DEPTH_BIT, device, memAllocator},
				};

				//swap
				frameImageList[renderEnum::renderFrameId::swap_rf].createSwapImageList(swapStruct::swapImageList, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, device);

				createFrameBufferList(frameImageList[renderEnum::renderFrameId::conv_rf], frameImageList[renderEnum::renderFrameId::swap_rf], renderTemplate.render, imageExtent, device);
				break;
			}
			case renderEnum::renderTemplateId::offScreen_rt: {
				frameImageList.resize(1);

				//color、depth、view
				frameImageList[renderEnum::renderFrameId::conv_rf].imageList = {
					{VK_IMAGE_TYPE_2D, colorFormat, imageExtent, 1, 1, msaa,                  VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, device, memAllocator},
					{VK_IMAGE_TYPE_2D, depthFormat, imageExtent, 1, 1, msaa,                  VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,                      VK_IMAGE_ASPECT_DEPTH_BIT, device, memAllocator},
					{VK_IMAGE_TYPE_2D, colorFormat, imageExtent, 1, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, device, memAllocator},
				};

				createFrameBufferList(frameImageList[renderEnum::renderFrameId::conv_rf], renderTemplate.render, imageExtent, device);
				break;
			}
			case renderEnum::renderTemplateId::deferred_rt: {
				frameImageList.resize(1);

				//position、normal、albedo、depth
				frameImageList[renderEnum::renderFrameId::conv_rf].imageList = {
					{VK_IMAGE_TYPE_2D, renderInfo::dsPosColorFormat, imageExtent, 1, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, device, memAllocator},
					{VK_IMAGE_TYPE_2D, colorFormat,                  imageExtent, 1, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, device, memAllocator},
					{VK_IMAGE_TYPE_2D, colorFormat,                  imageExtent, 1, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, device, memAllocator},
					{VK_IMAGE_TYPE_2D, depthFormat,                  imageExtent, 1, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,                      VK_IMAGE_ASPECT_DEPTH_BIT, device, memAllocator},
				};

				createFrameBufferList(frameImageList[renderEnum::renderFrameId::conv_rf], renderTemplate.render, imageExtent, device);
				break;
			}
			case renderEnum::renderTemplateId::shadowMap_rt: {
				frameImageList.resize(1);

				//depth
				frameImageList[renderEnum::renderFrameId::conv_rf].imageList = {
					{VK_IMAGE_TYPE_2D, depthFormat, imageExtent, 1, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, device, memAllocator},
				};

				createFrameBufferList(frameImageList[renderEnum::renderFrameId::conv_rf], renderTemplate.render, imageExtent, device);
				break;
			}
			}
		}

		void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
			//清理frameImageList
			frameImageList[renderEnum::renderFrameId::conv_rf].clear(device, memAllocator);

			if (frameImageList.size() > renderEnum::renderFrameId::swap_rf) {
				frameImageList[renderEnum::renderFrameId::swap_rf].clearView(device);
			}
			frameImageList.clear();

			//清理frameBufferList
			for (renderStruct::renderFrameNode& frameNode : frameBufferList) {
				frameNode.clear(device);
			}
			frameBufferList.clear();
		}

	private:
		std::vector<VkClearValue> clearValueList{};     //清理值列表



		//创建frameBufferList
		void createFrameBufferList(const bufferStruct::imageBufferListNode& conv, const VkRenderPass& render, const VkExtent2D& extent, const VkDevice& device) {
			frameBufferList = {
				{render, conv.getViewList(), clearValueList, extent, device}
			};
		}

		//创建frameBufferList，swap版
		void createFrameBufferList(const bufferStruct::imageBufferListNode& conv, const bufferStruct::imageBufferListNode& swap, const VkRenderPass& render, const VkExtent2D& extent, const VkDevice& device) {
			std::vector<VkImageView> viewList = conv.getViewList();
			viewList.push_back({});     //预留 swap 槽位

			for (const bufferStruct::imageBufferNode& imageNode : swap.imageList) {
				viewList.back() = imageNode.view;
				frameBufferList.push_back(renderStruct::renderFrameNode{ render, viewList, clearValueList, extent, device });
			}
		}
	};
}

//sampler	结构空间
namespace samplerStruct {
	//samplerConfig 传参结点
	struct samplerConfigParamNode {
		//预设指定
		samplerEnum::magId::type		 magType		 = samplerEnum::magId::linear;					//放大过滤
		samplerEnum::minId::type         minType		 = samplerEnum::minId::linear;					//缩小过滤
		samplerEnum::mipmapId::type		 mipmapType		 = samplerEnum::mipmapId::nearest;				//mip 过滤
		samplerEnum::addressId::type	 addressType	 = samplerEnum::addressId::repeat;				//寻址模式（U/V/W 同值）
		samplerEnum::anisoId::type		 anisoType		 = samplerEnum::anisoId::x16;					//各向异性
		samplerEnum::lodSourceId::type   lodSourceType	 = samplerEnum::lodSourceId::none;				//maxLod 两态：none = 0（不运算）/ clampNone = 不限制（交给纹理 mip 链）
		samplerEnum::compareId::type     compareType     = samplerEnum::compareId::off;					//比较采样（阴影）
		samplerEnum::borderColorId::type borderColorType = samplerEnum::borderColorId::intOpaqueBlack;	//越界返回色
	};

	//samplerConfig 结点
	struct samplerConfigNode {
		VkSamplerCreateInfo samplerInfo{};



		//创建 采样器配置
		void createSamplerConfigNode(const samplerConfigParamNode& param) {
			samplerInfo = {};
			samplerInfo.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerInfo.minFilter    = createMin(param.minType);			//预设：缩小过滤
			samplerInfo.borderColor  = createBorderColor(param);			//预设：越界返回色
			createCompare(param);											//预设：比较采样

			samplerInfo.mipLodBias	 = 0.0f;								//无预设项
			samplerInfo.minLod		 = 0.0f;								//无预设项
			samplerInfo.unnormalizedCoordinates = VK_FALSE;					//无预设项：只用归一化坐标

			//预设指定
			samplerInfo.magFilter    = createMag(param.magType);
			samplerInfo.mipmapMode   = createMipmap(param.mipmapType);
			samplerInfo.addressModeU = createAddress(param.addressType);
			samplerInfo.addressModeV = samplerInfo.addressModeU;
			samplerInfo.addressModeW = samplerInfo.addressModeU;
			createAniso(param.anisoType);
			samplerInfo.maxLod = createMaxLod(param);
		}

	private:
		//映射 放大过滤预设 → VkFilter
		VkFilter createMag(const samplerEnum::magId::type& magType) const {
			VkFilter filter = VK_FILTER_NEAREST;

			switch (magType) {
			case samplerEnum::magId::nearest: { filter = VK_FILTER_NEAREST;	break; }
			case samplerEnum::magId::linear:  { filter = VK_FILTER_LINEAR;	break; }
			default: {
				Log::printError(__FILE__, __LINE__, "samplerMagId 未写预设处理：值 " + std::to_string(static_cast<int>(magType)));
				break;
			}}

			return filter;
		}

		//映射 缩小过滤预设 → VkFilter
		VkFilter createMin(const samplerEnum::minId::type& minType) const {
			VkFilter filter = VK_FILTER_LINEAR;

			switch (minType) {
			case samplerEnum::minId::nearest: { filter = VK_FILTER_NEAREST;	break; }
			case samplerEnum::minId::linear:  { filter = VK_FILTER_LINEAR;	break; }
			default: {
				Log::printError(__FILE__, __LINE__, "minId 未写预设处理：值 " + std::to_string(static_cast<int>(minType)));
				break;
			}}

			return filter;
		}

		//映射 mip 过滤预设 → VkSamplerMipmapMode
		VkSamplerMipmapMode createMipmap(const samplerEnum::mipmapId::type& mipmapType) const {
			VkSamplerMipmapMode mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;

			switch (mipmapType) {
			case samplerEnum::mipmapId::nearest: { mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;	break; }
			case samplerEnum::mipmapId::linear:  { mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;	break; }
			default: {
				Log::printError(__FILE__, __LINE__, "samplerMipmapId 未写预设处理：值 " + std::to_string(static_cast<int>(mipmapType)));
				break;
			}}

			return mipmapMode;
		}

		//映射 寻址预设 → VkSamplerAddressMode
		VkSamplerAddressMode createAddress(const samplerEnum::addressId::type& addressType) const {
			VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;

			switch (addressType) {
			case samplerEnum::addressId::repeat:       { addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;			break; }
			case samplerEnum::addressId::clampEdge:    { addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;	break; }
			case samplerEnum::addressId::mirrorRepeat: { addressMode = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;	break; }
			case samplerEnum::addressId::clampBorder:  { addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;	break; }
			default: {
				Log::printError(__FILE__, __LINE__, "samplerAddressId 未写预设处理：值 " + std::to_string(static_cast<int>(addressType)));
				break;
			}}

			return addressMode;
		}

		//填 各向异性（预设等级 → enable + maxAnisotropy；这里只填请求值，设备限制在创建时夹取）
		void createAniso(const samplerEnum::anisoId::type& anisoType) {
			switch (anisoType) {
			case samplerEnum::anisoId::off: { samplerInfo.anisotropyEnable = VK_FALSE;	samplerInfo.maxAnisotropy = 1.0f;	break; }
			case samplerEnum::anisoId::x1:  { samplerInfo.anisotropyEnable = VK_TRUE;	samplerInfo.maxAnisotropy = 1.0f;	break; }
			case samplerEnum::anisoId::x2:  { samplerInfo.anisotropyEnable = VK_TRUE;	samplerInfo.maxAnisotropy = 2.0f;	break; }
			case samplerEnum::anisoId::x4:  { samplerInfo.anisotropyEnable = VK_TRUE;	samplerInfo.maxAnisotropy = 4.0f;	break; }
			case samplerEnum::anisoId::x8:  { samplerInfo.anisotropyEnable = VK_TRUE;	samplerInfo.maxAnisotropy = 8.0f;	break; }
			case samplerEnum::anisoId::x16: { samplerInfo.anisotropyEnable = VK_TRUE;	samplerInfo.maxAnisotropy = 16.0f;	break; }
			default: {
				Log::printError(__FILE__, __LINE__, "samplerAnisoId 未写预设处理：值 " + std::to_string(static_cast<int>(anisoType)));
				break;
			}}
		}

		//填 比较采样（compareId 预设 → compareEnable + compareOp）
		void createCompare(const samplerConfigParamNode& param) {
			switch (param.compareType) {
			case samplerEnum::compareId::off:         { samplerInfo.compareEnable = VK_FALSE;	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;		 break; }	//无比较
			case samplerEnum::compareId::lessOrEqual: { samplerInfo.compareEnable = VK_TRUE;	samplerInfo.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL; break; }	//阴影比较
			default: {
				Log::printError(__FILE__, __LINE__, "compareId 未写预设处理：值 " + std::to_string(static_cast<int>(param.compareType)));
				break;
			}}
		}

		//映射 越界返回色预设 → VkBorderColor
		VkBorderColor createBorderColor(const samplerConfigParamNode& param) const {
			VkBorderColor borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

			switch (param.borderColorType) {
			case samplerEnum::borderColorId::intOpaqueBlack:        { borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;		 break; }
			case samplerEnum::borderColorId::floatOpaqueBlack:      { borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;		 break; }
			case samplerEnum::borderColorId::floatOpaqueWhite:      { borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;		 break; }
			case samplerEnum::borderColorId::floatTransparentBlack: { borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK; break; }
			default: {
				Log::printError(__FILE__, __LINE__, "borderColorId 未写预设处理：值 " + std::to_string(static_cast<int>(param.borderColorType)));
				break;
			}}

			//校验 开了比较却用整数越界色 ⇒ 采样器创建会失败
			if (param.compareType != samplerEnum::compareId::off && borderColor == VK_BORDER_COLOR_INT_OPAQUE_BLACK) {
				Log::printError(__FILE__, __LINE__, "比较采样（compareId != off）必须用 FLOAT 系越界色");
			}

			return borderColor;
		}

		//映射 maxLod 来源预设 → maxLod（唯一从运行期取值的维度）
		float createMaxLod(const samplerConfigParamNode& param) const {
			float maxLod = 0.0f;

			switch (param.lodSourceType) {
			case samplerEnum::lodSourceId::none:      { maxLod = 0.0f;				 break; }	//无：不运算
			case samplerEnum::lodSourceId::clampNone: { maxLod = VK_LOD_CLAMP_NONE;	 break; }	//不限制：由纹理 mip 链兜底
			default: {
				Log::printError(__FILE__, __LINE__, "samplerLodSourceId 未写预设处理：值 " + std::to_string(static_cast<int>(param.lodSourceType)));
				break;
			}}

			return maxLod;
		}
	};
}

//desc		结构空间
namespace descStruct {
	//描述符 配置结点
	struct descConfigNode {
		VkDescriptorSetLayoutBinding layout{};
		VkWriteDescriptorSet         write{};



		descConfigNode(uint32_t id, VkDescriptorType type, VkShaderStageFlags stage, uint32_t count = 1) {
			layout.descriptorCount	  = count;
			layout.pImmutableSamplers = nullptr;
			layout.binding			  = id;
			layout.descriptorType	  = type;
			layout.stageFlags		  = stage;

			write.sType			  = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write.dstArrayElement = 0;
			write.descriptorCount = 1;
			write.dstBinding	  = id;
			write.descriptorType  = type;
		}

		descConfigNode() = delete;



		const VkWriteDescriptorSet& setWrite(const VkDescriptorSet& dstSet, const VkBuffer& buffer, VkDeviceSize range) {
			bufferInfo.offset = 0;
			bufferInfo.buffer = buffer;
			bufferInfo.range  = range;

			write.dstSet	  = dstSet;
			write.pBufferInfo = &bufferInfo;

			return write;
		}

		const VkWriteDescriptorSet& setWrite(const VkDescriptorSet& dstSet, const VkSampler& sampler, const VkImageView& imageView) {
			imageInfo.sampler	  = sampler;
			imageInfo.imageView	  = imageView;
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			write.dstSet	 = dstSet;
			write.pImageInfo = &imageInfo;

			return write;
		}

	private:
		VkDescriptorBufferInfo bufferInfo{};
		VkDescriptorImageInfo  imageInfo{};
	};

	//描述符 写入结点
	struct descWriteNode {
		uint32_t         binding{};
		VkDescriptorType type{};

		VkBuffer         buffer{};
		VkDeviceSize     range{};
		VkSampler        sampler{};
		VkImageView      view{};
	};

	//描述符 反射配置结点
	struct descReflectConfigNode {
		pipelineEnum::procPipelineId::type pipeType{};
		std::vector<std::string>           shaderFileList{};
	};

	//spv 数据结点
	struct spvDataNode {
		const spvStruct::moduleNode* module(const std::string& file) const {
			for (const spvStruct::moduleNode* module : spvStruct::moduleList) {
				if (module->file == file) { return module; }
			}

			if (debugInfo::LogSwitch) { 
				Log::printError(__FILE__, __LINE__, "该 spv 不在反射表里（请重新构建）：" + file); 
			}

			return nullptr;
		}
	};

	inline spvDataNode spvData{};

	//spv 文件 → 着色器阶段
	VkShaderStageFlagBits reflectStage(const std::string& filename) {
		const spvStruct::moduleNode* module = spvData.module(filename);

		return module == nullptr ? static_cast<VkShaderStageFlagBits>(0) : module->stage;
	}
	
	//spv 文件 → 顶点输入表
	std::span<const spvStruct::inputNode> reflectInput(const std::string& filename) {
		const spvStruct::moduleNode* module = spvData.module(filename);

		return module == nullptr ? std::span<const spvStruct::inputNode>{} : std::span<const spvStruct::inputNode>{ module->inputList, module->inputCount };
	}

	//spv 文件 → 描述绑定表
	std::span<const spvStruct::bindingNode> reflectFile(const std::string& filename) {
		const spvStruct::moduleNode* module = spvData.module(filename);

		return module == nullptr ? std::span<const spvStruct::bindingNode>{} : std::span<const spvStruct::bindingNode>{ module->bindingList, module->bindingCount };
	}



	//描述符 结点
	struct descNode {
		std::vector<descStruct::descConfigNode> descConfig{};
		std::vector<descReflectConfigNode>		descReflectConfigList{};

		VkDescriptorPool			 pool{};			//描述符池
		VkDescriptorSetLayout		 layout{};			//描述符布局
		uint32_t					 maxSet{};			//描述符池上限
		bufferStruct::descBufferNode descBuffer{};		//描述符缓冲



		void createReflectConfig(pipelineEnum::procPipelineId::type tPipeType, std::vector<std::string> tShaderFileList) {
			descReflectConfigList.push_back(descReflectConfigNode{ tPipeType, std::move(tShaderFileList) });
		}
		
		void createReflectLayout(descEnum::descType::type tDescType, const VkDevice& device) {
			for (const descReflectConfigNode& reflectConfig : descReflectConfigList) {
				for (const std::string& file : reflectConfig.shaderFileList) {
					for (const spvStruct::bindingNode& reflect : descStruct::reflectFile(file)) {
						//当前布局只支持单集合（GLSL 无 set 语法时反射出的集合号恒为 0）
						if (debugInfo::LogSwitch && reflect.set != 0) {
							Log::printError(__FILE__, __LINE__, pipelineInfo::procDescName[tDescType] + " 运行时 spv 的描述符集号非 0：" + reflect.name);
						}

						bool isMerge = false;
						for (descStruct::descConfigNode& merge : descConfig) {
							if (merge.layout.binding != reflect.binding) { continue; }

							if (debugInfo::LogSwitch && merge.layout.descriptorType != reflect.type) {
								Log::printError(__FILE__, __LINE__, pipelineInfo::procDescName[tDescType] + " 绑定 " + std::to_string(reflect.binding) + " 在不同着色器里类型不一致");
							}

							if (debugInfo::LogSwitch && merge.layout.descriptorCount != reflect.count) {
								Log::printError(__FILE__, __LINE__, pipelineInfo::procDescName[tDescType] + " 绑定 " + std::to_string(reflect.binding) + " 在不同着色器里数组长度不一致");
							}

							merge.layout.stageFlags = static_cast<VkShaderStageFlags>(merge.layout.stageFlags | reflect.stage);
							isMerge = true;
							break;
						}

						if (!isMerge) {
							descConfig.push_back(descStruct::descConfigNode{ reflect.binding, reflect.type, static_cast<VkShaderStageFlags>(reflect.stage), reflect.count });
						}
					}
				}
			}

			createDescNode(device);
		}

		void createDescNode(const VkDevice& device) {
			createPool(device);
			createLayout(device);
		}

		void clear(const VkDevice& device) {
			vkDestroyDescriptorPool(device, pool, nullptr);
			vkDestroyDescriptorSetLayout(device, layout, nullptr);
		}



		//分配
		bool alloc(const VkDevice& device, VkDescriptorSet& dstSet, pipelineEnum::procPipelineId::type pipeType) {
			VkDescriptorSetAllocateInfo setAllocInfo{
				.sType				= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
				.descriptorPool		= pool,
				.descriptorSetCount = 1,
				.pSetLayouts	    = &layout
			};

			if (vkAllocateDescriptorSets(device, &setAllocInfo, &dstSet) != VK_SUCCESS) {
				Log::printError(__FILE__, __LINE__, pipelineInfo::procPipelineName[pipeType] + "分配描述符集失败");
				return false;
			}

			return true;
		}

		//写入
		void writeFrom(const VkDevice& device, const VkDescriptorSet& dstSet, const std::vector<descWriteNode>& requestList) {
			std::vector<VkWriteDescriptorSet> writeList{};
			writeList.reserve(requestList.size());

			for (const auto& request : requestList) {
				if (debugInfo::LogSwitch && (static_cast<size_t>(request.binding) >= descConfig.size() || descConfig[request.binding].layout.binding != request.binding)) {
					Log::printError(__FILE__, __LINE__, "描述符写入的 binding 与布局表不一致");
				}

				if (debugInfo::LogSwitch && descConfig[request.binding].layout.descriptorType != request.type) {
					Log::printError(__FILE__, __LINE__, "描述符写入的类型与布局表不一致");
				}

				if (request.type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
					writeList.push_back(descConfig[request.binding].setWrite(dstSet, request.sampler, request.view));
				}
				else {
					writeList.push_back(descConfig[request.binding].setWrite(dstSet, request.buffer, request.range));
				}
			}

			vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeList.size()), writeList.data(), 0, nullptr);
		}

		//获取 着色器文件列表
		const std::vector<std::string>& getShaderFileList(pipelineEnum::procPipelineId::type tPipeType) const {
			for (const descReflectConfigNode& reflectConfig : descReflectConfigList) {
				if (reflectConfig.pipeType == tPipeType) { return reflectConfig.shaderFileList; }
			}

			if (debugInfo::LogSwitch) { Log::printError(__FILE__, __LINE__, "描述符家族未登记该管线的着色器文件：" + pipelineInfo::procPipelineName[tPipeType]); }

			static const std::vector<std::string> emptyList{};
			return emptyList;
		}

	private:
		void createPool(const VkDevice& device) {
			if (debugInfo::LogSwitch && maxSet == 0) {
				Log::printError(__FILE__, __LINE__, "描述符池上限（maxSet）未设置");
			}

			//池需求 = maxSet × 每个 set 里该 binding 的数组长度；同类描述符按类型汇总
			std::vector<VkDescriptorPoolSize> poolSize{};
			for (const auto& i : descConfig) {
				uint32_t need = maxSet * i.layout.descriptorCount;

				bool merged = false;
				for (auto& size : poolSize) {
					if (size.type == i.layout.descriptorType) {
						size.descriptorCount += need;
						merged = true;
						break;
					}
				}

				if (!merged) {
					poolSize.push_back(VkDescriptorPoolSize{ .type = i.layout.descriptorType, .descriptorCount = need });
				}
			}

			VkDescriptorPoolCreateInfo poolInfo{
				.sType		   = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
				.maxSets	   = maxSet,
				.poolSizeCount = static_cast<uint32_t>(poolSize.size()),
				.pPoolSizes	   = poolSize.data()
			};

			if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &pool) != VK_SUCCESS) {
				Log::printError(__FILE__, __LINE__, "描述符池创建失败");
			}
		}

		void createLayout(const VkDevice& device) {
			std::vector<VkDescriptorSetLayoutBinding> layoutBinding{};
			for (const auto& i : descConfig) {
				layoutBinding.push_back(i.layout);
			}

			VkDescriptorSetLayoutCreateInfo layoutInfo{
				.sType		  = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
				.pNext		  = nullptr,
				.bindingCount = static_cast<uint32_t>(layoutBinding.size()),
				.pBindings	  = layoutBinding.data()
			};

			if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &layout) != VK_SUCCESS) {
				Log::printError(__FILE__, __LINE__, "描述符布局配置失败");
			}
		}
	};
}

//vertex	结构空间
namespace vertexStruct {
	//顶点绑定配置结点
	struct vertexBindingConfigNode {
		size_t						stride = 0;								//绑定步长
		vertexEnum::rateId::type	rateType = vertexEnum::rateId::vertex;	//输入速率
	};
	
	//顶点属性配置结点
	struct vertexAttributeConfigNode {
		vertexEnum::formatId::type formatType = vertexEnum::formatId::float3;	//格式
		uint32_t				   binding	  = 0;								//绑定，通过不同的 offset 和 format 从同一个 buffer 里“切”出不同的字段，如果数据都在同一个 buffer 里，binding 都是 0
		uint32_t				   offset	  = 0;								//偏移
	};

	//顶点布局配置结点
	struct vertexLayoutConfigNode {
		std::vector<vertexBindingConfigNode>   binding{};
		std::vector<vertexAttributeConfigNode> attrib{};
	};

	//顶点布局结点
	struct vertexLayoutNode {
		std::vector<VkVertexInputBindingDescription>   bindingList{};	//绑定列表
		std::vector<VkVertexInputAttributeDescription> attributeList{};	//属性列表



		void createVertexLayoutNode(const vertexLayoutConfigNode& tLayoutConfig) {
			bindingList.resize(tLayoutConfig.binding.size());
			for (size_t i = 0; i < tLayoutConfig.binding.size(); i++) {
				bindingList[i].binding   = static_cast<uint32_t>(i);
				bindingList[i].stride    = static_cast<uint32_t>(tLayoutConfig.binding[i].stride);
				bindingList[i].inputRate = createRate(tLayoutConfig.binding[i].rateType);
			}

			attributeList.resize(tLayoutConfig.attrib.size());
			for (size_t i = 0; i < tLayoutConfig.attrib.size(); i++) {
				attributeList[i].binding  = tLayoutConfig.attrib[i].binding;
				attributeList[i].location = static_cast<uint32_t>(i);
				attributeList[i].format   = createFormat(tLayoutConfig.attrib[i].formatType);
				attributeList[i].offset   = tLayoutConfig.attrib[i].offset;
			}
		}

	private:
		VkVertexInputRate createRate(vertexEnum::rateId::type tRateType) {
			VkVertexInputRate rate = VK_VERTEX_INPUT_RATE_VERTEX;

			switch (tRateType) {
			case vertexEnum::rateId::vertex:   { rate = VK_VERTEX_INPUT_RATE_VERTEX;	break; }	//逐顶点
			case vertexEnum::rateId::instance: { rate = VK_VERTEX_INPUT_RATE_INSTANCE;	break; }	//逐实例
			default: {
				Log::printError(__FILE__, __LINE__, "rateId 未写预设处理：值 " + std::to_string(static_cast<int>(tRateType)));
				break;
			}}

			return rate;
		}

		VkFormat createFormat(vertexEnum::formatId::type formatType) {
			VkFormat format = VK_FORMAT_UNDEFINED;

			switch (formatType) {
			case vertexEnum::formatId::float2: { format = VK_FORMAT_R32G32_SFLOAT;		break; }	//2×float：uiText 字形位置/纹理坐标
			case vertexEnum::formatId::float3: { format = VK_FORMAT_R32G32B32_SFLOAT;	break; }	//3×float：坐标 / uv3 / 法线
			default: {
				Log::printError(__FILE__, __LINE__, "formatId 未写预设处理：值 " + std::to_string(static_cast<int>(formatType)));
				break;
			}}

			return format;
		}
	};

	//顶点网格结点：CPU 侧网格 = 顶点 + 索引
	template<typename VertexNode>
	struct vertexMeshNode {
		std::vector<VertexNode>	vertexs{};	//顶点数组
		std::vector<uint32_t>	indexs{};	//索引数组
	};



	//pipeTerrain
	namespace terrainVertex {
		struct vertexNode {
			glm::vec3 position;		//坐标									
			glm::vec3 texCoord;		//uv，xy用于循环平铺，z用于纹理数组索引	
			glm::vec3 normal;		//法线									



			static vertexLayoutNode createLayout() {
				vertexLayoutNode layout{};
				layout.createVertexLayoutNode(vertexLayoutConfigNode{
					.binding = { vertexBindingConfigNode{ .stride = sizeof(vertexNode) } },
					.attrib = {
						vertexAttributeConfigNode{ .offset = static_cast<uint32_t>(offsetof(vertexNode, position)) },
						vertexAttributeConfigNode{ .offset = static_cast<uint32_t>(offsetof(vertexNode, texCoord)) },
						vertexAttributeConfigNode{ .offset = static_cast<uint32_t>(offsetof(vertexNode, normal)) },
					},
				});

				return layout;
			}

			inline static const vertexLayoutNode layout{ createLayout() };
		};

		vertexMeshNode<vertexNode> mesh[plateEnum::terrainLayer::enumCount];	//顶点网格（按层）
	}

	//model
	namespace modelVertex {
		struct vertexNode {
			glm::vec3 position;		//坐标									
			glm::vec2 texCoord;		//uv									
			glm::vec3 normal;		//法线									



			static vertexLayoutNode createLayout() {
				vertexLayoutNode layout{};
				layout.createVertexLayoutNode(vertexLayoutConfigNode{
					.binding = { vertexBindingConfigNode{ .stride = sizeof(vertexNode) } },
					.attrib   = {
						vertexAttributeConfigNode{ .offset = static_cast<uint32_t>(offsetof(vertexNode, position)) },
						vertexAttributeConfigNode{ .formatType = vertexEnum::formatId::float2, .offset = static_cast<uint32_t>(offsetof(vertexNode, texCoord)) },
						vertexAttributeConfigNode{ .offset = static_cast<uint32_t>(offsetof(vertexNode, normal)) },
					},
				});

				return layout;
			}

			inline static const vertexLayoutNode layout{ createLayout() };

			//服务loadModel
			bool operator==(const vertexNode& other) const {
				return position == other.position && texCoord == other.texCoord && normal == other.normal;
			}
		};

		//服务loadModel：顶点去重用的哈希（原 std::hash<vertexNode> 特化，改为本命名空间内的自定义 hasher）
		struct vertexNodeHash {
			size_t operator()(const vertexNode& vertex) const {
				return (std::hash<glm::vec3>()(vertex.position) ^ (std::hash<glm::vec3>()(vertex.normal) << 1) ^ (std::hash<glm::vec2>()(vertex.texCoord) << 2));
			}
		};

		vertexMeshNode<vertexNode>				sMesh[bufferEnum::procStaticModelId::enumCount];	//静态模型网格
		std::vector<vertexMeshNode<vertexNode>>	dMesh[bufferEnum::procDynamicModelId::enumCount];	//动态模型网格
	}

	//pipeSkybox
	namespace skyboxVertex {
		struct vertexNode {
			glm::vec3 position;		//坐标									



			static vertexLayoutNode createLayout() {
				vertexLayoutNode layout{};
				layout.createVertexLayoutNode(vertexLayoutConfigNode{
					.binding = { vertexBindingConfigNode{ .stride = sizeof(vertexNode) } },
					.attrib   = {
						vertexAttributeConfigNode{ .offset = static_cast<uint32_t>(offsetof(vertexNode, position)) },
					},
				});

				return layout;
			}

			inline static const vertexLayoutNode layout{ createLayout() };
		};

		//顶点数组
		std::vector<vertexNode> vertexs = {
			{glm::vec3{ 16.0f, 16.0f,-16.0f}},
			{glm::vec3{-16.0f, 16.0f,-16.0f}},
			{glm::vec3{-16.0f,-16.0f,-16.0f}},
			{glm::vec3{ 16.0f,-16.0f,-16.0f}},
						
			{glm::vec3{ 16.0f, 16.0f, 16.0f}},
			{glm::vec3{-16.0f, 16.0f, 16.0f}},
			{glm::vec3{-16.0f,-16.0f, 16.0f}},
			{glm::vec3{ 16.0f,-16.0f, 16.0f}}
		};

		//索引数组
		std::vector<uint32_t> indexs = {
			1,4,5,0,4,1,
			6,3,2,7,3,6,
			3,4,0,7,4,3,
			6,1,5,2,1,6,
			7,5,4,6,5,7,
			2,0,1,3,0,2,
		};
	}

	//pipeMine
	namespace mineVertex {
		struct vertexNode {
			glm::vec3 position;		//坐标									



			static vertexLayoutNode createLayout() {
				vertexLayoutNode layout{};
				layout.createVertexLayoutNode(vertexLayoutConfigNode{
					.binding = { vertexBindingConfigNode{ .stride = sizeof(vertexNode) } },
					.attrib   = {
						vertexAttributeConfigNode{ .offset = static_cast<uint32_t>(offsetof(vertexNode, position)) },
					},
				});

				return layout;
			}

			inline static const vertexLayoutNode layout{ createLayout() };
		};

		//顶点数组
		std::vector<vertexNode> vertexs = { 
			{vertexInfo::vertexID[0]},
			{vertexInfo::vertexID[1]},
			{vertexInfo::vertexID[2]},
			{vertexInfo::vertexID[3]},

			{vertexInfo::vertexID[4]},
			{vertexInfo::vertexID[5]},
			{vertexInfo::vertexID[6]},
			{vertexInfo::vertexID[7]}
		};

		//索引数组 面
		/*std::vector<uint32_t> indexs = {
			5,4,1,1,4,0,
			2,3,6,6,3,7,
			0,4,3,3,4,7,
			5,1,6,6,1,2,
			4,5,7,7,5,6,
			1,0,2,2,0,3
		};*/

		//索引数组 线
		std::vector<uint32_t> indexs = {
			0,1,1,5,5,4,4,0,
			6,2,2,3,3,7,7,6,
			7,3,3,0,0,4,4,7,
			1,2,2,6,6,5,5,1,
			2,3,3,0,0,1,1,2,
			4,5,5,6,6,7,7,4
		};
	}

	//pipeUIText
	namespace uiTextVertex {
		struct vertexNode {
			static vertexLayoutNode createLayout() {
				vertexLayoutNode layout{};
				layout.createVertexLayoutNode(vertexLayoutConfigNode{
					.binding = { vertexBindingConfigNode{ .stride = sizeof(glm::vec4) }, vertexBindingConfigNode{ .stride = sizeof(glm::vec4) } },
					.attrib   = {
						vertexAttributeConfigNode{ .formatType = vertexEnum::formatId::float2, .offset = 0 },
						vertexAttributeConfigNode{ .formatType = vertexEnum::formatId::float2, .binding = 1, .offset = sizeof(glm::vec2) },
					},
				});

				return layout;
			}

			inline static const vertexLayoutNode layout{ createLayout() };
		};
	}

	//pipeOSMirror
	namespace osMirrorVertex {
		struct vertexNode {
			glm::vec3 position;



			static vertexLayoutNode createLayout() {
				vertexLayoutNode layout{};
				layout.createVertexLayoutNode(vertexLayoutConfigNode{
					.binding = { vertexBindingConfigNode{ .stride = sizeof(vertexNode) } },
					.attrib   = {
						vertexAttributeConfigNode{ .offset = static_cast<uint32_t>(offsetof(vertexNode, position)) },
					},
				});

				return layout;
			}

			inline static const vertexLayoutNode layout{ createLayout() };
		};

		//顶点数组
		std::vector<vertexNode> vertexs = {
			{vertexInfo::vertexID[0]},
			{vertexInfo::vertexID[1]},
			{vertexInfo::vertexID[2]},
			{vertexInfo::vertexID[3]},

			{vertexInfo::vertexID[4]},
			{vertexInfo::vertexID[5]},
			{vertexInfo::vertexID[6]},
			{vertexInfo::vertexID[7]}
		};

		//索引数组 面
		std::vector<uint32_t> indexs = {
			4,5,7,7,5,6,
		};
	}

	//offScreen
	namespace offScreenVertex {
		struct vertexNode {
			static vertexLayoutNode createLayout() {
				vertexLayoutNode layout{};
				layout.createVertexLayoutNode(vertexLayoutConfigNode{});

				return layout;
			}

			inline static const vertexLayoutNode layout{ createLayout() };
		};
	}

	//shadow
	namespace shadowVertex {
		struct vertexNode {
			glm::vec3 position;		//坐标									



			static vertexLayoutNode createLayout() {
				vertexLayoutNode layout{};
				layout.createVertexLayoutNode(vertexLayoutConfigNode{
					.binding = { vertexBindingConfigNode{ .stride = sizeof(vertexNode) } },
					.attrib   = {
						vertexAttributeConfigNode{ .offset = static_cast<uint32_t>(offsetof(vertexNode, position)) },
					},
				});

				return layout;
			}

			inline static const vertexLayoutNode layout{ createLayout() };
		};

		vertexMeshNode<vertexNode> tMesh[plateEnum::terrainLayer::enumCount];						//地形网格
		vertexMeshNode<vertexNode> sMesh[bufferEnum::procStaticModelId::enumCount];					//静态模型网格
		std::vector<vertexMeshNode<vertexNode>> dMesh[bufferEnum::procDynamicModelId::enumCount];	//动态模型网格



		//缩位生成地形阴影顶点（只保位置）；shadow 是"有意的第二份"⇒ 生成后必须与源网格对拍
		void loadTerrainVertexs(int layer) {
			const std::vector<terrainVertex::vertexNode>& sourceVerts = terrainVertex::mesh[layer].vertexs;
			std::vector<vertexNode>&					  targetVerts = tMesh[layer].vertexs;

			const size_t vertCount = sourceVerts.size();
			targetVerts.clear();
			targetVerts.reserve(vertCount);

			for (size_t i = 0; i < vertCount; ++i) {
				targetVerts.push_back(vertexNode{ sourceVerts[i].position });
			}

			if (targetVerts.size() != vertCount) {
				Log::printError(__FILE__, __LINE__, "地形阴影顶点缩位条数与源网格不一致：" + std::to_string(targetVerts.size()) + " ≠ " + std::to_string(vertCount));
			}
		}
	}
}

//pipeline  结构空间
namespace pipelineStruct {
	//pipeConfig 传参结点
	struct pipeConfigParamNode {
		pipelineEnum::primitiveId::type     primitiveType = pipelineEnum::primitiveId::triangleList;	//图元
		pipelineEnum::rasterId::type        rasterType	  = pipelineEnum::rasterId::fill_back_ccw;		//光栅
		pipelineEnum::biasId::type          biasType	  = pipelineEnum::biasId::polygonOffset;		//深度偏移
		pipelineEnum::depthId::type         depthType	  = pipelineEnum::depthId::test_write_less;		//深度
		pipelineEnum::stencilId::type       stencilType	  = pipelineEnum::stencilId::none;				//模板
		pipelineEnum::blendId::type         blendType	  = pipelineEnum::blendId::opaque;				//颜色混合
		pipelineEnum::writeMaskId::type     writeMaskType = pipelineEnum::writeMaskId::rgba;			//颜色写入
		pipelineEnum::logicOpId::type       logicOpType	  = pipelineEnum::logicOpId::disable;			//颜色运算
		pipelineEnum::dynamicId::type       dynamicType	  = pipelineEnum::dynamicId::none;				//动态状态
	};

	//pipeConfig 结点
	struct pipeConfigNode {
		//预设指定
		VkPipelineInputAssemblyStateCreateInfo			 primitiveState{};	        //图元配置
		VkPipelineRasterizationStateCreateInfo			 rasterizer{};			    //光栅化
		VkPipelineDepthStencilStateCreateInfo			 depthStencil{};		    //深度和模板
		VkPipelineColorBlendStateCreateInfo				 colorAttachmentState{};    //颜色附件配置
		std::vector<VkPipelineColorBlendAttachmentState> colorAttachmentList{};     //颜色附件列表（前向模式、延迟模式、阴影模式）
		std::vector<VkDynamicState>                      pipeDynamicStageList{};    //管线动态阶段列表
		VkPipelineDynamicStateCreateInfo                 pipeDynamicStageState{};   //管线动态阶段配置

		//运行指定
		std::vector<VkPipelineShaderStageCreateInfo>     shaderStageList{};         //着色阶段列表（顶点模式、网格模式）
		VkPipelineVertexInputStateCreateInfo             vertInputStage{};          //顶点配置
		std::vector<VkVertexInputBindingDescription>     vertBindingList{};         //顶点绑定
		std::vector<VkVertexInputAttributeDescription>   vertAttributeList{};       //顶点属性
		VkViewport										 viewport{};			    //视口区域
		VkRect2D										 scissor{};				    //裁剪区域
		VkPipelineViewportStateCreateInfo				 viewportState{};		    //视口配置
		VkPipelineMultisampleStateCreateInfo			 multisampling{};		    //多重采样
		VkGraphicsPipelineCreateInfo					 pipeInfo{};			    //管线信息


		
		//创建 管线配置
		void createPipeConfigNode(std::span<const std::string> shaderFileList, const vertexStruct::vertexLayoutNode& vertexLayout, VkSampleCountFlagBits msaa, const renderStruct::renderTemplateNode& tRenderTemplateNode, VkPipelineLayout pipeLayout, const pipeConfigParamNode& param, const VkDevice& device) {
			primitiveState        = createPrimitive   (param.primitiveType);										//图元
			rasterizer            = createRaster	  (param.rasterType, param.biasType);							//光栅
			depthStencil          = createDepthStencil(param.depthType, param.stencilType);							//深度 + 模板
			colorAttachmentState  = createBlend       (param.blendType, param.writeMaskType, param.logicOpType);	//颜色混合
			pipeDynamicStageState = createDynamic     (param.dynamicType);											//动态状态

			//着色阶段列表
			createShaderStageList(shaderFileList, device);

			//顶点输入
			vertBindingList   = vertexLayout.bindingList;
			vertAttributeList = vertexLayout.attributeList;

			vertInputStage.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertInputStage.vertexBindingDescriptionCount   = static_cast<uint32_t>(vertBindingList.size());
			vertInputStage.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertAttributeList.size());
			vertInputStage.pVertexBindingDescriptions	   = vertBindingList.data();
			vertInputStage.pVertexAttributeDescriptions    = vertAttributeList.data();

			//视口区域
			viewport.x		  = 0.0f;
			viewport.y		  = 0.0f;
			//viewport.width    = (float)extent.width;
			//viewport.height   = (float)extent.height;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;

			//裁剪区域
			scissor.offset = { 0, 0 };
			//scissor.extent = extent;

			//视口配置
			viewportState.sType			= VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportState.viewportCount = 1;
			viewportState.pViewports	= &viewport;
			viewportState.scissorCount	= 1;
			viewportState.pScissors		= &scissor;

			//多重采样
			multisampling.sType					= VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multisampling.sampleShadingEnable	= VK_FALSE;
			multisampling.rasterizationSamples	= msaa;
			multisampling.minSampleShading		= 1.0f;													
			multisampling.pSampleMask			= nullptr;													
			multisampling.alphaToCoverageEnable = VK_FALSE;											
			multisampling.alphaToOneEnable		= VK_FALSE;												

			//颜色附件：按渲染目标模式决定数量，前向 1、延迟 3、阴影 0，并用第 1 项（createBlend 铺的配置模板）填充
			uint32_t attachCount = 0;
			switch (tRenderTemplateNode.renderAttType) {
			case renderEnum::renderAttId::forward_ra:
				attachCount = 1;
				break;
			case renderEnum::renderAttId::deferred_ra:
				attachCount = 3;
				break;
			case renderEnum::renderAttId::shadowMap_ra:
				attachCount = 0;
				break;
			}

			VkPipelineColorBlendAttachmentState attachmentTemplate = colorAttachmentList.empty() ? VkPipelineColorBlendAttachmentState{} : colorAttachmentList[0];
			colorAttachmentList.resize(attachCount, attachmentTemplate);
			colorAttachmentState.attachmentCount = static_cast<uint32_t>(colorAttachmentList.size());
			colorAttachmentState.pAttachments	 = colorAttachmentList.data();

			//管线信息
			pipeInfo.sType				 = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipeInfo.stageCount          = static_cast<uint32_t>(shaderStageList.size());
			pipeInfo.pStages             = shaderStageList.data();
			pipeInfo.pVertexInputState   = &vertInputStage;
			pipeInfo.pInputAssemblyState = &primitiveState;
			pipeInfo.pViewportState		 = &viewportState;
			pipeInfo.pRasterizationState = &rasterizer;
			pipeInfo.pMultisampleState	 = &multisampling;
			pipeInfo.pDepthStencilState  = &depthStencil;
			pipeInfo.pColorBlendState	 = &colorAttachmentState;
			pipeInfo.pDynamicState		 = &pipeDynamicStageState;
			pipeInfo.layout			     = pipeLayout;
			pipeInfo.renderPass          = tRenderTemplateNode.render;
			pipeInfo.subpass			 = 0;
			pipeInfo.basePipelineHandle  = VK_NULL_HANDLE;		
			pipeInfo.basePipelineIndex	 = -1;
		}

		void clear(const VkDevice& device) {
			for (const VkPipelineShaderStageCreateInfo& shaderStage : shaderStageList) {
				if (shaderStage.module != VK_NULL_HANDLE) { vkDestroyShaderModule(device, shaderStage.module, nullptr); }
			}

			shaderStageList.clear();
		}

	private:
		//创建 图元
		VkPipelineInputAssemblyStateCreateInfo createPrimitive(pipelineEnum::primitiveId::type primitiveType) {
			VkPipelineInputAssemblyStateCreateInfo primitive{};
			primitive.sType					 = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			primitive.primitiveRestartEnable = VK_FALSE;				//关闭图元重启（仅对 triangleStrip / lineStrip 生效）

			switch (primitiveType) {
			case pipelineEnum::primitiveId::pointList:                  { primitive.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;					break; }	//点列表：		粒子、调试点云
			case pipelineEnum::primitiveId::lineList:                   { primitive.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;						break; }	//线段列表：	线框、选择框描边
			case pipelineEnum::primitiveId::lineStrip:                  { primitive.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;					break; }	//线段条带：	轨迹、曲线可视化
			case pipelineEnum::primitiveId::triangleList:               { primitive.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;					break; }	//三角形列表：	网格/地形/模型主力（＝现有硬编码）
			case pipelineEnum::primitiveId::triangleStrip:              { primitive.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;				break; }	//三角形条带：	UI 四边形、带状网格
			case pipelineEnum::primitiveId::triangleFan:                { primitive.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;					break; }	//三角形扇：	扇形/锥体（GL 遗留，条带+退化索引可替代）
			case pipelineEnum::primitiveId::lineListWithAdjacency:      { primitive.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY;		break; }	//邻接线段列表（需 geometryShader）
			case pipelineEnum::primitiveId::lineStripWithAdjacency:     { primitive.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY;		break; }	//邻接线段条带
			case pipelineEnum::primitiveId::triangleListWithAdjacency:  { primitive.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY;	break; }	//邻接三角形列表：轮廓/法线外扩
			case pipelineEnum::primitiveId::triangleStripWithAdjacency: { primitive.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY; break; }	//邻接三角形条带
			case pipelineEnum::primitiveId::patchList:                  { primitive.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;					break; }	//面片列表（需 tessellationShader）
			default: {
				Log::printError(__FILE__, __LINE__, "primitiveId 未写预设处理：值 " + std::to_string(static_cast<int>(primitiveType)));
				break;
			}}

			return primitive;
		}

		//创建 光栅
		VkPipelineRasterizationStateCreateInfo createRaster(pipelineEnum::rasterId::type rasterType, pipelineEnum::biasId::type biasType) {
			VkPipelineRasterizationStateCreateInfo rasterState{};
			rasterState.sType					= VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterState.lineWidth				= 1.0f;				//固定常量：线宽
			rasterState.depthClampEnable		= VK_FALSE;			//仅 fill_back_ccw_clampDepth 会改：夹紧深度
			rasterState.rasterizerDiscardEnable	= VK_FALSE;			//仅 discardAll				  会改：丢弃全部图元

			switch (rasterType) {
			case pipelineEnum::rasterId::fill_none_ccw: {					//填充·不剔·逆时针：双面薄片
				rasterState.polygonMode = VK_POLYGON_MODE_FILL;
				rasterState.cullMode    = VK_CULL_MODE_NONE;
				rasterState.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
				break;
			}
			case pipelineEnum::rasterId::fill_none_cw: {					//填充·不剔·顺时针
				rasterState.polygonMode = VK_POLYGON_MODE_FILL;
				rasterState.cullMode    = VK_CULL_MODE_NONE;
				rasterState.frontFace   = VK_FRONT_FACE_CLOCKWISE;
				break;
			}
			case pipelineEnum::rasterId::fill_front_ccw: {					//填充·剔正面·逆时针：天空盒内侧/镜像
				rasterState.polygonMode = VK_POLYGON_MODE_FILL;
				rasterState.cullMode    = VK_CULL_MODE_FRONT_BIT;
				rasterState.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
				break;
			}
			case pipelineEnum::rasterId::fill_front_cw: {					//填充·剔正面·顺时针
				rasterState.polygonMode = VK_POLYGON_MODE_FILL;
				rasterState.cullMode    = VK_CULL_MODE_FRONT_BIT;
				rasterState.frontFace   = VK_FRONT_FACE_CLOCKWISE;
				break;
			}
			case pipelineEnum::rasterId::fill_back_ccw: {					//填充·剔背面·逆时针：不透明实体标准（＝现有硬编码）
				rasterState.polygonMode = VK_POLYGON_MODE_FILL;
				rasterState.cullMode    = VK_CULL_MODE_BACK_BIT;
				rasterState.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
				break;
			}
			case pipelineEnum::rasterId::fill_back_cw: {					//填充·剔背面·顺时针：镜像资源
				rasterState.polygonMode = VK_POLYGON_MODE_FILL;
				rasterState.cullMode    = VK_CULL_MODE_BACK_BIT;
				rasterState.frontFace   = VK_FRONT_FACE_CLOCKWISE;
				break;
			}
			case pipelineEnum::rasterId::fill_front_and_back: {				//填充·双面·逆时针：厚薄片/透明排序
				rasterState.polygonMode = VK_POLYGON_MODE_FILL;
				rasterState.cullMode    = VK_CULL_MODE_FRONT_AND_BACK;
				rasterState.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
				break;
			}
			case pipelineEnum::rasterId::line_none_ccw: {					//线框·不剔·逆时针：调试/技术图
				rasterState.polygonMode = VK_POLYGON_MODE_LINE;
				rasterState.cullMode    = VK_CULL_MODE_NONE;
				rasterState.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
				break;
			}
			case pipelineEnum::rasterId::line_none_cw: {					//线框·不剔·顺时针
				rasterState.polygonMode = VK_POLYGON_MODE_LINE;
				rasterState.cullMode    = VK_CULL_MODE_NONE;
				rasterState.frontFace   = VK_FRONT_FACE_CLOCKWISE;
				break;
			}
			case pipelineEnum::rasterId::line_front_ccw: {					//线框·剔正面·逆时针：内轮廓
				rasterState.polygonMode = VK_POLYGON_MODE_LINE;
				rasterState.cullMode    = VK_CULL_MODE_FRONT_BIT;
				rasterState.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
				break;
			}
			case pipelineEnum::rasterId::line_front_cw: {					//线框·剔正面·顺时针
				rasterState.polygonMode = VK_POLYGON_MODE_LINE;
				rasterState.cullMode    = VK_CULL_MODE_FRONT_BIT;
				rasterState.frontFace   = VK_FRONT_FACE_CLOCKWISE;
				break;
			}
			case pipelineEnum::rasterId::line_back_ccw: {					//线框·剔背面·逆时针：选择框描边/隐藏线
				rasterState.polygonMode = VK_POLYGON_MODE_LINE;
				rasterState.cullMode    = VK_CULL_MODE_BACK_BIT;
				rasterState.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
				break;
			}
			case pipelineEnum::rasterId::line_back_cw: {					//线框·剔背面·顺时针
				rasterState.polygonMode = VK_POLYGON_MODE_LINE;
				rasterState.cullMode    = VK_CULL_MODE_BACK_BIT;
				rasterState.frontFace   = VK_FRONT_FACE_CLOCKWISE;
				break;
			}
			case pipelineEnum::rasterId::line_front_and_back: {				//线框·双面·逆时针：全部边线
				rasterState.polygonMode = VK_POLYGON_MODE_LINE;
				rasterState.cullMode    = VK_CULL_MODE_FRONT_AND_BACK;
				rasterState.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
				break;
			}
			case pipelineEnum::rasterId::point_none_ccw: {					//点·不剔·逆时针：顶点可视化
				rasterState.polygonMode = VK_POLYGON_MODE_POINT;
				rasterState.cullMode    = VK_CULL_MODE_NONE;
				rasterState.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
				break;
			}
			case pipelineEnum::rasterId::point_none_cw: {					//点·不剔·顺时针
				rasterState.polygonMode = VK_POLYGON_MODE_POINT;
				rasterState.cullMode    = VK_CULL_MODE_NONE;
				rasterState.frontFace   = VK_FRONT_FACE_CLOCKWISE;
				break;
			}
			case pipelineEnum::rasterId::point_front_ccw: {					//点·剔正面·逆时针：内面顶点可视化
				rasterState.polygonMode = VK_POLYGON_MODE_POINT;
				rasterState.cullMode    = VK_CULL_MODE_FRONT_BIT;
				rasterState.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
				break;
			}
			case pipelineEnum::rasterId::point_front_cw: {					//点·剔正面·顺时针
				rasterState.polygonMode = VK_POLYGON_MODE_POINT;
				rasterState.cullMode    = VK_CULL_MODE_FRONT_BIT;
				rasterState.frontFace   = VK_FRONT_FACE_CLOCKWISE;
				break;
			}
			case pipelineEnum::rasterId::point_back_ccw: {					//点·剔背面·逆时针：外表面顶点可视化
				rasterState.polygonMode = VK_POLYGON_MODE_POINT;
				rasterState.cullMode    = VK_CULL_MODE_BACK_BIT;
				rasterState.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
				break;
			}
			case pipelineEnum::rasterId::point_back_cw: {					//点·剔背面·顺时针
				rasterState.polygonMode = VK_POLYGON_MODE_POINT;
				rasterState.cullMode    = VK_CULL_MODE_BACK_BIT;
				rasterState.frontFace   = VK_FRONT_FACE_CLOCKWISE;
				break;
			}
			case pipelineEnum::rasterId::point_front_and_back: {			//点·双面·逆时针：全部顶点可视化
				rasterState.polygonMode = VK_POLYGON_MODE_POINT;
				rasterState.cullMode    = VK_CULL_MODE_FRONT_AND_BACK;
				rasterState.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
				break;
			}
			case pipelineEnum::rasterId::fill_back_ccw_clampDepth: {		//填充·剔背面·逆时针·夹紧深度（需 depthClamp）
				rasterState.polygonMode      = VK_POLYGON_MODE_FILL;
				rasterState.cullMode         = VK_CULL_MODE_BACK_BIT;
				rasterState.frontFace        = VK_FRONT_FACE_COUNTER_CLOCKWISE;
				rasterState.depthClampEnable = VK_TRUE;
				break;
			}
			case pipelineEnum::rasterId::discardAll: {						//丢弃全部图元：只走顶点/网格阶段（光栅三字段为占位）
				rasterState.polygonMode             = VK_POLYGON_MODE_FILL;
				rasterState.cullMode                = VK_CULL_MODE_NONE;
				rasterState.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
				rasterState.rasterizerDiscardEnable = VK_TRUE;
				break;
			}
			default: {
				Log::printError(__FILE__, __LINE__, "rasterId 未写预设处理：值 " + std::to_string(static_cast<int>(rasterType)));
				break;
			}}

			switch (biasType) {
			case pipelineEnum::biasId::none: {						//不偏移：主通道默认
				rasterState.depthBiasEnable         = VK_FALSE;
				rasterState.depthBiasConstantFactor = 0.0f;
				rasterState.depthBiasSlopeFactor    = 0.0f;
				rasterState.depthBiasClamp          = 0.0f;
				break;
			}
			case pipelineEnum::biasId::polygonOffset: {				//多边形偏移：常量 1.0/斜率 1.0/不夹紧（＝现有硬编码）
				rasterState.depthBiasEnable         = VK_TRUE;
				rasterState.depthBiasConstantFactor = 1.0f;
				rasterState.depthBiasSlopeFactor    = 1.0f;
				rasterState.depthBiasClamp          = 0.0f;
				break;
			}
			case pipelineEnum::biasId::shadowOffset: {				//阴影斜率偏移：常量 0.0/斜率 1.0/不夹紧
				rasterState.depthBiasEnable         = VK_TRUE;
				rasterState.depthBiasConstantFactor = 0.0f;
				rasterState.depthBiasSlopeFactor    = 1.0f;
				rasterState.depthBiasClamp          = 0.0f;
				break;
			}
			default: {
				Log::printError(__FILE__, __LINE__, "biasId 未写预设处理：值 " + std::to_string(static_cast<int>(biasType)));
				break;
			}}

			return rasterState;
		}

		//创建 深度模板
		VkPipelineDepthStencilStateCreateInfo  createDepthStencil(pipelineEnum::depthId::type depthType, pipelineEnum::stencilId::type stencilType) {
			VkPipelineDepthStencilStateCreateInfo depthState{};
			depthState.sType		  = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			depthState.front		  = {};				//固定常量：模板 front/back 归零（stencil 预设只改其中子字段）
			depthState.back			  = {};				//固定常量：模板 front/back 归零（stencil 预设只改其中子字段）
			depthState.minDepthBounds = 0.0f;			//仅 *_bounds 深度预设会改：深度范围下界
			depthState.maxDepthBounds = 1.0f;			//仅 *_bounds 深度预设会改：深度范围上界

			switch (depthType) {
			case pipelineEnum::depthId::test_never: {					//测·永不通过：强制走模板 depthFail 分支
				depthState.depthTestEnable         = VK_TRUE;
				depthState.depthWriteEnable        = VK_TRUE;
				depthState.depthCompareOp          = VK_COMPARE_OP_NEVER;
				depthState.depthBoundsTestEnable   = VK_FALSE;
				break;
			}
			case pipelineEnum::depthId::test_write_less: {				//测·写·小于：不透明物体标准（＝现有硬编码）
				depthState.depthTestEnable         = VK_TRUE;
				depthState.depthWriteEnable        = VK_TRUE;
				depthState.depthCompareOp          = VK_COMPARE_OP_LESS;
				depthState.depthBoundsTestEnable   = VK_FALSE;
				break;
			}
			case pipelineEnum::depthId::test_write_equal: {				//测·写·等于：共面/等深二次绘制
				depthState.depthTestEnable         = VK_TRUE;
				depthState.depthWriteEnable        = VK_TRUE;
				depthState.depthCompareOp          = VK_COMPARE_OP_EQUAL;
				depthState.depthBoundsTestEnable   = VK_FALSE;
				break;
			}
			case pipelineEnum::depthId::test_write_lessEqual: {			//测·写·≤：共面贴合、阴影深度比较
				depthState.depthTestEnable         = VK_TRUE;
				depthState.depthWriteEnable        = VK_TRUE;
				depthState.depthCompareOp          = VK_COMPARE_OP_LESS_OR_EQUAL;
				depthState.depthBoundsTestEnable   = VK_FALSE;
				break;
			}
			case pipelineEnum::depthId::test_write_greater: {			//测·写·大于：reversed-Z
				depthState.depthTestEnable         = VK_TRUE;
				depthState.depthWriteEnable        = VK_TRUE;
				depthState.depthCompareOp          = VK_COMPARE_OP_GREATER;
				depthState.depthBoundsTestEnable   = VK_FALSE;
				break;
			}
			case pipelineEnum::depthId::test_write_notEqual: {			//测·写·不等：遮罩式绘制
				depthState.depthTestEnable         = VK_TRUE;
				depthState.depthWriteEnable        = VK_TRUE;
				depthState.depthCompareOp          = VK_COMPARE_OP_NOT_EQUAL;
				depthState.depthBoundsTestEnable   = VK_FALSE;
				break;
			}
			case pipelineEnum::depthId::test_write_greaterEqual: {		//测·写·≥：reversed-Z 共面
				depthState.depthTestEnable         = VK_TRUE;
				depthState.depthWriteEnable        = VK_TRUE;
				depthState.depthCompareOp          = VK_COMPARE_OP_GREATER_OR_EQUAL;
				depthState.depthBoundsTestEnable   = VK_FALSE;
				break;
			}
			case pipelineEnum::depthId::test_write_always: {			//测·写·始终通过：深度预通道
				depthState.depthTestEnable         = VK_TRUE;
				depthState.depthWriteEnable        = VK_TRUE;
				depthState.depthCompareOp          = VK_COMPARE_OP_ALWAYS;
				depthState.depthBoundsTestEnable   = VK_FALSE;
				break;
			}
			case pipelineEnum::depthId::test_noWrite_less: {			//测·不写·小于：只测不写（遮挡剔除）
				depthState.depthTestEnable         = VK_TRUE;
				depthState.depthWriteEnable        = VK_FALSE;
				depthState.depthCompareOp          = VK_COMPARE_OP_LESS;
				depthState.depthBoundsTestEnable   = VK_FALSE;
				break;
			}
			case pipelineEnum::depthId::test_noWrite_equal: {			//测·不写·等于：等深处叠加、不改深度
				depthState.depthTestEnable         = VK_TRUE;
				depthState.depthWriteEnable        = VK_FALSE;
				depthState.depthCompareOp          = VK_COMPARE_OP_EQUAL;
				depthState.depthBoundsTestEnable   = VK_FALSE;
				break;
			}
			case pipelineEnum::depthId::test_noWrite_lessEqual: {		//测·不写·≤：天空盒/背景层
				depthState.depthTestEnable         = VK_TRUE;
				depthState.depthWriteEnable        = VK_FALSE;
				depthState.depthCompareOp          = VK_COMPARE_OP_LESS_OR_EQUAL;
				depthState.depthBoundsTestEnable   = VK_FALSE;
				break;
			}
			case pipelineEnum::depthId::test_noWrite_greater: {			//测·不写·大于：reversed-Z 只测不写
				depthState.depthTestEnable         = VK_TRUE;
				depthState.depthWriteEnable        = VK_FALSE;
				depthState.depthCompareOp          = VK_COMPARE_OP_GREATER;
				depthState.depthBoundsTestEnable   = VK_FALSE;
				break;
			}
			case pipelineEnum::depthId::test_noWrite_notEqual: {		//测·不写·不等：深度不一致处显示（遮罩）
				depthState.depthTestEnable         = VK_TRUE;
				depthState.depthWriteEnable        = VK_FALSE;
				depthState.depthCompareOp          = VK_COMPARE_OP_NOT_EQUAL;
				depthState.depthBoundsTestEnable   = VK_FALSE;
				break;
			}
			case pipelineEnum::depthId::test_noWrite_greaterEqual: {	//测·不写·≥：reversed-Z 只测不写（共面）
				depthState.depthTestEnable         = VK_TRUE;
				depthState.depthWriteEnable        = VK_FALSE;
				depthState.depthCompareOp          = VK_COMPARE_OP_GREATER_OR_EQUAL;
				depthState.depthBoundsTestEnable   = VK_FALSE;
				break;
			}
			case pipelineEnum::depthId::test_noWrite_always: {			//测·不写·始终通过：永远可见前景层
				depthState.depthTestEnable         = VK_TRUE;
				depthState.depthWriteEnable        = VK_FALSE;
				depthState.depthCompareOp          = VK_COMPARE_OP_ALWAYS;
				depthState.depthBoundsTestEnable   = VK_FALSE;
				break;
			}
			case pipelineEnum::depthId::noDepth: {						//不参与深度：纯 2D/UI 与后处理（compare 为占位）
				depthState.depthTestEnable         = VK_FALSE;
				depthState.depthWriteEnable        = VK_FALSE;
				depthState.depthCompareOp          = VK_COMPARE_OP_LESS;
				depthState.depthBoundsTestEnable   = VK_FALSE;
				break;
			}
			case pipelineEnum::depthId::test_write_less_bounds: {		//测·写·小于·范围：级联阴影深度切片（需 depthBounds）
				depthState.depthTestEnable         = VK_TRUE;
				depthState.depthWriteEnable        = VK_TRUE;
				depthState.depthCompareOp          = VK_COMPARE_OP_LESS;
				depthState.depthBoundsTestEnable   = VK_TRUE;
				depthState.minDepthBounds          = 0.0f;				//(0,1) 静态空操作兜底：配 viewportScissorBounds 时被动态值覆盖，无影响
				depthState.maxDepthBounds          = 1.0f;
				break;
			}
			case pipelineEnum::depthId::test_noWrite_less_bounds: {		//测·不写·小于·范围：同上但不写深度
				depthState.depthTestEnable         = VK_TRUE;
				depthState.depthWriteEnable        = VK_FALSE;
				depthState.depthCompareOp          = VK_COMPARE_OP_LESS;
				depthState.depthBoundsTestEnable   = VK_TRUE;
				depthState.minDepthBounds          = 0.0f;				//(0,1) 静态空操作兜底：配 viewportScissorBounds 时被动态值覆盖，无影响
				depthState.maxDepthBounds          = 1.0f;
				break;
			}
			default: {
				Log::printError(__FILE__, __LINE__, "depthId 未写预设处理：值 " + std::to_string(static_cast<int>(depthType)));
				break;
			}}

			switch (stencilType) {
			case pipelineEnum::stencilId::none: {						//不测试：其余字段被忽略（保持默认全 0＝KEEP）
				depthState.stencilTestEnable = VK_FALSE; 
				break; 
			}	
			case pipelineEnum::stencilId::writeRef1: {					//标记通道：写参考值 1，描边/遮罩第一步
				depthState.stencilTestEnable    = VK_TRUE;
				depthState.front.passOp         = VK_STENCIL_OP_REPLACE;
				depthState.front.compareOp      = VK_COMPARE_OP_ALWAYS;
				depthState.front.compareMask    = 0xFF;
				depthState.front.writeMask      = 0xFF;
				depthState.front.reference      = 1;
				depthState.back.passOp          = VK_STENCIL_OP_REPLACE;
				depthState.back.compareOp       = VK_COMPARE_OP_ALWAYS;
				depthState.back.compareMask     = 0xFF;
				depthState.back.writeMask       = 0xFF;
				depthState.back.reference       = 1;
				break;
			}
			case pipelineEnum::stencilId::drawEqual1: {					//仅标记处绘制：compare=EQUAL 1，模板不改写
				depthState.stencilTestEnable    = VK_TRUE;
				depthState.front.compareOp      = VK_COMPARE_OP_EQUAL;
				depthState.front.compareMask    = 0xFF;
				depthState.front.reference      = 1;
				depthState.back.compareOp       = VK_COMPARE_OP_EQUAL;
				depthState.back.compareMask     = 0xFF;
				depthState.back.reference       = 1;
				break;
			}
			case pipelineEnum::stencilId::drawNotEqual1: {				//仅未标记处绘制：compare=NOT_EQUAL 1（描边外扩）
				depthState.stencilTestEnable    = VK_TRUE;
				depthState.front.compareOp      = VK_COMPARE_OP_NOT_EQUAL;
				depthState.front.compareMask    = 0xFF;
				depthState.front.reference      = 1;
				depthState.back.compareOp       = VK_COMPARE_OP_NOT_EQUAL;
				depthState.back.compareMask     = 0xFF;
				depthState.back.reference       = 1;
				break;
			}
			case pipelineEnum::stencilId::drawNotEqual0: {				//仅已写入处绘制：compare=NOT_EQUAL 0
				depthState.stencilTestEnable    = VK_TRUE;
				depthState.front.compareOp      = VK_COMPARE_OP_NOT_EQUAL;
				depthState.front.compareMask    = 0xFF;
				depthState.front.reference      = 0;
				depthState.back.compareOp       = VK_COMPARE_OP_NOT_EQUAL;
				depthState.back.compareMask     = 0xFF;
				depthState.back.reference       = 0;
				break;
			}
			case pipelineEnum::stencilId::zPass: {						//阴影体 Z-pass：正面 INCR_WRAP、背面 DECR_WRAP，compare=ALWAYS
				depthState.stencilTestEnable    = VK_TRUE;
				depthState.front.passOp         = VK_STENCIL_OP_INCREMENT_AND_WRAP;
				depthState.front.compareOp      = VK_COMPARE_OP_ALWAYS;
				depthState.front.compareMask    = 0xFF;
				depthState.front.writeMask      = 0xFF;
				depthState.back.passOp          = VK_STENCIL_OP_DECREMENT_AND_WRAP;
				depthState.back.compareOp       = VK_COMPARE_OP_ALWAYS;
				depthState.back.compareMask     = 0xFF;
				depthState.back.writeMask       = 0xFF;
				break;
			}
			case pipelineEnum::stencilId::zFail: {						//阴影体 Z-fail：正面 depthFailOp=INCR_WRAP、背面=DECR_WRAP
				depthState.stencilTestEnable    = VK_TRUE;
				depthState.front.depthFailOp    = VK_STENCIL_OP_INCREMENT_AND_WRAP;
				depthState.front.compareOp      = VK_COMPARE_OP_ALWAYS;
				depthState.front.compareMask    = 0xFF;
				depthState.front.writeMask      = 0xFF;
				depthState.back.depthFailOp     = VK_STENCIL_OP_DECREMENT_AND_WRAP;
				depthState.back.compareOp       = VK_COMPARE_OP_ALWAYS;
				depthState.back.compareMask     = 0xFF;
				depthState.back.writeMask       = 0xFF;
				break;
			}
			default: {
				Log::printError(__FILE__, __LINE__, "stencilId 未写预设处理：值 " + std::to_string(static_cast<int>(stencilType)));
				break;
			}}

			return depthState;
		}

		//创建 颜色混合
		VkPipelineColorBlendStateCreateInfo    createBlend(pipelineEnum::blendId::type blendType, pipelineEnum::writeMaskId::type writeMaskType, pipelineEnum::logicOpId::type logicOpType) {
			VkPipelineColorBlendAttachmentState attachment{};
			attachment.srcColorBlendFactor  = VK_BLEND_FACTOR_ONE;		//仅 blendId 预设会改：源颜色因子
			attachment.dstColorBlendFactor  = VK_BLEND_FACTOR_ZERO;		//仅 blendId 预设会改：目标颜色因子
			attachment.colorBlendOp         = VK_BLEND_OP_ADD;			//仅 blendId 预设会改：颜色混合运算

			attachment.srcAlphaBlendFactor  = VK_BLEND_FACTOR_ONE;		//仅 blendId 预设会改：源 alpha 因子
			attachment.dstAlphaBlendFactor  = VK_BLEND_FACTOR_ZERO;		//仅 blendId 预设会改：目标 alpha 因子
			attachment.alphaBlendOp         = VK_BLEND_OP_ADD;			//仅 blendId 预设会改：alpha 混合运算

			switch (blendType) {
			case pipelineEnum::blendId::opaque: {						//不透明：直接覆盖，因子被忽略（占位＝上方默认 ONE/ZERO/ADD）
				attachment.blendEnable = VK_FALSE; 
				break; 
			}	
			case pipelineEnum::blendId::alphaBlend: {					//常规半透明：src*a + dst*(1-a)（玻璃/水面/粒子/UI）
				attachment.blendEnable          = VK_TRUE;

				attachment.srcColorBlendFactor  = VK_BLEND_FACTOR_SRC_ALPHA;
				attachment.dstColorBlendFactor  = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
				attachment.colorBlendOp         = VK_BLEND_OP_ADD;

				attachment.srcAlphaBlendFactor  = VK_BLEND_FACTOR_ONE;
				attachment.dstAlphaBlendFactor  = VK_BLEND_FACTOR_ZERO;
				attachment.alphaBlendOp         = VK_BLEND_OP_ADD;
				break;
			}
			case pipelineEnum::blendId::premultipliedAlpha: {			//预乘：src + dst*(1-a)，无黑边
				attachment.blendEnable          = VK_TRUE;

				attachment.srcColorBlendFactor  = VK_BLEND_FACTOR_ONE;
				attachment.dstColorBlendFactor  = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
				attachment.colorBlendOp         = VK_BLEND_OP_ADD;

				attachment.srcAlphaBlendFactor  = VK_BLEND_FACTOR_ONE;
				attachment.dstAlphaBlendFactor  = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
				attachment.alphaBlendOp         = VK_BLEND_OP_ADD;
				break;
			}
			case pipelineEnum::blendId::additive: {						//alpha 调制叠加：src*a + dst（光晕/火焰/能量）
				attachment.blendEnable          = VK_TRUE;

				attachment.srcColorBlendFactor  = VK_BLEND_FACTOR_SRC_ALPHA;
				attachment.dstColorBlendFactor  = VK_BLEND_FACTOR_ONE;
				attachment.colorBlendOp         = VK_BLEND_OP_ADD;

				attachment.srcAlphaBlendFactor  = VK_BLEND_FACTOR_ONE;
				attachment.dstAlphaBlendFactor  = VK_BLEND_FACTOR_ZERO;
				attachment.alphaBlendOp         = VK_BLEND_OP_ADD;
				break;
			}
			case pipelineEnum::blendId::additiveNoAlpha: {				//纯叠加：src + dst（辉光/激光/亮斑）
				attachment.blendEnable          = VK_TRUE;

				attachment.srcColorBlendFactor  = VK_BLEND_FACTOR_ONE;
				attachment.dstColorBlendFactor  = VK_BLEND_FACTOR_ONE;
				attachment.colorBlendOp         = VK_BLEND_OP_ADD;

				attachment.srcAlphaBlendFactor  = VK_BLEND_FACTOR_ONE;
				attachment.dstAlphaBlendFactor  = VK_BLEND_FACTOR_ZERO;
				attachment.alphaBlendOp         = VK_BLEND_OP_ADD;
				break;
			}
			case pipelineEnum::blendId::subtract: {						//相减：dst - src（差值、去色）
				attachment.blendEnable          = VK_TRUE;

				attachment.srcColorBlendFactor  = VK_BLEND_FACTOR_ONE;
				attachment.dstColorBlendFactor  = VK_BLEND_FACTOR_ONE;
				attachment.colorBlendOp         = VK_BLEND_OP_REVERSE_SUBTRACT;

				attachment.srcAlphaBlendFactor  = VK_BLEND_FACTOR_ONE;
				attachment.dstAlphaBlendFactor  = VK_BLEND_FACTOR_ZERO;
				attachment.alphaBlendOp         = VK_BLEND_OP_ADD;
				break;
			}
			case pipelineEnum::blendId::subtractAlpha: {				//按 alpha 相减：dst - src*a（遮罩相减）
				attachment.blendEnable          = VK_TRUE;

				attachment.srcColorBlendFactor  = VK_BLEND_FACTOR_SRC_ALPHA;
				attachment.dstColorBlendFactor  = VK_BLEND_FACTOR_ONE;
				attachment.colorBlendOp         = VK_BLEND_OP_REVERSE_SUBTRACT;

				attachment.srcAlphaBlendFactor  = VK_BLEND_FACTOR_ONE;
				attachment.dstAlphaBlendFactor  = VK_BLEND_FACTOR_ZERO;
				attachment.alphaBlendOp         = VK_BLEND_OP_ADD;
				break;
			}
			case pipelineEnum::blendId::multiply: {						//正片叠底：src*dst（阴影投射/染色/暗角）
				attachment.blendEnable          = VK_TRUE;

				attachment.srcColorBlendFactor  = VK_BLEND_FACTOR_DST_COLOR;
				attachment.dstColorBlendFactor  = VK_BLEND_FACTOR_ZERO;
				attachment.colorBlendOp         = VK_BLEND_OP_ADD;

				attachment.srcAlphaBlendFactor  = VK_BLEND_FACTOR_ONE;
				attachment.dstAlphaBlendFactor  = VK_BLEND_FACTOR_ZERO;
				attachment.alphaBlendOp         = VK_BLEND_OP_ADD;
				break;
			}
			case pipelineEnum::blendId::multiplyAlpha: {				//带 alpha 正片叠底：src*dst + dst*(1-a)
				attachment.blendEnable          = VK_TRUE;

				attachment.srcColorBlendFactor  = VK_BLEND_FACTOR_DST_COLOR;
				attachment.dstColorBlendFactor  = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
				attachment.colorBlendOp         = VK_BLEND_OP_ADD;

				attachment.srcAlphaBlendFactor  = VK_BLEND_FACTOR_ONE;
				attachment.dstAlphaBlendFactor  = VK_BLEND_FACTOR_ZERO;
				attachment.alphaBlendOp         = VK_BLEND_OP_ADD;
				break;
			}
			case pipelineEnum::blendId::screen: {						//滤色：src + dst*(1-src)（泛光/亮部叠加）
				attachment.blendEnable          = VK_TRUE;

				attachment.srcColorBlendFactor  = VK_BLEND_FACTOR_ONE;
				attachment.dstColorBlendFactor  = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
				attachment.colorBlendOp         = VK_BLEND_OP_ADD;

				attachment.srcAlphaBlendFactor  = VK_BLEND_FACTOR_ONE;
				attachment.dstAlphaBlendFactor  = VK_BLEND_FACTOR_ZERO;
				attachment.alphaBlendOp         = VK_BLEND_OP_ADD;
				break;
			}
			case pipelineEnum::blendId::darken: {						//变暗：逐通道取小（NPR/光照贴图暗部）
				attachment.blendEnable          = VK_TRUE;

				attachment.srcColorBlendFactor  = VK_BLEND_FACTOR_ONE;
				attachment.dstColorBlendFactor  = VK_BLEND_FACTOR_ONE;
				attachment.colorBlendOp         = VK_BLEND_OP_MIN;

				attachment.srcAlphaBlendFactor  = VK_BLEND_FACTOR_ONE;
				attachment.dstAlphaBlendFactor  = VK_BLEND_FACTOR_ZERO;
				attachment.alphaBlendOp         = VK_BLEND_OP_ADD;		//alpha 统一 ADD（因子 ONE/ZERO ⇒ alpha=srcA），与其余 11 项一致
				break;
			}
			case pipelineEnum::blendId::lighten: {						//变亮：逐通道取大（NPR/光照贴图亮部）
				attachment.blendEnable          = VK_TRUE;

				attachment.srcColorBlendFactor  = VK_BLEND_FACTOR_ONE;
				attachment.dstColorBlendFactor  = VK_BLEND_FACTOR_ONE;
				attachment.colorBlendOp         = VK_BLEND_OP_MAX;

				attachment.srcAlphaBlendFactor  = VK_BLEND_FACTOR_ONE;
				attachment.dstAlphaBlendFactor  = VK_BLEND_FACTOR_ZERO;
				attachment.alphaBlendOp         = VK_BLEND_OP_ADD;		//alpha 统一 ADD（因子 ONE/ZERO ⇒ alpha=srcA），与其余 11 项一致
				break;
			}
			default: {
				Log::printError(__FILE__, __LINE__, "blendId 未写预设处理：值 " + std::to_string(static_cast<int>(blendType)));
				break;
			}}

			switch (writeMaskType) {
			case pipelineEnum::writeMaskId::rgba:      { attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT; break; }	//写 RGBA：颜色与透明度都写
			case pipelineEnum::writeMaskId::rgb:       { attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT; break; }								//只写 RGB：保留目标 alpha（透明度累积）
			case pipelineEnum::writeMaskId::alphaOnly: { attachment.colorWriteMask = VK_COLOR_COMPONENT_A_BIT; break; }																						//只写 alpha：透明通道预通道
			case pipelineEnum::writeMaskId::none:      { attachment.colorWriteMask = 0; break; }																											//不写颜色：颜色附件仅供深度/模板流程
			default: {
				Log::printError(__FILE__, __LINE__, "writeMaskId 未写预设处理：值 " + std::to_string(static_cast<int>(writeMaskType)));
				break;
			}}

			//logicOp 与混合公式互斥（spec）：启用逻辑运算时统一收口，强制关掉附件混合
			if (logicOpType != pipelineEnum::logicOpId::disable) {
				attachment.blendEnable  = VK_FALSE;
			}

			//此处只初始化，在createPipeConfigNode配置
			colorAttachmentList.assign(1, attachment);



			VkPipelineColorBlendStateCreateInfo blendState{};
			blendState.sType			 = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			blendState.blendConstants[0] = 0.0f;			//固定常量：混合常量（无对应预设项）
			blendState.blendConstants[1] = 0.0f;			//固定常量：混合常量（无对应预设项）
			blendState.blendConstants[2] = 0.0f;			//固定常量：混合常量（无对应预设项）
			blendState.blendConstants[3] = 0.0f;			//固定常量：混合常量（无对应预设项）

			switch (logicOpType) {
			case pipelineEnum::logicOpId::disable: {		//不启用：	走混合公式（默认）
				blendState.logicOpEnable        = VK_FALSE;
				blendState.logicOp              = VK_LOGIC_OP_COPY;
				break;
			}
			case pipelineEnum::logicOpId::copy: {			//直接覆盖：源颜色直接写入（需 logicOp）
				blendState.logicOpEnable        = VK_TRUE;
				blendState.logicOp              = VK_LOGIC_OP_COPY;
				break;
			}
			case pipelineEnum::logicOpId::invert: {			//颜色取反：写入取反后的目标颜色
				blendState.logicOpEnable        = VK_TRUE;
				blendState.logicOp              = VK_LOGIC_OP_INVERT;
				break;
			}
			case pipelineEnum::logicOpId::xorOp: {			//异或：	源与目标逐位异或
				blendState.logicOpEnable        = VK_TRUE;
				blendState.logicOp              = VK_LOGIC_OP_XOR;
				break;
			}
			default: {
				Log::printError(__FILE__, __LINE__, "logicOpId 未写预设处理：值 " + std::to_string(static_cast<int>(logicOpType)));
				break;
			}}

			return blendState;
		}

		//创建 动态状态
		VkPipelineDynamicStateCreateInfo       createDynamic(pipelineEnum::dynamicId::type dynamicType) {
			switch (dynamicType) {
			case pipelineEnum::dynamicId::none:                       { pipeDynamicStageList = {}; break; }																						//无动态状态
			case pipelineEnum::dynamicId::viewportScissor:            { pipeDynamicStageList = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR }; break; }								//视口 + 裁剪（交换链重建/多分辨率/分屏）
			case pipelineEnum::dynamicId::viewportScissorLineWidth:   { pipeDynamicStageList = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_LINE_WIDTH }; break; }	// + 线宽	 （仅 line_* 光栅预设有效）
			case pipelineEnum::dynamicId::viewportScissorDepthBias:   { pipeDynamicStageList = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_DEPTH_BIAS }; break; }	// + 深度偏移（仅 depthBiasEnable=TRUE 有效）
			case pipelineEnum::dynamicId::viewportScissorBounds:      { pipeDynamicStageList = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_DEPTH_BOUNDS }; break; }	// + 深度范围（配合深度范围预设，级联阴影逐级设边界）
			case pipelineEnum::dynamicId::viewportScissorRasterDepth: {	//+ 光栅/深度批处理：一条管线服务多材质/双面/多图元
				pipeDynamicStageList = {
					VK_DYNAMIC_STATE_VIEWPORT,
					VK_DYNAMIC_STATE_SCISSOR,
					VK_DYNAMIC_STATE_CULL_MODE,
					VK_DYNAMIC_STATE_FRONT_FACE,
					VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY,
					VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE,
					VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE,
					VK_DYNAMIC_STATE_DEPTH_COMPARE_OP
				};
				break;
			}
			case pipelineEnum::dynamicId::viewportScissorAll: {			//+ 全部可选：上述并集 + 深度范围 + 常量混合 + 模板动态状态
				pipeDynamicStageList = {
					VK_DYNAMIC_STATE_VIEWPORT,
					VK_DYNAMIC_STATE_SCISSOR,
					VK_DYNAMIC_STATE_CULL_MODE,
					VK_DYNAMIC_STATE_FRONT_FACE,
					VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY,
					VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE,
					VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE,
					VK_DYNAMIC_STATE_DEPTH_COMPARE_OP,
					VK_DYNAMIC_STATE_DEPTH_BOUNDS,
					VK_DYNAMIC_STATE_BLEND_CONSTANTS,
					VK_DYNAMIC_STATE_STENCIL_TEST_ENABLE,
					VK_DYNAMIC_STATE_STENCIL_OP,
					VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK,
					VK_DYNAMIC_STATE_STENCIL_WRITE_MASK,
					VK_DYNAMIC_STATE_STENCIL_REFERENCE
				};
				break;
			}
			default: {
				Log::printError(__FILE__, __LINE__, "dynamicId 未写预设处理：值 " + std::to_string(static_cast<int>(dynamicType)));
				break;
			}}

			VkPipelineDynamicStateCreateInfo dynamicState{};
			dynamicState.sType			   = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicState.dynamicStateCount = static_cast<uint32_t>(pipeDynamicStageList.size());	//固定规则：按本结点持有的动态状态表
			dynamicState.pDynamicStates	   = pipeDynamicStageList.data();							//固定规则：指向本结点持有的动态状态表

			return dynamicState;
		}


		
		//创建 着色阶段列表
		void createShaderStageList(std::span<const std::string> shaderFileList, const VkDevice& device) {
			//校验 shaderFileList非空
			if (shaderFileList.empty()) { return; }

			for (const VkPipelineShaderStageCreateInfo& shaderStage : shaderStageList) {
				if (shaderStage.module != VK_NULL_HANDLE) { vkDestroyShaderModule(device, shaderStage.module, nullptr); }
			}

			shaderStageList.clear();
			shaderStageList.resize(shaderFileList.size());

			for (size_t i = 0; i < shaderFileList.size(); i++) {
				createShaderStage(shaderFileList[i], descStruct::reflectStage(shaderFileList[i]), static_cast<uint32_t>(i), device);
			}

			//校验 着色阶段不重复
			if (debugInfo::LogSwitch) {
				for (size_t i = 0; i < shaderStageList.size(); i++) {
					for (size_t j = i + 1; j < shaderStageList.size(); j++) {
						if (shaderStageList[i].stage == shaderStageList[j].stage) { Log::printError(__FILE__, __LINE__, "着色器文件列表阶段重复：" + shaderFileList[i] + " / " + shaderFileList[j]); }
					}
				}
			}
		}

		//创建 单个着色阶段：读 spv → 建模块 → 填阶段
		void createShaderStage(const std::string& filename, VkShaderStageFlagBits stage, int index, const VkDevice& device) {
			//读取着色器文件
			std::ifstream fileData(filename, std::ios::ate | std::ios::binary);
			if (!fileData.is_open()) {
				Log::printError(__FILE__, __LINE__, "着色器文件打开失败");
			}

			//创建着色器代码
			size_t fileSize = static_cast<size_t>(fileData.tellg());
			std::vector<char> shaderCode(fileSize);
			fileData.seekg(0);
			fileData.read(shaderCode.data(), fileSize);
			fileData.close();

			//创建着色器
			VkShaderModuleCreateInfo createInfo{};
			createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.codeSize = shaderCode.size();
			createInfo.pCode    = reinterpret_cast<const uint32_t*>(shaderCode.data());
			if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderStageList[index].module) != VK_SUCCESS) {
				Log::printError(__FILE__, __LINE__, "着色器创建失败");
			}

			//配置着色器状态
			shaderStageList[index].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStageList[index].pName  = "main";
			shaderStageList[index].stage  = stage;
		}

	public:
		//校验 顶点输入布局
		void checkVertexInput(std::span<const std::string> tShaderFileList) {
			//片段着色器的 in 同样是 Input 存储类 + Location ⇒ 必须按执行模型过滤出顶点阶段
			std::vector<spvStruct::inputNode> inputList{};
			for (const std::string& file : tShaderFileList) {
				for (const spvStruct::inputNode& input : descStruct::reflectInput(file)) {
					if (input.stage == VK_SHADER_STAGE_VERTEX_BIT) { inputList.push_back(input); }
				}
			}

			for (const VkVertexInputAttributeDescription& attribute : vertAttributeList) {
				const spvStruct::inputNode* found = nullptr;

				for (const spvStruct::inputNode& input : inputList) {
					if (input.location == attribute.location) { found = &input; break; }
				}

				if (found == nullptr) {
					Log::printError(__FILE__, __LINE__, "顶点输入对拍：声明了 location " + std::to_string(attribute.location) + " 但顶点着色器没有该输入");
					continue;
				}

				if (found->format != attribute.format) {
					Log::printError(__FILE__, __LINE__, "顶点输入对拍：location " + std::to_string(attribute.location) + " 格式不符（声明 " + std::to_string(static_cast<int>(attribute.format)) + " / 着色器 " + std::to_string(static_cast<int>(found->format)) + "）");
				}
			}
		}

		//校验 着色阶段列表
		void checkShaderStageList(pipelineEnum::shaderFlowId::type shaderFlowType) {
			const std::vector<VkPipelineShaderStageCreateInfo>& actual = shaderStageList;
			const std::vector<VkShaderStageFlagBits>&			expect = getShaderStageList(shaderFlowType);

			if (expect.size() != actual.size()) {
				Log::printError(__FILE__, __LINE__, "着色阶段数不符：期望 " + std::to_string(expect.size()) + " / 实得 " + std::to_string(actual.size()));
				return;
			}

			for (size_t i = 0; i < expect.size(); i++) {
				if (expect[i] != actual[i].stage) {
					Log::printError(__FILE__, __LINE__, "第 " + std::to_string(i) + " 个着色阶段不符：期望 " + std::to_string(static_cast<int>(expect[i])) + " / 实得 " + std::to_string(static_cast<int>(actual[i].stage)));
				}
			}
		}

		//获取 着色阶段列表
		std::vector<VkShaderStageFlagBits> getShaderStageList(pipelineEnum::shaderFlowId::type shaderFlowType) {
			switch (shaderFlowType) {
			case pipelineEnum::shaderFlowId::vert: { return { VK_SHADER_STAGE_VERTEX_BIT,  VK_SHADER_STAGE_FRAGMENT_BIT }; }									//顶点流程
			case pipelineEnum::shaderFlowId::mesh: { return { VK_SHADER_STAGE_TASK_BIT_NV, VK_SHADER_STAGE_MESH_BIT_NV, VK_SHADER_STAGE_FRAGMENT_BIT }; }		//网格流程
			default: {
				Log::printError(__FILE__, __LINE__, "shaderFlowId 未写预设处理：值 " + std::to_string(static_cast<int>(shaderFlowType)));
				return {};
			}}
		}
	};

	//常驻异步工作线程：单线程任务队列 + 条件变量，替代 detach 线程与忙等
	struct asyncWorker {
		std::thread						  thread;
		std::mutex						  mutex;
		std::condition_variable			  cv;
		std::queue<std::function<void()>> tasks;
		bool							  stop = false;
		size_t							  running = 0;



		void start() {
			thread = std::thread([this] { loop(); });
		}

		void loop() {
			for (;;) {
				std::function<void()> task;

				{
					std::unique_lock<std::mutex> lock(mutex);
					cv.wait(lock, [this] { return stop || !tasks.empty(); });

					if (stop && tasks.empty()) { return; }

					task = std::move(tasks.front());
					tasks.pop();
					running++;
				}

				task();

				{
					std::lock_guard<std::mutex> lock(mutex);
					running--;
				}
				cv.notify_all();
			}
		}

		void submit(std::function<void()> task) {
			{
				std::lock_guard<std::mutex> lock(mutex);
				tasks.push(std::move(task));
			}
			cv.notify_one();
		}

		//等待队列排空且当前任务执行完毕
		void waitIdle() {
			std::unique_lock<std::mutex> lock(mutex);
			cv.wait(lock, [this] { return tasks.empty() && running == 0; });
		}

		//停止接收、排空队列并退出线程
		void shutdown() {
			{
				std::lock_guard<std::mutex> lock(mutex);
				stop = true;
			}
			cv.notify_all();

			if (thread.joinable()) { thread.join(); }
		}
	};



	//pipeLayout结点
	struct pipeLayoutNode {
		VkPipelineLayout				   pipeLayout{};											//管线布局



		void createPipeLayoutNode(const VkDevice& device, const VkDescriptorSetLayout& dstLayout) {
			VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
			pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutInfo.setLayoutCount         = 1;
			pipelineLayoutInfo.pushConstantRangeCount = 0;
			pipelineLayoutInfo.pPushConstantRanges    = 0;
			pipelineLayoutInfo.pSetLayouts            = &dstLayout;

			if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipeLayout) != VK_SUCCESS) {
				Log::printError(__FILE__, __LINE__, "管线布局创建失败");
			}
		}

		void clear(const VkDevice& device) {
			vkDestroyPipelineLayout(device, pipeLayout, nullptr);
		}
	};

	//pipe结点
	struct pipelineNode {
		pipelineEnum::shaderFlowId::type	  shaderFlowType{};			//着色流程类型
		std::vector<std::string>              shaderFileList{};			//着色文件
		const vertexStruct::vertexLayoutNode* vertexLayout{};			//顶点布局
		renderEnum::renderTemplateId::type    renderTemplateType{};		//渲染类型
		descEnum::descType::type			  descType{};				//描述类型
		pipelineEnum::dynamicId::type         dynamicType{};			//动态类型
		pipelineStruct::pipeConfigParamNode   param{};					//管线配置参数

		pipelineStruct::pipeConfigNode		  pipeConfig{};				//管线配置
		VkPipeline							  pipeline{};				//管线
		bool							      isRecreate = true;		//重建标识



		void createPipelineNode(pipelineEnum::shaderFlowId::type tShaderFlowType, const std::vector<std::string>& tShaderFileList, const vertexStruct::vertexLayoutNode& tVertexLayout, renderEnum::renderTemplateId::type tRenderTemplateType, descEnum::descType::type tDescType, VkSampleCountFlagBits tMsaa, const renderStruct::renderTemplateNode& tRenderTemplateNode, const pipelineStruct::pipeLayoutNode& tPipeLayout, const pipelineStruct::pipeConfigParamNode& tParam, const VkDevice& tDevice) {
			shaderFlowType     = tShaderFlowType;
			shaderFileList     = tShaderFileList;
			vertexLayout       = &tVertexLayout;
			renderTemplateType = tRenderTemplateType;
			descType		   = tDescType;
			dynamicType        = tParam.dynamicType;
			param              = tParam;

			VkSampleCountFlagBits msaa = tRenderTemplateNode.renderAttType == renderEnum::renderAttId::forward_ra ? tMsaa : VK_SAMPLE_COUNT_1_BIT;
			pipeConfig.createPipeConfigNode(shaderFileList, tVertexLayout, msaa, tRenderTemplateNode, tPipeLayout.pipeLayout, tParam, tDevice);

			if (debugInfo::LogSwitch) {
				pipeConfig.checkVertexInput(shaderFileList);			//校验 顶点输入
				pipeConfig.checkShaderStageList(shaderFlowType);		//校验 着色阶段
			}
		}

		void clear(const VkDevice& device) {
			vkDestroyPipeline(device, pipeline, nullptr);
			pipeConfig.clear(device);
		}

		void clearSwapChain(const VkDevice& device) {
			vkDestroyPipeline(device, pipeline, nullptr);
		}
	};
}



//todo后续慢慢拆分
//buffer    结构空间
namespace bufferStruct {
	//todo 后续放到ubo结构空间
	//点光源结点
	struct pointLightNode {
		glm::vec3 position;     //位置
		float radius;           //半径

		glm::vec3 color;        //颜色
		float placeholder;      //占位符
	};

	//平行光结点
	struct dirLightNode {
		glm::vec3 direction;    //方向
		float scale;            //强度

		glm::vec3 color;        //颜色
		float placeholder;      //占位符
	};

	//聚光灯结点
	struct spotLightNode {
		glm::vec3 position;     //位置
		float inner;            //内圆锥

		glm::vec3 color;        //颜色
		float outer;            //外圆锥

		glm::vec3 direction;    //方向
		float placeholder;      //占位符
	};

	//mvp统一结点
	struct mvpUBO {
		glm::mat4 model;            //模型矩阵，用于将模型变换到世界坐标系
		glm::mat4 modelTI;          //模型转置逆矩阵，用于计算光照法线
		glm::mat4 view;             //视图矩阵，用于将世界坐标系变换到观察者坐标系
		glm::mat4 proj;             //投影矩阵，用于将观察者坐标系变换到裁剪坐标系
	};

	//前向光照统一结点
	struct forwardLightUBO {
		glm::vec3 viewPos;          //观察者坐标，用于计算镜面反射
		float currentHour;          //当前小时，通过gameTime(游戏运行时长)%24得到，用于周期性效果

		glm::vec3 gLightColor;      //全局光照颜色，决定环境光和漫反射基础颜色
		float ambientScale;         //环境光缩放，决定环境光强度

		glm::vec3 gLightPos;        //全局光照坐标，点光源坐标，决定漫反射效果
		float diffuseScale;         //漫反射缩放，决定漫反射强度

		glm::vec3 gLightDir;        //全局光照方向，平行光坐标，决定漫反射效果
		float specularScale;        //镜面反射缩放，决定镜面反射强度

		glm::vec3 flashPos;         //聚光灯坐标
		float flashCutOff;          //聚光灯内圆锥

		glm::vec3 flashDir;         //聚光灯方向
		float outerCutOff;          //聚光灯外圆锥
	};

	//延迟光照统一结点
	struct deferredLightUBO {
		glm::vec3 viewPos;          //观察者坐标，用于计算镜面反射
		float currentHour;          //当前小时，通过gameTime(游戏运行时长)%24得到，用于周期性效果

		float ambientScale;         //环境光强度
		float diffuseScale;         //漫反射强度
		float specularScale;        //镜面反射强度
		float placeholder;          //占位符

		pointLightNode pointLightList[1];
		dirLightNode   dirLightList[1];
		spotLightNode  spotLightList[1];
	};

	//延迟阴影统一结点
	struct deferredShadowUBO {
		glm::mat4 shadowMvp;        //阴影mvp
		glm::vec2 shadowExtent;     //阴影贴图大小
	};

	//阴影mvp统一结点
	struct shadowMvpUBO {
		glm::mat4 mvp;              //阴影mvp变换
	};



	//todo 后续放到pipelineStruct结构空间
	/*管线缓冲区*/
	struct pipeTerrainNode {
		bool isEnable;																					//启用标识
		pipelineEnum::procPipelineId::type pipeType;													//管线索引
		samplerEnum::procSamplerId::type samplerType;													//采样索引

		meshBufferNode    mesh[pipelineInfo::commandThreadCount][plateEnum::terrainLayer::enumCount];	//网格缓冲
		textureBufferNode texture{};																	//纹理缓冲
		baseBufferNode    uniform;																		//统一缓冲
		mvpUBO            ubo;

		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;																	//描述缓冲



		pipeTerrainNode(bool tEnable, pipelineEnum::procPipelineId::type tPipeType, samplerEnum::procSamplerId::type tSamplerType) :isEnable(tEnable), pipeType(tPipeType), samplerType(tSamplerType) {}

		pipeTerrainNode() = delete;



		//创建地形纹理（KTX 图集按 tile 切成纹理数组）与网格缓冲
		void create(const std::string& imageFile, VkFormat colorFormat, const VkExtent2D& tileExtent, const VkExtent2D& atlasExtent, cmdStruct::cmdNode& cmd, const VkDevice& device, const VkQueue& graphicsQueue, const VmaAllocator& memAllocator, descStruct::descNode& descriptor, const std::vector<VkSampler>& samplerList) {
			texture.textureFile = imageFile;

			if (isEnable) {
				texture.createKtxArray(colorFormat, tileExtent, atlasExtent, cmd, device, graphicsQueue, memAllocator);

				for (int thread = 0; thread < pipelineInfo::commandThreadCount; thread++) {
					for (int layer = 0; layer < plateEnum::terrainLayer::enumCount; layer++) {
						mesh[thread][layer].create(vertexStruct::terrainVertex::mesh[layer].vertexs, vertexStruct::terrainVertex::mesh[layer].indexs, cmd, device, graphicsQueue, memAllocator);
					}
				}

				uniform = baseBufferNode{ sizeof(ubo), memAllocator };
			}

			//描述符集
			if (isEnable) {
				if (descriptor.alloc(device, descriptorSet, pipeType)) {
					descriptor.writeFrom(device, descriptorSet, {
						{ .binding = 0, .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         .buffer  = uniform.buffer,			.range = sizeof(ubo) },
						{ .binding = 1, .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .sampler = samplerList[samplerType], .view = texture.imageNode.view }
					});
				}
			}
		}

		//清理结构体，descriptorSet由描述符池管理，无需手动清理
		void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
			for (int thread = 0; thread < pipelineInfo::commandThreadCount; thread++) {
				for (int layer = 0; layer < plateEnum::terrainLayer::enumCount; layer++) {
					mesh[thread][layer].clear(memAllocator);
				}
			}

			texture.clear(device, memAllocator);

			vmaDestroyBuffer(memAllocator, uniform.buffer, uniform.bufferMem);
		}
	};



	struct modelStaticNode {
		samplerEnum::procSamplerId::type samplerType;	//采样索引

		meshBufferNode    mesh;							//网格缓冲
		textureBufferNode texture{};					//纹理缓冲
		baseBufferNode    uniform;						//统一缓冲
		mvpUBO            ubo;

		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;	//描述缓冲

		//用于屏蔽超出视觉范围模型的命令提交和统一缓冲更新
		glm::dvec3 modelWorldPos;						//模型世界坐标（d世界坐标系），在初始化时赋值
		glm::dvec3 currentWorldPos;						//当前世界坐标（d世界坐标系），在更新统一缓冲时赋值
		bool       isVisible = true;					//模型可见标识，在提交命令时赋值



		modelStaticNode(samplerEnum::procSamplerId::type tSamplerType, const glm::dvec3& tModelWorldPos, const std::string& tTextureFile /*, const std::string& tNormalFile*/) :samplerType(tSamplerType) {

			texture.textureFile = tTextureFile;
			//texture.normalFile = tNormalFile;
			modelWorldPos = tModelWorldPos;
		}

		modelStaticNode() {}



		void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
			mesh.clear(memAllocator);

			texture.clear(device, memAllocator);
			vmaDestroyBuffer(memAllocator, uniform.buffer, uniform.bufferMem);
		}
	};

	struct modelDynamicSubNode {
		meshBufferNode    mesh;							//网格缓冲
		textureBufferNode texture{};					//纹理缓冲
		baseBufferNode    uniform;						//统一缓冲
		mvpUBO            ubo;

		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;	//描述缓冲



		void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
			mesh.clear(memAllocator);

			texture.clear(device, memAllocator);
			vmaDestroyBuffer(memAllocator, uniform.buffer, uniform.bufferMem);
		}
	};

	struct modelDynamicNode {
		//程序化索引
		samplerEnum::procSamplerId::type samplerType;  //采样索引

		//动态模型结点列表
		std::vector<modelDynamicSubNode> node;

		//用于屏蔽超出视觉范围模型的命令提交和统一缓冲更新
		glm::dvec3 modelWorldPos;           //模型世界坐标（d世界坐标系），在初始化时赋值
		glm::dvec3 currentWorldPos;         //当前世界坐标（d世界坐标系），在更新统一缓冲时赋值
		bool isVisible = true;              //模型可见标识，在提交命令时赋值



		modelDynamicNode(samplerEnum::procSamplerId::type tSamplerType, const glm::dvec3& tModelWorldPos, const std::vector<std::string>& tTextureFile /*, const std::vector<std::string>& tNormalFile*/) :samplerType(tSamplerType) {

			node.resize(tTextureFile.size());
			for (int i = 0; i < tTextureFile.size(); i++) {
				node[i].texture.textureFile = tTextureFile[i];
				//node[i].texture.normalFile = tNormalFile[i];
			}

			modelWorldPos = tModelWorldPos;
		}

		modelDynamicNode() {}



		void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
			for (bufferStruct::modelDynamicSubNode& subNode : node) {
				subNode.clear(device, memAllocator);
			}
		}
	};
	
	struct pipeModelNode {
		bool isEnable;									//启用标识
		pipelineEnum::procPipelineId::type pipeType;	//管线索引

		std::vector<bufferStruct::modelStaticNode>  sNode{};
		std::vector<bufferStruct::modelDynamicNode> dNode{};



		pipeModelNode(bool tEnable, pipelineEnum::procPipelineId::type tPipeType) :isEnable(tEnable), pipeType(tPipeType) {}

		pipeModelNode() = delete;



		void create(cmdStruct::cmdNode& cmd, const VkDevice& device, const VkQueue& graphicsQueue, const VmaAllocator& memAllocator, descStruct::descNode& descFamily, const std::vector<VkSampler>& samplerList) {
			sNode.resize(bufferEnum::procStaticModelId::enumCount);
			dNode.resize(bufferEnum::procDynamicModelId::enumCount);

			sNode[bufferEnum::procStaticModelId::temple_m] = bufferStruct::modelStaticNode { samplerEnum::procSamplerId::staticModel_sn,  glm::dvec3(0, 32, 0), fileInfo::image::temple };
			dNode[bufferEnum::procDynamicModelId::droid_m] = bufferStruct::modelDynamicNode{ samplerEnum::procSamplerId::dynamicModel_sl, glm::dvec3(0, 32, 0), std::vector<std::string>{ fileInfo::image::droidBodyC, fileInfo::image::droidHeadC, fileInfo::image::droidMiscC } /*, std::vector<std::string>{ fileInfo::image::droidBodyN, fileInfo::image::droidHeadN, fileInfo::image::droidMiscN }*/ };

			if (debugInfo::LogSwitch) {
				for (int i = 0; i < sNode.size(); i++) {
					if (sNode[i].texture.textureFile.empty()) { Log::printError(__FILE__, __LINE__, "静态模型槽位未落位：id = " + std::to_string(i)); }
				}

				for (int i = 0; i < dNode.size(); i++) {
					if (dNode[i].node.empty()) { Log::printError(__FILE__, __LINE__, "动态模型槽位未落位：id = " + std::to_string(i)); }
				}
			}

			for (int i = 0; i < dNode.size(); i++) {
				vertexStruct::modelVertex::dMesh[i].resize(dNode[i].node.size());
			}

			if (isEnable) {
				//model-sNode + model-dNode：**并行转码**（A 段 loadKtx 纯 CPU 并发；B 段 uploadKtx 回主线程做 Vulkan）
				std::vector<decltype(&sNode[0].texture)> textureList{};		//要加载的贴图结点（顺序 sNode → dNode，与原串行一致）
				for (bufferStruct::modelStaticNode& modelNode : sNode) { textureList.push_back(&modelNode.texture); }
				for (bufferStruct::modelDynamicNode& dynamicNode : dNode) {
					for (bufferStruct::modelDynamicSubNode& subNode : dynamicNode.node) { textureList.push_back(&subNode.texture); }
				}

				std::vector<ktxLoadNode> loadList(textureList.size());		//每张贴图的 A 段结果
				std::atomic<size_t>      loadCursor{ 0 };
				std::vector<std::thread> loadThread{};
				const size_t             threadCount = std::min<size_t>(std::max(1u, std::thread::hardware_concurrency()), textureList.size());

				for (size_t t = 0; t < threadCount; t++) {				//工作窃取：谁空谁取下一条
					loadThread.emplace_back([&] {
						for (size_t i = loadCursor.fetch_add(1); i < textureList.size(); i = loadCursor.fetch_add(1)) { loadList[i] = textureList[i]->loadKtx(); }
					});
				}

				for (std::thread& loadWorker : loadThread) { loadWorker.join(); }

				for (size_t i = 0; i < textureList.size(); i++) {		//B 段：主线程上传（失败按原语义 printError 挂起）
					if (!loadList[i].error.empty()) { Log::printError(__FILE__, __LINE__, loadList[i].error); continue; }

					textureList[i]->uploadKtx(loadList[i], cmd, device, graphicsQueue, memAllocator);
				}

				//网格缓冲
				for (int i = 0; i < sNode.size(); i++) {
					loadStaticModel(fileInfo::model::staticList[i], vertexStruct::modelVertex::sMesh[i], vertexStruct::shadowVertex::sMesh[i]);

					sNode[i].mesh.create(vertexStruct::modelVertex::sMesh[i].vertexs, vertexStruct::modelVertex::sMesh[i].indexs, cmd, device, graphicsQueue, memAllocator);
				}

				for (int i = 0; i < dNode.size(); i++) {
					vertexStruct::shadowVertex::dMesh[i].resize(vertexStruct::modelVertex::dMesh[i].size());

					loadDynamicModel(fileInfo::model::dynamicList[i], vertexStruct::modelVertex::dMesh[i], vertexStruct::shadowVertex::dMesh[i]);

					for (int j = 0; j < dNode[i].node.size(); j++) {
						dNode[i].node[j].mesh.create(vertexStruct::modelVertex::dMesh[i][j].vertexs, vertexStruct::modelVertex::dMesh[i][j].indexs, cmd, device, graphicsQueue, memAllocator);
					}
				}

				//统一缓冲
				for (bufferStruct::modelStaticNode& modelNode : sNode) {
					modelNode.uniform = baseBufferNode{ sizeof(modelNode.ubo), memAllocator };
				}

				for (bufferStruct::modelDynamicNode& dynamicNode : dNode) {
					for (bufferStruct::modelDynamicSubNode& subNode : dynamicNode.node) {
						subNode.uniform = baseBufferNode{ sizeof(subNode.ubo), memAllocator };
					}
				}
			}

			//描述符集（本管线自己的那套：UBO + 贴图采样 ✓）
			for (int i = 0; i < (int)sNode.size(); i++) {
				if (descFamily.alloc(device, sNode[i].descriptorSet, pipeType)) {
					descFamily.writeFrom(device, sNode[i].descriptorSet, {
						{ .binding = 0, .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         .buffer = sNode[i].uniform.buffer, .range = sizeof(sNode[i].ubo) },
						{ .binding = 1, .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .sampler = samplerList[sNode[i].samplerType], .view = sNode[i].texture.imageNode.view },
					});
				}
			}

			for (int i = 0; i < (int)dNode.size(); i++) {
				for (int j = 0; j < (int)dNode[i].node.size(); j++) {
					if (descFamily.alloc(device, dNode[i].node[j].descriptorSet, pipeType)) {
						descFamily.writeFrom(device, dNode[i].node[j].descriptorSet, {
							{ .binding = 0, .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         .buffer = dNode[i].node[j].uniform.buffer, .range = sizeof(dNode[i].node[j].ubo) },
							{ .binding = 1, .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .sampler = samplerList[dNode[i].samplerType], .view = dNode[i].node[j].texture.imageNode.view },
						});
					}
				}
			}
		}

		void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
			for (bufferStruct::modelStaticNode& modelNode : sNode) {
				modelNode.clear(device, memAllocator);
			}

			for (bufferStruct::modelDynamicNode& dynamicNode : dNode) {
				dynamicNode.clear(device, memAllocator);
			}
		}



		//加载静态模型
		void loadStaticModel(const std::string& filename, vertexStruct::vertexMeshNode<vertexStruct::modelVertex::vertexNode>& meshNode, vertexStruct::vertexMeshNode<vertexStruct::shadowVertex::vertexNode>& shadowMeshNode) {
			tinygltf::Model model;

			if (!openGltf(filename, model)) { return; }

			//遍历所有mesh和primitive
			std::unordered_map<vertexStruct::modelVertex::vertexNode, uint32_t, vertexStruct::modelVertex::vertexNodeHash> uniqueVertices{};

			for (const tinygltf::Mesh& mesh : model.meshes) {
				loadGltfModel(model, mesh, meshNode, shadowMeshNode, uniqueVertices);
			}

			//资产内容自检：不放进日志开关（发布版也要能看到），用 printWarning 不挂起
			if (meshNode.vertexs.empty()) {
				Log::printWarning(__FILE__, __LINE__, "加载的模型不包含任何顶点");
			}

			if (meshNode.indexs.empty()) {
				Log::printWarning(__FILE__, __LINE__, "加载的模型不包含任何索引");
			}
		}

		//加载动态模型
		void loadDynamicModel(const std::string& filename, std::vector<vertexStruct::vertexMeshNode<vertexStruct::modelVertex::vertexNode>>& meshList, std::vector<vertexStruct::vertexMeshNode<vertexStruct::shadowVertex::vertexNode>>& shadowMeshList) {
			tinygltf::Model model;

			if (!openGltf(filename, model)) { return; }

			//mesh 数与子结点数必须一一对应，否则下面按下标写入会越界
			if (model.meshes.size() != meshList.size()) {
				Log::printError(__FILE__, __LINE__, "glTF的mesh数量与子结点数量不一致: \t" + filename);
			}

			//遍历所有mesh和primitive
			std::unordered_map<vertexStruct::modelVertex::vertexNode, uint32_t, vertexStruct::modelVertex::vertexNodeHash> uniqueVertices{};

			for (int i = 0; i < model.meshes.size(); i++) {
				loadGltfModel(model, model.meshes[i], meshList[i], shadowMeshList[i], uniqueVertices);

				//资产内容自检：同静态模型，不进日志开关
				if (meshList[i].vertexs.empty()) {
					Log::printWarning(__FILE__, __LINE__, "加载的模型不包含任何顶点");
				}

				if (meshList[i].indexs.empty()) {
					Log::printWarning(__FILE__, __LINE__, "加载的模型不包含任何索引");
				}
			}
		}

	private:
		//读取 glTF（.glb 走二进制，其余走 ASCII）；返回是否成功，文件与结构异常统一在此报错
		bool openGltf(const std::string& filename, tinygltf::Model& model) {
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
				Log::print("glTF警告: " + warn);    //因为使用KTX管理贴图，故屏蔽gltf缺失贴图的报错
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
			return ret;
		}

		//加载glTF模型
		void loadGltfModel(const tinygltf::Model& model, const tinygltf::Mesh& mesh, vertexStruct::vertexMeshNode<vertexStruct::modelVertex::vertexNode>& meshNode, vertexStruct::vertexMeshNode<vertexStruct::shadowVertex::vertexNode>& shadowMeshNode, std::unordered_map<vertexStruct::modelVertex::vertexNode, uint32_t, vertexStruct::modelVertex::vertexNodeHash>& uniqueVertices) {
			for (const auto& primitive : mesh.primitives) {
				//获取顶点数据  
				const float* positions = nullptr;
				const float* texCoords = nullptr;
				const float* normals = nullptr;
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
					vertexStruct::modelVertex::vertexNode vertex{};

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
						uniqueVertices[vertex] = static_cast<uint32_t>(meshNode.vertexs.size());

						meshNode.vertexs.push_back(vertex);
						shadowMeshNode.vertexs.push_back(vertexStruct::shadowVertex::vertexNode{ vertex.position });
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
								meshNode.indexs.push_back(primitiveVertexIndices[originalVertexIndex]);
							}
						}
					}
					else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
						const uint32_t* indices = reinterpret_cast<const uint32_t*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);

						for (size_t i = 0; i < indexCount; i++) {
							uint32_t originalVertexIndex = indices[i];
							if (originalVertexIndex < primitiveVertexIndices.size()) {
								meshNode.indexs.push_back(primitiveVertexIndices[originalVertexIndex]);
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
						meshNode.indexs.push_back(primitiveVertexIndices[i]);
						meshNode.indexs.push_back(primitiveVertexIndices[i + 1]);
						meshNode.indexs.push_back(primitiveVertexIndices[i + 2]);
					}
				}
			}
		}
	};



	struct pipeSkyboxNode {
		bool isEnable;									//启用标识
		pipelineEnum::procPipelineId::type pipeType;	//管线索引

		meshBufferNode  mesh;							//网格缓冲
		baseBufferNode  uniform;						//统一缓冲
		mvpUBO          ubo;
		baseBufferNode  lightUniform;
		forwardLightUBO lightUbo;

		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;	//描述缓冲



		pipeSkyboxNode(bool tEnable, pipelineEnum::procPipelineId::type tPipeType) :isEnable(tEnable), pipeType(tPipeType) {}

		pipeSkyboxNode() = delete;



		//创建网格缓冲
		void create(cmdStruct::cmdNode& cmd, const VkDevice& device, const VkQueue& graphicsQueue, const VmaAllocator& memAllocator, descStruct::descNode& descriptor) {
			if (isEnable) {
				mesh.create(vertexStruct::skyboxVertex::vertexs, vertexStruct::skyboxVertex::indexs, cmd, device, graphicsQueue, memAllocator);

				//统一缓冲
				uniform      = baseBufferNode{ sizeof(ubo),      memAllocator };
				lightUniform = baseBufferNode{ sizeof(lightUbo), memAllocator };

				//描述符集（本管线自己的那套：纯 UBO，无全局资源依赖）
				if (descriptor.alloc(device, descriptorSet, pipeType)) {
					descriptor.writeFrom(device, descriptorSet, {
						{ .binding = 0, .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .buffer = uniform.buffer,      .range = sizeof(ubo) },
						{ .binding = 1, .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .buffer = lightUniform.buffer, .range = sizeof(lightUbo) },
					});
				}
			}
		}

		void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
			mesh.clear(memAllocator);

			vmaDestroyBuffer(memAllocator, uniform.buffer, uniform.bufferMem);
			vmaDestroyBuffer(memAllocator, lightUniform.buffer, lightUniform.bufferMem);
		}
	};

	struct pipeMineNode {
		bool isEnable;									//启用标识
		pipelineEnum::procPipelineId::type pipeType;	//管线索引

		meshBufferNode mesh;							//网格缓冲
		baseBufferNode uniform;							//统一缓冲
		mvpUBO         ubo;

		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;	//描述缓冲



		pipeMineNode(bool tEnable, pipelineEnum::procPipelineId::type tPipeType) :isEnable(tEnable), pipeType(tPipeType) {}

		pipeMineNode() = delete;



		//创建网格缓冲
		void create(cmdStruct::cmdNode& cmd, const VkDevice& device, const VkQueue& graphicsQueue, const VmaAllocator& memAllocator, descStruct::descNode& descriptor) {
			if (isEnable) {
				mesh.create(vertexStruct::mineVertex::vertexs, vertexStruct::mineVertex::indexs, cmd, device, graphicsQueue, memAllocator);

				//统一缓冲
				uniform = baseBufferNode{ sizeof(ubo), memAllocator };

				//描述符集（本管线自己的那套：纯 UBO）
				if (descriptor.alloc(device, descriptorSet, pipeType)) {
					descriptor.writeFrom(device, descriptorSet, {
						{ .binding = 0, .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .buffer = uniform.buffer, .range = sizeof(ubo) },
					});
				}
			}
		}

		void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
			mesh.clear(memAllocator);

			vmaDestroyBuffer(memAllocator, uniform.buffer, uniform.bufferMem);
		}
	};

	struct pipeUITextNode {
		bool isEnable;																		//启用标识
		pipelineEnum::procPipelineId::type pipeType;										//管线索引
		samplerEnum::procSamplerId::type samplerType;										//采样索引

		baseBufferNode vertex{};															//网格缓冲
		uiStruct::msdfGlyphAtlasNode msdfGlyphAtlas{ fileInfo::font::mapleMonoNormalNFCN, fileInfo::font::uiTextCharset };	//字体图集
		textureBufferNode texture{};														//纹理缓冲，MTSDF 图集，RGBA8，mip=1

		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;										//描述缓冲



		pipeUITextNode(bool tEnable, pipelineEnum::procPipelineId::type tPipeType, samplerEnum::procSamplerId::type tSamplerType) :isEnable(tEnable), pipeType(tPipeType), samplerType(tSamplerType) {}

		pipeUITextNode() = delete;



		//创建字体图集纹理与顶点缓冲
		void create(cmdStruct::cmdNode& cmd, const VkDevice& device, const VkQueue& graphicsQueue, const VmaAllocator& memAllocator, descStruct::descNode& descFamily, const std::vector<VkSampler>& samplerList) {
			if (isEnable) {
				vertex = baseBufferNode{ pipelineInfo::uiTextGlyphCount * 6 * sizeof(glm::vec4), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, memAllocator };	//6 顶点/字形 × uiTextGlyphCount 字形上限

				texture.createText(msdfGlyphAtlas.data.data(), msdfGlyphAtlas.data.size(), static_cast<uint32_t>(msdfGlyphAtlas.width), static_cast<uint32_t>(msdfGlyphAtlas.height), cmd, device, graphicsQueue, memAllocator);
			}

			//描述符集（本管线自己的那套：图集采样 ✓）
			if (isEnable && descFamily.alloc(device, descriptorSet, pipeType)) {
				descFamily.writeFrom(device, descriptorSet, {
					{ .binding = 0, .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .sampler = samplerList[samplerType], .view = texture.imageNode.view },
				});
			}
		}

		void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
			vmaDestroyBuffer(memAllocator, vertex.buffer, vertex.bufferMem);
			texture.clear(device, memAllocator);
		}
	};

	struct pipeOSMirrorNode {
		bool isEnable;									//启用标识
		pipelineEnum::procPipelineId::type pipeType;	//管线索引
		samplerEnum::procSamplerId::type samplerType;	//采样索引

		meshBufferNode  mesh;							//网格缓冲
		baseBufferNode  uniform;						//统一缓冲
		mvpUBO          ubo;
		baseBufferNode  lightUniform;
		forwardLightUBO lightUbo;

		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;	//描述缓冲



		pipeOSMirrorNode(bool tEnable, pipelineEnum::procPipelineId::type tPipeType, samplerEnum::procSamplerId::type tSamplerType) :isEnable(tEnable), pipeType(tPipeType), samplerType(tSamplerType) {}

		pipeOSMirrorNode() = delete;



		//创建网格缓冲
		void create(cmdStruct::cmdNode& cmd, const VkDevice& device, const VkQueue& graphicsQueue, const VmaAllocator& memAllocator, descStruct::descNode& descriptor, const std::vector<VkSampler>& samplerList, VkImageView frameView) {
			if (isEnable) {
				mesh.create(vertexStruct::osMirrorVertex::vertexs, vertexStruct::osMirrorVertex::indexs, cmd, device, graphicsQueue, memAllocator);

				//统一缓冲
				uniform      = baseBufferNode{ sizeof(ubo),      memAllocator };
				lightUniform = baseBufferNode{ sizeof(lightUbo), memAllocator };

				//描述符集（本管线自己的那套：UBO×2 + 帧图组合采样）
				if (descriptor.alloc(device, descriptorSet, pipeType)) {
					descriptor.writeFrom(device, descriptorSet, {
						{ .binding = 0, .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         .buffer = uniform.buffer,      .range = sizeof(ubo) },
						{ .binding = 1, .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         .buffer = lightUniform.buffer, .range = sizeof(lightUbo) },
						{ .binding = 2, .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .sampler = samplerList[samplerType], .view = frameView },
					});
				}
			}
		}

		//交换链重建刷新
		void updateFrameDesc(descStruct::descNode& descFamily, const std::vector<VkSampler>& samplerList, VkImageView frameView, const VkDevice& device, const VmaAllocator& memAllocator) {
			if (!isEnable) { return; }
			descFamily.writeFrom(device, descriptorSet, {
				{ .binding = 0, .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         .buffer = uniform.buffer,      .range = sizeof(ubo) },
				{ .binding = 1, .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         .buffer = lightUniform.buffer, .range = sizeof(lightUbo) },
				{ .binding = 2, .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .sampler = samplerList[samplerType], .view = frameView },
			});
		}

		void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
			mesh.clear(memAllocator);

			vmaDestroyBuffer(memAllocator, uniform.buffer, uniform.bufferMem);
			vmaDestroyBuffer(memAllocator, lightUniform.buffer, lightUniform.bufferMem);
		}
	};

	struct pipeOSScenceNode {
		bool isEnable;									//启用标识
		pipelineEnum::procPipelineId::type pipeType;	//管线索引
		samplerEnum::procSamplerId::type samplerType;	//采样索引

		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;	//描述缓冲



		pipeOSScenceNode(bool tEnable, pipelineEnum::procPipelineId::type tPipeType, samplerEnum::procSamplerId::type tSamplerType) :isEnable(tEnable), pipeType(tPipeType), samplerType(tSamplerType) {}

		pipeOSScenceNode() = delete;



		//创建描述符集
		void create(descStruct::descNode& descFamily, const std::vector<VkSampler>& samplerList, VkImageView frameView, const VkDevice& device, const VmaAllocator& memAllocator) {
			if (!isEnable) { return; }

			if (descFamily.alloc(device, descriptorSet, pipeType)) {
				descFamily.writeFrom(device, descriptorSet, {
					{ .binding = 2, .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .sampler = samplerList[samplerType], .view = frameView },
				});
			}
		}

		//交换链重建刷新
		void updateFrameDesc(descStruct::descNode& descFamily, const std::vector<VkSampler>& samplerList, VkImageView frameView, const VkDevice& device, const VmaAllocator& memAllocator) {
			if (!isEnable) { return; }
			descFamily.writeFrom(device, descriptorSet, {
				{ .binding = 2, .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .sampler = samplerList[samplerType], .view = frameView },
			});
		}

		void clear(const VkDevice& device, const VmaAllocator& memAllocator) {

		}
	};

	struct pipeDSScenceNode {
		bool isEnable;									//启用标识
		pipelineEnum::procPipelineId::type pipeType;	//管线索引
		samplerEnum::procSamplerId::type samplerType;	//采样索引

		baseBufferNode    lightUniform;					//统一缓冲
		deferredLightUBO  lightUbo;
		baseBufferNode    shadowUniform;
		deferredShadowUBO shadowUbo;

		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;	//描述缓冲



		pipeDSScenceNode(bool tEnable, pipelineEnum::procPipelineId::type tPipeType, samplerEnum::procSamplerId::type tSamplerType) :isEnable(tEnable), pipeType(tPipeType), samplerType(tSamplerType) {}

		pipeDSScenceNode() = delete;



		void create(const VmaAllocator& memAllocator, descStruct::descNode& descFamily, const std::vector<VkSampler>& samplerList,
			const std::array<VkImageView, 3>& dsScenceViews, VkSampler shadowSampler, VkImageView shadowView, const VkDevice& device) {
			if (isEnable) {
				lightUniform  = baseBufferNode{ sizeof(lightUbo),  memAllocator };
				shadowUniform = baseBufferNode{ sizeof(shadowUbo), memAllocator };

				if (descFamily.alloc(device, descriptorSet, pipeType)) {
					descFamily.writeFrom(device, descriptorSet, {
						{ .binding = 0, .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         .buffer = lightUniform.buffer,  .range = sizeof(lightUbo) },
						{ .binding = 1, .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         .buffer = shadowUniform.buffer, .range = sizeof(shadowUbo) },
						{ .binding = 2, .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .sampler = samplerList[samplerType], .view = dsScenceViews[0] },
						{ .binding = 3, .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .sampler = samplerList[samplerType], .view = dsScenceViews[1] },
						{ .binding = 4, .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .sampler = samplerList[samplerType], .view = dsScenceViews[2] },
						{ .binding = 5, .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .sampler = shadowSampler,               .view = shadowView },
					});
				}
			}
		}

		//交换链重建刷新
		void updateFrameDesc(descStruct::descNode& descFamily, const std::vector<VkSampler>& samplerList, const std::array<VkImageView, 3>& dsScenceViews, VkSampler shadowSampler, VkImageView shadowView, const VkDevice& device, const VmaAllocator& memAllocator) {
			if (!isEnable) { return; }
			descFamily.writeFrom(device, descriptorSet, {
				{ .binding = 0, .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         .buffer = lightUniform.buffer,  .range = sizeof(lightUbo) },
				{ .binding = 1, .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         .buffer = shadowUniform.buffer, .range = sizeof(shadowUbo) },
				{ .binding = 2, .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .sampler = samplerList[samplerType], .view = dsScenceViews[0] },
				{ .binding = 3, .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .sampler = samplerList[samplerType], .view = dsScenceViews[1] },
				{ .binding = 4, .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .sampler = samplerList[samplerType], .view = dsScenceViews[2] },
				{ .binding = 5, .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .sampler = shadowSampler,            .view = shadowView },
			});
		}

		void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
			vmaDestroyBuffer(memAllocator, lightUniform.buffer,  lightUniform.bufferMem);
			vmaDestroyBuffer(memAllocator, shadowUniform.buffer, shadowUniform.bufferMem);
		}
	};



	struct shadowTerrainNode {
		meshBufferNode mesh[pipelineInfo::commandThreadCount][plateEnum::terrainLayer::enumCount];	//网格缓冲，索引直接用源网格的
		baseBufferNode shadowMvpUniform;															//统一缓冲
		shadowMvpUBO   shadowMvpUbo;

		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;												//描述缓冲



		void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
			for (int thread = 0; thread < pipelineInfo::commandThreadCount; thread++) {
				for (int layer = 0; layer < plateEnum::terrainLayer::enumCount; layer++) {
					mesh[thread][layer].clear(memAllocator);
				}
			}

			vmaDestroyBuffer(memAllocator, shadowMvpUniform.buffer, shadowMvpUniform.bufferMem);
		}
	};

	struct shadowModelNode {
		meshBufferNode mesh;							//网格缓冲，索引直接用源网格的
		baseBufferNode shadowMvpUniform;				//统一缓冲
		shadowMvpUBO   shadowMvpUbo;

		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;	//描述缓冲



		void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
			mesh.clear(memAllocator);

			vmaDestroyBuffer(memAllocator, shadowMvpUniform.buffer, shadowMvpUniform.bufferMem);
		}
	};
	
	struct pipeShadowNode {
		bool isEnable;															//启用标识
		pipelineEnum::procPipelineId::type pipeType;							//管线索引
		samplerEnum::procSamplerId::type samplerType;							//采样索引

		bufferStruct::shadowTerrainNode shadow_t{};								//地形阴影
		std::vector<bufferStruct::shadowModelNode> shadow_ms{};					//静态模型阴影
		std::vector<std::vector<bufferStruct::shadowModelNode>> shadow_md{};	//动态模型阴影

		glm::mat4 shadowProj{};													//辅助shadowMvp计算
		glm::mat4 shadowVP{};



		pipeShadowNode(bool tEnable, pipelineEnum::procPipelineId::type tPipeType, samplerEnum::procSamplerId::type tSamplerType) :isEnable(tEnable), pipeType(tPipeType), samplerType(tSamplerType) {}

		pipeShadowNode() = delete;



		//创建阴影结点列表与网格缓冲（顶点随源网格，索引直接用源网格的）
		void create(bool terrainEnable, bool modelEnable, cmdStruct::cmdNode& cmd, const VkDevice& device, const VkQueue& graphicsQueue, const VmaAllocator& memAllocator, descStruct::descNode& descFamily) {
			shadow_ms.resize(bufferEnum::procStaticModelId::enumCount);
			shadow_md.resize(bufferEnum::procDynamicModelId::enumCount);

			if (modelEnable) {
				for (int i = 0; i < shadow_md.size(); i++) {
					shadow_md[i].resize(vertexStruct::modelVertex::dMesh[i].size());
				}
			}

			if (isEnable) {
				//shadow-pipeTerrain
				if (terrainEnable) {
					for (int layer = 0; layer < plateEnum::terrainLayer::enumCount; layer++) {
						vertexStruct::shadowVertex::loadTerrainVertexs(layer);

						for (int thread = 0; thread < pipelineInfo::commandThreadCount; thread++) {
							shadow_t.mesh[thread][layer].create(vertexStruct::shadowVertex::tMesh[layer].vertexs, cmd, device, graphicsQueue, memAllocator);
						}
					}
				}

				//shadow-model
				if (modelEnable) {
					//shadow-sNode
					for (int i = 0; i < shadow_ms.size(); i++) {
						shadow_ms[i].mesh.create(vertexStruct::shadowVertex::sMesh[i].vertexs, cmd, device, graphicsQueue, memAllocator);
					}

					//shadow-dNode
					for (int i = 0; i < shadow_md.size(); i++) {
						for (int j = 0; j < shadow_md[i].size(); j++) {
							shadow_md[i][j].mesh.create(vertexStruct::shadowVertex::dMesh[i][j].vertexs, cmd, device, graphicsQueue, memAllocator);
						}
					}
				}

				//统一缓冲
				if (terrainEnable) {
					shadow_t.shadowMvpUniform = baseBufferNode{ sizeof(shadow_t.shadowMvpUbo), memAllocator };
				}

				if (modelEnable) {
					for (bufferStruct::shadowModelNode& shadowNode : shadow_ms) {
						shadowNode.shadowMvpUniform = baseBufferNode{ sizeof(shadowNode.shadowMvpUbo), memAllocator };
					}

					for (std::vector<bufferStruct::shadowModelNode>& shadowNodeList : shadow_md) {
						for (bufferStruct::shadowModelNode& shadowNode : shadowNodeList) {
							shadowNode.shadowMvpUniform = baseBufferNode{ sizeof(shadowNode.shadowMvpUbo), memAllocator };
						}
					}
				}
			}

			//描述符集
			if (isEnable && terrainEnable && descFamily.alloc(device, shadow_t.descriptorSet, pipeType)) {
				descFamily.writeFrom(device, shadow_t.descriptorSet, {
					{ .binding = 0, .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .buffer = shadow_t.shadowMvpUniform.buffer, .range = sizeof(shadow_t.shadowMvpUbo) },
				});
			}

			for (bufferStruct::shadowModelNode& shadowNode : shadow_ms) {
				if (isEnable && modelEnable && descFamily.alloc(device, shadowNode.descriptorSet, pipeType)) {
					descFamily.writeFrom(device, shadowNode.descriptorSet, {
						{ .binding = 0, .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .buffer = shadowNode.shadowMvpUniform.buffer, .range = sizeof(shadowNode.shadowMvpUbo) },
					});
				}
			}

			for (std::vector<bufferStruct::shadowModelNode>& shadowNodeList : shadow_md) {
				for (bufferStruct::shadowModelNode& shadowNode : shadowNodeList) {
					if (isEnable && modelEnable && descFamily.alloc(device, shadowNode.descriptorSet, pipeType)) {
						descFamily.writeFrom(device, shadowNode.descriptorSet, {
							{ .binding = 0, .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .buffer = shadowNode.shadowMvpUniform.buffer, .range = sizeof(shadowNode.shadowMvpUbo) },
						});
					}
				}
			}
		}

		void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
			shadow_t.clear(device, memAllocator);

			for (bufferStruct::shadowModelNode& shadowNode : shadow_ms) {
				shadowNode.clear(device, memAllocator);
			}

			for (std::vector<bufferStruct::shadowModelNode>& shadowNodeList : shadow_md) {
				for (bufferStruct::shadowModelNode& shadowNode : shadowNodeList) {
					shadowNode.clear(device, memAllocator);
				}
			}
		}
	};
}