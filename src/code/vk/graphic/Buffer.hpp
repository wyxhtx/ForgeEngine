#pragma once



class Buffer {
public:
	bufferStruct::commandNode command{};



	Buffer(Device& Device) :Device{ Device } {
		createCommandPool();

		descriptors.createDescriptorPool(Device.device);
		descriptors.createDescriptorSetLayout(Device.device);

		createMeshBuffer();
		createUniformBuffer();

		createTexture();
		samplers.createTextureSampler(Device.device, opaqueTerrain.mipLevel, model.mipLevel);
		createDescriptorSet();

		createDepthResources();
		createColorResources();

		initUniformBuffer();

		if (debugInfo::LogSwitch) {
			Log::printTitle("————————Buffer已创建");

			Log::delimiter();
		}
	}

	~Buffer() {
		for (int i = 0; i < swapChainFramebuffers.size(); i++) {
			vkDestroyFramebuffer(Device.device, swapChainFramebuffers[i], nullptr);
		}

		vkDestroyCommandPool(Device.device, command.commandPool, nullptr);
		descriptors.clear(Device.device);
		samplers.clear(Device.device);

		opaqueTerrain.clear(Device.device, Device.allocator);
		model.clear(Device.device, Device.allocator);
		skybox.clear(Device.device, Device.allocator);
		mine.clear(Device.device, Device.allocator);
		uiText.clear(Device.device, Device.allocator);

		vkDestroyImageView(Device.device, colorImage.view, nullptr);
		vmaDestroyImage(Device.allocator, colorImage.image, colorImage.imageMemory);

		vkDestroyImageView(Device.device, depthImage.view, nullptr);
		vmaDestroyImage(Device.allocator, depthImage.image, depthImage.imageMemory);

		if (debugInfo::LogSwitch) {
			Log::print("Buffer已清除");
		}
	}

private:
	Device& Device;

	std::vector<VkFramebuffer> swapChainFramebuffers;   //在一个渲染通道中所必要的一组图像附件（attachment）的集合

	bufferList::descriptor descriptors;
	bufferList::sampler samplers;

	bufferStruct::terrainBufferNode opaqueTerrain{ fileInfo::image::cube,textureInfo::arrayLayer };
	bufferStruct::modelBufferNode	model{ fileInfo::image::temple };
	bufferStruct::skyboxBufferNode	skybox;
	bufferStruct::baseBufferNode	mine;
	bufferStruct::uiTextBufferNode	uiText;

	bufferStruct::imageNode depthImage;
	bufferStruct::imageNode colorImage;



	void createCommandPool() {
		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = Device.queueFamilyIndex;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		if (vkCreateCommandPool(Device.device, &poolInfo, nullptr, &command.commandPool) != VK_SUCCESS) {
			Log::error(__FILE__, __LINE__, "命令池创建失败");
			throw;
		}
	}

	void createMeshBuffer() {
		//opaqueTerrain
		createVertexBuffer(terrainVertex::vertexs.opaqueTerrain, opaqueTerrain.vertex[0].buffer, opaqueTerrain.vertex[0].bufferMemory);
		createIndexBuffer(terrainVertex::indexs.opaqueTerrain, opaqueTerrain.index[0].buffer, opaqueTerrain.index[0].bufferMemory);

		//model
		loadModel();
		createVertexBuffer(modelVertex::vertexs.model, model.vertex.buffer, model.vertex.bufferMemory);
		createIndexBuffer(modelVertex::indexs.model, model.index.buffer, model.index.bufferMemory);

		//skybox
		createVertexBuffer(skyboxVertex::vertexs, skybox.vertex.buffer, skybox.vertex.bufferMemory);
		createIndexBuffer(skyboxVertex::indexs, skybox.index.buffer, skybox.index.bufferMemory);

		//mine
		createVertexBuffer(mineVertex::vertexs, mine.vertex.buffer, mine.vertex.bufferMemory);
		createIndexBuffer(mineVertex::indexs, mine.index.buffer, mine.index.bufferMemory);

		//uiText
		createBuffer(2048 * sizeof(glm::vec4), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uiText.vertex.buffer, uiText.vertex.bufferMemory);
	}

