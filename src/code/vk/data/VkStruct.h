#pragma once



namespace deviceStruct {
    struct queueIndice {
        //四大操作支持索引
        int support = -1;

        //呈现索引
        int present = -1;

        //存储显卡使用队列数量
        int count = -1;
    };

    struct deviceNode {
        //显卡
        std::vector<VkPhysicalDevice> physicalDevices;

        //队列
        std::vector<deviceStruct::queueIndice> queueIndices;
    };
}

namespace swapStruct {
    struct swapchainSupportNode {
        VkSurfaceCapabilitiesKHR capabilities{};
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };
}

namespace bufferStruct {
    struct descriptorNode {
        VkDescriptorPool pool;
        VkDescriptorSetLayout layout;       //在图形管线中绑定资源（如统一缓冲区、图像、采样器等）到着色器中使用
    };

    struct commandNode {
        VkCommandPool commandPool;

        std::vector<VkCommandBuffer> commandBuffers[bufferInfo::commandThreadSize];     //使用双重命令缓冲防止更新区块卡顿

        int currentThreadID = 0;						                                //当前地形线程命令缓冲区id
        uint32_t processingThreadSize = 1;                                              //背后线程数量,避免上个线程没结束就开启新线程

        VkCommandBufferAllocateInfo allocInfo{};
        VkCommandBufferBeginInfo beginInfo{};
    };

    struct showUITextNode {
        glm::dvec3 pos;
        glm::dvec3 chunkPos;
        glm::ivec3 minepos{};
    };



    /*缓冲结点*/
    struct imageNode {
        VkImage image;
        VmaAllocation imageMemory;
        VkImageView view;
    };

    struct bufferNode {
        VkBuffer buffer;
        VmaAllocation bufferMemory;
    };

    struct baseUBO {
        glm::mat4 model;            //模型矩阵，用于将模型变换到世界坐标系
        glm::mat4 modelTI;          //模型转置逆矩阵，用于计算光照法线
        glm::mat4 view;             //视图矩阵，用于将世界坐标系变换到观察者坐标系
        glm::mat4 proj;             //投影矩阵，用于将观察者坐标系变换到裁剪坐标系
    };

    //注意着色器内存对齐
    struct lightUBO {
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



    /*管线缓冲区*/
    struct terrainBufferNode {
        //网格缓冲
        bufferNode vertex[bufferInfo::commandThreadSize][terrainEnum::terrainLayer::count];
        bufferNode index[bufferInfo::commandThreadSize][terrainEnum::terrainLayer::count];

        //纹理贴图缓冲
        const char* textureFile;
        imageNode texture;

        //高光贴图缓冲
        const char* specularFile;
        imageNode specular;

        //纹理参数
        int samplerId;
        uint32_t arrayLayer;
        uint32_t mipLevel = 1;

        //统一缓冲
        bufferNode uniform;
        baseUBO ubo;
        bufferNode lightUniform;
        lightUBO lightUbo;

        //描述符缓冲
        VkDescriptorSet descriptorSet;



        terrainBufferNode(int tempSamplerId) {
            textureFile = fileInfo::image::cube;
            specularFile = fileInfo::image::cubeSpecular;
            arrayLayer = textureInfo::arrayLayer;

            samplerId = tempSamplerId;
        }

        terrainBufferNode() = delete;

        //清理结构体，descriptorSet由描述符池管理，无需手动清理
        void clear(VkDevice& device, VmaAllocator& allocator) {
            for (int thread = 0; thread < bufferInfo::commandThreadSize; thread++) {
                for (int layer = 0; layer < terrainEnum::terrainLayer::count; layer++) {
                    vmaDestroyBuffer(allocator, vertex[thread][layer].buffer, vertex[thread][layer].bufferMemory);
                    vmaDestroyBuffer(allocator, index[thread][layer].buffer, index[thread][layer].bufferMemory);
                }
            }

            vmaDestroyImage(allocator, texture.image, texture.imageMemory);
            vkDestroyImageView(device, texture.view, nullptr);

            vmaDestroyImage(allocator, specular.image, specular.imageMemory);
            vkDestroyImageView(device, specular.view, nullptr);

            vmaDestroyBuffer(allocator, uniform.buffer, uniform.bufferMemory);
            vmaDestroyBuffer(allocator, lightUniform.buffer, lightUniform.bufferMemory);
        }
    };

