#pragma once



class Music {
public:
    //todo主进程循环再开始播放
    //todo退出时释放资源

    Music() {
        std::thread(&Music::createBackgroundMusic, this).detach();

        if (debugInfo::LogSwitch) {
            Log::printTitle("————————Music已创建");

            Log::delimiter();
        }
    }

    ~Music() {
        if (debugInfo::LogSwitch) {
            Log::print("Music已清除");
        }
    }

private:
    void createBackgroundMusic() {
        // 初始化ALUT
        alutInit(NULL, 0);
        // 加载音频文件
        ALuint buffer = alutCreateBufferFromFile(fileInfo::music::throughTheMounta);
        if (buffer == AL_NONE) {
            Log::error(__FILE__, __LINE__, "音频文件加载失败");
            alutExit();
            return;
        }

        // 创建音频源
        ALuint source;
        alGenSources(1, &source);
        alSourcei(source, AL_BUFFER, buffer);
        // 设置音频源循环播放
        alSourcei(source, AL_LOOPING, AL_TRUE);
        // 播放音频
        alSourcePlay(source);

        //等待主线程初始化完成
        while (!threadCommInfo::musicLock);

        // 等待音频播放完成并继续循环
        ALint source_state;
        while (threadCommInfo::musicLock) {
            alGetSourcei(source, AL_SOURCE_STATE, &source_state);
            if (source_state != AL_PLAYING) {
                // 如果音频停止，重新播放
                alSourcePlay(source);
            }
            alutSleep(0.1f);  //避免紧循环
        }

        // 清理资源
        alDeleteSources(1, &source);
        alDeleteBuffers(1, &buffer);
        alutExit();
    }
};