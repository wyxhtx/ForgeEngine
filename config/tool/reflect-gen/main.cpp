//===========================================================================
// reflect-gen —— 构建期反射工具：.spv → ShaderReflectData.h（类型 ＋ 数据）
//
// 位置：config\tool\reflect-gen（自带 main、不进引擎 exe）
// 解析内核只此一份（引擎侧不再有解析器）：引擎只 include 生成头，查表使用
// 依赖：STL ＋ <vulkan/vulkan_core.h> ＋ "data/VkEnum.h" ＋ "data/VkInfo.h"（取 spirvInfo 常量）
//===========================================================================
#include <array>
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <cstdlib>
#include <deque>
#include <fstream>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include <vulkan/vulkan_core.h>
#include <glm/glm.hpp>

#include "data/VkEnum.h"
#include "data/VkInfo.h"



namespace reflectGen {
    using ByteList = std::vector<unsigned char>;

    struct BindingRecord {
        uint32_t           set{};
        uint32_t           binding{};
        uint32_t           count{ 1 };
        VkDescriptorType   type{};
        VkShaderStageFlags stage{};
        std::string        name{};
    };

    struct InputRecord {
        uint32_t              location{};
        VkFormat              format{};
        VkShaderStageFlagBits stage{};
        std::string           name{};
    };

    struct Module {
        std::string              file{};
        std::string              sourcePath{};
        std::string              tag{};
        uint64_t                 hash{};
        uint64_t                 byteCount{};
        VkShaderStageFlagBits    stage{};
        std::vector<BindingRecord> bindingList{};
        std::vector<InputRecord>   inputList{};
    };



    //基础
    uint64_t hashBytes(const ByteList& bytes) {
        uint64_t hash = 1469598103934665603ull;

        for (const unsigned char byte : bytes) { hash = (hash ^ byte) * 1099511628211ull; }

        return hash;
    }

    uint32_t readWord(const ByteList& bytes, size_t index) {
        const size_t offset = index * 4;

        return static_cast<uint32_t>(bytes[offset]) | (static_cast<uint32_t>(bytes[offset + 1]) << 8)
             | (static_cast<uint32_t>(bytes[offset + 2]) << 16) | (static_cast<uint32_t>(bytes[offset + 3]) << 24);
    }

    VkShaderStageFlagBits getStage(uint32_t model) {
        if (model == spirvInfo::executionModel::vertex)   { return VK_SHADER_STAGE_VERTEX_BIT; }
        if (model == spirvInfo::executionModel::fragment) { return VK_SHADER_STAGE_FRAGMENT_BIT; }
        if (model == spirvInfo::executionModel::compute)  { return VK_SHADER_STAGE_COMPUTE_BIT; }
        if (model == spirvInfo::executionModel::task)     { return VK_SHADER_STAGE_TASK_BIT_NV; }
        if (model == spirvInfo::executionModel::mesh)     { return VK_SHADER_STAGE_MESH_BIT_NV; }

        return static_cast<VkShaderStageFlagBits>(0);
    }

    bool readFile(const std::string& path, ByteList& bytes) {
        std::ifstream fileData(path, std::ios::ate | std::ios::binary);
        if (!fileData.is_open()) { return false; }

        const size_t fileSize = static_cast<size_t>(fileData.tellg());

        bytes.resize(fileSize);
        fileData.seekg(0);
        fileData.read(reinterpret_cast<char*>(bytes.data()), fileSize);
        fileData.close();

        return true;
    }

    std::string keyOf(const std::string& file);		//定义在生成段（此处前置声明）



