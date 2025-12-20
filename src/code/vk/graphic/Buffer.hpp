#pragma once



class Buffer {
public:
	bufferStruct::commandNode command{};



	Buffer(Device& Device) :Device{ Device } {
		createScResource();
		procRenderPass.createRenderPassInfo(InitSwapchain.scExtent, InitSwapchain.colorFormat, InitSwapchain.depthFormat, InitSwapchain.msaaSamples, Device.device, Device.allocator);

		command.createCommandPool(Device.device, Device.queueFamilyIndex);
		command.initCommandBuffer(uint32_t(procRenderPass.node[bufferEnum::procRenderId::scScence].scPtr->framebuffer.size()));

		createMeshBuffer();
		createUniformBuffer();
		createTexture();
		procModel.getMaxModelMip();
		procSampler.createTextureSampler(Device.device, terrain.mipLevel, procModel.sModelMip, procModel.dModelMip);
		createDescriptorSet();
		initUniformBuffer();

		if (debugInfo::LogSwitch) {
			Log::printTitle("————————Buffer已创建");

			Log::delimiter();
		}
	}

	~Buffer() {
		procRenderPass.clear(Device.device, Device.allocator);
		procDescriptor.clear(Device.device);
		procSampler.clear(Device.device);

		command.clear(Device.device);

		terrain.clear(Device.device, Device.allocator);
		procModel.clear(Device.device, Device.allocator);
		skybox.clear(Device.device, Device.allocator);
		mine.clear(Device.device, Device.allocator);
		osMirror.clear(Device.device, Device.allocator);
		osScence.clear(Device.device, Device.allocator);
		dsScence.clear(Device.device, Device.allocator);
		uiText.clear(Device.device, Device.allocator);

		if (debugInfo::LogSwitch) {
			Log::print("Buffer已清除");
		}
	}

private:
	Device& Device;

	Swapchain InitSwapchain{ Device };

	bufferList::procRenderNode		procRenderPass{};
	bufferList::procDescriptorNode	procDescriptor{ Device.device };
	bufferList::procSamplerNode		procSampler;

	bufferStruct::terrainBufferNode  terrain	{ pipelineEnum::procPipelineId::terrain,  bufferEnum::procSamplerId::terrain };
	bufferList::procModelNode		 procModel	{ pipelineEnum::procPipelineId::model	  };
	bufferStruct::skyboxBufferNode	 skybox		{ pipelineEnum::procPipelineId::skybox	  };
	bufferStruct::mineBufferNode	 mine		{ pipelineEnum::procPipelineId::mine	  };
	bufferStruct::osMirrorBufferNode osMirror	{ pipelineEnum::procPipelineId::osMirror, bufferEnum::procSamplerId::offScreen };
	bufferStruct::osScenceBufferNode osScence	{ pipelineEnum::procPipelineId::osScence, bufferEnum::procSamplerId::offScreen };
	bufferStruct::dsScenceBufferNode dsScence	{ pipelineEnum::procPipelineId::dsScence, bufferEnum::procSamplerId::offScreen };
	bufferStruct::uiTextBufferNode	 uiText		{ pipelineEnum::procPipelineId::uiText,   bufferEnum::procSamplerId::uiText };



