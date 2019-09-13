#ifndef MEDIAPLAYER_H
#define MEDIAPLAYER_H

class Stream;

#include <stream/Stream.h>
#include <sync/MediaClock.h>
#include <player/PlayerState.h>
#include <decoder/AudioDecoder.h>
#include <decoder/VideoDecoder.h>
#include <device/AudioDevice.h>
#include <device/VideoDevice.h>
#include <sync/MediaSync.h>
#include <convertor/AudioResampler.h>
#include <common/Log.h>
#include <message/MessageCenter.h>
#include <stream/IStreamListener.h>

class MediaPlayer : public IStreamListener {

    const char *const TAG = "MediaPlayer";

    const char *const OPT_LOW_RESOLUTION = "lowres";

    const char *const OPT_THREADS = "threads";

    const char *const OPT_REF_COUNTED_FRAMES = "refcounted_frames";
private:

    Mutex mutex;

    Condition condition;

    /// 播放器状态
    PlayerState *playerState = nullptr;

    /// 媒体同步器
    MediaSync *mediaSync = nullptr;

    /// 媒体流
    Stream *mediaStream = nullptr;

    /// 音频解码器
    AudioDecoder *audioDecoder = nullptr;

    /// 视频解码器
    VideoDecoder *videoDecoder = nullptr;

    /// 视频输出设备
    VideoDevice *videoDevice = nullptr;

    /// 音频输出设备
    AudioDevice *audioDevice = nullptr;

    /// 音频重采样器
    AudioResampler *audioResampler = nullptr;

    /// 消息事件处理
    MessageCenter *messageCenter = nullptr;

    // 解码上下文
    AVFormatContext *formatContext = nullptr;

public:
    MediaPlayer();

    virtual ~MediaPlayer();

    int reset();

    int create();

    int start();

    void resume();

    void pause();

    void stop();

    int destroy();

    void setDataSource(const char *url, int64_t offset = 0, const char *headers = nullptr);

    void seekTo(float timeMs);

    void setLooping(int looping);

    void setVolume(float leftVolume, float rightVolume);

    void setMute(int mute);

    void setRate(float rate);

    void setPitch(float pitch);

    int getRotate();

    int getVideoWidth();

    int getVideoHeight();

    long getCurrentPosition();

    long getDuration();

    int isPlaying();

    int isLooping();

    int getMetadata(AVDictionary **metadata);

    MessageQueue *getMessageQueue();

    PlayerState *getPlayerState();

    void pcmQueueCallback(uint8_t *stream, int len);

    void setAudioDevice(AudioDevice *audioDevice);

    void setMediaSync(MediaSync *mediaSync);

    MediaSync *getMediaSync() const;

    void setVideoDevice(VideoDevice *videoDevice);

    void setMessageCenter(MessageCenter *msgCenter);

    void onStartOpenStream() override;

    void onEndOpenStream(int videoIndex, int audioIndex) override;

    void setMessageListener(IMessageListener *messageListener);

    void setFormatContext(AVFormatContext *formatContext);

private:

    void togglePause();

    // prepareAsync decoder with stream_index
    int openDecoder(int streamIndex);

    int closeDecoder(int streamIndex, AVCodecContext *pContext, AVDictionary *pDictionary);

    // open an audio output device
    int openAudioDevice(int64_t wanted_channel_layout, int wanted_nb_channels, int wanted_sample_rate);

    int notifyMsg(int what);

    int notifyMsg(int what, int arg1);

    int notifyMsg(int what, int arg1, int arg2);

    int checkParams();
};


#endif //MEDIAPLAYER_H