    //解析（字节 → 阶段 / 绑定表 / 顶点输入）
    bool parseShader(const ByteList& bytes, const std::string& file, Module& module, std::string& error) {
        module.file       = keyOf(file);
        module.sourcePath = file;
        module.hash      = hashBytes(bytes);
        module.byteCount = bytes.size();

        //按 SPIR-V 字流解释：头 5 字 + 指令流，每字 4 字节
        if (bytes.size() < 20 || bytes.size() % 4 != 0) { error = file + " reflect failed: illegal byte size"; return false; }

        const uint32_t wordTotal = static_cast<uint32_t>(bytes.size() / 4);
        if (readWord(bytes, 0) != spirvInfo::magicNumber) { error = file + " reflect failed: not SPIR-V"; return false; }

        const uint32_t bound = readWord(bytes, 3);

        std::vector<uint32_t>    bindingList(bound, spirvInfo::invalidId);       //Binding 装饰
        std::vector<uint32_t>    setList(bound, spirvInfo::invalidId);           //DescriptorSet 装饰
        std::vector<uint32_t>    blockList(bound, spirvInfo::invalidId);         //Block / BufferBlock 装饰
        std::vector<uint32_t>    locationList(bound, spirvInfo::invalidId);      //Location 装饰
        std::vector<uint32_t>    elementList(bound, spirvInfo::invalidId);       //数组元素类型
        std::vector<uint32_t>    lengthList(bound, spirvInfo::invalidId);        //数组长度常量 id
        std::vector<uint32_t>    valueList(bound, spirvInfo::invalidId);         //常量值
        std::vector<uint32_t>    pointeeList(bound, spirvInfo::invalidId);       //指针指向的类型
        std::vector<uint32_t>    vectorCountList(bound, spirvInfo::invalidId);   //向量分量数
        std::vector<uint32_t>    vectorCompList(bound, spirvInfo::invalidId);    //向量分量类型 id
        std::vector<uint32_t>    floatList(bound, spirvInfo::invalidId);         //浮点类型标记
        std::vector<uint32_t>    imageDimList(bound, spirvInfo::invalidId);      //图像维度
        std::vector<uint32_t>    imageSampledList(bound, spirvInfo::invalidId);  //图像采样标记
        std::vector<uint32_t>    sampledImageList(bound, spirvInfo::invalidId);  //组合采样器包着的图像
        std::vector<uint32_t>    samplerList(bound, spirvInfo::invalidId);       //采样器类型标记
        std::vector<uint32_t>    structList(bound, spirvInfo::invalidId);        //结构体类型标记
        std::vector<uint32_t>    varTypeList(bound, spirvInfo::invalidId);       //变量的指针类型
        std::vector<uint32_t>    varStorageList(bound, spirvInfo::invalidId);    //变量的存储类
        std::deque<std::string>  namePool{};                                     //id 名字（deque：引用稳定）
        std::vector<uint32_t>    nameIndexList(bound, spirvInfo::invalidId);     //id → namePool 下标

        //一趟扫描：装饰 / 名字 / 常量 / 类型 / 变量 / 入口点
        for (size_t offset = 5; offset < wordTotal; ) {
            const uint32_t wordCount = readWord(bytes, offset) >> 16;
            const uint32_t opcode    = readWord(bytes, offset) & 0xFFFF;

            if (wordCount == 0 || offset + wordCount > wordTotal) { error = file + " reflect failed: instruction out of range (word " + std::to_string(offset) + "）"; return false; }

            if (opcode == spirvInfo::opCode::entryPoint && wordCount >= 3) {
                module.stage = getStage(readWord(bytes, offset + 1));
            }
            else if (opcode == spirvInfo::opCode::decorate && wordCount >= 3) {
                const uint32_t target = readWord(bytes, offset + 1);
                const uint32_t kind   = readWord(bytes, offset + 2);

                if (target < bound) {
                    if (kind == spirvInfo::decoration::block || kind == spirvInfo::decoration::bufferBlock) { blockList[target] = kind; }
                    else if (kind == spirvInfo::decoration::binding && wordCount >= 4)       { bindingList[target] = readWord(bytes, offset + 3); }
                    else if (kind == spirvInfo::decoration::descriptorSet && wordCount >= 4) { setList[target] = readWord(bytes, offset + 3); }
                    else if (kind == spirvInfo::decoration::location && wordCount >= 4)      { locationList[target] = readWord(bytes, offset + 3); }
                }
            }
            else if (opcode == spirvInfo::opCode::name && wordCount >= 3) {
                const uint32_t target = readWord(bytes, offset + 1);

                if (target < bound) {
                    const char* text = reinterpret_cast<const char*>(&bytes[offset * 4 + 8]);
                    namePool.emplace_back(text, strnlen(text, (wordCount - 2) * 4));
                    nameIndexList[target] = static_cast<uint32_t>(namePool.size() - 1);
                }
            }
            else if (opcode == spirvInfo::opCode::constant && wordCount >= 4) {
                const uint32_t target = readWord(bytes, offset + 2);

                if (target < bound) { valueList[target] = readWord(bytes, offset + 3); }
            }
            else if (opcode == spirvInfo::opCode::typePointer && wordCount >= 4) {
                const uint32_t target = readWord(bytes, offset + 1);

                if (target < bound) { pointeeList[target] = readWord(bytes, offset + 3); }
            }
            else if (opcode == spirvInfo::opCode::typeArray && wordCount >= 4) {
                const uint32_t target = readWord(bytes, offset + 1);

                if (target < bound) { elementList[target] = readWord(bytes, offset + 2); lengthList[target] = readWord(bytes, offset + 3); }
            }
            else if (opcode == spirvInfo::opCode::typeRuntimeArray && wordCount >= 3) {
                const uint32_t target = readWord(bytes, offset + 1);

                if (target < bound) { elementList[target] = readWord(bytes, offset + 2); }
            }
            else if (opcode == spirvInfo::opCode::typeVector && wordCount >= 4) {
                const uint32_t target = readWord(bytes, offset + 1);

                if (target < bound) { vectorCompList[target] = readWord(bytes, offset + 2); vectorCountList[target] = readWord(bytes, offset + 3); }
            }
            else if (opcode == spirvInfo::opCode::typeFloat && wordCount >= 3) {
                const uint32_t target = readWord(bytes, offset + 1);

                if (target < bound) { floatList[target] = target; }
            }
            else if (opcode == spirvInfo::opCode::typeSampledImage && wordCount >= 3) {
                const uint32_t target = readWord(bytes, offset + 1);

                if (target < bound) { sampledImageList[target] = readWord(bytes, offset + 2); }
            }
            else if (opcode == spirvInfo::opCode::typeImage && wordCount >= 8) {
                const uint32_t target = readWord(bytes, offset + 1);

                if (target < bound) { imageDimList[target] = readWord(bytes, offset + 3); imageSampledList[target] = readWord(bytes, offset + 7); }
            }
            else if (opcode == spirvInfo::opCode::typeSampler && wordCount >= 2) {
                const uint32_t target = readWord(bytes, offset + 1);

                if (target < bound) { samplerList[target] = target; }
            }
            else if (opcode == spirvInfo::opCode::typeStruct && wordCount >= 2) {
                const uint32_t target = readWord(bytes, offset + 1);

                if (target < bound) { structList[target] = target; }
            }
            else if (opcode == spirvInfo::opCode::variable && wordCount >= 4) {
                const uint32_t varId = readWord(bytes, offset + 2);

                if (varId < bound) { varTypeList[varId] = readWord(bytes, offset + 1); varStorageList[varId] = readWord(bytes, offset + 3); }
            }

            offset += wordCount;
        }

        if (module.stage == 0) { error = file + " entry point model unknown (no valid OpEntryPoint?)"; return false; }

        const auto nameOf = [&](uint32_t id) -> std::string {
            return nameIndexList[id] == spirvInfo::invalidId ? std::string{} : namePool[nameIndexList[id]];
        };

        //分类：描述符绑定（只看 UniformConstant / Uniform / StorageBuffer 且带 binding）
        for (uint32_t id = 1; id < bound; id++) {
            if (varTypeList[id] == spirvInfo::invalidId || bindingList[id] == spirvInfo::invalidId) { continue; }
            if (varStorageList[id] != spirvInfo::storageClass::uniformConstant && varStorageList[id] != spirvInfo::storageClass::uniform
             && varStorageList[id] != spirvInfo::storageClass::storageBuffer) { continue; }

            //剥掉数组，拿到元素类型与数组长度
            uint32_t elementId = pointeeList[varTypeList[id]];
            uint32_t count     = 1;

            if (elementId != spirvInfo::invalidId && elementList[elementId] != spirvInfo::invalidId) {
                const uint32_t lengthId = lengthList[elementId];
                count     = (lengthId != spirvInfo::invalidId && valueList[lengthId] != spirvInfo::invalidId) ? valueList[lengthId] : 1;
                elementId = elementList[elementId];
            }

            if (elementId == spirvInfo::invalidId) { error = file + " reflect failed: unknown descriptor type: " + nameOf(id); return false; }

            bool isKnown = false;
            VkDescriptorType type{};

            if (sampledImageList[elementId] != spirvInfo::invalidId) {
                type    = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                isKnown = true;
            }
            else if (samplerList[elementId] != spirvInfo::invalidId) {
                type    = VK_DESCRIPTOR_TYPE_SAMPLER;
                isKnown = true;
            }
            else if (imageDimList[elementId] != spirvInfo::invalidId) {
                if (imageDimList[elementId] == spirvInfo::dimSubpassData)          { type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT; }
                else if (imageSampledList[elementId] == spirvInfo::sampledStorage) { type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE; }
                else                                                               { type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE; }
                isKnown = true;
            }
            else if (structList[elementId] != spirvInfo::invalidId) {
                if (varStorageList[id] == spirvInfo::storageClass::storageBuffer || blockList[elementId] == spirvInfo::decoration::bufferBlock) { type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; }
                else                                                                                                                           { type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; }
                isKnown = true;
            }

            if (!isKnown) { error = file + " reflect failed: unrecognized descriptor type: " + nameOf(id); return false; }

            module.bindingList.push_back(BindingRecord{
                .set     = (setList[id] == spirvInfo::invalidId) ? 0u : setList[id],
                .binding = bindingList[id],
                .count   = count,
                .type    = type,
                .stage   = static_cast<VkShaderStageFlags>(module.stage),
                .name    = nameOf(id),
            });
        }

        //分类：顶点输入（只看 Input 存储类且带 Location；向量 float2/3/4）
        for (uint32_t id = 1; id < bound; id++) {
            if (varTypeList[id] == spirvInfo::invalidId || locationList[id] == spirvInfo::invalidId) { continue; }
            if (varStorageList[id] != spirvInfo::storageClass::input) { continue; }

            const uint32_t typeId = pointeeList[varTypeList[id]];

            if (typeId == spirvInfo::invalidId || vectorCountList[typeId] == spirvInfo::invalidId) { continue; }

            VkFormat format = VK_FORMAT_UNDEFINED;

            if (floatList[vectorCompList[typeId]] != spirvInfo::invalidId) {
                if (vectorCountList[typeId] == 2)      { format = VK_FORMAT_R32G32_SFLOAT; }
                else if (vectorCountList[typeId] == 3) { format = VK_FORMAT_R32G32B32_SFLOAT; }
                else if (vectorCountList[typeId] == 4) { format = VK_FORMAT_R32G32B32A32_SFLOAT; }
            }

            module.inputList.push_back(InputRecord{ .location = locationList[id], .format = format, .stage = module.stage, .name = nameOf(id) });
        }

        //规范排序（引擎与工具看到同一顺序）
        std::sort(module.bindingList.begin(), module.bindingList.end(), [](const BindingRecord& left, const BindingRecord& right) {
            return left.set != right.set ? left.set < right.set : left.binding < right.binding;
        });
        std::sort(module.inputList.begin(), module.inputList.end(), [](const InputRecord& left, const InputRecord& right) { return left.location < right.location; });

        return true;
    }