	//创建交换链附件
	void createScResource() {
		bufferStruct::scFramebufferNode& frameNode = procRenderPass.getScFramebufferNode(bufferEnum::procRenderId::scScence);

		createImage(InitSwapchain.scExtent.width, InitSwapchain.scExtent.height, 1, 1, InitSwapchain.colorFormat, InitSwapchain.msaaSamples, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, frameNode.color.image, frameNode.color.imageMemory);
		createImageView(frameNode.color.image, frameNode.color.view, 1, 1, VK_IMAGE_VIEW_TYPE_2D, InitSwapchain.colorFormat, VK_IMAGE_ASPECT_COLOR_BIT);

		createImage(InitSwapchain.scExtent.width, InitSwapchain.scExtent.height, 1, 1, InitSwapchain.depthFormat, InitSwapchain.msaaSamples, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, frameNode.depth.image, frameNode.depth.imageMemory);
		createImageView(frameNode.depth.image, frameNode.depth.view, 1, 1, VK_IMAGE_VIEW_TYPE_2D, InitSwapchain.depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

		InitSwapchain.createScImage(frameNode.scImage);
		frameNode.scView.resize(frameNode.scImage.size());
		for (int i = 0; i < frameNode.scImage.size(); i++) {
			createImageView(frameNode.scImage[i], frameNode.scView[i], 1, 1, VK_IMAGE_VIEW_TYPE_2D, InitSwapchain.colorFormat, VK_IMAGE_ASPECT_COLOR_BIT);
		}
	}

	void createMeshBuffer() {
		//terrain
		for (int thread = 0; thread < bufferInfo::commandThreadSize; thread++) {
			for (int layer = 0; layer < terrainEnum::terrainLayer::count; layer++) {
				createVertexBuffer(terrainVertex::vertexs[layer], terrain.vertex[thread][layer]);
				createIndexBuffer(terrainVertex::indexs[layer], terrain.index[thread][layer]);
			}
		}

		//model-sNode
		for (int i = 0; i < bufferEnum::procStaticModelId::count; i++) {
			loadStaticModel(procModel.sNode[i].modelFile, modelVertex::sVertexs[i], modelVertex::sIndexs[i]);
			createVertexBuffer(modelVertex::sVertexs[i], procModel.sNode[i].vertex);
			createIndexBuffer(modelVertex::sIndexs[i], procModel.sNode[i].index);
		}

		//model-dNode
		for (int i = 0; i < bufferEnum::procDynamicModelId::count; i++) {
			loadDynamicModel(procModel.dNode[i].modelFile, modelVertex::dVertexs[i], modelVertex::dIndexs[i]);

			for (int j = 0; j < procModel.dNode[i].node.size(); j++) {
				createVertexBuffer(modelVertex::dVertexs[i][j], procModel.dNode[i].node[j].vertex);
				createIndexBuffer(modelVertex::dIndexs[i][j], procModel.dNode[i].node[j].index);
			}
		}

		//skybox
		createVertexBuffer(skyboxVertex::vertexs, skybox.vertex);
		createIndexBuffer(skyboxVertex::indexs, skybox.index);

		//mine
		createVertexBuffer(mineVertex::vertexs, mine.vertex);
		createIndexBuffer(mineVertex::indexs, mine.index);

		//osMirror
		createVertexBuffer(osMirrorVertex::vertexs, osMirror.vertex);
		createIndexBuffer(osMirrorVertex::indexs, osMirror.index);

		//osScence
		createVertexBuffer(osScenceVertex::vertexs, osScence.vertex);
		createIndexBuffer(osScenceVertex::indexs, osScence.index);

		//uiText
		createBuffer(2048 * sizeof(glm::vec4), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uiText.vertex);
	}

	void createUniformBuffer() {
		//terrain
		createBuffer(sizeof(terrain.ubo), terrain.uniform);
		createBuffer(sizeof(terrain.lightUbo), terrain.lightUniform);

		//model-sNode
		for (int i = 0; i < bufferEnum::procStaticModelId::count; i++) {
			createBuffer(sizeof(procModel.sNode[i].ubo), procModel.sNode[i].uniform);
			createBuffer(sizeof(procModel.sNode[i].lightUbo), procModel.sNode[i].lightUniform);
		}

		//model-dNode
		for (int i = 0; i < bufferEnum::procDynamicModelId::count; i++) {
			for (int j = 0; j < procModel.dNode[i].node.size(); j++) {
				createBuffer(sizeof(procModel.dNode[i].node[j].ubo), procModel.dNode[i].node[j].uniform);
			}

			createBuffer(sizeof(procModel.dNode[i].lightUbo), procModel.dNode[i].lightUniform);
		}

		//skybox
		createBuffer(sizeof(skybox.ubo), skybox.uniform);
		createBuffer(sizeof(skybox.lightUbo), skybox.lightUniform);

		//mine
		createBuffer(sizeof(mine.ubo), mine.uniform);

		//osMirror
		createBuffer(sizeof(osMirror.ubo), osMirror.uniform);
		createBuffer(sizeof(osMirror.lightUbo), osMirror.lightUniform);

		//dsScence
		createBuffer(sizeof(dsScence.lightUbo), dsScence.lightUniform);
	}

	void createTexture() {
		//terrain
		createTextureKtxArrayNode(terrain.textureFile, terrain.texture, terrain.arrayLayer, terrain.mipLevel, VK_FORMAT_R8G8B8A8_UNORM, 4);
		createTextureKtxArrayNode(terrain.specularFile, terrain.specular, terrain.arrayLayer, terrain.mipLevel, VK_FORMAT_R8G8_UNORM, 2);

		//model-sNode
		for (int i = 0; i < bufferEnum::procStaticModelId::count; i++) {
			createTextureKtxNode(procModel.sNode[i].textureFile, procModel.sNode[i].texture, procModel.sNode[i].mipLevel);
			createTextureKtxNode(procModel.sNode[i].normalFile,  procModel.sNode[i].normal,  procModel.sNode[i].mipLevel);
		}

		//model-dNode
		for (int i = 0; i < bufferEnum::procDynamicModelId::count; i++) {
			for (int j = 0; j < procModel.dNode[i].node.size(); j++) {
				createTextureKtxNode(procModel.dNode[i].node[j].textureFile, procModel.dNode[i].node[j].texture, procModel.dNode[i].mipLevel);
				createTextureKtxNode(procModel.dNode[i].node[j].normalFile,  procModel.dNode[i].node[j].normal,  procModel.dNode[i].mipLevel);
			}
		}

		//uiText
		createTextNode(uiText.texture);
	}

	void createDescriptorSet() {
		/*配置scenceForward描述符*/
		procDescriptor.setAllocInfo(bufferEnum::procDescriptorId::scenceForward);

		//terrain
		if (vkAllocateDescriptorSets(Device.device, &procDescriptor.allocInfo, &terrain.descriptorSet) == VK_SUCCESS) {
			std::array<VkWriteDescriptorSet, 4> tempWrite{
				procDescriptor.getWrite(0, terrain.descriptorSet, terrain.uniform.buffer, sizeof(terrain.ubo)),
				procDescriptor.getWrite(1, terrain.descriptorSet, terrain.lightUniform.buffer, sizeof(terrain.lightUbo)),
				procDescriptor.getWrite(2, terrain.descriptorSet, procSampler.node[terrain.samplerId], terrain.texture.view),
				procDescriptor.getWrite(3, terrain.descriptorSet, procSampler.node[terrain.samplerId], terrain.specular.view)
			};

			vkUpdateDescriptorSets(Device.device, uint32_t(tempWrite.size()), tempWrite.data(), 0, nullptr);
		}
		else {
			Log::printError(__FILE__, __LINE__, "terrain分配描述符集失败");
			throw;
		}

		//model-sNode
		for (int i = 0; i < bufferEnum::procStaticModelId::count; i++) {
			if (vkAllocateDescriptorSets(Device.device, &procDescriptor.allocInfo, &procModel.sNode[i].descriptorSet) == VK_SUCCESS) {
				std::array<VkWriteDescriptorSet, 3> tempWrite{
					procDescriptor.getWrite(0, procModel.sNode[i].descriptorSet, procModel.sNode[i].uniform.buffer, sizeof(procModel.sNode[i].ubo)),
					procDescriptor.getWrite(1, procModel.sNode[i].descriptorSet, procModel.sNode[i].lightUniform.buffer, sizeof(procModel.sNode[i].lightUbo)),
					procDescriptor.getWrite(2, procModel.sNode[i].descriptorSet, procSampler.node[procModel.sNode[i].samplerId], procModel.sNode[i].texture.view)
				};

				vkUpdateDescriptorSets(Device.device, uint32_t(tempWrite.size()), tempWrite.data(), 0, nullptr);
			}
			else {
				Log::printError(__FILE__, __LINE__, "model-sNode_" + std::to_string(i) + ":\t分配描述符集失败");
				throw;
			}
		}

		//model-dNode
		for (int i = 0; i < bufferEnum::procDynamicModelId::count; i++) {
			for (int j = 0; j < procModel.dNode[i].node.size(); j++) {
				if (vkAllocateDescriptorSets(Device.device, &procDescriptor.allocInfo, &procModel.dNode[i].node[j].descriptorSet) == VK_SUCCESS) {
					std::array<VkWriteDescriptorSet, 3> tempWrite{
						procDescriptor.getWrite(0, procModel.dNode[i].node[j].descriptorSet, procModel.dNode[i].node[j].uniform.buffer, sizeof(procModel.dNode[i].node[j].ubo)),
						procDescriptor.getWrite(1, procModel.dNode[i].node[j].descriptorSet, procModel.dNode[i].lightUniform.buffer, sizeof(procModel.dNode[i].lightUbo)),
						procDescriptor.getWrite(2, procModel.dNode[i].node[j].descriptorSet, procSampler.node[procModel.dNode[i].samplerId], procModel.dNode[i].node[j].texture.view)
					};

					vkUpdateDescriptorSets(Device.device, uint32_t(tempWrite.size()), tempWrite.data(), 0, nullptr);
				}
				else {
					Log::printError(__FILE__, __LINE__, "model-dNode_" + std::to_string(i) + "_" + std::to_string(j) + ":\t分配描述符集失败");
					throw;
				}
			}
		}

		//skybox
		if (vkAllocateDescriptorSets(Device.device, &procDescriptor.allocInfo, &skybox.descriptorSet) == VK_SUCCESS) {
			std::array<VkWriteDescriptorSet, 2> tempWrite{
				procDescriptor.getWrite(0, skybox.descriptorSet, skybox.uniform.buffer, sizeof(skybox.ubo)),
				procDescriptor.getWrite(1, skybox.descriptorSet, skybox.lightUniform.buffer, sizeof(skybox.lightUbo))
			};

			vkUpdateDescriptorSets(Device.device, uint32_t(tempWrite.size()), tempWrite.data(), 0, nullptr);
		}
		else {
			Log::printError(__FILE__, __LINE__, "skybox分配描述符集失败");
			throw;
		}

		//mine
		if (vkAllocateDescriptorSets(Device.device, &procDescriptor.allocInfo, &mine.descriptorSet) == VK_SUCCESS) {
			std::array<VkWriteDescriptorSet, 1> tempWrite{
				procDescriptor.getWrite(0, mine.descriptorSet, mine.uniform.buffer, sizeof(mine.ubo)),
			};


			vkUpdateDescriptorSets(Device.device, uint32_t(tempWrite.size()), tempWrite.data(), 0, nullptr);
		}
		else {
			Log::printError(__FILE__, __LINE__, "mine分配描述符集失败");
			throw;
		}

		//osMirror
		bufferStruct::osFramebufferNode& osMirrorFrameNode = procRenderPass.getOsFramebufferNode(bufferEnum::procRenderId::osMirror);
		if (vkAllocateDescriptorSets(Device.device, &procDescriptor.allocInfo, &osMirror.descriptorSet) == VK_SUCCESS) {
			std::array<VkWriteDescriptorSet, 3> tempWrite{
				procDescriptor.getWrite(0, osMirror.descriptorSet, osMirror.uniform.buffer, sizeof(osMirror.ubo)),
				procDescriptor.getWrite(1, osMirror.descriptorSet, osMirror.lightUniform.buffer, sizeof(osMirror.lightUbo)),
				procDescriptor.getWrite(2, osMirror.descriptorSet, procSampler.node[osMirror.samplerId], osMirrorFrameNode.osView.view)
			};

			vkUpdateDescriptorSets(Device.device, uint32_t(tempWrite.size()), tempWrite.data(), 0, nullptr);
		}
		else {
			Log::printError(__FILE__, __LINE__, "osMirror分配描述符集失败");
			throw;
		}

		//osScence
		bufferStruct::osFramebufferNode& osScenceFrameNode = procRenderPass.getOsFramebufferNode(bufferEnum::procRenderId::osScence);
		if (vkAllocateDescriptorSets(Device.device, &procDescriptor.allocInfo, &osScence.descriptorSet) == VK_SUCCESS) {
			std::array<VkWriteDescriptorSet, 1> tempWrite{
				procDescriptor.getWrite(2, osScence.descriptorSet, procSampler.node[osScence.samplerId], osScenceFrameNode.osView.view)
			};

			vkUpdateDescriptorSets(Device.device, uint32_t(tempWrite.size()), tempWrite.data(), 0, nullptr);
		}
		else {
			Log::printError(__FILE__, __LINE__, "osScence分配描述符集失败");
			throw;
		}



		/*配置scenceDeferred描述符*/
		//procDescriptor.setAllocInfo(bufferEnum::procDescriptorId::scenceDeferred);



		/*配置dsScence描述符*/
		procDescriptor.setAllocInfo(bufferEnum::procDescriptorId::dsScence);

		//dsScence
		bufferStruct::dsFramebufferNode& dsScenceFrameNode = procRenderPass.getDsFramebufferNode(bufferEnum::procRenderId::dsScence);
		if (vkAllocateDescriptorSets(Device.device, &procDescriptor.allocInfo, &dsScence.descriptorSet) == VK_SUCCESS) {
			std::array<VkWriteDescriptorSet, 4> tempWrite{
				procDescriptor.getWrite(0, dsScence.descriptorSet, dsScence.lightUniform.buffer, sizeof(dsScence.lightUbo)),
				procDescriptor.getWrite(1, dsScence.descriptorSet, procSampler.node[dsScence.samplerId], dsScenceFrameNode.position.view),
				procDescriptor.getWrite(2, dsScence.descriptorSet, procSampler.node[dsScence.samplerId], dsScenceFrameNode.normal.view),
				procDescriptor.getWrite(3, dsScence.descriptorSet, procSampler.node[dsScence.samplerId], dsScenceFrameNode.albedo.view)
			};

			vkUpdateDescriptorSets(Device.device, uint32_t(tempWrite.size()), tempWrite.data(), 0, nullptr);
		}
		else {
			Log::printError(__FILE__, __LINE__, "dsScence分配描述符集失败");
			throw;
		}



		/*配置uiText描述符*/
		procDescriptor.setAllocInfo(bufferEnum::procDescriptorId::uiText);

		//uiText
		if (vkAllocateDescriptorSets(Device.device, &procDescriptor.allocInfo, &uiText.descriptorSet) == VK_SUCCESS) {
			std::array<VkWriteDescriptorSet, 1> tempWrite{
				procDescriptor.getWrite(0, uiText.descriptorSet, procSampler.node[uiText.samplerId], uiText.texture.view)
			};

			vkUpdateDescriptorSets(Device.device, uint32_t(tempWrite.size()), tempWrite.data(), 0, nullptr);
		}
		else {
			Log::printError(__FILE__, __LINE__, "uiText分配描述符集失败");
			throw;
		}
	}

	void initUniformBuffer() {
		//设置透视投影，参数依次是视野角度、宽高比、近裁剪面和远裁剪面，以及proj[1][1] *= -1轴向修正
		glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)InitSwapchain.scExtent.width / (float)InitSwapchain.scExtent.height, 0.1f, plateInfo::cubeSightRange);
		proj[1][1] *= -1;

