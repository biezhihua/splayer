#ifndef MEDIADECODER_H
#define MEDIADECODER_H

#include <common/Thread.h>
#include <common/Log.h>
#include <player/PlayerState.h>
#include <queue/PacketQueue.h>
#include <queue/FrameQueue.h>

class MediaDecoder : public Runnable {
    const char *const TAG = "MediaDecoder";

public:
    MediaDecoder(AVCodecContext *codecContext, AVStream *stream, int streamIndex, PlayerState *playerState,
                     AVPacket *flushPacket);

    virtual ~MediaDecoder();

    virtual void start();

    virtual void stop();

    virtual void flush();

    int pushPacket(AVPacket *pkt);

    int getPacketSize();

    int getStreamIndex();

    AVStream *getStream();

    AVCodecContext *getCodecContext();

    int getMemorySize();

    int hasEnoughPackets();

    virtual bool isFinished();

    void pushFlushPacket();

    void pushNullPacket();

    virtual void run();

protected:

    Mutex mutex;

    Condition condition;

    bool abortRequest;

    PlayerState *playerState;

    /// 数据包队列
    PacketQueue *packetQueue;

    AVCodecContext *codecContext;

    AVStream *stream;

    int streamIndex;

    /// 是否已结束
    int finished;

    AVPacket *flushPacket;
};


#endif //MEDIADECODER_H
