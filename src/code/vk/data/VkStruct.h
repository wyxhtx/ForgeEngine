#pragma once



//engine    结构空间
namespace engineStruct {
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
	//cmd线程结点
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

	//todo后续写成pipe结点
	//cmd配置结点
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

	//cmd录制结点
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
				for (uint32_t i = 0; i < tCount; i++) {
					vkCmdDraw(cmd, 4, 1, i * 4, 0);
				}
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
	
	//cmd缓冲结点
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



	//cmd结点
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
			else {
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

		//HUD 默认参数
		float defaultFontSizePx      = 14.0f;               //默认屏幕字号
		float defaultLineHeightRatio = 1.3f;                //默认行距系数
		float defaultMarginPx        = 8.0f;                //默认页边距

		//暂存排版，每帧重建，段缓冲容量跨帧保留
		uiStruct::uiTextLayoutNode uiTextLayout;

		glm::vec4* vertCursor = nullptr;    //本帧顶点写游标，写入过程中前进
		uint32_t   glyphCount = 0;          //本帧已写入字形数



		msdfGlyphAtlasNode(const std::string& fontPath) {
			//1、初始化 FreeType 库，该库的单例操作不能多线程
			msdfgen::FreetypeHandle* ft = msdfgen::initializeFreetype();
			if (!ft) {
				Log::printError(__FILE__, __LINE__, "freetype库初始化失败");
			}

			//2、加载字体
			msdfgen::FontHandle* font = msdfgen::loadFont(ft, fontPath.c_str());
			if (!font) {
				msdfgen::deinitializeFreetype(ft);
				Log::printError(__FILE__, __LINE__, "字体加载失败");
			}

			//3、取字符集：生成字形几何列表
			//缩放系数：1.0
			//Charset::ASCII 只覆盖西文，要中文需换 GB2312/自定义字符集
			std::vector<msdf_atlas::GlyphGeometry> glyphs;
			msdf_atlas::FontGeometry fontGeometry(&glyphs);
			fontGeometry.loadCharset(font, 1.0, msdf_atlas::Charset::ASCII);

			//4、边缘着色：让直角/尖角保持锐利，不被圆角化
			//maxCornerAngle = 3.0°，夹角小于该值视为直角
			//默认seed：0 
			const double maxCornerAngle = 3.0;
			for (msdf_atlas::GlyphGeometry& glyph : glyphs) {
				glyph.edgeColoring(&msdfgen::edgeColoringInkTrap, maxCornerAngle, 0);
			}



			//5、打包：把全部字形塞进一张图集
			//POWER_OF_TWO_RECTANGLE：尺寸取 2 的幂矩形，便于采样/mip
			//setScale(emSize)          ：烘焙尺度，像素/em
			//setPixelRange(2.0)    ：SDF 边缘带宽 → 渲染端需用 pixelRange × scale
			//setMiterLimit(1.0)    ：尖角斜接上限
			msdf_atlas::TightAtlasPacker packer;
			packer.setDimensionsConstraint(msdf_atlas::DimensionsConstraint::POWER_OF_TWO_RECTANGLE);
			packer.setScale(emSize);
			packer.setPixelRange(2.0);
			packer.setMiterLimit(1.0);
			if (packer.pack(glyphs.data(), static_cast<int>(glyphs.size())) != 0) {
				msdfgen::destroyFont(font);
				msdfgen::deinitializeFreetype(ft);
				Log::printError(__FILE__, __LINE__, "msdf图集打包失败");
			}
			packer.getDimensions(width, height);

			//6、生成 MTSDF 位图，4 通道：RGB 三向距离 + A 真实距离，线程 4 只用于栅格化，与 FreeType 生命周期无关
			msdf_atlas::ImmediateAtlasGenerator<float, 4, msdf_atlas::mtsdfGenerator, msdf_atlas::BitmapAtlasStorage<unsigned char, 4>> generator(width, height);
			msdf_atlas::GeneratorAttributes attributes;
			generator.setAttributes(attributes);
			generator.setThreadCount(4);
			generator.generate(glyphs.data(), static_cast<int>(glyphs.size()));

			//7、取位图只读视图，逐行翻转拷贝成RGBA8，因为原点在左下，所以下面翻成左上，原因：FreeType/msdfgen 位图原点在左下，Vulkan 纹理上传按左上
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

			//8、逐字形填度量表
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
				uiTextLayout.prevHash = hash;
			}
		}

	private:
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

		//获取 msdfGlyphNode
		const msdfGlyphNode* getGlyph(uint32_t codepoint) const {
			const auto it = map.find(codepoint);

			return (it == map.end()) ? nullptr : &it->second;
		}

		//设置 msdfGlyphNode，单个字形 → 顶点，写四顶点、推进光标、计数
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

			mapped->x = left;  mapped->y = top;    mapped->z = u0; mapped->w = v0; mapped++;
			mapped->x = right; mapped->y = top;    mapped->z = u1; mapped->w = v0; mapped++;
			mapped->x = left;  mapped->y = bottom; mapped->z = u0; mapped->w = v1; mapped++;
			mapped->x = right; mapped->y = bottom; mapped->z = u1; mapped->w = v1; mapped++;

