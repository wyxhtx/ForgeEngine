#pragma once



//SPIR-V 反射：直接解字节码，读出描述符绑定真值（set、binding、类型、数组长度、变量名、阶段）
//用途：校验手写描述符配置是否与着色器一致，后续也可由它直接生成布局
//取舍：只手解本引擎用得到的少量指令，不引外部库、不抛异常、随单 TU 编译
namespace spirvTool {
	//SPIR-V 字节码头共 5 字：魔数、版本、生成器、id 上限、schema
	const uint32_t magicNumber = 0x07230203;

	//SPIR-V 的 id 0 恒非法，故取全 1 当"未定义"哨兵
	const uint32_t invalidId = 0xFFFFFFFF;

	//图像维度：子通道数据即输入附件
	const uint32_t dimSubpassData = 6;

	//图像采样标记为 2 时是存储图像
	const uint32_t sampledStorage = 2;

	//SPIR-V 指令操作码（只列用到的）
	namespace opCode {
		const uint32_t name             = 5;     //OpName：取变量名，仅用于日志
		const uint32_t entryPoint       = 15;    //OpEntryPoint：取执行模型，校验文件与阶段是否对应
		const uint32_t typeImage        = 25;    //OpTypeImage：图像类型
		const uint32_t typeSampler      = 26;    //OpTypeSampler：采样器类型
		const uint32_t typeSampledImage = 27;    //OpTypeSampledImage：组合采样器类型
		const uint32_t typeArray        = 28;    //OpTypeArray：定长数组，取描述符数组长度
		const uint32_t typeRuntimeArray = 29;    //OpTypeRuntimeArray：运行期数组，长度按 1 处理
		const uint32_t typeStruct       = 30;    //OpTypeStruct：结构体，区分统一缓冲与存储缓冲
		const uint32_t typePointer      = 32;    //OpTypePointer：取存储类与指向类型
		const uint32_t constant         = 43;    //OpConstant：取数组长度常量
		const uint32_t variable         = 59;    //OpVariable：全局变量，即描述符本体
		const uint32_t decorate         = 71;    //OpDecorate：取 set、binding、Block
	}

	//SPIR-V 装饰编号（只列用到的）
	namespace decoration {
		const uint32_t block         = 2;    //Block：统一缓冲结构体
		const uint32_t bufferBlock   = 3;    //BufferBlock：存储缓冲结构体（旧写法）
		const uint32_t binding       = 33;   //Binding：绑定号
		const uint32_t descriptorSet = 34;   //DescriptorSet：描述符集号
	}

	//SPIR-V 存储类编号（只列用到的）
	namespace storageClass {
		const uint32_t uniformConstant = 0;    //采样器与图像
		const uint32_t uniform         = 2;    //统一缓冲、旧写法存储缓冲
		const uint32_t storageBuffer   = 12;   //存储缓冲
	}

	//SPIR-V 执行模型编号（只列本引擎用到的阶段）
	namespace executionModel {
		const uint32_t vertex   = 0;     //顶点
		const uint32_t fragment = 4;     //片段
		const uint32_t compute  = 5;     //计算
		const uint32_t task     = 5267;  //任务（网格管线）
		const uint32_t mesh     = 5268;  //网格（网格管线）
	}



	//反射出的单个描述符绑定
	struct reflectBindingNode {
		uint32_t              set{};       //描述符集号，GLSL 无 set 语法故恒为 0
		uint32_t              binding{};   //绑定号
		uint32_t              count{1};    //数组长度，非数组为 1
		VkDescriptorType      type{};      //描述符类型
		VkShaderStageFlagBits stage{};     //使用该绑定的阶段，由调用方按文件传入
		std::string           name{};      //变量名，仅用于日志
	};

	//一条待反射的着色器：文件加阶段
	struct reflectShaderNode {
		std::string           file{};      //spv 文件路径
		VkShaderStageFlagBits stage{};     //该文件的着色器阶段
	};



	//执行模型 → 着色器阶段，未知模型返回 0
	VkShaderStageFlagBits getStage(uint32_t model) {
		if (model == executionModel::vertex)   { return VK_SHADER_STAGE_VERTEX_BIT; }
		if (model == executionModel::fragment) { return VK_SHADER_STAGE_FRAGMENT_BIT; }
		if (model == executionModel::compute)  { return VK_SHADER_STAGE_COMPUTE_BIT; }
		if (model == executionModel::task)     { return VK_SHADER_STAGE_TASK_BIT_NV; }
		if (model == executionModel::mesh)     { return VK_SHADER_STAGE_MESH_BIT_NV; }

		return static_cast<VkShaderStageFlagBits>(0);
	}