    //生成
    //把传进来的路径归一成与引擎 fileInfo::shader::* 相同的相对键（构建期 CMake 传的是绝对路径）
    std::string keyOf(const std::string& file) {
        const size_t at = file.find("src/assets/");

        return at == std::string::npos ? file : file.substr(at);
    }

    std::string tagOf(const std::string& file) {
        const size_t slash = file.find_last_of("/\\");
        const size_t dot   = file.find_last_of('.');

        return file.substr(slash + 1, (dot == std::string::npos ? file.size() : dot) - slash - 1);
    }

    std::string escape(const std::string& text) {
        std::string out{};

        for (const char ch : text) {
            if (ch == '\\' || ch == '"') { out += '\\'; }
            out += ch;
        }

        return out;
    }

    std::string descriptorName(VkDescriptorType type) {
        switch (type) {
            case VK_DESCRIPTOR_TYPE_SAMPLER:                return "VK_DESCRIPTOR_TYPE_SAMPLER";
            case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: return "VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER";
            case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:          return "VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE";
            case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:          return "VK_DESCRIPTOR_TYPE_STORAGE_IMAGE";
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:         return "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER";
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:         return "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER";
            case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:       return "VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT";
            default:                                        return "static_cast<VkDescriptorType>(" + std::to_string(static_cast<int>(type)) + ")";
        }
    }