			cursorX += glyph.advance * scale;
			letterCount++;
		}

		//设置 文本，默认布局
		void setText(glm::vec4*& mapped, uint32_t& letterCount, const std::string& text, float positionX, float positionY) const {
			setText(mapped, letterCount, text, positionX, positionY, bufferEnum::textAlignId::left, defaultFontSizePx);
		}

		//设置 文本，指定布局
		void setText(glm::vec4*& mapped, uint32_t& letterCount, const std::string& text, float positionX, float positionY, bufferEnum::textAlignId::type align, float fontSizePx) const {
			if (map.empty() || !mapped) {
				return;
			}

			const float scale   = fontSizePx / emSize;
			float       originX = positionX;

			//对齐
			switch (align) {
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

				setGlyph(mapped, letterCount, *glyph, cursorX, positionY, scale);
			}
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

		//固定宽度，填充空格
		std::string padFixedWidth(const std::string& text, size_t width) {
			std::string result = text;

			if (result.size() < width) {
				result.append(width - result.size(), ' ');
			}

			return result;
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
	};
}



//buffer    结构空间
namespace bufferStruct {
	//baseBuffer结点
	struct baseBufferNode {
		VkBuffer      buffer{};
		VmaAllocation bufferMem{};



		//创建cpuBuffer
		baseBufferNode(const void* bufferData, VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags memProperty, const VmaAllocator& memAllocator) {
			createBuffer(bufferSize, bufferUsage, memProperty, memAllocator);
			createMemMap(bufferData, bufferSize, memAllocator);
		}

		//todo
		//baseBufferNode() = delete;
		baseBufferNode() {}



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

		//创建memMap
		void createMemMap(const void* bufferData, VkDeviceSize bufferSize, const VmaAllocator& memAllocator) {
			void* memMap{};

			vmaMapMemory(memAllocator, bufferMem, &memMap);
			memcpy(memMap, bufferData, bufferSize);
			vmaUnmapMemory(memAllocator, bufferMem);
		}
	};

	//imageBuffer结点
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

	//imageBufferList结点
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

	//textureBuffer结点
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

			result = ktxTexture2_TranscodeBasis(ktxTexture, KTX_TTF_BC7_RGBA, 0);
			if (result != KTX_SUCCESS) {
				ktxTexture2_Destroy(ktxTexture);
				Log::printError(__FILE__, __LINE__, "KTX贴图转码失败");
				return;
			}



			mipLevel   = ktxTexture->numLevels;
			arrayLayer = 1;
			imageNode.create(VK_IMAGE_TYPE_2D, VK_FORMAT_BC7_UNORM_BLOCK, VkExtent2D{ ktxTexture->baseWidth, ktxTexture->baseHeight }, mipLevel, arrayLayer, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, device, memAllocator);

			baseBufferNode cpuBuffer{ ktxTexture->pData, ktxTexture->dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, memAllocator };
			cmd.copyBufferToImage(device, graphicsQueue, cpuBuffer.buffer, imageNode.image, makeImageCopyRegion(ktxTexture->baseWidth, ktxTexture->baseHeight, mipLevel, ktxTexture), mipLevel, arrayLayer, VK_FORMAT_BC7_UNORM_BLOCK);
			cpuBuffer.clear(memAllocator);
			ktxTexture2_Destroy(ktxTexture);
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
		renderEnum::renderAttId::type      renderAttType      = renderEnum::renderAttId::count;
		renderEnum::renderTemplateId::type renderTemplateType = renderEnum::renderTemplateId::count;

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
				frameImageList.resize(renderEnum::renderFrameId::count);

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
			for (int i = 0; i < frameBufferList.size(); i++) {
				frameBufferList[i].clear(device);
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

			for (int i = 0; i < swap.imageList.size(); i++) {
				viewList.back() = swap.imageList[i].view;
				frameBufferList.push_back(renderStruct::renderFrameNode{ render, viewList, clearValueList, extent, device });
			}
		}
	};
}

//pipeline  结构空间
namespace pipelineStruct {
	//shader结点
	struct shaderNode {
		VkShaderModule shader{};                            //着色器
		VkPipelineShaderStageCreateInfo shaderStage{};      //着色器状态



		//创建着色器结点
		void createShaderNode(const VkDevice& device, const std::string& filename, VkShaderStageFlagBits stage) {
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
			if (vkCreateShaderModule(device, &createInfo, nullptr, &shader) != VK_SUCCESS) {
				Log::printError(__FILE__, __LINE__, "着色器创建失败");
			}

			//配置着色器状态
			shaderStage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStage.pName  = "main";
			shaderStage.stage  = stage;
			shaderStage.module = shader;
		}

		void clear(const VkDevice& device) {
			vkDestroyShaderModule(device, shader, nullptr);
		}
	};