	void createUniformBuffer() {
		//opaqueTerrain
		createBuffer(sizeof(opaqueTerrain.ubo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, opaqueTerrain.uniform.buffer, opaqueTerrain.uniform.bufferMemory);

		//model
		createBuffer(sizeof(model.ubo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, model.uniform.buffer, model.uniform.bufferMemory);

		//skybox
		createBuffer(sizeof(skybox.ubo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, skybox.uniform.buffer, skybox.uniform.bufferMemory);

		//mine
		createBuffer(sizeof(mine.ubo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, mine.uniform.buffer, mine.uniform.bufferMemory);
	}

	void createTexture() {
		//opaqueTerrain
		createTextureKtxArrayNode(opaqueTerrain.texture.image, opaqueTerrain.texture.imageMemory, opaqueTerrain.texture.view, opaqueTerrain.imageFile, opaqueTerrain.arrayLayer, opaqueTerrain.mipLevel);

		//model
		createTextureKtxNode(model.texture.image, model.texture.imageMemory, model.texture.view, model.imageFile, model.mipLevel);

		//uiText
		createTextNode(uiText.texture.image, uiText.texture.imageMemory, uiText.texture.view);
	}

	void createDescriptorSet() {
		//添加新元素时注意bufferScenceNodeSize的限制
		VkDescriptorSetAllocateInfo allocInfo = {};
		VkDescriptorBufferInfo bufferInfo = {};
		VkDescriptorImageInfo imageInfo = {};
		std::array<VkWriteDescriptorSet, 2> writeDescriptorSetScence = {};
		{
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.pNext = nullptr;
			allocInfo.descriptorSetCount = 1;

			bufferInfo.offset = 0;

			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			//写入 uniform 缓冲区的描述符信息
			writeDescriptorSetScence[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSetScence[0].dstBinding = 0;
			writeDescriptorSetScence[0].dstArrayElement = 0;
			writeDescriptorSetScence[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writeDescriptorSetScence[0].descriptorCount = 1;
			//写入组合图像采样器的描述符信息
			writeDescriptorSetScence[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSetScence[1].dstBinding = 1;
			writeDescriptorSetScence[1].dstArrayElement = 0;
			writeDescriptorSetScence[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writeDescriptorSetScence[1].descriptorCount = 1;
		}

		//报错时启用
		/*VkResult  result = vkAllocateDescriptorSets(Device.device, &allocInfo, &model.descriptorSet);
		if (result != VK_SUCCESS) {
			Log::error(__FILE__, __LINE__, std::to_string(result));
			throw;
		}*/

		//配置scence描述符
		allocInfo.descriptorPool = descriptors.scence.pool;
		allocInfo.pSetLayouts = &descriptors.scence.layout;

		//opaqueTerrain
		if (vkAllocateDescriptorSets(Device.device, &allocInfo, &opaqueTerrain.descriptorSet) != VK_SUCCESS) {
			Log::error(__FILE__, __LINE__, "分配描述符集失败");
			throw;
		}
		{
			bufferInfo.range = sizeof(opaqueTerrain.ubo);
			bufferInfo.buffer = opaqueTerrain.uniform.buffer;
			imageInfo.sampler = samplers.terrain;
			imageInfo.imageView = opaqueTerrain.texture.view;

			writeDescriptorSetScence[0].dstSet = opaqueTerrain.descriptorSet;
			writeDescriptorSetScence[0].pBufferInfo = &bufferInfo;
			writeDescriptorSetScence[1].dstSet = opaqueTerrain.descriptorSet;
			writeDescriptorSetScence[1].pImageInfo = &imageInfo;
		}

		vkUpdateDescriptorSets(Device.device, uint32_t(writeDescriptorSetScence.size()), writeDescriptorSetScence.data(), 0, nullptr);

		//model
		if (vkAllocateDescriptorSets(Device.device, &allocInfo, &model.descriptorSet) != VK_SUCCESS) {
			Log::error(__FILE__, __LINE__, "分配描述符集失败");
			throw;
		}
		{

			bufferInfo.range = sizeof(model.ubo);
			bufferInfo.buffer = model.uniform.buffer;
			imageInfo.sampler = samplers.model;
			imageInfo.imageView = model.texture.view;

			writeDescriptorSetScence[0].dstSet = model.descriptorSet;
			writeDescriptorSetScence[0].pBufferInfo = &bufferInfo;
			writeDescriptorSetScence[1].dstSet = model.descriptorSet;
			writeDescriptorSetScence[1].pImageInfo = &imageInfo;
		}

		vkUpdateDescriptorSets(Device.device, uint32_t(writeDescriptorSetScence.size()), writeDescriptorSetScence.data(), 0, nullptr);

		//skybox
		if (vkAllocateDescriptorSets(Device.device, &allocInfo, &skybox.descriptorSet) != VK_SUCCESS) {
			Log::error(__FILE__, __LINE__, "分配描述符集失败");
			throw;
		}
		{
			bufferInfo.range = sizeof(skybox.ubo);
			bufferInfo.buffer = skybox.uniform.buffer;

			writeDescriptorSetScence[0].dstSet = skybox.descriptorSet;
			writeDescriptorSetScence[0].pBufferInfo = &bufferInfo;
		}

		vkUpdateDescriptorSets(Device.device, 1, &writeDescriptorSetScence[0], 0, nullptr);

		//mine
		if (vkAllocateDescriptorSets(Device.device, &allocInfo, &mine.descriptorSet) != VK_SUCCESS) {
			Log::error(__FILE__, __LINE__, "分配描述符集失败");
			throw;
		}
		{
			bufferInfo.range = sizeof(mine.ubo);
			bufferInfo.buffer = mine.uniform.buffer;

			writeDescriptorSetScence[0].dstSet = mine.descriptorSet;
			writeDescriptorSetScence[0].pBufferInfo = &bufferInfo;
		}

		vkUpdateDescriptorSets(Device.device, 1, &writeDescriptorSetScence[0], 0, nullptr);

		//配置uitext描述符
		allocInfo.descriptorPool = descriptors.uiText.pool;
		allocInfo.pSetLayouts = &descriptors.uiText.layout;

		//uiText
		if (vkAllocateDescriptorSets(Device.device, &allocInfo, &uiText.descriptorSet) != VK_SUCCESS) {
			Log::error(__FILE__, __LINE__, "分配描述符集失败");
			throw;
		}

		std::array<VkWriteDescriptorSet, 1> writeDescriptorSetUIText = {};
		{
			writeDescriptorSetUIText[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSetUIText[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writeDescriptorSetUIText[0].dstBinding = 0;
			writeDescriptorSetUIText[0].descriptorCount = 1;

			imageInfo.sampler = samplers.uiText;
			imageInfo.imageView = uiText.texture.view;

			writeDescriptorSetUIText[0].dstSet = uiText.descriptorSet;
			writeDescriptorSetUIText[0].pImageInfo = &imageInfo;
		}

		vkUpdateDescriptorSets(Device.device, 1, writeDescriptorSetUIText.data(), 0, nullptr);
	}

	void createDepthResources() {
		createImage(Device.swapChainExtent.width, Device.swapChainExtent.height, 1, 1, Device.depthFormat, Device.msaaSamples, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage.image, depthImage.imageMemory);
		createImageView(depthImage.image, depthImage.view, 1, 1, VK_IMAGE_VIEW_TYPE_2D, Device.depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
	}

	void createColorResources() {
		createImage(Device.swapChainExtent.width, Device.swapChainExtent.height, 1, 1, Device.imageFormat, Device.msaaSamples, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorImage.image, colorImage.imageMemory);
		createImageView(colorImage.image, colorImage.view, 1, 1, VK_IMAGE_VIEW_TYPE_2D, Device.imageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
		transitionImageLayout(colorImage.image, 1, 1, Device.imageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	}

	void initUniformBuffer() {
		//设置透视投影，参数依次是视野角度、宽高比、近裁剪面和远裁剪面，以及proj[1][1] *= -1轴向修正
		glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)Device.swapChainExtent.width / (float)Device.swapChainExtent.height, 0.1f, plateInfo::cubeSightRange);

		proj[1][1] *= -1;

		opaqueTerrain.ubo.base = glm::translate(glm::mat4(1), glm::vec3(0));
		opaqueTerrain.ubo.proj = proj;
		opaqueTerrain.ubo.ambientStrength = 1;
		//opaqueTerrain.ubo.baseLight = lightInfo::baseLight;

		model.ubo.proj = proj;
		skybox.ubo.proj = proj;
		mine.ubo.proj = proj;
	}

private:
	//将vertexNode类中的顶点映射到buffer类里对应的VkBuffer
	void createVertexBuffer(auto& node, VkBuffer& vertexBuffer, VmaAllocation& vertexBufferMemory) {
		//获取顶点数组大小
		VkDeviceSize bufferSize = sizeof(node[0]) * node.size();

		//创建临时缓冲，方便CPU访问
		VkBuffer tempBuffer;
		VmaAllocation tempMemory;
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, tempBuffer, tempMemory);

		//内存映射
		createMapMemory(tempMemory, node.data(), (size_t)bufferSize);

		//创建顶点缓冲，方便GPU访问
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

		//临时缓冲拷贝到顶点缓冲
		copyBuffer(tempBuffer, vertexBuffer, bufferSize);

		vmaDestroyBuffer(Device.allocator, tempBuffer, tempMemory);
	}

	//将vertexNode类中的索引映射到buffer类里对应的VkBuffer
	void createIndexBuffer(std::vector<uint32_t>& node, VkBuffer& indexBuffer, VmaAllocation& indexBufferMemory) {
		//获取索引数组大小
		VkDeviceSize bufferSize = sizeof(node[0]) * node.size();

		//创建临时缓冲，方便CPU访问
		VkBuffer tempBuffer;
		VmaAllocation tempMemory;
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, tempBuffer, tempMemory);

		//内存映射
		createMapMemory(tempMemory, node.data(), (size_t)bufferSize);

		//创建索引缓冲，方便GPU访问
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

		//临时缓冲拷贝到索引缓冲
		copyBuffer(tempBuffer, indexBuffer, bufferSize);

		vmaDestroyBuffer(Device.allocator, tempBuffer, tempMemory);
	}

	void loadModel() {
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, fileInfo::model::temple.c_str())) {
			throw std::runtime_error(warn + err);
		}

		std::unordered_map<modelVertex::vertexNode, uint32_t> uniqueVertices{};

		for (const auto& shape : shapes) {
			for (const auto& index : shape.mesh.indices) {
				modelVertex::vertexNode vertex{};

				//翻转模型
				vertex.position = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
				};

				vertex.texCoord = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
				};

				if (uniqueVertices.count(vertex) == 0) {
					uniqueVertices[vertex] = static_cast<uint32_t>(modelVertex::vertexs.model.size());
					modelVertex::vertexs.model.push_back(vertex);
				}

				modelVertex::indexs.model.push_back(uniqueVertices[vertex]);
			}
		}
	}

	//创建纹理KTX数组结点，使用无损压缩
	void createTextureKtxArrayNode(VkImage& textureImage, VmaAllocation& textureImageMemory, VkImageView& textureView, char const* image, uint32_t arrayLayer, uint32_t& mipLevel) {
		ktxResult result;
		ktxTexture2* ktxTexture;

		result = ktxTexture2_CreateFromNamedFile(image, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture);

		if (result != KTX_SUCCESS) {
			Log::error(__FILE__, __LINE__, "KTX贴图加载失败");
			throw;
		}

		uint32_t texWidth = ktxTexture->baseWidth;
		uint32_t texHeight = ktxTexture->baseHeight;
		mipLevel = ktxTexture->numLevels;
		ktx_uint8_t* textureData = ktxTexture->pData;
		ktx_size_t textureSize = ktxTexture->dataSize;
		VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

		VkBuffer tempBuffer;
		VmaAllocation tempMemory;

		createBuffer(textureSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, tempBuffer, tempMemory);
		createMapMemory(tempMemory, textureData, static_cast<size_t>(textureSize));
		createImage(textureInfo::tileWidth, textureInfo::tileHeight, arrayLayer, mipLevel, format, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);

		transitionImageLayout(textureImage, arrayLayer, mipLevel, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		copyBufferToImageArray(textureInfo::tileWidth, textureInfo::tileHeight, textureInfo::atlasWidth, textureInfo::atlasHeight, texWidth, texHeight, arrayLayer, tempBuffer, textureImage);
		transitionImageLayout(textureImage, arrayLayer, mipLevel, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		createImageView(textureImage, textureView, arrayLayer, mipLevel, VK_IMAGE_VIEW_TYPE_2D_ARRAY, format, VK_IMAGE_ASPECT_COLOR_BIT);

		vmaDestroyBuffer(Device.allocator, tempBuffer, tempMemory);
		ktxTexture2_Destroy(ktxTexture);
	}

	//创建纹理KTX结点，使用微损压缩 + mip
	void createTextureKtxNode(VkImage& textureImage, VmaAllocation& textureImageMemory, VkImageView& textureView, char const* image, uint32_t& mipLevel) {
		ktxResult result;
		ktxTexture2* ktxTexture;

		result = ktxTexture2_CreateFromNamedFile(image, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture);
		result = ktxTexture2_TranscodeBasis(ktxTexture, KTX_TTF_BC7_RGBA, 0);

		if (result != KTX_SUCCESS) {
			Log::error(__FILE__, __LINE__, "KTX贴图加载失败");
			throw;
		}

		uint32_t texWidth = ktxTexture->baseWidth;
		uint32_t texHeight = ktxTexture->baseHeight;
		mipLevel = ktxTexture->numLevels;
		ktx_uint8_t* textureData = ktxTexture->pData;
		ktx_size_t textureSize = ktxTexture->dataSize;
		VkFormat format = VK_FORMAT_BC7_UNORM_BLOCK;

		VkBuffer tempBuffers;
		VmaAllocation tempMemory;

		//创建临时缓冲，方便CPU访问
		createBuffer(textureSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, tempBuffers, tempMemory);
		//内存映射
		createMapMemory(tempMemory, textureData, static_cast<size_t>(textureSize));
		//创建纹理缓冲，方便GPU访问
		createImage(texWidth, texHeight, 1, mipLevel, format, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);

		transitionImageLayout(textureImage, 1, mipLevel, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);					//图像格式转换为传送最佳
		copyBufferToImageKtx(texWidth, texHeight, mipLevel, ktxTexture, tempBuffers, textureImage);												//临时缓冲拷贝到图像缓冲
		transitionImageLayout(textureImage, 1, mipLevel, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);	//图像格式转换为采样最佳

		createImageView(textureImage, textureView, 1, mipLevel, VK_IMAGE_VIEW_TYPE_2D, format, VK_IMAGE_ASPECT_COLOR_BIT);

		vmaDestroyBuffer(Device.allocator, tempBuffers, tempMemory);
		ktxTexture2_Destroy(ktxTexture);
	}

	//创建纹理文本结点
	void createTextNode(VkImage& textureImage, VmaAllocation& textureImageMemory, VkImageView& textureView) {
		const uint32_t fontWidth = STB_FONT_consolas_24_latin1_BITMAP_WIDTH;
		const uint32_t fontHeight = STB_FONT_consolas_24_latin1_BITMAP_HEIGHT;
		VkDeviceSize imageSize = static_cast<VkDeviceSize>(fontWidth) * static_cast<VkDeviceSize>(fontHeight);

		static unsigned char font24pixels[fontHeight][fontWidth];
		stb_font_consolas_24_latin1(uiText.stbFontData, font24pixels, fontHeight);

		//创建临时缓冲，方便CPU访问
		VkBuffer tempBuffers;
		VmaAllocation tempMemory;
		createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, tempBuffers, tempMemory);

		//内存映射
		createMapMemory(tempMemory, &font24pixels[0][0], static_cast<size_t>(imageSize));

		//创建纹理缓冲，方便GPU访问
		createImage(fontWidth, fontHeight, 1, 1, VK_FORMAT_R8_UNORM, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, uiText.texture.image, uiText.texture.imageMemory);

		//图像格式转换为传送最佳
		transitionImageLayout(textureImage, 1, 1, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		//临时缓冲拷贝到图像缓冲
		copyBufferToImage(static_cast<uint32_t>(fontWidth), static_cast<uint32_t>(fontHeight), tempBuffers, textureImage);
		//图像格式转换为采样最佳
		transitionImageLayout(textureImage, 1, 1, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		vmaDestroyBuffer(Device.allocator, tempBuffers, tempMemory);

		createImageView(textureImage, textureView, 1, 1, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
	}

	void createImageView(VkImage image, VkImageView& view, uint32_t arrayLayer, uint32_t mipLevel, VkImageViewType viewType, VkFormat format, VkImageAspectFlags aspectFlags) {
		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image;
		viewInfo.viewType = viewType;
		viewInfo.format = format;
		viewInfo.subresourceRange.aspectMask = aspectFlags;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = mipLevel;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = arrayLayer;

		if (vkCreateImageView(Device.device, &viewInfo, nullptr, &view) != VK_SUCCESS) {
			Log::error(__FILE__, __LINE__, "纹理图像视图创建失败");
			throw;
		}
	}



	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VmaAllocation& allocation) {
		VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferInfo.size = size;
		bufferInfo.usage = usage;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_UNKNOWN; // 使用 VMA_MEMORY_USAGE_UNKNOWN 将内存属性交由属性参数控制
		allocInfo.requiredFlags = properties;		// 设置所需的内存属性

		vmaCreateBuffer(Device.allocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr);
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
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = mipLevel;
		imageInfo.arrayLayers = arrayLayer;
		imageInfo.format = format;
		imageInfo.tiling = tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.samples = numSamples;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		allocInfo.requiredFlags = properties;

		if (vmaCreateImage(Device.allocator, &imageInfo, &allocInfo, &image, &allocation, nullptr) != VK_SUCCESS) {
			Log::error(__FILE__, __LINE__, "图像创建失败");
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
			Log::error(__FILE__, __LINE__, "无支持的布局转换");
			throw;
		}

		// 在命令缓冲区中插入图像内存屏障，以实现布局转换
		vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		// 结束单次命令缓冲区，提交并执行命令
		endSingleTimeCommands(commandBuffer);
	}

	void copyBufferToImageArray(uint32_t tileWidth, uint32_t tileHeight, uint32_t atlasWidth, uint32_t atlasHeight, uint32_t texWidth, uint32_t texHeight, uint32_t arrayLayer, VkBuffer buffer, VkImage image) {
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();
		std::vector<VkBufferImageCopy> regions{ arrayLayer };			//vulkan存储方式是从左往右从上往下一个个存入像素点数据，所以按下述方法遍历
		uint32_t pixelOffset = atlasWidth * tileWidth * tileHeight * 4;

		for (uint32_t h = 0; h < atlasHeight; h++) {
			for (uint32_t w = 0; w < atlasWidth; w++) {
				uint32_t id = w + h * atlasWidth;
				uint32_t offset = w * 64 + h * pixelOffset;

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
	void createPipelineLayout(pipelineList::pipelineLayout& pipelineLayout) {
		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
		pipelineLayoutInfo.pPushConstantRanges = 0; // Optional

		//scence
		pipelineLayoutInfo.pSetLayouts = &descriptors.scence.layout;

		if (vkCreatePipelineLayout(Device.device, &pipelineLayoutInfo, nullptr, &pipelineLayout.scence) != VK_SUCCESS) {
			Log::error(__FILE__, __LINE__, "管线布局创建失败");
		}

		//uiText
		pipelineLayoutInfo.pSetLayouts = &descriptors.uiText.layout;

		if (vkCreatePipelineLayout(Device.device, &pipelineLayoutInfo, nullptr, &pipelineLayout.uiText) != VK_SUCCESS) {
			Log::error(__FILE__, __LINE__, "管线布局创建失败");
		}
	}

	void createFramebuffers(std::vector<VkImageView>& swapChainImageViews, VkRenderPass& renderPass) {
		swapChainFramebuffers.resize(swapChainImageViews.size());

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.width = Device.swapChainExtent.width;
		framebufferInfo.height = Device.swapChainExtent.height;
		framebufferInfo.layers = 1;

		for (size_t i = 0; i < swapChainImageViews.size(); i++) {
			//将subpass描述的内容呈现到屏幕上,必须对应
			std::array<VkImageView, 3> attachments = { colorImage.view,depthImage.view,swapChainImageViews[i] };

			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebufferInfo.pAttachments = attachments.data();

			if (vkCreateFramebuffer(Device.device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
				Log::error(__FILE__, __LINE__, "帧缓冲创建失败");
				throw;
			}
		}
	}

	void drawUIText(int fps) {
		static std::ostringstream oss;
		float width = 5.0;
		float height = 5.0;

		uiText.beginTextUpdate(Device.allocator);

		//世界坐标
		oss << std::fixed << std::setprecision(0);	//设置输出流的格式,不保留小数
		oss << "worldpos:  " << "x:" << std::setw(8) << std::left << showUIText.pos.x << "y:" << std::setw(8) << std::left << showUIText.pos.y << " z:" << std::setw(8) << std::left << showUIText.pos.z;
		uiText.addText(oss.str(), width, height, bufferEnum::TextAlign::alignLeft);
		oss.str("");

		//区块坐标
		oss << "chunkpos:  " << "x:" << std::setw(8) << std::left << showUIText.chunkPos.x << "y:" << std::setw(8) << std::left << showUIText.chunkPos.y << " z:" << std::setw(8) << std::left << showUIText.chunkPos.z;
		uiText.addText(oss.str(), width, height += 15, bufferEnum::TextAlign::alignLeft);
		oss.str("");

		//选择框坐标
		oss << "mineboxpos:" << "x:" << std::setw(8) << std::left << showUIText.minepos.x << "y:" << std::setw(8) << std::left << showUIText.minepos.y << " z:" << std::setw(8) << std::left << showUIText.minepos.z;
		uiText.addText(oss.str(), width, height += 15, bufferEnum::TextAlign::alignLeft);
		oss.str("");

		//帧率
		oss << "fps:       " << std::setw(8) << std::left << fps;
		uiText.addText(oss.str(), width, height += 15, bufferEnum::TextAlign::alignLeft);
		oss.str("");

		uiText.endTextUpdate(Device.allocator);
	}

	void initCommandBuffers() {
		for (int i = 0; i < bufferInfo::commandThreadSize; i++) {
			command.commandBuffers[i].resize(swapChainFramebuffers.size());
		}

		command.allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		command.allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		command.allocInfo.commandPool = command.commandPool;

		command.beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		command.beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		command.beginInfo.pInheritanceInfo = nullptr; // Optional

		command.clearValues[0].color = { 105.0f / 255.f, 105.0f / 255.f, 105.0f / 255.f, 1.0f };
		command.clearValues[1].depthStencil = { 1.0f, 0 };

		command.renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		command.renderPassInfo.renderArea.offset = { 0, 0 };
		command.renderPassInfo.clearValueCount = static_cast<uint32_t>(command.clearValues.size());
		command.renderPassInfo.pClearValues = command.clearValues.data();
	}

	//初始化时和重建交换链时调用
	void renderPassCommandBuffers(VkRenderPass& renderPass) {
		command.renderPassInfo.renderPass = renderPass;
		command.renderPassInfo.renderArea.extent = Device.swapChainExtent;
	}

	void createCommandBuffers(int threadID, pipelineList::procPipeline& procPipeline) {
		command.allocInfo.commandBufferCount = (uint32_t)command.commandBuffers[threadID].size();

		if (vkAllocateCommandBuffers(Device.device, &command.allocInfo, command.commandBuffers[threadID].data()) != VK_SUCCESS) {
			Log::error(__FILE__, __LINE__, "基础命令缓冲创建失败");
			throw;
		}

		VkDeviceSize offsets[] = { 0 };	//绑定管线过程中指定各顶点缓冲区的偏移量

		for (int i = 0; i < command.commandBuffers[threadID].size(); i++) {
			command.renderPassInfo.framebuffer = swapChainFramebuffers[i];

			vkResetCommandBuffer(command.commandBuffers[threadID][i], 0);
			vkBeginCommandBuffer(command.commandBuffers[threadID][i], &command.beginInfo);
			vkCmdBeginRenderPass(command.commandBuffers[threadID][i], &command.renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			//opaqueTerrain
			vkCmdBindPipeline(command.commandBuffers[threadID][i], VK_PIPELINE_BIND_POINT_GRAPHICS, procPipeline.opaqueTerrain.pipeline);
			vkCmdBindDescriptorSets(command.commandBuffers[threadID][i], VK_PIPELINE_BIND_POINT_GRAPHICS, procPipeline.opaqueTerrain.pipelineLayout, 0, 1, &opaqueTerrain.descriptorSet, 0, nullptr);
			vkCmdBindVertexBuffers(command.commandBuffers[threadID][i], 0, 1, &opaqueTerrain.vertex[threadID].buffer, offsets);
			vkCmdBindIndexBuffer(command.commandBuffers[threadID][i], opaqueTerrain.index[threadID].buffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(command.commandBuffers[threadID][i], (uint32_t)terrainVertex::indexs.opaqueTerrain.size(), 1, 0, 0, 0);

			//model
			vkCmdBindPipeline(command.commandBuffers[threadID][i], VK_PIPELINE_BIND_POINT_GRAPHICS, procPipeline.model.pipeline);
			vkCmdBindDescriptorSets(command.commandBuffers[threadID][i], VK_PIPELINE_BIND_POINT_GRAPHICS, procPipeline.model.pipelineLayout, 0, 1, &model.descriptorSet, 0, nullptr);
			vkCmdBindVertexBuffers(command.commandBuffers[threadID][i], 0, 1, &model.vertex.buffer, offsets);
			vkCmdBindIndexBuffer(command.commandBuffers[threadID][i], model.index.buffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(command.commandBuffers[threadID][i], (uint32_t)modelVertex::indexs.model.size(), 1, 0, 0, 0);

			//skybox
			vkCmdBindPipeline(command.commandBuffers[threadID][i], VK_PIPELINE_BIND_POINT_GRAPHICS, procPipeline.skybox.pipeline);
			vkCmdBindDescriptorSets(command.commandBuffers[threadID][i], VK_PIPELINE_BIND_POINT_GRAPHICS, procPipeline.skybox.pipelineLayout, 0, 1, &skybox.descriptorSet, 0, nullptr);
			vkCmdBindVertexBuffers(command.commandBuffers[threadID][i], 0, 1, &skybox.vertex.buffer, offsets);
			vkCmdBindIndexBuffer(command.commandBuffers[threadID][i], skybox.index.buffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(command.commandBuffers[threadID][i], (uint32_t)skyboxVertex::indexs.size(), 1, 0, 0, 0);

			//mine
			vkCmdBindPipeline(command.commandBuffers[threadID][i], VK_PIPELINE_BIND_POINT_GRAPHICS, procPipeline.mine.pipeline);
			vkCmdBindDescriptorSets(command.commandBuffers[threadID][i], VK_PIPELINE_BIND_POINT_GRAPHICS, procPipeline.mine.pipelineLayout, 0, 1, &mine.descriptorSet, 0, nullptr);
			vkCmdBindVertexBuffers(command.commandBuffers[threadID][i], 0, 1, &mine.vertex.buffer, offsets);
			vkCmdBindIndexBuffer(command.commandBuffers[threadID][i], mine.index.buffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(command.commandBuffers[threadID][i], (uint32_t)mineVertex::indexs.size(), 1, 0, 0, 0);

			//uiText
			vkCmdBindPipeline(command.commandBuffers[threadID][i], VK_PIPELINE_BIND_POINT_GRAPHICS, procPipeline.uiText.pipeline);
			vkCmdBindDescriptorSets(command.commandBuffers[threadID][i], VK_PIPELINE_BIND_POINT_GRAPHICS, procPipeline.uiText.pipelineLayout, 0, 1, &uiText.descriptorSet, 0, NULL);
			vkCmdBindVertexBuffers(command.commandBuffers[threadID][i], 0, 1, &uiText.vertex.buffer, offsets);
			vkCmdBindVertexBuffers(command.commandBuffers[threadID][i], 1, 1, &uiText.vertex.buffer, offsets);
			for (uint32_t j = 0; j < uiText.numLetters; j++) {	
				vkCmdDraw(command.commandBuffers[threadID][i], 4, 1, j * 4, 0);		//不用循环文本底部有连线
			}

			vkCmdEndRenderPass(command.commandBuffers[threadID][i]);
			if (vkEndCommandBuffer(command.commandBuffers[threadID][i]) != VK_SUCCESS) {
				Log::error(__FILE__, __LINE__, "记录命令缓冲区失败");
			}
		}
	}

	void recreateBuffer(int threadID) {
		vkFreeCommandBuffers(Device.device, command.commandPool, static_cast<uint32_t>(command.commandBuffers[threadID].size()), command.commandBuffers[threadID].data());
		vmaDestroyBuffer(Device.allocator, opaqueTerrain.vertex[threadID].buffer, opaqueTerrain.vertex[threadID].bufferMemory);
		vmaDestroyBuffer(Device.allocator, opaqueTerrain.index[threadID].buffer, opaqueTerrain.index[threadID].bufferMemory);

		createVertexBuffer(terrainVertex::vertexs.opaqueTerrain, opaqueTerrain.vertex[threadID].buffer, opaqueTerrain.vertex[threadID].bufferMemory);
		createIndexBuffer(terrainVertex::indexs.opaqueTerrain, opaqueTerrain.index[threadID].buffer, opaqueTerrain.index[threadID].bufferMemory);
	}

	void updateUniformBuffer(const glm::dvec3 pos, const glm::mat4 view, const playerStruct::selectBoxNode box) {
		static std::chrono::steady_clock::time_point time0 = std::chrono::steady_clock::now();	//游戏起始时刻
		static std::chrono::steady_clock::time_point time1;										//游戏当前时刻
		static float gameTime;																	//游戏运行时长，最多支持连续运行194天（现实时间）

		glm::vec3 offsetPos = playerInfo::initSpawnPos - playerInfo::logicOriginPos;



		time1 = std::chrono::steady_clock::now();
		gameTime = std::chrono::duration_cast<std::chrono::milliseconds>(time1 - time0).count() / 1000.0f;

		//opaqueTerrain
		//opaqueTerrain.ubo.ambientStrength = (sin(gameTime) + 1) / 2;//暂时无用
		opaqueTerrain.ubo.view = view;

		//model
		model.ubo.base = glm::translate(glm::mat4(1), offsetPos);	//位移
		model.ubo.base *= glm::scale(glm::mat4(1), glm::vec3(0.1f));														//缩放
		//model.ubo.base *= glm::rotate(glm::mat4(1), gameTime * glm::radians(60.f), coordsInfo::worldUp);					//旋转
		model.ubo.view = view;

		//skybox
		skybox.ubo.base = glm::translate(glm::mat4(1), glm::vec3(pos - playerInfo::logicOriginPos));
		skybox.ubo.currentHour = std::fmod(gameTime, 24.0f);																//取值范围[0,24)，含小数部分
		skybox.ubo.view = view;

		//mine
		mine.ubo.base = glm::translate(glm::mat4(1), box.minepos);
		mine.ubo.base *= glm::scale(glm::mat4(1), glm::vec3(1) * box.isVisible);
		mine.ubo.view = view;

		//showUIText
		showUIText.pos = glm::floor(pos);
		showUIText.chunkPos = glm::floor(pos / (double)plateInfo::chunkLength);
		showUIText.minepos = box.minepos;

		createMapMemory(opaqueTerrain.uniform.bufferMemory, &opaqueTerrain.ubo, sizeof(opaqueTerrain.ubo));
		createMapMemory(model.uniform.bufferMemory, &model.ubo, sizeof(model.ubo));
		createMapMemory(skybox.uniform.bufferMemory, &skybox.ubo, sizeof(skybox.ubo));
		createMapMemory(mine.uniform.bufferMemory, &mine.ubo, sizeof(mine.ubo));
	}

	void cleanupSwapChain(int threadID) {
		vkFreeCommandBuffers(Device.device, command.commandPool, static_cast<uint32_t>(command.commandBuffers[threadID].size()), command.commandBuffers[threadID].data());

		for (size_t i = 0; i < swapChainFramebuffers.size(); i++) {
			vkDestroyFramebuffer(Device.device, swapChainFramebuffers[i], nullptr);
		}

		vkDestroyImageView(Device.device, depthImage.view, nullptr);
		vmaDestroyImage(Device.allocator, depthImage.image, depthImage.imageMemory);

		vkDestroyImageView(Device.device, colorImage.view, nullptr);
		vmaDestroyImage(Device.allocator, colorImage.image, colorImage.imageMemory);
	}

	void recreateSwapChain() {
		createDepthResources();
		createColorResources();
	}

private:
	bufferStruct::showUITextNode showUIText{};
};