	//反射一个 spv 文件，返回描述符绑定表（按 binding 升序，无描述符返回空表）
	//三趟扫指令流：先收装饰名字常量，再收类型与变量，最后按变量分类
	std::vector<reflectBindingNode> reflectFile(const std::string& filename, VkShaderStageFlagBits stage) {
		//读字节码
		std::ifstream fileData(filename, std::ios::ate | std::ios::binary);
		if (!fileData.is_open()) {
			Log::printError(__FILE__, __LINE__, filename + " 反射失败：着色器文件打开失败");
		}

		size_t fileSize = static_cast<size_t>(fileData.tellg());
		std::vector<char> byteList(fileSize);
		fileData.seekg(0);
		fileData.read(byteList.data(), fileSize);
		fileData.close();

		//按 SPIR-V 字流解释：头 5 字 + 指令流，每字 4 字节
		if (fileSize < 20 || fileSize % 4 != 0) {
			Log::printError(__FILE__, __LINE__, filename + " 反射失败：字节码长度非法");
		}

		std::vector<uint32_t> code(fileSize / 4);
		std::memcpy(code.data(), byteList.data(), fileSize);
		if (code[0] != magicNumber) {
			Log::printError(__FILE__, __LINE__, filename + " 反射失败：不是 SPIR-V 字节码");
		}

		//id 取值 1 到 bound-1，故开 bound 长的表，槽位留着哨兵表示未定义
		uint32_t bound = code[3];

		std::vector<uint32_t>    bindingList(bound, invalidId);       //Binding 装饰
		std::vector<uint32_t>    setList(bound, invalidId);           //DescriptorSet 装饰
		std::vector<uint32_t>    blockList(bound, invalidId);         //Block 或 BufferBlock 装饰
		std::vector<uint32_t>    elementList(bound, invalidId);       //数组元素类型
		std::vector<uint32_t>    lengthList(bound, invalidId);        //数组长度常量 id
		std::vector<uint32_t>    valueList(bound, invalidId);         //常量值
		std::vector<uint32_t>    pointeeList(bound, invalidId);       //指针指向的类型
		std::vector<uint32_t>    imageDimList(bound, invalidId);      //图像维度
		std::vector<uint32_t>    imageSampledList(bound, invalidId);  //图像采样标记
		std::vector<uint32_t>    sampledImageList(bound, invalidId);  //组合采样器包着的图像
		std::vector<uint32_t>    samplerList(bound, invalidId);       //采样器标记
		std::vector<uint32_t>    structList(bound, invalidId);        //结构体标记
		std::vector<uint32_t>    varTypeList(bound, invalidId);       //变量的指针类型
		std::vector<uint32_t>    varStorageList(bound, invalidId);    //变量的存储类
		std::vector<std::string> nameList(bound);                     //id 名字

		uint32_t model = invalidId;                                   //入口点执行模型

		//指令流每字：高 16 位是字长，低 16 位是操作码
		for (size_t offset = 5; offset < code.size(); ) {
			uint32_t wordCount = code[offset] >> 16;
			uint32_t opcode    = code[offset] & 0xFFFF;

			if (wordCount == 0 || offset + wordCount > code.size()) {
				Log::printError(__FILE__, __LINE__, filename + " 反射失败：指令越界");
			}

			//第一趟只收装饰、名字、常量、入口点，它们都在指令流前段，供后面两趟查表
			if (opcode == opCode::decorate && wordCount >= 3) {
				uint32_t target = code[offset + 1];
				uint32_t kind   = code[offset + 2];

				if (target < bound) {
					if (kind == decoration::block || kind == decoration::bufferBlock) { blockList[target] = kind; }
					else if (kind == decoration::binding && wordCount >= 4)           { bindingList[target] = code[offset + 3]; }
					else if (kind == decoration::descriptorSet && wordCount >= 4)     { setList[target] = code[offset + 3]; }
				}
			}
			else if (opcode == opCode::name && wordCount >= 3) {
				uint32_t target = code[offset + 1];

				if (target < bound) {
					const char* text = reinterpret_cast<const char*>(&code[offset + 2]);
					nameList[target] = std::string(text, strnlen(text, (wordCount - 2) * 4));
				}
			}
			else if (opcode == opCode::constant && wordCount >= 4) {
				uint32_t target = code[offset + 2];

				if (target < bound) {
					valueList[target] = code[offset + 3];
				}
			}
			else if (opcode == opCode::entryPoint && wordCount >= 3) {
				model = code[offset + 1];
			}

			offset += wordCount;
		}

		//第二趟收类型与变量
		for (size_t offset = 5; offset < code.size(); ) {
			uint32_t wordCount = code[offset] >> 16;
			uint32_t opcode    = code[offset] & 0xFFFF;

			if (opcode == opCode::typePointer && wordCount >= 4) {
				uint32_t target = code[offset + 1];

				if (target < bound) {
					pointeeList[target] = code[offset + 3];
				}
			}
			else if (opcode == opCode::typeArray && wordCount >= 4) {
				uint32_t target = code[offset + 1];

				if (target < bound) {
					elementList[target] = code[offset + 2];
					lengthList[target]  = code[offset + 3];
				}
			}
			else if (opcode == opCode::typeRuntimeArray && wordCount >= 3) {
				uint32_t target = code[offset + 1];

				if (target < bound) {
					elementList[target] = code[offset + 2];
				}
			}
			else if (opcode == opCode::typeSampledImage && wordCount >= 3) {
				uint32_t target = code[offset + 1];

				if (target < bound) {
					sampledImageList[target] = code[offset + 2];
				}
			}
			else if (opcode == opCode::typeImage && wordCount >= 8) {
				uint32_t target = code[offset + 1];

				if (target < bound) {
					imageDimList[target]     = code[offset + 3];
					imageSampledList[target] = code[offset + 7];
				}
			}
			else if (opcode == opCode::typeSampler && wordCount >= 2) {
				uint32_t target = code[offset + 1];

				if (target < bound) {
					samplerList[target] = target;
				}
			}
			else if (opcode == opCode::typeStruct && wordCount >= 2) {
				uint32_t target = code[offset + 1];

				if (target < bound) {
					structList[target] = target;
				}
			}
			else if (opcode == opCode::variable && wordCount >= 4) {
				uint32_t varId = code[offset + 2];

				if (varId < bound) {
					varTypeList[varId]    = code[offset + 1];
					varStorageList[varId] = code[offset + 3];
				}
			}

			offset += wordCount;
		}

		//第三趟按变量分类出描述符
		std::vector<reflectBindingNode> result{};

		for (uint32_t id = 1; id < bound; id++) {
			//只看描述符存储类且带 binding 装饰的全局变量，push constant 无 binding 天然排除
			if (varTypeList[id] == invalidId || bindingList[id] == invalidId) { continue; }
			if (varStorageList[id] != storageClass::uniformConstant && varStorageList[id] != storageClass::uniform && varStorageList[id] != storageClass::storageBuffer) { continue; }

			//剥掉数组，拿到元素类型与数组长度
			uint32_t elementId = pointeeList[varTypeList[id]];
			uint32_t count     = 1;

			if (elementId != invalidId && elementList[elementId] != invalidId) {
				uint32_t lengthId = lengthList[elementId];
				count     = (lengthId != invalidId && valueList[lengthId] != invalidId) ? valueList[lengthId] : 1;
				elementId = elementList[elementId];
			}

			if (elementId == invalidId) {
				Log::printError(__FILE__, __LINE__, filename + " 反射失败：描述符类型解析不出：" + nameList[id]);
			}

			//按元素类型分类
			bool isKnown = false;
			VkDescriptorType type{};

			if (sampledImageList[elementId] != invalidId) {
				type    = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				isKnown = true;
			}
			else if (samplerList[elementId] != invalidId) {
				type    = VK_DESCRIPTOR_TYPE_SAMPLER;
				isKnown = true;
			}
			else if (imageDimList[elementId] != invalidId) {
				if (imageDimList[elementId] == dimSubpassData)          { type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT; }
				else if (imageSampledList[elementId] == sampledStorage) { type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE; }
				else                                                    { type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE; }
				isKnown = true;
			}
			else if (structList[elementId] != invalidId) {
				if (varStorageList[id] == storageClass::storageBuffer || blockList[elementId] == decoration::bufferBlock) { type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; }
				else                                                                                                     { type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; }
				isKnown = true;
			}

			if (!isKnown) {
				Log::printError(__FILE__, __LINE__, filename + " 反射失败：描述符类型不认识：" + nameList[id]);
			}

			result.push_back(reflectBindingNode{
				.set     = (setList[id] == invalidId) ? 0u : setList[id],
				.binding = bindingList[id],
				.count   = count,
				.type    = type,
				.stage   = stage,
				.name    = nameList[id],
			});
		}

		//入口点执行模型与传入阶段必须对应，防"传错文件"
		VkShaderStageFlagBits modelStage = getStage(model);

		if (stage != 0 && modelStage != 0 && modelStage != stage) {
			Log::printError(__FILE__, __LINE__, filename + " 反射失败：文件阶段与传入阶段不符");
		}

		//按绑定号升序
		std::sort(result.begin(), result.end(), [](const reflectBindingNode& left, const reflectBindingNode& right) { return left.binding < right.binding; });

		return result;
	}
}