	//pipeConfig结点
	struct pipeConfigNode {
		std::vector<VkPipelineShaderStageCreateInfo>     shaderStageList{};         //着色阶段列表（顶点模式、网格模式）
		VkPipelineVertexInputStateCreateInfo             vertInputStage{};          //顶点配置
		VkPipelineInputAssemblyStateCreateInfo			 primitiveState{};	        //图元配置
		VkViewport										 viewport{};			    //视口区域
		VkRect2D										 scissor{};				    //裁剪区域
		VkPipelineViewportStateCreateInfo				 viewportState{};		    //视口配置
		VkPipelineRasterizationStateCreateInfo			 rasterizer{};			    //光栅化
		VkPipelineMultisampleStateCreateInfo			 multisampling{};		    //多重采样
		VkPipelineDepthStencilStateCreateInfo			 depthStencil{};		    //深度和模板
		std::vector<VkPipelineColorBlendAttachmentState> colorAttachmentList{};     //颜色附件列表（前向模式、延迟模式、阴影模式）
		VkPipelineColorBlendStateCreateInfo				 colorAttachmentState{};    //颜色附件配置
		std::vector<VkDynamicState>                      pipeDynamicStageList{};    //管线动态阶段列表（Vulkan 1.4 共 48 个）
		VkPipelineDynamicStateCreateInfo                 pipeDynamicStageState{};   //管线动态阶段配置
		VkGraphicsPipelineCreateInfo					 pipeInfo{};			    //管线信息



		void createPipeConfigNode(const std::vector<pipelineStruct::shaderNode>& shaderList, VkSampleCountFlagBits msaa, renderEnum::renderAttId::type renderAttType, const VkRenderPass& render, const VkPipelineLayout& pipeLayout) {
			//着色阶段列表
			shaderStageList.resize(shaderList.size());
			for (int i = 0; i < shaderList.size(); i++) {
				shaderStageList[i] = shaderList[i].shaderStage;
			}

			//图元配置
			primitiveState.sType					 = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			primitiveState.topology				 = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;		//图元装配方式
			primitiveState.primitiveRestartEnable = VK_FALSE;

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

			//光栅化
			rasterizer.sType				   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterizer.depthClampEnable		   = VK_FALSE;
			rasterizer.rasterizerDiscardEnable = VK_FALSE;
			rasterizer.polygonMode			   = VK_POLYGON_MODE_FILL;				//决定几何产生图片的内容
			rasterizer.lineWidth			   = 1.0f;
			rasterizer.cullMode				   = VK_CULL_MODE_BACK_BIT;				//面剔除
			rasterizer.frontFace			   = VK_FRONT_FACE_COUNTER_CLOCKWISE;	//面的加载顺序
			rasterizer.depthBiasEnable		   = VK_TRUE;							//启用深度偏移，让mine能覆盖方块
			rasterizer.depthBiasConstantFactor = 1.0f;								//常量偏移量
			rasterizer.depthBiasClamp		   = 0.0f;								//不限制最小偏移
			rasterizer.depthBiasSlopeFactor    = 1.0f;								//基于斜率的深度偏移

			//多重采样
			multisampling.sType					= VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multisampling.sampleShadingEnable	= VK_FALSE;
			multisampling.rasterizationSamples	= msaa;
			multisampling.minSampleShading		= 1.0f;													
			multisampling.pSampleMask			= nullptr;													
			multisampling.alphaToCoverageEnable = VK_FALSE;											
			multisampling.alphaToOneEnable		= VK_FALSE;												

			//深度和模板
			depthStencil.sType				   = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			depthStencil.depthTestEnable	   = VK_TRUE;							//指定是否应该将新的深度缓冲区与深度缓冲区进行比较，以确认是否应该被丢弃
			depthStencil.depthWriteEnable	   = VK_TRUE;							//指定通过深度测试的新的片段深度是否应该被实际写入深度缓冲区
			depthStencil.depthCompareOp		   = VK_COMPARE_OP_LESS;				//指定执行保留或者丢弃片段的比较细节
			depthStencil.depthBoundsTestEnable = VK_FALSE;							//depthBoundsTestEnable, minDepthBounds 和 maxDepthBounds 字段用于可选择的优化深度绑定测试
			depthStencil.minDepthBounds		   = 0.0f;
			depthStencil.maxDepthBounds		   = 1.0f;
			depthStencil.stencilTestEnable	   = VK_FALSE;							//用于配置模版缓冲区的操作
			depthStencil.front				   = {};
			depthStencil.back				   = {};

			//颜色附件列表
			VkPipelineColorBlendAttachmentState colorAttachmentNode{
				.blendEnable         = VK_FALSE,
				.srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
				.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
				.colorBlendOp		 = VK_BLEND_OP_ADD,
				.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
				.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
				.alphaBlendOp		 = VK_BLEND_OP_ADD,
				.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
			};

			switch (renderAttType) {
			case renderEnum::renderAttId::forward_ra:
				colorAttachmentList.push_back(colorAttachmentNode);
				break;
			case renderEnum::renderAttId::deferred_ra:
				colorAttachmentList.push_back(colorAttachmentNode);
				colorAttachmentList.push_back(colorAttachmentNode);
				colorAttachmentList.push_back(colorAttachmentNode);
				break;
			case renderEnum::renderAttId::shadowMap_ra:
				//阴影管线无需颜色附件
				break;
			}

			//颜色附件配置
			colorAttachmentState.sType			   = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			colorAttachmentState.logicOpEnable	   = VK_FALSE;
			colorAttachmentState.logicOp		   = VK_LOGIC_OP_COPY;		
			colorAttachmentState.attachmentCount   = static_cast<uint32_t>(colorAttachmentList.size());
			colorAttachmentState.pAttachments	   = colorAttachmentList.data();
			colorAttachmentState.blendConstants[0] = 0.0f;				
			colorAttachmentState.blendConstants[1] = 0.0f;				
			colorAttachmentState.blendConstants[2] = 0.0f;				
			colorAttachmentState.blendConstants[3] = 0.0f;				

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
			pipeInfo.pDynamicState		 = nullptr;					
			pipeInfo.layout			     = pipeLayout;
			pipeInfo.renderPass          = render;
			pipeInfo.subpass			 = 0;
			pipeInfo.basePipelineHandle  = VK_NULL_HANDLE;		
			pipeInfo.basePipelineIndex	 = -1;
		}