    std::string formatName(VkFormat format) {
        switch (format) {
            case VK_FORMAT_R32G32_SFLOAT:        return "VK_FORMAT_R32G32_SFLOAT";
            case VK_FORMAT_R32G32B32_SFLOAT:     return "VK_FORMAT_R32G32B32_SFLOAT";
            case VK_FORMAT_R32G32B32A32_SFLOAT:  return "VK_FORMAT_R32G32B32A32_SFLOAT";
            default:                             return "VK_FORMAT_UNDEFINED";
        }
    }

    std::string stageName(VkShaderStageFlagBits stage) {
        switch (stage) {
            case VK_SHADER_STAGE_VERTEX_BIT:   return "VK_SHADER_STAGE_VERTEX_BIT";
            case VK_SHADER_STAGE_FRAGMENT_BIT: return "VK_SHADER_STAGE_FRAGMENT_BIT";
            case VK_SHADER_STAGE_COMPUTE_BIT:  return "VK_SHADER_STAGE_COMPUTE_BIT";
            case VK_SHADER_STAGE_TASK_BIT_NV:  return "VK_SHADER_STAGE_TASK_BIT_NV";
            case VK_SHADER_STAGE_MESH_BIT_NV:  return "VK_SHADER_STAGE_MESH_BIT_NV";
            default:                           return "static_cast<VkShaderStageFlagBits>(0)";
        }
    }