		//terrain
		terrain.ubo.model				= glm::translate(glm::mat4(1), glm::vec3(0));
		terrain.ubo.modelTI				= glm::transpose(glm::inverse(terrain.ubo.model));
		terrain.ubo.proj				= proj;
		terrain.lightUbo.gLightColor	= lightInfo::gLightColor;
		terrain.lightUbo.diffuseScale	= lightInfo::diffuseScale;
		terrain.lightUbo.gLightDir		= lightInfo::gLightDir;
		terrain.lightUbo.specularScale	= lightInfo::specularScale;

		//model-sNode
		for (int i = 0; i < bufferEnum::procStaticModelId::count; i++) {
			procModel.sNode[i].ubo.proj				  = proj;
			procModel.sNode[i].lightUbo.gLightColor	  = lightInfo::gLightColor;
			procModel.sNode[i].lightUbo.diffuseScale  = lightInfo::diffuseScale;
			procModel.sNode[i].lightUbo.gLightDir	  = lightInfo::gLightDir;
			procModel.sNode[i].lightUbo.specularScale = lightInfo::specularScale;
		}

		//model-dNode
		for (int i = 0; i < bufferEnum::procDynamicModelId::count; i++) {
			for (int j = 0; j < procModel.dNode[i].node.size(); j++) {
				procModel.dNode[i].node[j].ubo.proj = proj;
			}

			procModel.dNode[i].lightUbo.gLightColor   = lightInfo::gLightColor;
			procModel.dNode[i].lightUbo.diffuseScale  = lightInfo::diffuseScale;
			procModel.dNode[i].lightUbo.gLightDir	  = lightInfo::gLightDir;
			procModel.dNode[i].lightUbo.specularScale = lightInfo::specularScale;
		}

		//skybox
		skybox.ubo.proj = proj;
			
		//mine
		mine.ubo.proj = proj;

		//osMirror
		osMirror.ubo.proj = proj;
		osMirror.lightUbo.gLightColor	= lightInfo::gLightColor;
		osMirror.lightUbo.diffuseScale  = lightInfo::diffuseScale;
		osMirror.lightUbo.gLightDir		= lightInfo::gLightDir;
		osMirror.lightUbo.specularScale = lightInfo::specularScale;