		//设置管线动态处理的阶段
		void setPipeDynamicStage(const std::vector<VkDynamicState>& list) {
			pipeDynamicStageList                    = list;
			pipeDynamicStageState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			pipeDynamicStageState.dynamicStateCount = static_cast<uint32_t>(pipeDynamicStageList.size());
			pipeDynamicStageState.pDynamicStates    = pipeDynamicStageList.data();

			pipeInfo.pDynamicState = &pipeDynamicStageState;
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
		VkPipelineLayout pipeLayout{};      //管线布局



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
		std::vector<pipelineStruct::shaderNode> shaderList{};   //着色器列表
		pipelineStruct::pipeConfigNode          pipeConfig{};   //管线配置
		VkPipeline                              pipeline{};     //管线



		//创建顶点管线结点
		void createPipelineNode(VkSampleCountFlagBits msaa, const renderStruct::renderTemplateNode& renderTemplateNode, const pipelineStruct::pipeLayoutNode& pipeLayoutNode, const std::vector<std::string>& shaderFileList, const VkDevice& device) {
			//创建shaderList
			shaderList.resize(shaderFileList.size());
			if (shaderFileList.size() == pipelineEnum::shaderNodeId::vert::count) {
				shaderList[pipelineEnum::shaderNodeId::vert::vert].createShaderNode(device, shaderFileList[0], VK_SHADER_STAGE_VERTEX_BIT);
				shaderList[pipelineEnum::shaderNodeId::vert::frag].createShaderNode(device, shaderFileList[1], VK_SHADER_STAGE_FRAGMENT_BIT);
			}
			else if (shaderFileList.size() == pipelineEnum::shaderNodeId::mesh::count) {
				//todo 待写网格着色
			}

			//创建pipeConfig
			pipeConfig.createPipeConfigNode(shaderList, msaa, renderTemplateNode.renderAttType, renderTemplateNode.render, pipeLayoutNode.pipeLayout);
		}

		void clear(const VkDevice& device) {
			vkDestroyPipeline(device, pipeline, nullptr);

			for (auto shaderNode : shaderList) {
				shaderNode.clear(device);
			}
		}

		void clearSwapChain(const VkDevice& device) {
			vkDestroyPipeline(device, pipeline, nullptr);
		}
	};
}



//todo后续慢慢拆分
//buffer    结构空间
namespace bufferStruct {
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



	/*管线缓冲区*/
	struct pipeTerrainNode {
		//程序化索引
		bool isEnable;
		pipelineEnum::procPipelineId::type pipeType;    //管线索引
		bufferEnum::procSamplerId::type samplerType;    //采样索引

		//todo baseBufferNode    mesh[bufferInfo::commandThreadCount][plateEnum::terrainLayer::count];     //网格缓冲
		baseBufferNode vertex[bufferInfo::commandThreadCount][plateEnum::terrainLayer::count];
		baseBufferNode index[bufferInfo::commandThreadCount][plateEnum::terrainLayer::count];

		textureBufferNode texture{};                    //纹理缓冲

		//统一缓冲
		baseBufferNode uniform;
		mvpUBO ubo;

		//描述缓冲
		VkDescriptorSet descriptorSet;



		pipeTerrainNode(bool tEnable, pipelineEnum::procPipelineId::type tPipeType, bufferEnum::procSamplerId::type tSamplerType) :isEnable(tEnable), pipeType(tPipeType), samplerType(tSamplerType) {}

		pipeTerrainNode() = delete;



		//创建地形纹理：KTX 图集按 tile 切成纹理数组
		void create(const std::string& imageFile, VkFormat colorFormat, const VkExtent2D& tileExtent, const VkExtent2D& atlasExtent, cmdStruct::cmdNode& cmd, const VkDevice& device, const VkQueue& graphicsQueue, const VmaAllocator& memAllocator) {
			texture.textureFile = imageFile;

			if (isEnable) {
				texture.createKtxArray(colorFormat, tileExtent, atlasExtent, cmd, device, graphicsQueue, memAllocator);
			}
		}

		//清理结构体，descriptorSet由描述符池管理，无需手动清理
		void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
			for (int thread = 0; thread < bufferInfo::commandThreadCount; thread++) {
				for (int layer = 0; layer < plateEnum::terrainLayer::count; layer++) {
					vmaDestroyBuffer(memAllocator, vertex[thread][layer].buffer, vertex[thread][layer].bufferMem);
					vmaDestroyBuffer(memAllocator, index[thread][layer].buffer, index[thread][layer].bufferMem);
				}
			}

