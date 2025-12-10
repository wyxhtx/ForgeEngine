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

    //system("cd src\\image_build && png_to_ktx2.bat");   //编译纹理
    //Log::print("");
    //system("cd src\\shader_build && terrain_gts.bat");  //编译地形着色器
    //Log::print("");
    system("cd src\\shader_build && model_gts.bat");    //编译模型着色器
    Log::print("");



    //加载音乐线程耗时长，所以先声明
    Music InitMusic{};
    Window InitWindow{};
    Device InitDevice{ InitWindow.window };
    Pipeline InitPipeline{ InitDevice };

    fpsStruct::tickNode deltaTime{};
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