    struct staticModelNode {
        std::string modelFile;
        bufferNode vertex;
        bufferNode index;

        const char* textureFile;
        imageNode texture;

        int samplerId;
        uint32_t mipLevel;                  //在创建texture时赋值

        bufferNode uniform;
        baseUBO ubo;
        bufferNode lightUniform;
        lightUBO lightUbo;

        VkDescriptorSet descriptorSet;



        staticModelNode(const std::string tempModelFile, const char* tempTextureFile, int tempSamplerId) {
            modelFile = tempModelFile;
            textureFile = tempTextureFile;
            samplerId = tempSamplerId;
        }

        staticModelNode() {}

        void clear(VkDevice& device, VmaAllocator& allocator) {
            vmaDestroyBuffer(allocator, vertex.buffer, vertex.bufferMemory);
            vmaDestroyBuffer(allocator, index.buffer, index.bufferMemory);

            vmaDestroyImage(allocator, texture.image, texture.imageMemory);
            vkDestroyImageView(device, texture.view, nullptr);

            vmaDestroyBuffer(allocator, uniform.buffer, uniform.bufferMemory);
            vmaDestroyBuffer(allocator, lightUniform.buffer, lightUniform.bufferMemory);
        }
    };

    struct dynamicModelListNode {
        bufferNode vertex;
        bufferNode index;

        const char* textureFile;
        imageNode texture;

        bufferNode uniform;
        baseUBO ubo;

        VkDescriptorSet descriptorSet;



        void clear(VkDevice& device, VmaAllocator& allocator) {
            vmaDestroyBuffer(allocator, vertex.buffer, vertex.bufferMemory);
            vmaDestroyBuffer(allocator, index.buffer, index.bufferMemory);

            vmaDestroyImage(allocator, texture.image, texture.imageMemory);
            vkDestroyImageView(device, texture.view, nullptr);

            vmaDestroyBuffer(allocator, uniform.buffer, uniform.bufferMemory);
        }
    };

    struct dynamicModelNode {
        std::string modelFile;
        std::vector<dynamicModelListNode> node;

        bufferNode lightUniform;
        lightUBO lightUbo;

        int samplerId;
        uint32_t mipLevel;



        dynamicModelNode(const std::string tempModelFile, std::vector<const char*> tempTextureFile, int tempSamplerId) {
            modelFile = tempModelFile;

            node.resize(tempTextureFile.size());
            for (int i = 0; i < tempTextureFile.size(); i++) {
                node[i].textureFile = tempTextureFile[i];
            }

            samplerId = tempSamplerId;
        }

        dynamicModelNode() {}

        void clear(VkDevice& device, VmaAllocator& allocator) {
            for (int i = 0; i < node.size(); i++) {
                node[i].clear(device, allocator);
            }

            vmaDestroyBuffer(allocator, lightUniform.buffer, lightUniform.bufferMemory);
        }
    };

    struct skyboxBufferNode {
        bufferNode vertex;
        bufferNode index;

        bufferNode uniform;
        baseUBO ubo;
        bufferNode lightUniform;
        lightUBO lightUbo;

        VkDescriptorSet descriptorSet;



        void clear(VkDevice& device, VmaAllocator& allocator) {
            vmaDestroyBuffer(allocator, vertex.buffer, vertex.bufferMemory);
            vmaDestroyBuffer(allocator, index.buffer, index.bufferMemory);

            vmaDestroyBuffer(allocator, uniform.buffer, uniform.bufferMemory);
            vmaDestroyBuffer(allocator, lightUniform.buffer, lightUniform.bufferMemory);
        }
    };

    struct baseBufferNode {
        bufferNode vertex;
        bufferNode index;

        bufferNode uniform;
        baseUBO ubo;

        VkDescriptorSet descriptorSet;



        void clear(VkDevice& device, VmaAllocator& allocator) {
            vmaDestroyBuffer(allocator, vertex.buffer, vertex.bufferMemory);
            vmaDestroyBuffer(allocator, index.buffer, index.bufferMemory);

            vmaDestroyBuffer(allocator, uniform.buffer, uniform.bufferMemory);
        }
    };

    struct offScreenBufferNode {
        bufferNode vertex;
        bufferNode index;

        uint32_t width = bufferInfo::osSize;    //宽
        uint32_t height = bufferInfo::osSize;   //高