			texture.clear(device, memAllocator);

			vmaDestroyBuffer(memAllocator, uniform.buffer, uniform.bufferMem);
		}
	};



	struct modelStaticNode {
		//程序化索引
		bufferEnum::procSamplerId::type samplerType;  //采样索引

		//网格缓冲
		baseBufferNode vertex;
		baseBufferNode index;

		//纹理缓冲
		textureBufferNode texture{};

		//统一缓冲
		baseBufferNode uniform;
		mvpUBO ubo;

		//描述缓冲
		VkDescriptorSet descriptorSet;

		//用于屏蔽超出视觉范围模型的命令提交和统一缓冲更新
		glm::dvec3 modelWorldPos;           //模型世界坐标（d世界坐标系），在初始化时赋值
		glm::dvec3 currentWorldPos;         //当前世界坐标（d世界坐标系），在更新统一缓冲时赋值
		bool isVisible = true;              //模型可见标识，在提交命令时赋值



		modelStaticNode(bufferEnum::procSamplerId::type tSamplerType, const glm::dvec3& tModelWorldPos, const std::string& tTextureFile /*, const std::string& tNormalFile*/) :samplerType(tSamplerType) {

			texture.textureFile = tTextureFile;
			//texture.normalFile = tNormalFile;
			modelWorldPos = tModelWorldPos;
		}

		modelStaticNode() = delete;



		void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
			vmaDestroyBuffer(memAllocator, vertex.buffer, vertex.bufferMem);
			vmaDestroyBuffer(memAllocator, index.buffer, index.bufferMem);