    void emitModule(std::ostream& out, const Module& module) {
        const std::string tag = module.tag;

        if (!module.bindingList.empty()) {
            out << "\tinline constexpr bindingNode " << tag << "_bindingList[]{\n";
            for (const BindingRecord& record : module.bindingList) {
                out << "\t\t{ .set = " << record.set << ", .binding = " << record.binding << ", .count = " << record.count
                    << ", .type = " << descriptorName(record.type) << ", .stage = " << stageName(static_cast<VkShaderStageFlagBits>(record.stage))
                    << ", .name = \"" << escape(record.name) << "\" },\n";
            }
            out << "\t};\n";
        }

        if (!module.inputList.empty()) {
            out << "\tinline constexpr inputNode " << tag << "_inputList[]{\n";
            for (const InputRecord& record : module.inputList) {
                out << "\t\t{ .location = " << record.location << ", .format = " << formatName(record.format)
                    << ", .stage = " << stageName(record.stage) << ", .name = \"" << escape(record.name) << "\" },\n";
            }
            out << "\t};\n";
        }

        out << "\tinline constexpr moduleNode " << tag << "{\n"
            << "\t\t.file         = \"" << escape(module.file) << "\",\n"
            << "\t\t.hash         = 0x" << std::hex << module.hash << std::dec << "ull,\n"
            << "\t\t.byteCount    = " << module.byteCount << ",\n"
            << "\t\t.stage        = " << stageName(module.stage) << ",\n"
            << "\t\t.bindingList  = " << (module.bindingList.empty() ? "nullptr" : tag + "_bindingList") << ",\n"
            << "\t\t.bindingCount = " << module.bindingList.size() << ",\n"
            << "\t\t.inputList    = " << (module.inputList.empty() ? "nullptr" : tag + "_inputList") << ",\n"
            << "\t\t.inputCount   = " << module.inputList.size() << ",\n"
            << "\t};\n\n";
    }

