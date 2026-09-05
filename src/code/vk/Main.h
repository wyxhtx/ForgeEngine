#pragma once



/*编译配置*/
//避免Debug下并行化奇卡无比
#ifdef _DEBUG
#define _ITERATOR_DEBUG_LEVEL 1		//STL基本检查
#define _HAS_ITERATOR_DEBUGGING 0
#endif	//不收集STL调试信息



/*系统库文件*/
#include <array>
#include <cmath>
#include <execution>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <set>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>
#include <windows.h>



/*外部库文件*/
//glfw框架库，其内自带vulkan头文件
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

//glm数学库
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

//算法库
#include <kmeans/kmeans.hpp>	//聚类算法，划分区块所属区域，让域内区块聚合，减少域外链结构

//噪音库
#include <FastNoise/FastNoise.h>
#include <FastNoise/Metadata.h>

//内存库
#define VMA_IMPLEMENTATION
#define VMA_DEBUG_INITIALIZE_ALLOCATIONS 1
#define VMA_DETECT_CORRUPTION 1
#pragma warning(push)
#pragma warning(disable : 6386)
#pragma warning(disable : 6387)
#pragma warning(disable : 26110)
#pragma warning(disable : 26495)
#pragma warning(disable : 26819)
#include <vk_mem_alloc.h>
#pragma warning(pop)

//纹理库
#include <KHR/khr_df.h>
#include <ktx.h>
#include <ktxvulkan.h>

//模型库
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION  
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tiny_gltf.h>

//文本库
#pragma warning(push)
#pragma warning(disable : 26495)
#include <stb/stb_font_consolas_24_latin1.inl>

//MSDF 文本库（freetype 读轮廓 → msdfgen 距离场 → msdf-atlas-gen 图集）
#include <msdfgen.h>
#include <msdfgen-ext.h>
#include <msdf-atlas-gen/msdf-atlas-gen.h>
#pragma warning(pop)

//音频库
#include <AL/al.h>
#include <AL/alc.h>
#include <alut.h>



/*内部库文件*/
#include "tool/Log.hpp"

#include "../forge/data/ForgeEnum.h"
#include "../forge/data/ForgeInfo.h"
#pragma warning(push)
#pragma warning(disable: 26495)
#include "../forge/data/ForgeStruct.h"
#pragma warning(pop)
#include "../forge/data/ForgeList.h"

#include "data/VkEnum.h"
#include "data/VkInfo.h"
#include "data/VkVertex.h"
#pragma warning(push)
#pragma warning(disable: 26495)
#include "data/VkStruct.h"
#pragma warning(pop)
#include "data/VkList.h"

#include "tool/Camera.hpp"
#include "tool/Music.hpp"



#include "../forge/base/Loop.hpp"
#include "../forge/world/Chunk.hpp"
#include "../forge/world/Storage.hpp"
#include "../forge/base/Debug.hpp"
#include "../forge/world/Plate.hpp"

#include "base/Window.hpp"
#pragma warning(push)
#pragma warning(disable : 6237)
#include "base/Instance.hpp"
#pragma warning(pop)
#include "base/Device.hpp"

#include "graphic/Swapchain.hpp"
#include "graphic/Buffer.hpp"
#include "graphic/Pipeline.hpp"