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
    //system("cd src\\shader_build && glsl_to_spirv.bat"); //编译着色器
    //system("cd src\\shader_build && dsScence_gts.bat"); //编译延迟着色器
    //system("cd src\\shader_build && terrain_gts.bat");  //编译地形着色器
    //system("cd src\\shader_build && model_gts.bat");    //编译模型着色器



    //Music    InitMusic{};            //加载音乐线程耗时长，所以先声明
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
    while (lockInfo::queueThreadLock.exchange(true)) { std::this_thread::yield(); }
    vkDeviceWaitIdle(InitDevice.device);    //等待所有GPU工作完成
    lockInfo::queueThreadLock.store(false);

    return 0;
}