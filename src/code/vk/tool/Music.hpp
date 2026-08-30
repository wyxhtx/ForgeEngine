#pragma once



class Music {
public:
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
        alutInit(NULL, 0);                                                              //初始化ALUT
        ALuint buffer = alutCreateBufferFromFile(fileInfo::music::throughTheMounta);    //加载音频文件
        if (buffer == AL_NONE) {
            Log::printError(__FILE__, __LINE__, "音频文件加载失败");
            alutExit();
            return;
        }

        ALuint source;                              //创建音频源
        alGenSources(1, &source);
        alSourcei(source, AL_BUFFER, buffer);
        alSourcei(source, AL_LOOPING, AL_TRUE);     //设置音频源循环播放



        while (!lockInfo::musicThreadLock.load()) { std::this_thread::yield(); }        //确保主线程初始化完成后播放音乐
        ALint source_state;                                                             //等待音频播放完成并继续循环
        alSourcePlay(source);                                                           //播放音频

        while (lockInfo::musicThreadLock.load()) {
            alGetSourcei(source, AL_SOURCE_STATE, &source_state);
            if (source_state != AL_PLAYING) { alSourcePlay(source); }                   //如果音频停止，重新播放
            alutSleep(0.1f);                                                            //避免紧循环
        }

        //清理资源
        alDeleteSources(1, &source);
        alDeleteBuffers(1, &buffer);
        alutExit();
    }
};