#pragma once



/*外部库文件（带 *_IMPLEMENTATION 实现宏：只能被唯一 TU include，不能进 External.h）*/
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

//模型库
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION  
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tiny_gltf.h>



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