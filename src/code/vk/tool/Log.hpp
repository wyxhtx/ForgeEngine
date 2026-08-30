#pragma once



//前向声明logThreadLock
namespace lockInfo {
    extern std::mutex logThreadLock;       //日志线程锁，用于原子同步各线程调用日志
}

namespace Log {
    //输出标题
    void printTitle(auto text) {
        std::lock_guard<std::mutex> lock(lockInfo::logThreadLock);
        std::cout << text << ":" << std::endl;
    }

    //不使用科学计数法，使用固定法输出小数
    void printFixed(auto text) {
        std::lock_guard<std::mutex> lock(lockInfo::logThreadLock);
        std::cout << "\t" << std::fixed << text << std::endl;
    }

    //输出坐标
    void printPos(glm::vec3 text) {
        std::lock_guard<std::mutex> lock(lockInfo::logThreadLock);
        std::cout << "\t" << text.x << "\t" << text.y << "\t" << text.z << std::endl;
    }

    //输出报错后暂停
    void printError(std::string file, int line, std::string text) {
        std::lock_guard<std::mutex> lock(lockInfo::logThreadLock);
        std::cerr << std::endl
            << "------" << "错误" << std::endl
            << "------" << file << std::endl 
            << "------" << line << std::endl 
            << "------" << text << std::endl;
        while (true);
    }

    //输出警告
    void printWarning(std::string file, int line, std::string text) {
        std::lock_guard<std::mutex> lock(lockInfo::logThreadLock);
        std::cerr << std::endl
            << "------" << "警告" << std::endl
            << "------" << file << std::endl
            << "------" << line << std::endl
            << "------" << text << std::endl;
    }

    //输出
    void print(auto text) {
        std::lock_guard<std::mutex> lock(lockInfo::logThreadLock);
        std::cout << "\t" << text << std::endl;
    }

    //输出行间间隔
    void delimiter() {
        std::lock_guard<std::mutex> lock(lockInfo::logThreadLock);
        std::cout << std::endl << std::endl << std::endl;
    }

    //清屏
    void clear() {
        std::lock_guard<std::mutex> lock(lockInfo::logThreadLock);
        std::cout << "\033c";
    }
};