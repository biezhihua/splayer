#ifndef AUDIODECODER_H
#define AUDIODECODER_H

#include <decoder/MediaDecoder.h>
#include <player/PlayerState.h>

class AudioDecoder : public MediaDecoder {
public:
    AudioDecoder(AVCodecContext *avctx, AVStream *stream, int streamIndex, PlayerState *playerState,
                     AVPacket *flushPacket, Condition *pCondition);

    virtual ~AudioDecoder();

    int getAudioFrame(AVFrame *frame);


};


#endif //AUDIODECODER_H