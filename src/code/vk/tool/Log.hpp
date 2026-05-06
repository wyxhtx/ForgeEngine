#pragma once



//前向声明logThreadLock
namespace lockInfo {
    extern std::atomic<bool> logThreadLock;
}

namespace Log {
    //输出标题
    void printTitle(auto text) {
        while (lockInfo::logThreadLock.exchange(true)) { std::this_thread::yield(); }

        std::cout << text << ":" << std::endl;

        lockInfo::logThreadLock.store(false);
    }

    //不使用科学计数法，使用固定法输出小数
    void printFixed(auto text) {
        while (lockInfo::logThreadLock.exchange(true)) { std::this_thread::yield(); }

        std::cout << "\t" << std::fixed << text << std::endl;

        lockInfo::logThreadLock.store(false);
    }

    //输出坐标
    void printPos(glm::vec3 text) {
        while (lockInfo::logThreadLock.exchange(true)) { std::this_thread::yield(); }

        std::cout << "\t" << text.x << "\t" << text.y << "\t" << text.z << std::endl;

        lockInfo::logThreadLock.store(false);
    }

    //输出报错后暂停
    void printError(std::string file, int line, std::string text) {
        while (lockInfo::logThreadLock.exchange(true)) { std::this_thread::yield(); }

        std::cerr << std::endl
            << "------" << "错误" << std::endl
            << "------" << file << std::endl 
            << "------" << line << std::endl 
            << "------" << text << std::endl;
        while (true);

        lockInfo::logThreadLock.store(false);
    }

    //输出警告
    void printWarning(std::string file, int line, std::string text) {
        while (lockInfo::logThreadLock.exchange(true)) { std::this_thread::yield(); }

        std::cerr << std::endl
            << "------" << "警告" << std::endl
            << "------" << file << std::endl
            << "------" << line << std::endl
            << "------" << text << std::endl;

        lockInfo::logThreadLock.store(false);
    }

    //输出
    void print(auto text) {
        while (lockInfo::logThreadLock.exchange(true)) { std::this_thread::yield(); }

        std::cout << "\t" << text << std::endl;

        lockInfo::logThreadLock.store(false);
    }

    //输出行间间隔
    void delimiter() {
        while (lockInfo::logThreadLock.exchange(true)) { std::this_thread::yield(); }

        std::cout << std::endl << std::endl << std::endl;

        lockInfo::logThreadLock.store(false);
    }

    //清屏
    void clear() {
        while (lockInfo::logThreadLock.exchange(true)) { std::this_thread::yield(); }

        std::cout << "\033c";

        lockInfo::logThreadLock.store(false);
    }
};