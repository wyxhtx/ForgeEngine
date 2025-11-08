#pragma once



namespace Log {
    //输出标题
    void printTitle(auto text) {
        std::cout << text << ":" << std::endl;
    }

    //不使用科学计数法，使用固定法输出小数
    void printFixed(auto text) {
        std::cout << "\t" << std::fixed << text << std::endl;
    }

    //输出坐标
    void printPos(glm::vec3 text) {
        std::cout << "\t" << text.x << "\t" << text.y << "\t" << text.z << std::endl;
    }

    //输出
    void print(auto text) {
        std::cout << "\t" << text << std::endl;
    }

    //输出报错
    void error(std::string file, int line, std::string text) {
        std::cerr << std::endl 
            << "------" << file << std::endl 
            << "------" << line << std::endl 
            << "------" << text << std::endl;

        while (true);
    }

    //输出行间间隔
    void delimiter() {
        std::cout << std::endl << std::endl << std::endl;
    }

    //清屏
    void clear() {
        std::cout << "\033c";
    }
};