    //与 SDK spirv-reflect 对拍
    bool findSdkTool(std::string& path) {
        const char* sdk = std::getenv("VULKAN_SDK");
        if (sdk == nullptr) { return false; }

        std::string root = sdk;
        for (char& ch : root) { if (ch == '/') { ch = '\\'; } }

        const std::string candidate = root + "\\Bin\\spirv-reflect.exe";

        std::ifstream probe(candidate);
        if (!probe.is_open()) { return false; }

        path = candidate;
        return true;
    }

    std::string winPath(std::string path) {
        for (char& ch : path) { if (ch == '/') { ch = '\\'; } }

        return path;
    }

    bool verifyWithSdk(const std::string& sdkTool, const Module& module, const std::string& dumpDir, std::string& report) {
        const std::string dumpPath = winPath(dumpDir + "\\verify_" + module.tag + ".txt");
        const std::string command  = "\"" + winPath(sdkTool) + "\" \"" + winPath(module.sourcePath) + "\" > \"" + dumpPath + "\" 2>&1";

        //cmd /c 的坑：命令以引号开头时首尾引号会被吞 ⇒ 整体再套一层引号
        if (std::system(("\"" + command + "\"").c_str()) != 0) { report = "[verify] spirv-reflect run failed - skipped: " + module.file; return true; }

        std::ifstream dump(dumpPath);
        std::vector<uint32_t> sdkBindingList{};

        std::string line{};
        while (std::getline(dump, line)) {
            const size_t head = line.find_first_not_of(" \t");
            if (head == std::string::npos || line.compare(head, 7, "binding") != 0) { continue; }		//缩进后的 "binding  : N"

            const size_t colon = line.find(':', head);
            if (colon == std::string::npos) { continue; }

            size_t at = colon + 1;
            while (at < line.size() && (line[at] < '0' || line[at] > '9')) { at++; }
            if (at >= line.size()) { continue; }

            sdkBindingList.push_back(static_cast<uint32_t>(std::stoul(line.substr(at))));
        }

        dump.close();
        std::remove(dumpPath.c_str());

        std::vector<uint32_t> ourBindingList{};
        for (const BindingRecord& record : module.bindingList) { ourBindingList.push_back(record.binding); }

        if (sdkBindingList.empty() && !ourBindingList.empty()) { report = "[verify] cannot parse spirv-reflect output - skipped: " + module.file; return true; }

        std::sort(sdkBindingList.begin(), sdkBindingList.end());
        sdkBindingList.erase(std::unique(sdkBindingList.begin(), sdkBindingList.end()), sdkBindingList.end());

        if (sdkBindingList != ourBindingList) {
            report = "[verify] mismatch with spirv-reflect: " + module.file;
            return false;
        }

        report = "[verify] ok: " + module.file + " (bindings " + std::to_string(ourBindingList.size()) + ")";
        return true;
    }
}

