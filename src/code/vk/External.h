#pragma once



/*编译配置*/
//避免Debug下并行化奇卡无比
#ifdef _DEBUG
#define _ITERATOR_DEBUG_LEVEL 1	//STL基本检查													
#define _HAS_ITERATOR_DEBUGGING 0
#endif							//不收集STL调试信息												



/*系统库文件*/
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <execution>
#include <filesystem>
#include <format>
#include <forward_list>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <mutex>
#include <numeric>
#include <queue>
#include <set>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
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

//纹理库
#include <KHR/khr_df.h>
#include <ktx.h>
#include <ktxvulkan.h>

//MSDF	文本库（freetype 读轮廓 → msdfgen 距离场 → msdf-atlas-gen 图集）
#include <msdfgen.h>
#include <msdfgen-ext.h>
#include <msdf-atlas-gen/msdf-atlas-gen.h>

//音频库
#include <AL/al.h>
#include <AL/alc.h>
#include <alut.h>