			texture.clear(device, memAllocator);
			vmaDestroyBuffer(memAllocator, uniform.buffer, uniform.bufferMem);
		}
	};

	struct modelDynamicSubNode {
		//网格缓冲
		baseBufferNode vertex;
		baseBufferNode index;

		//纹理缓冲
		textureBufferNode texture{};

		//统一缓冲
		baseBufferNode uniform;
		mvpUBO ubo;

		//描述缓冲
		VkDescriptorSet descriptorSet;



		void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
			vmaDestroyBuffer(memAllocator, vertex.buffer, vertex.bufferMem);
			vmaDestroyBuffer(memAllocator, index.buffer, index.bufferMem);

			texture.clear(device, memAllocator);
			vmaDestroyBuffer(memAllocator, uniform.buffer, uniform.bufferMem);
		}
	};

	struct modelDynamicNode {
		//程序化索引
		bufferEnum::procSamplerId::type samplerType;  //采样索引

		//动态模型结点列表
		std::vector<modelDynamicSubNode> node;

		//用于屏蔽超出视觉范围模型的命令提交和统一缓冲更新
		glm::dvec3 modelWorldPos;           //模型世界坐标（d世界坐标系），在初始化时赋值
		glm::dvec3 currentWorldPos;         //当前世界坐标（d世界坐标系），在更新统一缓冲时赋值
		bool isVisible = true;              //模型可见标识，在提交命令时赋值



		modelDynamicNode(bufferEnum::procSamplerId::type tSamplerType, const glm::dvec3& tModelWorldPos, const std::vector<std::string>& tTextureFile /*, const std::vector<std::string>& tNormalFile*/) :samplerType(tSamplerType) {

			node.resize(tTextureFile.size());
			for (int i = 0; i < tTextureFile.size(); i++) {
				node[i].texture.textureFile = tTextureFile[i];
				//node[i].texture.normalFile = tNormalFile[i];
			}

			modelWorldPos = tModelWorldPos;
		}

		modelDynamicNode() = delete;



		void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
			for (int i = 0; i < node.size(); i++) {
				node[i].clear(device, memAllocator);
			}
		}
	};
	
	struct pipeModelNode {
		//程序化索引
		bool isEnable;
		pipelineEnum::procPipelineId::type pipeType;  //管线索引

		std::vector<bufferStruct::modelStaticNode>  sNode{};
		std::vector<bufferStruct::modelDynamicNode> dNode{};



		pipeModelNode(bool tEnable, pipelineEnum::procPipelineId::type tPipeType) :isEnable(tEnable), pipeType(tPipeType) {}

		pipeModelNode() = delete;



		void create(cmdStruct::cmdNode& cmd, const VkDevice& device, const VkQueue& graphicsQueue, const VmaAllocator& memAllocator) {
			sNode.emplace_back(bufferEnum::procSamplerId::staticModel_sn, glm::dvec3(0, 32, 0), fileInfo::image::temple);

			dNode.emplace_back(bufferEnum::procSamplerId::dynamicModel_sl, glm::dvec3(0, 32, 0), std::vector<std::string>{ fileInfo::image::droidBodyC, fileInfo::image::droidHeadC, fileInfo::image::droidMiscC } /*, std::vector<std::string>{ fileInfo::image::droidBodyN, fileInfo::image::droidHeadN, fileInfo::image::droidMiscN }*/);



			for (int i = 0; i < bufferEnum::procDynamicModelId::count; i++) {
				modelVertex::dVertexs[i].resize(dNode[i].node.size());
				modelVertex::dIndexs[i].resize(dNode[i].node.size());
			}

			if (isEnable) {
				//model-sNode
				for (int i = 0; i < bufferEnum::procStaticModelId::count; i++) {
					sNode[i].texture.createKtx(cmd, device, graphicsQueue, memAllocator);
				}

				//model-dNode
				for (int i = 0; i < bufferEnum::procDynamicModelId::count; i++) {
					for (int j = 0; j < dNode[i].node.size(); j++) {
						dNode[i].node[j].texture.createKtx(cmd, device, graphicsQueue, memAllocator);
					}
				}
			}
		}

		void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
			for (int i = 0; i < bufferEnum::procStaticModelId::count; i++) {
				sNode[i].clear(device, memAllocator);
			}

			for (int i = 0; i < bufferEnum::procDynamicModelId::count; i++) {
				dNode[i].clear(device, memAllocator);
			}
		}



		//获取最大模型mip
		std::pair<uint32_t, uint32_t> getMaxModelMip() {
			uint32_t sModelMip = 0;         //无模型时保持 0，避免对空列表取 max 的未定义行为
			uint32_t dModelMip = 0;

			for (const bufferStruct::modelStaticNode& i : sNode) {
				sModelMip = std::max(sModelMip, i.texture.mipLevel);
			}

			for (const bufferStruct::modelDynamicNode& i : dNode) {
				for (const bufferStruct::modelDynamicSubNode& j : i.node) {
					dModelMip = std::max(dModelMip, j.texture.mipLevel);
				}
			}

			return { sModelMip, dModelMip };   //使用模型最大mip创建采样器
		}

		//加载静态模型
		void loadStaticModel(const std::string& filename, std::vector<modelVertex::vertexNode>& vertexs, std::vector<uint32_t>& indexs, std::vector<shadowVertex::vertexNode>& shadowVertexs) {
			tinygltf::Model model;

			if (!openGltf(filename, model)) { return; }

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

			if (!openGltf(filename, model)) { return; }

			//mesh 数与子结点数必须一一对应，否则下面按下标写入会越界
			if (model.meshes.size() != vertexsList.size()) {
				Log::printError(__FILE__, __LINE__, "glTF的mesh数量与子结点数量不一致: \t" + filename);
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
		void loadGltfModel(const tinygltf::Model& model, const tinygltf::Mesh& mesh, std::vector<modelVertex::vertexNode>& vertexs, std::vector<uint32_t>& indexs, std::vector<shadowVertex::vertexNode>& shadowVertexs, std::unordered_map<modelVertex::vertexNode, uint32_t>& uniqueVertices) {
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
	};



	struct pipeSkyboxNode {
		//程序化索引
		bool isEnable;
		pipelineEnum::procPipelineId::type pipeType;  //管线索引

		//网格缓冲
		baseBufferNode vertex;
		baseBufferNode index;

		//统一缓冲
		baseBufferNode uniform;
		mvpUBO ubo;
		baseBufferNode lightUniform;
		forwardLightUBO lightUbo;

		//描述缓冲
		VkDescriptorSet descriptorSet;



		pipeSkyboxNode(bool tEnable, pipelineEnum::procPipelineId::type tPipeType) :isEnable(tEnable), pipeType(tPipeType) {}

		pipeSkyboxNode() = delete;



		void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
			vmaDestroyBuffer(memAllocator, vertex.buffer, vertex.bufferMem);
			vmaDestroyBuffer(memAllocator, index.buffer, index.bufferMem);

			vmaDestroyBuffer(memAllocator, uniform.buffer, uniform.bufferMem);
			vmaDestroyBuffer(memAllocator, lightUniform.buffer, lightUniform.bufferMem);
		}
	};

	struct pipeMineNode {
		//程序化索引
		bool isEnable;
		pipelineEnum::procPipelineId::type pipeType;  //管线索引

		//网格缓冲
		baseBufferNode vertex;
		baseBufferNode index;

		//统一缓冲
		baseBufferNode uniform;
		mvpUBO ubo;

		//描述缓冲
		VkDescriptorSet descriptorSet;



		pipeMineNode(bool tEnable, pipelineEnum::procPipelineId::type tPipeType) :isEnable(tEnable), pipeType(tPipeType) {}

		pipeMineNode() = delete;



		void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
			vmaDestroyBuffer(memAllocator, vertex.buffer, vertex.bufferMem);
			vmaDestroyBuffer(memAllocator, index.buffer, index.bufferMem);

			vmaDestroyBuffer(memAllocator, uniform.buffer, uniform.bufferMem);
		}
	};

	struct pipeUITextNode {
		//程序化索引
		bool isEnable;
		pipelineEnum::procPipelineId::type pipeType;  //管线索引
		bufferEnum::procSamplerId::type samplerType;  //采样索引

		baseBufferNode  vertex{};                                                           //网格缓冲
		uiStruct::msdfGlyphAtlasNode msdfGlyphAtlas{ fileInfo::font::sourceHanSansSC };     //字体图集
		textureBufferNode texture{};                                                        //纹理缓冲，MTSDF 图集，RGBA8，mip=1

		//描述缓冲
		VkDescriptorSet descriptorSet;



		pipeUITextNode(bool tEnable, pipelineEnum::procPipelineId::type tPipeType, bufferEnum::procSamplerId::type tSamplerType) :isEnable(tEnable), pipeType(tPipeType), samplerType(tSamplerType) {}

		pipeUITextNode() = delete;



		//创建字体图集纹理
		void create(cmdStruct::cmdNode& cmd, const VkDevice& device, const VkQueue& graphicsQueue, const VmaAllocator& memAllocator) {
			if (isEnable) {
				texture.createText(msdfGlyphAtlas.data.data(), msdfGlyphAtlas.data.size(), static_cast<uint32_t>(msdfGlyphAtlas.width), static_cast<uint32_t>(msdfGlyphAtlas.height), cmd, device, graphicsQueue, memAllocator);
			}
		}

		void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
			vmaDestroyBuffer(memAllocator, vertex.buffer, vertex.bufferMem);
			texture.clear(device, memAllocator);
		}
	};

	struct pipeOSMirrorNode {
		//程序化索引
		bool isEnable;
		pipelineEnum::procPipelineId::type pipeType;  //管线索引
		bufferEnum::procSamplerId::type samplerType;  //采样索引

		//网格缓冲
		baseBufferNode vertex;
		baseBufferNode index;

		//统一缓冲
		baseBufferNode uniform;
		mvpUBO ubo;
		baseBufferNode lightUniform;
		forwardLightUBO lightUbo;

		//描述缓冲
		VkDescriptorSet descriptorSet;



		pipeOSMirrorNode(bool tEnable, pipelineEnum::procPipelineId::type tPipeType, bufferEnum::procSamplerId::type tSamplerType) :isEnable(tEnable), pipeType(tPipeType), samplerType(tSamplerType) {}

		pipeOSMirrorNode() = delete;



		void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
			vmaDestroyBuffer(memAllocator, vertex.buffer, vertex.bufferMem);
			vmaDestroyBuffer(memAllocator, index.buffer, index.bufferMem);

			vmaDestroyBuffer(memAllocator, uniform.buffer, uniform.bufferMem);
			vmaDestroyBuffer(memAllocator, lightUniform.buffer, lightUniform.bufferMem);
		}
	};

	struct pipeOSScenceNode {
		//程序化索引
		bool isEnable;
		pipelineEnum::procPipelineId::type pipeType;  //管线索引
		bufferEnum::procSamplerId::type samplerType;  //采样索引

		//描述缓冲
		VkDescriptorSet descriptorSet;



		pipeOSScenceNode(bool tEnable, pipelineEnum::procPipelineId::type tPipeType, bufferEnum::procSamplerId::type tSamplerType) :isEnable(tEnable), pipeType(tPipeType), samplerType(tSamplerType) {}

		pipeOSScenceNode() = delete;



		void clear(const VkDevice& device, const VmaAllocator& memAllocator) {

		}
	};

	struct pipeDSScenceNode {
		//程序化索引
		bool isEnable;
		pipelineEnum::procPipelineId::type pipeType;  //管线索引
		bufferEnum::procSamplerId::type samplerType;  //采样索引

		//统一缓冲
		baseBufferNode lightUniform;
		deferredLightUBO lightUbo;
		baseBufferNode shadowUniform;
		deferredShadowUBO shadowUbo;

		//描述缓冲
		VkDescriptorSet descriptorSet;



		pipeDSScenceNode(bool tEnable, pipelineEnum::procPipelineId::type tPipeType, bufferEnum::procSamplerId::type tSamplerType) :isEnable(tEnable), pipeType(tPipeType), samplerType(tSamplerType) {}

		pipeDSScenceNode() = delete;



		void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
			vmaDestroyBuffer(memAllocator, lightUniform.buffer,  lightUniform.bufferMem);
			vmaDestroyBuffer(memAllocator, shadowUniform.buffer, shadowUniform.bufferMem);
		}
	};



	struct shadowTerrainNode {
		//网格缓冲，索引直接用源网格的
		baseBufferNode vertex[bufferInfo::commandThreadCount][plateEnum::terrainLayer::count];

		//统一缓冲
		baseBufferNode shadowMvpUniform;
		shadowMvpUBO shadowMvpUbo;

		//描述缓冲
		VkDescriptorSet descriptorSet;



		void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
			for (int thread = 0; thread < bufferInfo::commandThreadCount; thread++) {
				for (int layer = 0; layer < plateEnum::terrainLayer::count; layer++) {
					vmaDestroyBuffer(memAllocator, vertex[thread][layer].buffer, vertex[thread][layer].bufferMem);
				}
			}

			vmaDestroyBuffer(memAllocator, shadowMvpUniform.buffer, shadowMvpUniform.bufferMem);
		}
	};

	struct shadowModelNode {
		//网格缓冲，索引直接用源网格的
		baseBufferNode vertex;

		//统一缓冲
		baseBufferNode shadowMvpUniform;
		shadowMvpUBO shadowMvpUbo;

		//描述缓冲
		VkDescriptorSet descriptorSet;



		void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
			vmaDestroyBuffer(memAllocator, vertex.buffer, vertex.bufferMem);

			vmaDestroyBuffer(memAllocator, shadowMvpUniform.buffer, shadowMvpUniform.bufferMem);
		}
	};
	
	struct pipeShadowNode {
		//程序化索引
		bool isEnable;
		pipelineEnum::procPipelineId::type pipeType;  //管线索引
		bufferEnum::procSamplerId::type samplerType;  //采样索引

		bufferStruct::shadowTerrainNode shadow_t{};                             //地形阴影
		std::vector<bufferStruct::shadowModelNode> shadow_ms{};                 //静态模型阴影
		std::vector<std::vector<bufferStruct::shadowModelNode>> shadow_md{};    //动态模型阴影

		//辅助shadowMvp计算
		glm::mat4 shadowProj{};
		glm::mat4 shadowVP{};



		pipeShadowNode(bool tEnable, pipelineEnum::procPipelineId::type tPipeType, bufferEnum::procSamplerId::type tSamplerType) :isEnable(tEnable), pipeType(tPipeType), samplerType(tSamplerType) {}

		pipeShadowNode() = delete;



		//创建阴影结点列表：按模型 id 数量准备
		void create() {
			shadow_ms.resize(bufferEnum::procStaticModelId::count);
			shadow_md.resize(bufferEnum::procDynamicModelId::count);
		}

		void clear(const VkDevice& device, const VmaAllocator& memAllocator) {
			shadow_t.clear(device, memAllocator);

			for (int i = 0; i < shadow_ms.size(); i++) {
				shadow_ms[i].clear(device, memAllocator);
			}

			for (int i = 0; i < shadow_md.size(); i++) {
				for (int j = 0; j < shadow_md[i].size(); j++) {
					shadow_md[i][j].clear(device, memAllocator);
				}
			}
		}
	};



	struct descriptorConfigNode {
		VkDescriptorPoolSize         pool{};
		VkDescriptorSetLayoutBinding layout{};
		VkWriteDescriptorSet         write{};



		descriptorConfigNode(uint32_t id, VkDescriptorType type, VkShaderStageFlags stage, uint32_t count) {
			pool.type = type;
			pool.descriptorCount = count;

			layout.descriptorCount = 1;
			layout.pImmutableSamplers = nullptr;
			layout.binding        = id;
			layout.descriptorType = type;
			layout.stageFlags     = stage;

			write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write.dstArrayElement = 0;
			write.descriptorCount = 1;
			write.dstBinding      = id;
			write.descriptorType  = type;

			bufferInfo.offset = 0;
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}

		descriptorConfigNode() = delete;



		const VkWriteDescriptorSet& setWrite(const VkDescriptorSet& dstSet, const VkBuffer& buffer, VkDeviceSize range) {
			bufferInfo.buffer = buffer;
			bufferInfo.range  = range;

			write.dstSet      = dstSet;
			write.pBufferInfo = &bufferInfo;

			return write;
		}

		const VkWriteDescriptorSet& setWrite(const VkDescriptorSet& dstSet, const VkSampler& sampler, const VkImageView& imageView) {
			imageInfo.sampler   = sampler;
			imageInfo.imageView = imageView;

			write.dstSet     = dstSet;
			write.pImageInfo = &imageInfo;

			return write;
		}

	private:
		VkDescriptorBufferInfo bufferInfo{};
		VkDescriptorImageInfo imageInfo{};
	};

	struct descriptorNode {
		std::vector<bufferStruct::descriptorConfigNode> config{};

		VkDescriptorPool pool{};            //描述符池
		VkDescriptorSetLayout layout{};     //描述符布局



		void createDescriptorNode(const VkDevice& device) {
			createPool(device);
			createLayout(device);
		}

		void clear(const VkDevice& device) {
			vkDestroyDescriptorPool(device, pool, nullptr);
			vkDestroyDescriptorSetLayout(device, layout, nullptr);
		}

	private:
		void createPool(const VkDevice& device) {
			std::vector<VkDescriptorPoolSize> poolSize{};
			for (const auto& i : config) {
				poolSize.push_back(i.pool);
			}

			VkDescriptorPoolCreateInfo poolInfo{
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
				.maxSets = config[0].pool.descriptorCount,
				.poolSizeCount = static_cast<uint32_t>(poolSize.size()),
				.pPoolSizes = poolSize.data()
			};

			if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &pool) != VK_SUCCESS) {
				Log::printError(__FILE__, __LINE__, "描述符池创建失败");
			}
		}

		void createLayout(const VkDevice& device) {
			std::vector<VkDescriptorSetLayoutBinding> layoutBinding{};
			for (const auto& i : config) {
				layoutBinding.push_back(i.layout);
			}

			VkDescriptorSetLayoutCreateInfo layoutInfo{
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
				.pNext = nullptr,
				.bindingCount = static_cast<uint32_t>(layoutBinding.size()),
				.pBindings = layoutBinding.data()
			};

			if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &layout) != VK_SUCCESS) {
				Log::printError(__FILE__, __LINE__, "描述符布局配置失败");
			}
		}
	};
}