int main(int argc, char** argv) {
    std::string       outPath{};
    std::string       nameSpace = "descStruct";		//生成物的命名空间（引擎域）
    bool              verify = false;
    std::vector<std::string> fileList{};

    for (int i = 1; i < argc; i++) {
        const std::string arg = argv[i];
        if (arg == "--out" && i + 1 < argc) { outPath = argv[++i]; continue; }
        if (arg == "--namespace" && i + 1 < argc) { nameSpace = argv[++i]; continue; }
        if (arg == "--verify") { verify = true; continue; }

        fileList.push_back(arg);
    }

    if (outPath.empty() || fileList.empty()) {
        std::cerr << "usage: reflect-gen --out <header> [--namespace <ns>] [--verify] <spv...>\n";
        return 2;
    }

    const size_t slash = outPath.find_last_of("/\\");
    const std::string dumpDir = slash == std::string::npos ? std::string(".") : outPath.substr(0, slash);

    std::ofstream out(outPath, std::ios::trunc | std::ios::binary);		//二进制：生成物统一 LF，不受 Windows 文本模式影响
    if (!out.is_open()) { std::cerr << "cannot write: " << outPath << "\n"; return 2; }

    out << "//本文件由 config\\tool\\reflect-gen 生成：类型 ＋ 反射数据（勿手改）\n"
        << "//命名空间：" << nameSpace << "（类型与数据表平铺在同一命名空间；引擎侧查表在 descStruct::spvData）\n"
        << "//生成命令：reflect-gen --out " << outPath << " <src/assets/shader/**/*.spv>\n"
        << "#pragma once\n\n"
        << "#include <cstdint>\n"
        << "#include <string_view>\n"
        << "#include <vulkan/vulkan_core.h>\n\n"
        << "namespace " << nameSpace << " {\n"
        << "\tstruct bindingNode {\n"
        << "\t\tuint32_t           set{};\n"
        << "\t\tuint32_t           binding{};\n"
        << "\t\tuint32_t           count{ 1 };\n"
        << "\t\tVkDescriptorType   type{};\n"
        << "\t\tVkShaderStageFlags stage{};\n"
        << "\t\tconst char*        name{};\n"
        << "\t};\n\n"
        << "\tstruct inputNode {\n"
        << "\t\tuint32_t              location{};\n"
        << "\t\tVkFormat              format{};\n"
        << "\t\tVkShaderStageFlagBits stage{};\n"
        << "\t\tconst char*           name{};\n"
        << "\t};\n\n"
        << "\tstruct moduleNode {\n"
        << "\t\tstd::string_view           file{};\n"
        << "\t\tuint64_t                   hash{};\n"
        << "\t\tuint64_t                   byteCount{};\n"
        << "\t\tVkShaderStageFlagBits      stage{};\n"
        << "\t\tconst bindingNode*         bindingList{};\n"
        << "\t\tuint32_t                   bindingCount{};\n"
        << "\t\tconst inputNode*           inputList{};\n"
        << "\t\tuint32_t                   inputCount{};\n"
        << "\t};\n\n";		//类型段结束；数据段紧接着同一个命名空间（全部平铺）

    std::vector<reflectGen::Module> moduleList{};

    for (const std::string& file : fileList) {
        reflectGen::ByteList bytes{};
        if (!reflectGen::readFile(file, bytes)) { std::cerr << "cannot read: " << file << "\n"; return 1; }

        reflectGen::Module module{};
        module.tag = reflectGen::tagOf(file);

        std::string error{};
        if (!reflectGen::parseShader(bytes, file, module, error)) { std::cerr << error << "\n"; return 1; }

        reflectGen::emitModule(out, module);
        moduleList.push_back(std::move(module));
    }

    std::sort(moduleList.begin(), moduleList.end(), [](const reflectGen::Module& left, const reflectGen::Module& right) { return left.file < right.file; });

    out << "\tinline constexpr moduleNode const* const moduleList[]{\n";
    for (const reflectGen::Module& module : moduleList) { out << "\t\t&" << module.tag << ",\n"; }
    out << "\t};\n}";		//末尾不留换行（生成物 LF）
    if (verify) {
        std::string sdkTool{};
        if (!reflectGen::findSdkTool(sdkTool)) { std::cout << "[verify] spirv-reflect not found (VULKAN_SDK) - skipped\n"; }
        else {
            uint32_t okCount = 0, skipCount = 0;

            for (const reflectGen::Module& module : moduleList) {
                std::string report{};
                if (!reflectGen::verifyWithSdk(sdkTool, module, dumpDir, report)) { std::cerr << report << "\n"; return 1; }

                if (report.find("ok:") != std::string::npos) { okCount++; }
                else { std::cout << report << "\n"; skipCount++; }		//只有跳过/异常才逐条打印
            }

            std::cout << "[verify] ok: " << okCount << "/" << moduleList.size() << (skipCount == 0 ? "" : " (skipped " + std::to_string(skipCount) + ")") << "\n";
        }
    }

    std::cout << "generated: " << outPath << " (" << moduleList.size() << " spv)\n";
    return 0;
}