		//dsScence
		dsScence.lightUbo.gLightColor	= lightInfo::gLightColor;
		dsScence.lightUbo.diffuseScale	= lightInfo::diffuseScale;
		dsScence.lightUbo.gLightDir		= lightInfo::gLightDir;
		dsScence.lightUbo.specularScale = lightInfo::specularScale;
	}



	//将vertexNode类中的顶点映射到buffer类里对应的VkBuffer
	void createVertexBuffer(auto& node, bufferStruct::bufferNode& vertexBuffer) {
		//获取顶点数组大小
		VkDeviceSize bufferSize = sizeof(node[0]) * node.size();

		//创建临时缓冲，方便CPU访问
		bufferStruct::bufferNode tempBuffer;
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, tempBuffer);

		//内存映射
		createMapMemory(tempBuffer.bufferMemory, node.data(), (size_t)bufferSize);

		//创建顶点缓冲，方便GPU访问
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer);

		//临时缓冲拷贝到顶点缓冲
		copyBuffer(tempBuffer.buffer, vertexBuffer.buffer, bufferSize);

		vmaDestroyBuffer(Device.allocator, tempBuffer.buffer, tempBuffer.bufferMemory);
	}

	//将vertexNode类中的索引映射到buffer类里对应的VkBuffer
	void createIndexBuffer(std::vector<uint32_t>& node, bufferStruct::bufferNode& indexBuffer) {
		//获取索引数组大小
		VkDeviceSize bufferSize = sizeof(node[0]) * node.size();

		//创建临时缓冲，方便CPU访问
		bufferStruct::bufferNode tempBuffer;
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, tempBuffer);

		//内存映射
		createMapMemory(tempBuffer.bufferMemory, node.data(), (size_t)bufferSize);

		//创建索引缓冲，方便GPU访问
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer);

		//临时缓冲拷贝到索引缓冲
		copyBuffer(tempBuffer.buffer, indexBuffer.buffer, bufferSize);

		vmaDestroyBuffer(Device.allocator, tempBuffer.buffer, tempBuffer.bufferMemory);
	}

	//加载静态模型
	void loadStaticModel(const std::string& filename, std::vector<modelVertex::vertexNode>& vertexs, std::vector<uint32_t>& indexs) {
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
			loadGltfModel(model, mesh, vertexs, indexs, uniqueVertices);
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
	void loadDynamicModel(const std::string& filename, std::vector<std::vector<modelVertex::vertexNode>>& vertexsList, std::vector<std::vector<uint32_t>>& indexsList) {
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
			loadGltfModel(model, model.meshes[i], vertexsList[i], indexsList[i], uniqueVertices);

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
	
	void loadGltfModel(const tinygltf::Model& model, const tinygltf::Mesh& mesh, std::vector<modelVertex::vertexNode>& vertexs, std::vector<uint32_t>& indexs, std::unordered_map<modelVertex::vertexNode, uint32_t>& uniqueVertices) {
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

				//坐标系转换：从glTF(Y向上)转换为Forge(Z向上)  
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

	//创建纹理KTX数组结点，使用无损压缩
	void createTextureKtxArrayNode(const char* imageFile, bufferStruct::imageNode& imageBuffer, uint32_t arrayLayer, uint32_t& mipLevel, VkFormat format, uint32_t pixel) {
		ktxResult result;
		ktxTexture2* ktxTexture;

		result = ktxTexture2_CreateFromNamedFile(imageFile, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture);

		if (result != KTX_SUCCESS) {
			Log::printError(__FILE__, __LINE__, "KTX贴图加载失败");
			throw;
		}

		uint32_t texWidth = ktxTexture->baseWidth;
		uint32_t texHeight = ktxTexture->baseHeight;
		mipLevel = ktxTexture->numLevels;
		ktx_uint8_t* textureData = ktxTexture->pData;
		ktx_size_t textureSize = ktxTexture->dataSize;
		bufferStruct::bufferNode tempBuffer;



		createBuffer(textureSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, tempBuffer);
		createMapMemory(tempBuffer.bufferMemory, textureData, static_cast<size_t>(textureSize));
		createImage(textureInfo::tileWidth, textureInfo::tileHeight, arrayLayer, mipLevel, format, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, imageBuffer.image, imageBuffer.imageMemory);

		transitionImageLayout(imageBuffer.image, arrayLayer, mipLevel, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		copyBufferToImageArray(pixel, textureInfo::tileWidth, textureInfo::tileHeight, textureInfo::atlasWidth, textureInfo::atlasHeight, texWidth, texHeight, arrayLayer, tempBuffer.buffer, imageBuffer.image);
		transitionImageLayout(imageBuffer.image, arrayLayer, mipLevel, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		createImageView(imageBuffer.image, imageBuffer.view, arrayLayer, mipLevel, VK_IMAGE_VIEW_TYPE_2D_ARRAY, format, VK_IMAGE_ASPECT_COLOR_BIT);

		vmaDestroyBuffer(Device.allocator, tempBuffer.buffer, tempBuffer.bufferMemory);
		ktxTexture2_Destroy(ktxTexture);
	}

	//创建纹理KTX结点，使用微损压缩 + mip
	void createTextureKtxNode(const char* imageFile, bufferStruct::imageNode& imageBuffer, uint32_t& mipLevel) {
		ktxResult result;
		ktxTexture2* ktxTexture;

		if (!std::filesystem::exists(imageFile)) {
			Log::printError(__FILE__, __LINE__, "贴图文件不存在: " + std::string(imageFile));
		}

		result = ktxTexture2_CreateFromNamedFile(imageFile, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture);
		result = ktxTexture2_TranscodeBasis(ktxTexture, KTX_TTF_BC7_RGBA, 0);

		if (result != KTX_SUCCESS) {
			Log::printError(__FILE__, __LINE__, "KTX贴图加载失败");
			throw;
		}

		uint32_t texWidth = ktxTexture->baseWidth;
		uint32_t texHeight = ktxTexture->baseHeight;
		mipLevel = ktxTexture->numLevels;
		ktx_uint8_t* textureData = ktxTexture->pData;
		ktx_size_t textureSize = ktxTexture->dataSize;
		VkFormat format = VK_FORMAT_BC7_UNORM_BLOCK;
		bufferStruct::bufferNode tempBuffer;



		//创建临时缓冲，方便CPU访问
		createBuffer(textureSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, tempBuffer);
		//内存映射
		createMapMemory(tempBuffer.bufferMemory, textureData, static_cast<size_t>(textureSize));
		//创建纹理缓冲，方便GPU访问
		createImage(texWidth, texHeight, 1, mipLevel, format, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, imageBuffer.image, imageBuffer.imageMemory);

		transitionImageLayout(imageBuffer.image, 1, mipLevel, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);					//图像格式转换为传送最佳
		copyBufferToImageKtx(texWidth, texHeight, mipLevel, ktxTexture, tempBuffer.buffer, imageBuffer.image);												//临时缓冲拷贝到图像缓冲
		transitionImageLayout(imageBuffer.image, 1, mipLevel, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);	//图像格式转换为采样最佳

		createImageView(imageBuffer.image, imageBuffer.view, 1, mipLevel, VK_IMAGE_VIEW_TYPE_2D, format, VK_IMAGE_ASPECT_COLOR_BIT);

		vmaDestroyBuffer(Device.allocator, tempBuffer.buffer, tempBuffer.bufferMemory);
		ktxTexture2_Destroy(ktxTexture);
	}

	//创建纹理文本结点
	void createTextNode(bufferStruct::imageNode& imageBuffer) {
		const uint32_t fontWidth = STB_FONT_consolas_24_latin1_BITMAP_WIDTH;
		const uint32_t fontHeight = STB_FONT_consolas_24_latin1_BITMAP_HEIGHT;
		VkDeviceSize imageSize = static_cast<VkDeviceSize>(fontWidth) * static_cast<VkDeviceSize>(fontHeight);

		static unsigned char font24pixels[fontHeight][fontWidth];
		stb_font_consolas_24_latin1(uiText.stbFontData, font24pixels, fontHeight);

		//创建临时缓冲，方便CPU访问
		bufferStruct::bufferNode tempBuffer;
		createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, tempBuffer);

		//内存映射
		createMapMemory(tempBuffer.bufferMemory, &font24pixels[0][0], static_cast<size_t>(imageSize));

		//创建纹理缓冲，方便GPU访问
		createImage(fontWidth, fontHeight, 1, 1, VK_FORMAT_R8_UNORM, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, imageBuffer.image, imageBuffer.imageMemory);

		//图像格式转换为传送最佳
		transitionImageLayout(imageBuffer.image, 1, 1, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		//临时缓冲拷贝到图像缓冲
		copyBufferToImage(static_cast<uint32_t>(fontWidth), static_cast<uint32_t>(fontHeight), tempBuffer.buffer, imageBuffer.image);
		//图像格式转换为采样最佳
		transitionImageLayout(imageBuffer.image, 1, 1, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		vmaDestroyBuffer(Device.allocator, tempBuffer.buffer, tempBuffer.bufferMemory);

		createImageView(imageBuffer.image, imageBuffer.view, 1, 1, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
	}



	void createImageView(const VkImage& image, VkImageView& view, uint32_t arrayLayer, uint32_t mipLevel, VkImageViewType viewType, VkFormat format, VkImageAspectFlags aspectFlags) {
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType								= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image								= image;
		viewInfo.viewType							= viewType;
		viewInfo.format								= format;
		viewInfo.subresourceRange.aspectMask		= aspectFlags;
		viewInfo.subresourceRange.baseMipLevel		= 0;
		viewInfo.subresourceRange.levelCount		= mipLevel;
		viewInfo.subresourceRange.baseArrayLayer	= 0;
		viewInfo.subresourceRange.layerCount		= arrayLayer;

		if (vkCreateImageView(Device.device, &viewInfo, nullptr, &view) != VK_SUCCESS) {
			Log::printError(__FILE__, __LINE__, "图像视图创建失败");
			throw;
		}
	}

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, bufferStruct::bufferNode& bufferNode) {
		VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferInfo.size = size;
		bufferInfo.usage = usage;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_UNKNOWN; //使用 VMA_MEMORY_USAGE_UNKNOWN 将内存属性交由属性参数控制
		allocInfo.requiredFlags = properties;		//设置所需的内存属性

		vmaCreateBuffer(Device.allocator, &bufferInfo, &allocInfo, &bufferNode.buffer, &bufferNode.bufferMemory, nullptr);
	}

	//为createUniformBuffer服务的简略版
	void createBuffer(VkDeviceSize size, bufferStruct::bufferNode& bufferNode) {
		VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferInfo.size = size;
		bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_UNKNOWN;																//使用 VMA_MEMORY_USAGE_UNKNOWN 将内存属性交由属性参数控制
		allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;	//设置所需的内存属性

		vmaCreateBuffer(Device.allocator, &bufferInfo, &allocInfo, &bufferNode.buffer, &bufferNode.bufferMemory, nullptr);
	}

	void createMapMemory(VmaAllocation tempMemory, const void* sourceData, size_t dataSize) {
		//内存映射
		void* data;
		vmaMapMemory(Device.allocator, tempMemory, &data);
		memcpy(data, sourceData, dataSize);
		vmaUnmapMemory(Device.allocator, tempMemory);
	}

	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = command.commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(Device.device, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		VkBufferCopy copyRegion = {};
		copyRegion.srcOffset = 0; // Optional
		copyRegion.dstOffset = 0; // Optional
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(Device.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(Device.graphicsQueue);

		vkFreeCommandBuffers(Device.device, command.commandPool, 1, &commandBuffer);
	}

	void createImage(uint32_t width, uint32_t height, uint32_t arrayLayer, uint32_t mipLevel, VkFormat format, VkSampleCountFlagBits numSamples, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VmaAllocation& allocation) {
		VkImageCreateInfo imageInfo = {};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.format = format;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = mipLevel;
		imageInfo.arrayLayers = arrayLayer;
		imageInfo.samples = numSamples;
		imageInfo.tiling = tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		allocInfo.requiredFlags = properties;

		if (vmaCreateImage(Device.allocator, &imageInfo, &allocInfo, &image, &allocation, nullptr) != VK_SUCCESS) {
			Log::printError(__FILE__, __LINE__, "图像创建失败");
			throw;
		}
	}

	void transitionImageLayout(VkImage image, uint32_t arrayLayer, uint32_t mipLevel, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
		// 在图像布局转换之前，我们需要开始一个单次命令缓冲区以确保我们的操作能够被正确提交和执行
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		// 创建一个图像内存屏障对象，用于定义布局转换的参数
		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;							// 旧的图像布局
		barrier.newLayout = newLayout;							// 新的图像布局
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;	// 来源队列家族索引
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;	// 目标队列家族索引
		barrier.image = image;									// 需要进行布局转换的图像
		barrier.subresourceRange.baseMipLevel = 0;				// 子资源的起始mipmap级别
		barrier.subresourceRange.levelCount = mipLevel;			// 子资源的级别数量
		barrier.subresourceRange.baseArrayLayer = 0;			// 子资源的起始数组层
		barrier.subresourceRange.layerCount = arrayLayer;		// 子资源的数组层数量

		// 根据新的图像布局来确定子资源的方面掩码
		if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;		// 深度方面掩码

			// 如果图像格式含有模板部分，则添加模板方面掩码
			if (hasStencilComponent(format)) {
				barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT; // 模板方面掩码
			}
		}
		else {
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;		// 颜色方面掩码
		}

		// 定义源和目标管线阶段，以及相应的访问掩码
		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = 0;										// 源访问掩码
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;			// 目标访问掩码

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;				// 源管线阶段
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;				// 目标管线阶段
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;			// 源访问掩码
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;				// 目标访问掩码

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;					// 源管线阶段
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;		// 目标管线阶段
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
			barrier.srcAccessMask = 0;										// 源访问掩码
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT; // 目标访问掩码

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;				// 源管线阶段
			destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;	// 目标管线阶段
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		}
		else {
			// 如果出现无法识别的布局转换，输出错误信息并抛出异常
			Log::printError(__FILE__, __LINE__, "无支持的布局转换");
			throw;
		}

		// 在命令缓冲区中插入图像内存屏障，以实现布局转换
		vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		// 结束单次命令缓冲区，提交并执行命令
		endSingleTimeCommands(commandBuffer);
	}

	void copyBufferToImageArray(uint32_t pixel, uint32_t tileWidth, uint32_t tileHeight, uint32_t atlasWidth, uint32_t atlasHeight, uint32_t texWidth, uint32_t texHeight, uint32_t arrayLayer, VkBuffer buffer, VkImage image) {
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();
		std::vector<VkBufferImageCopy> regions{ arrayLayer };			//vulkan存储方式是从左往右从上往下一个个存入像素点数据，所以按下述方法遍历
		uint32_t wOffset = tileWidth * pixel;
		uint32_t hOffset = atlasWidth * tileWidth * tileHeight * pixel;

		for (uint32_t w = 0; w < atlasWidth; w++) {
			for (uint32_t h = 0; h < atlasHeight; h++) {
				uint32_t id = w + h * atlasWidth;
				uint32_t offset = w * wOffset + h * hOffset;

				regions[id].bufferOffset = offset;
				regions[id].bufferRowLength = texWidth;
				regions[id].bufferImageHeight = texHeight;
				regions[id].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				regions[id].imageSubresource.mipLevel = 0;
				regions[id].imageSubresource.baseArrayLayer = id;
				regions[id].imageSubresource.layerCount = 1;
				regions[id].imageOffset = { 0, 0, 0 };
				regions[id].imageExtent = { tileWidth, tileHeight, 1 };
			}
		}

		vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, arrayLayer, regions.data());
		endSingleTimeCommands(commandBuffer);
	}

	void copyBufferToImageKtx(uint32_t width, uint32_t height, uint32_t mipLevels, ktxTexture2* ktxTexture, VkBuffer buffer, VkImage image) {
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();
		std::vector<VkBufferImageCopy> regions{ mipLevels };

		for (uint32_t i = 0; i < mipLevels; i++) {
			ktx_size_t offset;
			ktxTexture2_GetImageOffset(ktxTexture, i, 0, 0, &offset);

			regions[i].bufferOffset = offset;
			regions[i].bufferRowLength = 0;
			regions[i].bufferImageHeight = 0;
			regions[i].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			regions[i].imageSubresource.mipLevel = i;
			regions[i].imageSubresource.baseArrayLayer = 0;
			regions[i].imageSubresource.layerCount = 1;
			regions[i].imageOffset = { 0, 0, 0 };
			regions[i].imageExtent = { width >> i, height >> i, 1 };
		}

		vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(regions.size()), regions.data());
		endSingleTimeCommands(commandBuffer);
	}

	void copyBufferToImage(uint32_t width, uint32_t height, VkBuffer buffer, VkImage image) {
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();
		VkBufferImageCopy region = {};

		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { width, height, 1 };

		vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
		endSingleTimeCommands(commandBuffer);
	}



	VkCommandBuffer beginSingleTimeCommands() {
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = command.commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(Device.device, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		return commandBuffer;
	}

	void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(Device.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(Device.graphicsQueue);

		vkFreeCommandBuffers(Device.device, command.commandPool, 1, &commandBuffer);
	}

	bool hasStencilComponent(VkFormat format) {
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}

public:
	void drawUIText(int fps) {
		static std::ostringstream oss;
		float width = 5.0;
		float height = 5.0;

		uiText.beginTextUpdate(Device.allocator);

		//世界坐标
		oss << std::fixed << std::setprecision(0);	//设置输出流的格式,不保留小数
		oss << "worldpos:  " << "x:" << std::setw(8) << std::left << showUIText.pos.x << "y:" << std::setw(8) << std::left << showUIText.pos.y << " z:" << std::setw(8) << std::left << showUIText.pos.z;
		uiText.addText(oss.str(), width, height, bufferEnum::textAlignId::left);
		oss.str("");

		//区块坐标
		oss << "chunkpos:  " << "x:" << std::setw(8) << std::left << showUIText.chunkPos.x << "y:" << std::setw(8) << std::left << showUIText.chunkPos.y << " z:" << std::setw(8) << std::left << showUIText.chunkPos.z;
		uiText.addText(oss.str(), width, height += 15, bufferEnum::textAlignId::left);
		oss.str("");

		//选择框坐标
		oss << "mineboxpos:" << "x:" << std::setw(8) << std::left << showUIText.minepos.x << "y:" << std::setw(8) << std::left << showUIText.minepos.y << " z:" << std::setw(8) << std::left << showUIText.minepos.z;
		uiText.addText(oss.str(), width, height += 15, bufferEnum::textAlignId::left);
		oss.str("");

		//帧率
		oss << "fps:       " << std::setw(8) << std::left << fps;
		uiText.addText(oss.str(), width, height += 15, bufferEnum::textAlignId::left);
		oss.str("");

		uiText.endTextUpdate(Device.allocator);
	}

	void createCommandBuffer(int threadId, pipelineList::procPipelineNode& procPipeline) {
		if (vkAllocateCommandBuffers(Device.device, &command.allocInfo, command.commandBuffer[threadId].data()) != VK_SUCCESS) {
			Log::printError(__FILE__, __LINE__, "基础命令缓冲创建失败");
			throw;
		}

		for (int frameId = 0; frameId < command.commandBuffer[threadId].size(); frameId++) {
			//开始渲染
			command.beginCommandBuffer(threadId, frameId);


			/*dsScence*/
			//vkCmdBeginRenderPass(command.commandBuffer[threadId][frameId], &procRenderPass.node[bufferEnum::procRenderId::dsScence].renderInfo, VK_SUBPASS_CONTENTS_INLINE);
			//
			//vkCmdEndRenderPass(command.commandBuffer[threadId][frameId]);


			/*osMirror*/
			//vkCmdBeginRenderPass(command.commandBuffer[threadId][frameId], &procRenderPass.node[bufferEnum::procRenderId::osMirror].renderInfo, VK_SUBPASS_CONTENTS_INLINE);
			//
			////model-sNode
			//for (int nodeId = 0; nodeId < bufferEnum::procStaticModelId::count; nodeId++) {
			//	command.createScenceCommandBuffer(threadId, frameId, procPipeline.getPipeline(procModel.pipeId), procPipeline.getPipeLayout(procModel.pipeId), procModel.sNode[nodeId].descriptorSet, procModel.sNode[nodeId].vertex.buffer, procModel.sNode[nodeId].index.buffer, (uint32_t)modelVertex::sIndexs[nodeId].size());
			//}
			//
			////model-dNode
			//for (int nodeId = 0; nodeId < bufferEnum::procDynamicModelId::count; nodeId++) {
			//	for (int subNode = 0; subNode < procModel.dNode[nodeId].node.size(); subNode++) {
			//		command.createScenceCommandBuffer(threadId, frameId, procPipeline.getPipeline(procModel.pipeId), procPipeline.getPipeLayout(procModel.pipeId), procModel.dNode[nodeId].node[subNode].descriptorSet, procModel.dNode[nodeId].node[subNode].vertex.buffer, procModel.dNode[nodeId].node[subNode].index.buffer, (uint32_t)modelVertex::dIndexs[nodeId][subNode].size());
			//	}
			//}
			//
			//vkCmdEndRenderPass(command.commandBuffer[threadId][frameId]);


			/*osScence*/
			vkCmdBeginRenderPass(command.commandBuffer[threadId][frameId], &procRenderPass.node[bufferEnum::procRenderId::osScence].renderInfo, VK_SUBPASS_CONTENTS_INLINE);

			//terrain
			for (int layer = 0; layer < terrainEnum::terrainLayer::count; layer++) {
				if (terrainVertex::vertexs[layer].size() && terrainVertex::indexs[layer].size()) {
					command.createScenceCommandBuffer(threadId, frameId, procPipeline.getPipeline(terrain.pipeId), procPipeline.getPipeLayout(terrain.pipeId), terrain.descriptorSet, terrain.vertex[threadId][layer].buffer, terrain.index[threadId][layer].buffer, (uint32_t)terrainVertex::indexs[layer].size());
				}
			}

			//model-sNode
			for (int nodeId = 0; nodeId < bufferEnum::procStaticModelId::count; nodeId++) {
				command.createScenceCommandBuffer(threadId, frameId, procPipeline.getPipeline(procModel.pipeId), procPipeline.getPipeLayout(procModel.pipeId), procModel.sNode[nodeId].descriptorSet, procModel.sNode[nodeId].vertex.buffer, procModel.sNode[nodeId].index.buffer, (uint32_t)modelVertex::sIndexs[nodeId].size());
			}

			//model-dNode
			for (int nodeId = 0; nodeId < bufferEnum::procDynamicModelId::count; nodeId++) {
				for (int subNode = 0; subNode < procModel.dNode[nodeId].node.size(); subNode++) {
					command.createScenceCommandBuffer(threadId, frameId, procPipeline.getPipeline(procModel.pipeId), procPipeline.getPipeLayout(procModel.pipeId), procModel.dNode[nodeId].node[subNode].descriptorSet, procModel.dNode[nodeId].node[subNode].vertex.buffer, procModel.dNode[nodeId].node[subNode].index.buffer, (uint32_t)modelVertex::dIndexs[nodeId][subNode].size());
				}
			}

			//skybox
			command.createScenceCommandBuffer(threadId, frameId, procPipeline.getPipeline(skybox.pipeId), procPipeline.getPipeLayout(skybox.pipeId), skybox.descriptorSet, skybox.vertex.buffer, skybox.index.buffer, (uint32_t)skyboxVertex::indexs.size());
			
			//mine
			command.createScenceCommandBuffer(threadId, frameId, procPipeline.getPipeline(mine.pipeId), procPipeline.getPipeLayout(mine.pipeId), mine.descriptorSet, mine.vertex.buffer, mine.index.buffer, (uint32_t)mineVertex::indexs.size());
			
			//osMirror
			//command.createScenceCommandBuffer(threadId, frameId, procPipeline.getPipeline(osMirror.pipeId), procPipeline.getPipeLayout(osMirror.pipeId), osMirror.descriptorSet, osMirror.vertex.buffer, osMirror.index.buffer, (uint32_t)osMirrorVertex::indexs.size());

			//dsScence
			//command.createScenceCommandBuffer(threadId, frameId, procPipeline.getPipeline(dsScence.pipeId), procPipeline.getPipeLayout(dsScence.pipeId), dsScence.descriptorSet, InitSwapchain.scExtent);

			//uiText
			command.createUITextCommandBuffer(threadId, frameId, procPipeline.getPipeline(uiText.pipeId), procPipeline.getPipeLayout(uiText.pipeId), uiText.descriptorSet, uiText.vertex.buffer, uiText.numLetters);

			vkCmdEndRenderPass(command.commandBuffer[threadId][frameId]);


			/*scScence*/
			procRenderPass.node[bufferEnum::procRenderId::scScence].setScFramebuffer(frameId);
			vkCmdBeginRenderPass(command.commandBuffer[threadId][frameId], &procRenderPass.node[bufferEnum::procRenderId::scScence].renderInfo, VK_SUBPASS_CONTENTS_INLINE);

			//osScence
			command.createScenceCommandBuffer(threadId, frameId, procPipeline.getPipeline(osScence.pipeId), procPipeline.getPipeLayout(osScence.pipeId), osScence.descriptorSet, osScence.vertex.buffer, osScence.index.buffer, (uint32_t)osScenceVertex::indexs.size());

			vkCmdEndRenderPass(command.commandBuffer[threadId][frameId]);

			
			//结束渲染
			command.endCommandBuffer(threadId, frameId);
		}
	}

	bool drawFrame() {
		return InitSwapchain.drawFrame(command.commandBuffer[command.currentThreadId]);
	}



	void recreateBuffer(int threadId, bool isLayer) {
		command.free(Device.device, threadId);

		if (isLayer) {
			int layer = terrainEnum::terrainLayer::Layer_0;		//先重建当前线程网格缓冲区
			if (terrainVertex::vertexs[layer].size() && terrainVertex::indexs[layer].size()) {
				vmaDestroyBuffer(Device.allocator, terrain.vertex[threadId][layer].buffer, terrain.vertex[threadId][layer].bufferMemory);
				vmaDestroyBuffer(Device.allocator, terrain.index[threadId][layer].buffer, terrain.index[threadId][layer].bufferMemory);

				createVertexBuffer(terrainVertex::vertexs[layer], terrain.vertex[threadId][layer]);
				createIndexBuffer(terrainVertex::indexs[layer], terrain.index[threadId][layer]);
			}

			layer = terrainEnum::terrainLayer::Layer_1;
			if (terrainVertex::vertexs[layer].size() && terrainVertex::indexs[layer].size()) {
				createTerrainVertexBuffer(threadId, layer);
				createTerrainIndexBuffer(threadId, layer);
			}
		}
		else {
			int layer = terrainEnum::terrainLayer::Layer_0;

			if (terrainVertex::vertexs[layer].size() && terrainVertex::indexs[layer].size()) {
				vmaDestroyBuffer(Device.allocator, terrain.vertex[threadId][layer].buffer, terrain.vertex[threadId][layer].bufferMemory);
				vmaDestroyBuffer(Device.allocator, terrain.index[threadId][layer].buffer, terrain.index[threadId][layer].bufferMemory);

				createVertexBuffer(terrainVertex::vertexs[layer], terrain.vertex[threadId][layer]);
				createIndexBuffer(terrainVertex::indexs[layer], terrain.index[threadId][layer]);
			}
		}
	}

	void updateUniformBuffer(const playerStruct::cameraNode& camera, const playerStruct::selectBoxNode& box) {
		static std::chrono::steady_clock::time_point time0 = std::chrono::steady_clock::now();	//游戏起始时刻
		static std::chrono::steady_clock::time_point time1;										//游戏当前时刻
		static float gameTime;																	//游戏运行时长，最多支持连续运行194天（现实时间）
		static float currentHour;																//当前小时，取值范围[0,24)，含小数部分

		glm::vec3 offsetPos = playerInfo::initSpawnPos - playerInfo::logicOriginPos;
		time1 = std::chrono::steady_clock::now();
		gameTime = std::chrono::duration_cast<std::chrono::milliseconds>(time1 - time0).count() / 1000.0f;
		currentHour = std::fmod(gameTime, 24.0f);



		//terrain
		{
			terrain.ubo.view			  = camera.view;
			terrain.lightUbo.viewPos	  = glm::vec3{ camera.pos - playerInfo::logicOriginPos };
			terrain.lightUbo.currentHour  = currentHour;
			terrain.lightUbo.ambientScale = lightInfo::ambientScale;
			terrain.lightUbo.gLightPos	  = offsetPos;
			terrain.lightUbo.flashPos	  = glm::vec3{ camera.pos - playerInfo::logicOriginPos };
			terrain.lightUbo.flashDir	  = camera.front;
			terrain.lightUbo.flashCutOff  = glm::cos(glm::radians(12.5f));
			terrain.lightUbo.outerCutOff  = glm::cos(glm::radians(14.5f));

			createMapMemory(terrain.uniform.bufferMemory, &terrain.ubo, sizeof(terrain.ubo));
			createMapMemory(terrain.lightUniform.bufferMemory, &terrain.lightUbo, sizeof(terrain.lightUbo));
		}

		//model-sNode
		{
			for (int i = 0; i < bufferEnum::procStaticModelId::count; i++) {
				procModel.sNode[i].ubo.model			 = glm::translate(glm::mat4(1), offsetPos);											//位移
				procModel.sNode[i].ubo.model			 *= glm::scale(glm::mat4(1), glm::vec3(0.1f));										//缩放
				procModel.sNode[i].ubo.modelTI			 = glm::transpose(glm::inverse(procModel.sNode[i].ubo.model));
				//procModel.sNode[i].ubo.model			 *= glm::rotate(glm::mat4(1), gameTime * glm::radians(60.f), coordsInfo::worldUp);	//旋转
				procModel.sNode[i].ubo.view				 = camera.view;
				procModel.sNode[i].lightUbo.viewPos		 = glm::vec3{ camera.pos - playerInfo::logicOriginPos };
				procModel.sNode[i].lightUbo.currentHour  = currentHour;
				procModel.sNode[i].lightUbo.ambientScale = lightInfo::ambientScale;//(sin(gameTime) + 1) / 2;
				procModel.sNode[i].lightUbo.gLightPos	 = offsetPos;// glm::vec3{ glm::vec3{std::fmod(gameTime * 16, 128.0f), 32.f, 0.f} - glm::vec3{ playerInfo::logicOriginPos } };
				procModel.sNode[i].lightUbo.flashPos	 = glm::vec3{ camera.pos - playerInfo::logicOriginPos };
				procModel.sNode[i].lightUbo.flashDir	 = camera.front;
				procModel.sNode[i].lightUbo.flashCutOff	 = glm::cos(glm::radians(12.5f));
				procModel.sNode[i].lightUbo.outerCutOff	 = glm::cos(glm::radians(14.5f));
			}

			for (int i = 0; i < bufferEnum::procStaticModelId::count; i++) {
				createMapMemory(procModel.sNode[i].uniform.bufferMemory, &procModel.sNode[i].ubo, sizeof(procModel.sNode[i].ubo));
				createMapMemory(procModel.sNode[i].lightUniform.bufferMemory, &procModel.sNode[i].lightUbo, sizeof(procModel.sNode[i].lightUbo));
			}
		}

		//model-dNode
		{
			for (int i = 0; i < bufferEnum::procDynamicModelId::count; i++) {
				for (int j = 0; j < procModel.dNode[i].node.size(); j++) {
					procModel.dNode[i].node[j].ubo.model   = glm::translate(glm::mat4(1), offsetPos);											//位移
					procModel.dNode[i].node[j].ubo.model   *= glm::scale(glm::mat4(1), glm::vec3(0.1f));										//缩放
					procModel.dNode[i].node[j].ubo.modelTI = glm::transpose(glm::inverse(procModel.dNode[i].node[j].ubo.model));
					procModel.dNode[i].node[j].ubo.view    = camera.view;
				}

				procModel.dNode[i].lightUbo.viewPos		 = glm::vec3{ camera.pos - playerInfo::logicOriginPos };
				procModel.dNode[i].lightUbo.currentHour  = currentHour;
				procModel.dNode[i].lightUbo.ambientScale = lightInfo::ambientScale;//(sin(gameTime) + 1) / 2;
				procModel.dNode[i].lightUbo.gLightPos	 = offsetPos;// glm::vec3{ glm::vec3{std::fmod(gameTime * 16, 128.0f), 32.f, 0.f} - glm::vec3{ playerInfo::logicOriginPos } };
				procModel.dNode[i].lightUbo.flashPos	 = glm::vec3{ camera.pos - playerInfo::logicOriginPos };
				procModel.dNode[i].lightUbo.flashDir	 = camera.front;
				procModel.dNode[i].lightUbo.flashCutOff  = glm::cos(glm::radians(12.5f));
				procModel.dNode[i].lightUbo.outerCutOff  = glm::cos(glm::radians(14.5f));
			}

			for (int i = 0; i < bufferEnum::procDynamicModelId::count; i++) {
				for (int j = 0; j < procModel.dNode[i].node.size(); j++) {
					createMapMemory(procModel.dNode[i].node[j].uniform.bufferMemory, &procModel.dNode[i].node[j].ubo, sizeof(procModel.dNode[i].node[j].ubo));
				}

				createMapMemory(procModel.dNode[i].lightUniform.bufferMemory, &procModel.dNode[i].lightUbo, sizeof(procModel.dNode[i].lightUbo));
			}
		}

		//skybox
		{
			skybox.ubo.view				= camera.view;
			skybox.lightUbo.currentHour = currentHour;

			createMapMemory(skybox.uniform.bufferMemory, &skybox.ubo, sizeof(skybox.ubo));
			createMapMemory(skybox.lightUniform.bufferMemory, &skybox.lightUbo, sizeof(skybox.lightUbo));
		}

		//mine
		{
			mine.ubo.model = glm::translate(glm::mat4(1), box.minepos);
			mine.ubo.model *= glm::scale(glm::mat4(1), glm::vec3(1) * box.isVisible);
			mine.ubo.view  = camera.view;

			createMapMemory(mine.uniform.bufferMemory, &mine.ubo, sizeof(mine.ubo));
		}

		//osMirror
		{
			osMirror.ubo.model				= glm::translate(glm::mat4(1), glm::vec3(-16.0f, 24.0f, -16.0f));
			osMirror.ubo.model				*= glm::scale(glm::mat4(1), glm::vec3(32.0f));
			osMirror.ubo.modelTI			= glm::transpose(glm::inverse(osMirror.ubo.model));
			osMirror.ubo.view				= camera.view;
			osMirror.lightUbo.viewPos		= glm::vec3{ camera.pos - playerInfo::logicOriginPos };
			osMirror.lightUbo.currentHour	= currentHour;
			osMirror.lightUbo.ambientScale	= lightInfo::ambientScale;
			osMirror.lightUbo.gLightPos		= offsetPos;
			osMirror.lightUbo.flashPos		= glm::vec3{ camera.pos - playerInfo::logicOriginPos };
			osMirror.lightUbo.flashDir		= camera.front;
			osMirror.lightUbo.flashCutOff	= glm::cos(glm::radians(12.5f));
			osMirror.lightUbo.outerCutOff	= glm::cos(glm::radians(14.5f));

			createMapMemory(osMirror.uniform.bufferMemory, &osMirror.ubo, sizeof(osMirror.ubo));
			createMapMemory(osMirror.lightUniform.bufferMemory, &osMirror.lightUbo, sizeof(osMirror.lightUbo));
		}

		//dsScence
		{
			dsScence.lightUbo.viewPos		= glm::vec3{ camera.pos - playerInfo::logicOriginPos };
			dsScence.lightUbo.currentHour	= currentHour;
			dsScence.lightUbo.ambientScale	= lightInfo::ambientScale;
			dsScence.lightUbo.gLightPos		= offsetPos;
			dsScence.lightUbo.flashPos		= glm::vec3{ camera.pos - playerInfo::logicOriginPos };
			dsScence.lightUbo.flashDir		= camera.front;
			dsScence.lightUbo.flashCutOff	= glm::cos(glm::radians(12.5f));
			dsScence.lightUbo.outerCutOff	= glm::cos(glm::radians(14.5f));

			createMapMemory(dsScence.lightUniform.bufferMemory, &dsScence.lightUbo, sizeof(dsScence.lightUbo));
		}

		//showUIText
		{
			showUIText.pos		= glm::floor(camera.pos);
			showUIText.chunkPos = glm::floor(camera.pos / (double)plateInfo::chunkLength);
			showUIText.minepos  = box.minepos;
		}
	}

	void cleanupSwapChain(int threadId) {
		procRenderPass.cleanupSwapChain(Device.device, Device.allocator);
		command.free(Device.device, threadId);
	}

	void recreateSwapChain() {
		InitSwapchain.recreateSwapChain();
		createScResource();
		procRenderPass.recreateSwapChain(InitSwapchain.scExtent, InitSwapchain.colorFormat, InitSwapchain.depthFormat, InitSwapchain.msaaSamples, Device.device, Device.allocator);



		//重建离屏描述符，因为交换链重建后osView也得重建
		procDescriptor.setAllocInfo(bufferEnum::procDescriptorId::scenceForward);

		//osMirror
		{
			bufferStruct::osFramebufferNode& osMirrorFrameNode = procRenderPass.getOsFramebufferNode(bufferEnum::procRenderId::osMirror);
			std::array<VkWriteDescriptorSet, 3> osMirrorWrite{
				procDescriptor.getWrite(0, osMirror.descriptorSet, osMirror.uniform.buffer, sizeof(osMirror.ubo)),
				procDescriptor.getWrite(1, osMirror.descriptorSet, osMirror.lightUniform.buffer, sizeof(osMirror.lightUbo)),
				procDescriptor.getWrite(2, osMirror.descriptorSet, procSampler.node[osMirror.samplerId], osMirrorFrameNode.osView.view)
			};
			vkUpdateDescriptorSets(Device.device, uint32_t(osMirrorWrite.size()), osMirrorWrite.data(), 0, nullptr);
		}

		//osScence
		{
			bufferStruct::osFramebufferNode& osScenceFrameNode = procRenderPass.getOsFramebufferNode(bufferEnum::procRenderId::osScence);
			std::array<VkWriteDescriptorSet, 1> osScenceWrite{
				procDescriptor.getWrite(2, osScence.descriptorSet, procSampler.node[osScence.samplerId], osScenceFrameNode.osView.view)
			};
			vkUpdateDescriptorSets(Device.device, uint32_t(osScenceWrite.size()), osScenceWrite.data(), 0, nullptr);
		}

		procDescriptor.setAllocInfo(bufferEnum::procDescriptorId::dsScence);

		//dsScence
		{
			bufferStruct::dsFramebufferNode& dsScenceFrameNode = procRenderPass.getDsFramebufferNode(bufferEnum::procRenderId::dsScence);
			std::array<VkWriteDescriptorSet, 4> dsScenceWrite{
				procDescriptor.getWrite(0, dsScence.descriptorSet, dsScence.lightUniform.buffer, sizeof(dsScence.lightUbo)),
				procDescriptor.getWrite(1, dsScence.descriptorSet, procSampler.node[dsScence.samplerId], dsScenceFrameNode.position.view),
				procDescriptor.getWrite(2, dsScence.descriptorSet, procSampler.node[dsScence.samplerId], dsScenceFrameNode.normal.view),
				procDescriptor.getWrite(3, dsScence.descriptorSet, procSampler.node[dsScence.samplerId], dsScenceFrameNode.albedo.view)
			};

			vkUpdateDescriptorSets(Device.device, uint32_t(dsScenceWrite.size()), dsScenceWrite.data(), 0, nullptr);
		}
	}

private:
	bufferStruct::showUITextNode showUIText{};



	//创建地形顶点缓冲
	void createTerrainVertexBuffer(int threadId, int layer) {
		VkDeviceSize bufferSize = sizeof(terrainVertex::vertexs[layer][0]) * terrainVertex::vertexs[layer].size();
		bufferStruct::bufferNode tempBuffer;

		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, tempBuffer);
		createMapMemory(tempBuffer.bufferMemory, terrainVertex::vertexs[layer].data(), (size_t)bufferSize);

		for (int i = 0; i < bufferInfo::commandThreadSize; i++) {
			int thread = (threadId + i) % 2;

			vmaDestroyBuffer(Device.allocator, terrain.vertex[thread][layer].buffer, terrain.vertex[thread][layer].bufferMemory);
			createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, terrain.vertex[thread][layer]);
			copyBuffer(tempBuffer.buffer, terrain.vertex[thread][layer].buffer, bufferSize);
		}

		vmaDestroyBuffer(Device.allocator, tempBuffer.buffer, tempBuffer.bufferMemory);
	}

	//创建地形索引缓冲
	void createTerrainIndexBuffer(int threadId, int layer) {
		VkDeviceSize bufferSize = sizeof(terrainVertex::indexs[layer][0]) * terrainVertex::indexs[layer].size();
		bufferStruct::bufferNode tempBuffer;

		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, tempBuffer);
		createMapMemory(tempBuffer.bufferMemory, terrainVertex::indexs[layer].data(), (size_t)bufferSize);

		for (int i = 0; i < bufferInfo::commandThreadSize; i++) {
			int thread = (threadId + i) % 2;

			vmaDestroyBuffer(Device.allocator, terrain.index[thread][layer].buffer, terrain.index[thread][layer].bufferMemory);
			createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, terrain.index[thread][layer]);
			copyBuffer(tempBuffer.buffer, terrain.index[thread][layer].buffer, bufferSize);
		}

		vmaDestroyBuffer(Device.allocator, tempBuffer.buffer, tempBuffer.bufferMemory);
	}

public:
	bufferList::procDescriptorNode& getDst() {
		return procDescriptor;
	}

	const VkRenderPass& getRender(bufferEnum::frameNodeId::type id) {
		return procRenderPass.nodePass[id].render;
	}

	const VkExtent2D& getScExtent() {
		return InitSwapchain.scExtent;
	}

	const VkSampleCountFlagBits& getMsaaSamples() {
		return InitSwapchain.msaaSamples;
	}
};