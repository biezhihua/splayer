#ifndef SOUNDTOUCH_WRAPPER_H
#define SOUNDTOUCH_WRAPPER_H

#include "SoundTouch.h"

using namespace std;

using namespace soundtouch;

class SoundTouchWrapper {

public:
    SoundTouchWrapper();

    virtual ~SoundTouchWrapper();

    // 初始化
    void create();

    // 转换
    int translate(short *data, float speed, float pitch, int len, int bytes_per_sample, int n_channel, int n_sampleRate);

    // 销毁
    void destroy();

    // 获取SoundTouch对象
    SoundTouch *getSoundTouch();

private:
    SoundTouch *mSoundTouch;
};


#endif //SOUNDTOUCH_WRAPPER_H
