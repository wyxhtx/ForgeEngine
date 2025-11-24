#include "Main.h"



int main() {
    //防止同一时间启用多个游戏进程
    HANDLE hMutex = CreateMutexA(nullptr, TRUE, windowInfo::gameName);

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        Log::printError(__FILE__, __LINE__, "同一时间只能启用一个游戏进程");
        return 0;
    }

    //加载dll
    SetDllDirectoryA(fileInfo::dll);

    //todo
    //编译临时着色器
    system("cd src\\shader && temp.bat");



    //加载音乐线程耗时长，所以先声明
    Music InitMusic{};
    Window InitWindow{};
    Device InitDevice{ InitWindow.window };
    Pipeline InitPipeline{ InitDevice };

    fpsStruct::tick deltaTime{};
    threadCommInfo::musicLock = true;



    //vulkan主循环
    while (!glfwWindowShouldClose(InitWindow.window)) {
        glfwPollEvents(); 

        if (InitWindow.isMin) {
            if (deltaTime.isUpdate(fpsInfo::getKeyFPS)) {
                InitWindow.getKey((float)deltaTime.dt);
            }

            InitPipeline.drawFrame(InitWindow.InitCamera);
        }
    }



    threadCommInfo::musicLock = false;
    vkDeviceWaitIdle(InitDevice.device);
    return 0;
}