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
        std::array<VkClearValue, 2> clearValues{};
        VkRenderPassBeginInfo renderPassInfo{};
    };

    struct showUITextNode {
        glm::dvec3 pos;
        glm::dvec3 chunkPos;
        glm::ivec3 minepos{};
    };



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
        glm::mat4 base;             //模型矩阵，用于将模型变换到世界坐标系
        glm::mat4 view;             //视图矩阵，用于将世界坐标系变换到观察者坐标系
        glm::mat4 proj;             //投影矩阵，用于将观察者坐标系变换到裁剪坐标系
    };
    
    //注意着色器内存对齐
    struct lightUBO {
        glm::vec3 baseLight;        //基础光照颜色
        float ambientStrength;      //环境光强度

        glm::vec3 lightPos;         //光源位置
        float currentHour;          //当前小时，通过gameTime(游戏运行时长)%24得到

        glm::vec3 viewPos;          //观察者坐标
        float specularStrength;     //镜面强度
    };



    struct terrainBufferNode {
        //网格缓冲
        bufferNode vertex[bufferInfo::commandThreadSize];
        bufferNode index[bufferInfo::commandThreadSize];

        //统一缓冲
        bufferNode uniform;
        bufferNode lightUniform;
        baseUBO ubo;
        lightUBO lightUbo;

        //纹理缓冲
        bufferStruct::imageNode texture;
        const char* imageFile;
        uint32_t arrayLayer;
        uint32_t mipLevel = 1;

        //描述符缓冲
        VkDescriptorSet descriptorSet;

        terrainBufferNode(const char* file, uint32_t layer) {
            imageFile = file;
            arrayLayer = layer;
        }

        terrainBufferNode() = delete;

        //清理结构体，descriptorSet由描述符池管理，无需手动清理
        void clear(VkDevice& device, VmaAllocator& allocator) {
            for (int i = 0; i < bufferInfo::commandThreadSize; i++) {
                vmaDestroyBuffer(allocator, vertex[i].buffer, vertex[i].bufferMemory);
                vmaDestroyBuffer(allocator, index[i].buffer, index[i].bufferMemory);
            }

            vmaDestroyBuffer(allocator, uniform.buffer, uniform.bufferMemory);
            vmaDestroyBuffer(allocator, lightUniform.buffer, lightUniform.bufferMemory);

            vmaDestroyImage(allocator, texture.image, texture.imageMemory);
            vkDestroyImageView(device, texture.view, nullptr);
        }
    };

    struct modelBufferNode {
        bufferNode vertex;
        bufferNode index;

        bufferNode uniform;
        bufferNode lightUniform;
        baseUBO ubo;
        lightUBO lightUbo;

        bufferStruct::imageNode texture;
        const char* imageFile;
        uint32_t mipLevel;                  //在创建texture时赋值

        VkDescriptorSet descriptorSet;

        modelBufferNode(const char* file) {
            imageFile = file;
        }

        modelBufferNode() = delete;

        void clear(VkDevice& device, VmaAllocator& allocator) {
            vmaDestroyBuffer(allocator, vertex.buffer, vertex.bufferMemory);
            vmaDestroyBuffer(allocator, index.buffer, index.bufferMemory);

            vmaDestroyBuffer(allocator, uniform.buffer, uniform.bufferMemory);
            vmaDestroyBuffer(allocator, lightUniform.buffer, lightUniform.bufferMemory);

            vmaDestroyImage(allocator, texture.image, texture.imageMemory);
            vkDestroyImageView(device, texture.view, nullptr);
        }
    };

    struct skyboxBufferNode {
        bufferNode vertex;
        bufferNode index;

        bufferNode uniform;
        bufferNode lightUniform;
        baseUBO ubo;
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

    struct uiTextBufferNode {
        bufferNode vertex;

        bufferStruct::imageNode texture;
        stb_fontchar* stbFontData = new stb_fontchar[STB_FONT_consolas_24_latin1_NUM_CHARS];    //字符数据
        glm::vec4* mapped = nullptr;
        uint32_t numLetters;

        VkDescriptorSet descriptorSet;

        void beginTextUpdate(VmaAllocator allocator) {
            vmaMapMemory(allocator, vertex.bufferMemory, (void**)&mapped);
            numLetters = 0;
        }

        void addText(std::string texts, float x, float y, bufferEnum::TextAlign::Type align) {
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
            case bufferEnum::TextAlign::alignRight:
                x -= textWidth;
                break;
            case bufferEnum::TextAlign::alignCenter:
                x -= textWidth / 2.0f;
                break;
            case bufferEnum::TextAlign::alignLeft:
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