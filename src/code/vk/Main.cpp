#include "Main.h"



int main() {
    SetConsoleOutputCP(CP_UTF8);   // 控制台按 UTF-8 输出，避免中文乱码
    //防止同一时间启用多个游戏进程
    HANDLE hMutex = CreateMutexA(nullptr, TRUE, windowInfo::gameName);
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        Log::printError(__FILE__, __LINE__, "同一时间只能启用一个游戏进程");
        return 0;
    }





    //Music    InitMusic{};                       //加载音乐线程耗时长，所以先声明
    Window   InitWindow{};
    Device   InitDevice{ InitWindow.window };
    Pipeline InitPipeline{ InitDevice };



    //锁定音乐队列
    while (lockInfo::musicThreadLock.exchange(true)) { std::this_thread::yield(); }

    //vulkan主循环
    while (!glfwWindowShouldClose(InitWindow.window)) {
        glfwPollEvents(); 

        if (InitWindow.isMin) {
            if (engineList::procTick.node[engineEnum::tickEnum::key].isUpdate()) {
                InitWindow.getKey((float)engineList::procTick.node[engineEnum::tickEnum::key].dt);
            }

            InitPipeline.drawFrame(InitWindow.InitCamera);
        }
    }

    //释放锁定音乐队列
    lockInfo::musicThreadLock.store(false);



    while (lockInfo::updatePlateFuncLock.load()) { std::this_thread::yield(); }
    while (lockInfo::recreateCmdFuncLock.load()) { std::this_thread::yield(); }
    std::lock_guard<std::mutex> lock(lockInfo::queueThreadLock);
    vkDeviceWaitIdle(InitDevice.device);    //等待所有GPU工作完成

    return 0;
}