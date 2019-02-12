//
// Created by biezhihua on 2019/2/4.
//

#ifndef SMUSIC_SMUSIC_PLAYER_H
#define SMUSIC_SMUSIC_PLAYER_H


#include <jni.h>
#include <string>
#include "SJavaMethods.h"
#include "SFFmpeg.h"
#include "SStatus.h"
#include "SError.h"
#include "SOpenSLES.h"

class SPlayer {

private:

    JavaVM *pJavaVM = NULL;

    string *pSource = NULL;

    SJavaMethods *pJavaMethods = NULL;

    SFFmpeg *pSFFmpeg = NULL;

    SStatus *pPlayerStatus = NULL;

    SOpenSLES *pOpenSLES = NULL;

public:

    pthread_t prepareDecodeThread;

    pthread_t startDecodeAudioThread;

    pthread_t playThread;

public:
    SPlayer(JavaVM *pVm, SJavaMethods *pMethods);

    ~SPlayer();

    void setSource(string *url);

    void prepare();

    void start();

    SFFmpeg *getSFFmpeg();

    SJavaMethods *getSJavaMethods();

    SStatus *getPlayerStatus();
};


#endif //SMUSIC_SMUSIC_PLAYER_H
