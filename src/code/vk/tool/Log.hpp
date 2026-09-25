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
	void printPos(const glm::vec3& text) {
		std::lock_guard<std::mutex> lock(lockInfo::logThreadLock);
		std::cout << "\t" << text.x << "\t" << text.y << "\t" << text.z << std::endl;
	}

	//输出报错后暂停
	void printError(const std::string& file, int line, const std::string& text) {
		std::lock_guard<std::mutex> lock(lockInfo::logThreadLock);
		std::cerr << std::endl
			<< "------" << "错误" << std::endl
			<< "------" << file << std::endl 
			<< "------" << line << std::endl 
			<< "------" << text << std::endl;
		while (true);
	}

	//输出警告
	void printWarning(const std::string& file, int line, const std::string& text) {
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

	//按 tab 位补齐：补到第 column 列（tab 宽按 8 计），供多行日志的列对齐；已到/超过目标列时留 1 个 tab 作间隔
	std::string tabPad(const std::string& text, size_t column) {
		std::string result = text;

		size_t tabs = text.size() < column ? (column - text.size() + 7) / 8 : 1;

		for (size_t i = 0; i < tabs; i++) {
			result += "\t";
		}

		return result;
	}
};