        bufferStruct::imageNode color;          //颜色附件
        bufferStruct::imageNode depth;          //深度附件
        bufferStruct::imageNode osView;         //离屏视图
        VkFramebuffer osFramebuffer;		    //离屏帧缓冲

        int samplerId;                          //采样器

        bufferNode uniform;                     //统一缓冲
        baseUBO ubo;
        bufferNode lightUniform;
        lightUBO lightUbo;

        VkDescriptorSet descriptorSet;          //描述符



        offScreenBufferNode(int tempSamplerId) {
            samplerId = tempSamplerId;
        }

        offScreenBufferNode() = delete;

        void clear(VkDevice& device, VmaAllocator& allocator) {
            vmaDestroyBuffer(allocator, vertex.buffer, vertex.bufferMemory);
            vmaDestroyBuffer(allocator, index.buffer, index.bufferMemory);

            vkDestroyImageView(device, color.view, nullptr);
            vmaDestroyImage(allocator, color.image, color.imageMemory);

            vkDestroyImageView(device, depth.view, nullptr);
            vmaDestroyImage(allocator, depth.image, depth.imageMemory);

            vkDestroyImageView(device, osView.view, nullptr);
            vmaDestroyImage(allocator, osView.image, osView.imageMemory);

            vkDestroyFramebuffer(device, osFramebuffer, nullptr);

            vmaDestroyBuffer(allocator, uniform.buffer, uniform.bufferMemory);
            vmaDestroyBuffer(allocator, lightUniform.buffer, lightUniform.bufferMemory);
        }
    };

    struct uiTextBufferNode {
        bufferNode vertex;

        imageNode texture;
        stb_fontchar* stbFontData = new stb_fontchar[STB_FONT_consolas_24_latin1_NUM_CHARS];    //字符数据
        glm::vec4* mapped = nullptr;
        uint32_t numLetters;
        int samplerId;

        VkDescriptorSet descriptorSet;



        uiTextBufferNode(int tempSamplerId) {
            samplerId = tempSamplerId;
        }

        uiTextBufferNode() = delete;

        void beginTextUpdate(VmaAllocator allocator) {
            vmaMapMemory(allocator, vertex.bufferMemory, (void**)&mapped);
            numLetters = 0;
        }

        void addText(std::string texts, float x, float y, bufferEnum::textAlign::Type align) {
            // 字符的起始索引
            const uint32_t firstChar = STB_FONT_consolas_24_latin1_FIRST_CHAR;

            // 以窗口大小为单位，计算字符宽度和高度
            const float charW = 1.5f * 1.0f / windowInfo::width;
            const float charH = 1.5f * 1.0f / windowInfo::height;

            // 窗口的宽度和高度
            x = (x / windowInfo::width * 2.0f) - 1.0f;
            y = (y / windowInfo::height * 2.0f) - 1.0f;

            // 计算文本宽度
            float textWidth = 0;
            for (char letter : texts) {
                stb_fontchar* charData = &stbFontData[(uint32_t)letter - firstChar];
                textWidth += charData->advance * charW;
            }

            // 根据对齐方式调整x坐标
            switch (align) {
            case bufferEnum::textAlign::alignRight:
                x -= textWidth;
                break;
            case bufferEnum::textAlign::alignCenter:
                x -= textWidth / 2.0f;
                break;
            case bufferEnum::textAlign::alignLeft:
                break;
            }

            // 为每个字符生成uv映射的四边形
            for (char letter : texts) {
                // 创建一个指向stb_fontchar类型数据的指针
                stb_fontchar* charData = &stbFontData[(uint32_t)letter - firstChar];

                // 设置四边形的uv坐标和位置
                mapped->x = (x + (float)charData->x0 * charW);
                mapped->y = (y + (float)charData->y0 * charH);
                mapped->z = charData->s0;
                mapped->w = charData->t0;
                mapped++;

                mapped->x = (x + (float)charData->x1 * charW);
                mapped->y = (y + (float)charData->y0 * charH);
                mapped->z = charData->s1;
                mapped->w = charData->t0;
                mapped++;

                mapped->x = (x + (float)charData->x0 * charW);
                mapped->y = (y + (float)charData->y1 * charH);
                mapped->z = charData->s0;
                mapped->w = charData->t1;
                mapped++;

                mapped->x = (x + (float)charData->x1 * charW);
                mapped->y = (y + (float)charData->y1 * charH);
                mapped->z = charData->s1;
                mapped->w = charData->t1;
                mapped++;

                x += charData->advance * charW;

                numLetters++;
            }
        }

        void endTextUpdate(VmaAllocator allocator) {
            vmaUnmapMemory(allocator, vertex.bufferMemory);
            mapped = nullptr;
        }

        void clear(VkDevice& device, VmaAllocator& allocator) {
            vmaDestroyBuffer(allocator, vertex.buffer, vertex.bufferMemory);

            vmaDestroyImage(allocator, texture.image, texture.imageMemory);
            vkDestroyImageView(device, texture.view, nullptr);
        }
    };



    /*创建流程优化结点*/
    //createDescriptorPool
    struct descriptorPoolNode {
        VkDescriptorPoolSize node{};

        descriptorPoolNode(VkDescriptorType type, uint32_t descriptorCount) {
            node.type            = type;
            node.descriptorCount = descriptorCount;
        }

        descriptorPoolNode() = delete;
    };

    //createDescriptorSetLayout
    struct descriptorLayoutNode {
        VkDescriptorSetLayoutBinding node{};

        descriptorLayoutNode(uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags) {
            node.descriptorCount    = 1;
            node.pImmutableSamplers = nullptr;

            node.binding            = binding;
            node.descriptorType     = descriptorType;
            node.stageFlags         = stageFlags;
        }

        descriptorLayoutNode() = delete;
    };

    //createDescriptorSet
    struct descriptorBufferNode {
        VkDescriptorBufferInfo node{};

        descriptorBufferNode() {
            node.offset = 0;
        }

        VkDescriptorBufferInfo* getNode(VkBuffer buffer, VkDeviceSize range) {
            node.buffer = buffer;
            node.range  = range;

            return &node;
        }
    };

    struct descriptorImageNode {
        VkDescriptorImageInfo node{};

        descriptorImageNode() {
            node.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }

        VkDescriptorImageInfo* getNode(VkSampler sampler, VkImageView imageView) {
            node.sampler   = sampler;
            node.imageView = imageView;

            return &node;
        }
    };

    struct descriptorAllocateNode {
        VkDescriptorSetAllocateInfo node{};

        descriptorAllocateNode() {
            node.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            node.pNext              = nullptr;
            node.descriptorSetCount = 1;
        }

        void setNode(VkDescriptorPool descriptorPool, const VkDescriptorSetLayout* pSetLayouts) {
            node.descriptorPool = descriptorPool;
            node.pSetLayouts    = pSetLayouts;
        }
    };

    struct writeDescriptorNode {
        VkWriteDescriptorSet node{};        //一定要初始化，不然报错

        writeDescriptorNode(uint32_t dstBinding, VkDescriptorType descriptorType) {
            node.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            node.dstArrayElement = 0;
            node.descriptorCount = 1;

            node.dstBinding      = dstBinding;
            node.descriptorType  = descriptorType;
        }

        writeDescriptorNode() = delete;

        void setBufferNode(VkDescriptorSet dstSet, const VkDescriptorBufferInfo* pBufferInfo) {
            node.dstSet      = dstSet;
            node.pBufferInfo = pBufferInfo;

            node.pImageInfo  = nullptr;      //清理pImageInfo
        }

        void setImageNode(VkDescriptorSet dstSet, const VkDescriptorImageInfo* pImageInfo) {
            node.dstSet      = dstSet;
            node.pImageInfo  = pImageInfo;

            node.pBufferInfo = nullptr;     //清理pBufferInfo
        }
    };
}

namespace pipelineStruct {
    struct pipelineNode {
        VkPipelineLayout& pipelineLayout;
        VkPipeline& pipeline;

        pipelineNode(VkPipelineLayout& layout, VkPipeline& pipe) :pipelineLayout(layout), pipeline(pipe) {}

        pipelineNode() = delete;    //禁用默认构造函数
    };
}

namespace fpsStruct {
    struct tick {
        double time0 = glfwGetTime();
        double time1 = 0.0;
        double dt = 0.0;

        //是否更新
        bool isUpdate(double fps) {
            time1 = glfwGetTime();

            if ((dt = time1 - time0) > 1.0 / fps) {
                time0 = time1;

                return true;
            }

            return false;
        }
    };
}

namespace timeStruct {
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