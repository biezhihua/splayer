
#include <SDLVideoDevice.h>

#include "SDLVideoDevice.h"

SDLVideoDevice::SDLVideoDevice() : VideoDevice() {
    create();
}

SDLVideoDevice::~SDLVideoDevice() {
    destroy();
}

int SDLVideoDevice::create() {
    unsigned int initFlags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER;

    if (!SDL_getenv("SDL_AUDIO_ALSA_SET_BUFFER_SIZE")) {
        SDL_setenv("SDL_AUDIO_ALSA_SET_BUFFER_SIZE", "1", 1);
    }

    if (SDL_Init(initFlags) != 0) {
        ALOGD(TAG, "%s init sdl fail code = %s", __func__, SDL_GetError());
        return -1;
    }

    Uint32 windowFlags = SDL_WINDOW_HIDDEN;

    windowFlags |= SDL_WINDOW_RESIZABLE;

    window = SDL_CreateWindow("测试", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              surfaceWidth, surfaceHeight,
                              windowFlags);

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");

    if (window == nullptr) {
        ALOGE(TAG, "%s create sdl window fail: %s", __func__, SDL_GetError());
        destroy();
        return -1;
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == nullptr) {
        ALOGE(TAG, "%s failed to initialize a hardware accelerated renderer: %s", __func__,
              SDL_GetError());
        renderer = SDL_CreateRenderer(window, -1, 0);
    }
    if (renderer == nullptr) {
        ALOGE(TAG, "%s create renderer fail: %s", __func__, SDL_GetError());
        destroy();
        return -1;
    }

    if (!SDL_GetRendererInfo(renderer, &rendererInfo)) {
        ALOGD(TAG, "initialized %s renderer", rendererInfo.name);
    }

    if (!window || !renderer || !rendererInfo.num_texture_formats) {
        ALOGD(TAG, "%s failed to create window or renderer: %s", __func__, SDL_GetError());
        destroy();
        return -1;
    }
    return 0;
}

int SDLVideoDevice::destroy() {
    if (videoTexture) {
        SDL_DestroyTexture(videoTexture);
        videoTexture = nullptr;
    }
    if (subtitleTexture) {
        SDL_DestroyTexture(subtitleTexture);
        subtitleTexture = nullptr;
    }
    if (visTexture) {
        SDL_DestroyTexture(visTexture);
        visTexture = nullptr;
    }
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }
    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    SDL_Quit();
    rendererInfo = {nullptr};
    return 0;
}

void SDLVideoDevice::terminate() {
    VideoDevice::terminate();
    destroy();
}

int
SDLVideoDevice::onInitTexture(int initTexture, int newWidth, int newHeight, TextureFormat newFormat,
                              BlendMode blendMode, int rotate) {
    Uint32 format;
    int access;
    int width;
    int height;
    if (!videoTexture || SDL_QueryTexture(videoTexture, &format, &access, &width, &height) < 0 || newWidth != width ||
        newHeight != height || newFormat != getSDLFormat(format)) {
        void *pixels;
        int pitch;
        if (videoTexture) {
            SDL_DestroyTexture(videoTexture);
        }
        if (!(videoTexture = SDL_CreateTexture(renderer, newFormat, SDL_TEXTUREACCESS_STREAMING, newWidth,
                                               newHeight))) {
            return -1;
        }
        if (SDL_SetTextureBlendMode(videoTexture, getSDLBlendMode(blendMode)) < 0) {
            return -1;
        }
        if (initTexture) {
            if (SDL_LockTexture(videoTexture, nullptr, &pixels, &pitch) < 0) {
                return -1;
            }
            memset(pixels, 0, pitch * newHeight);
            SDL_UnlockTexture(videoTexture);
        }
        ALOGD(TAG, "%s Created %dx%d texture with %s", __func__, newWidth, newHeight,
              SDL_GetPixelFormatName(newFormat));
    }
    return 0;
}

int SDLVideoDevice::onUpdateYUV(uint8_t *yData, int yPitch, uint8_t *uData, int uPitch, uint8_t *vData, int vPitch) {
    return SDL_UpdateYUVTexture(videoTexture, nullptr, yData, yPitch, uData, uPitch, vData, vPitch);
}

int SDLVideoDevice::onUpdateARGB(uint8_t *rgba, int pitch) {
    return VideoDevice::onUpdateARGB(rgba, pitch);
}

void SDLVideoDevice::onRequestRenderStart() {
    if (renderer != nullptr) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
    }
}

int SDLVideoDevice::onRequestRenderEnd(Frame *frame, bool flip) {
    if (frame) {
        SDL_Rect rect;
        calculateDisplayRect(&rect, surfaceLeftOffset, surfaceTopOffset, surfaceWidth, surfaceHeight,
                             frame->width, frame->height, frame->sar);
        setYuvConversionMode(frame->frame);
        SDL_RenderCopyEx(renderer, videoTexture, nullptr, &rect, 0, nullptr, flip ? SDL_FLIP_VERTICAL : SDL_FLIP_NONE);
        setYuvConversionMode(nullptr);
    }
    SDL_RenderPresent(renderer);
    return 0;
}

void SDLVideoDevice::calculateDisplayRect(SDL_Rect *rect,
                                          int xLeft, int yTop,
                                          int srcWidth, int scrHeight,
                                          int picWidth, int picHeight,
                                          AVRational picSar) {
    AVRational aspectRatio = picSar;
    int64_t width, height, x, y;

    if (av_cmp_q(aspectRatio, av_make_q(0, 1)) <= 0) {
        aspectRatio = av_make_q(1, 1);
    }

    aspectRatio = av_mul_q(aspectRatio, av_make_q(picWidth, picHeight));

    /* XXX: we suppose the screen has a 1.0 pixel ratio */
    height = scrHeight;
    width = av_rescale(height, aspectRatio.num, aspectRatio.den) & ~1;
    if (width > srcWidth) {
        width = srcWidth;
        height = av_rescale(width, aspectRatio.den, aspectRatio.num) & ~1;
    }
    x = (srcWidth - width) / 2;
    y = (scrHeight - height) / 2;
    rect->x = xLeft + x;
    rect->y = yTop + y;
    rect->w = FFMAX((int) width, 1);
    rect->h = FFMAX((int) height, 1);
    ALOGD(TAG, "%s x = %d y = %d w = %d h = %d", __func__, rect->x, rect->y, rect->w, rect->h);
}


void SDLVideoDevice::setYuvConversionMode(AVFrame *frame) {
#if SDL_VERSION_ATLEAST(2, 0, 8)
    SDL_YUV_CONVERSION_MODE mode = SDL_YUV_CONVERSION_AUTOMATIC;
    if (frame && (frame->format == AV_PIX_FMT_YUV420P || frame->format == AV_PIX_FMT_YUYV422 ||
                  frame->format == AV_PIX_FMT_UYVY422)) {
        if (frame->color_range == AVCOL_RANGE_JPEG) {
            mode = SDL_YUV_CONVERSION_JPEG;
            ALOGD(TAG, "%s mode = %s", __func__, "SDL_YUV_CONVERSION_JPEG");
        } else if (frame->colorspace == AVCOL_SPC_BT709) {
            mode = SDL_YUV_CONVERSION_BT709;
            ALOGD(TAG, "%s mode = %s", __func__, "SDL_YUV_CONVERSION_BT709");
        } else if (frame->colorspace == AVCOL_SPC_BT470BG || frame->colorspace == AVCOL_SPC_SMPTE170M ||
                   frame->colorspace == AVCOL_SPC_SMPTE240M) {
            mode = SDL_YUV_CONVERSION_BT601;
            ALOGD(TAG, "%s mode = %s", __func__, "SDL_YUV_CONVERSION_BT601");
        }
    }
    SDL_SetYUVConversionMode(mode);
#endif
}

SDL_BlendMode SDLVideoDevice::getSDLBlendMode(BlendMode mode) {
    switch (mode) {
        case BLEND_NONE:
            return SDL_BLENDMODE_NONE;
        case BLEND_BLEND:
            return SDL_BLENDMODE_BLEND;
        case BLEND_ADD:
            return SDL_BLENDMODE_ADD;
        case BLEND_MOD:
            return SDL_BLENDMODE_MOD;
        case BLEND_INVALID:
            return SDL_BLENDMODE_INVALID;
    }
    return SDL_BLENDMODE_NONE;
}

TextureFormat SDLVideoDevice::getSDLFormat(Uint32 format) {
    switch (format) {
        case SDL_PIXELFORMAT_RGB332:
            return FMT_RGB8;
        case SDL_PIXELFORMAT_RGB444:
            return FMT_RGB444;
        case SDL_PIXELFORMAT_RGB555:
            return FMT_RGB555;
        case SDL_PIXELFORMAT_BGR555:
            return FMT_BGR555;
        case SDL_PIXELFORMAT_RGB565:
            return FMT_RGB565;
        case SDL_PIXELFORMAT_BGR565:
            return FMT_BGR565;
        case SDL_PIXELFORMAT_RGB24:
            return FMT_RGB24;
        case SDL_PIXELFORMAT_BGR24:
            return FMT_BGR24;
        case SDL_PIXELFORMAT_RGB888:
            return FMT_0RGB32;
        case SDL_PIXELFORMAT_BGR888:
            return FMT_0BGR32;
        case SDL_PIXELFORMAT_RGBX8888:
            return FMT_NE;
        case SDL_PIXELFORMAT_BGRX8888:
            return FMT_NE;
        case SDL_PIXELFORMAT_ARGB8888:
            return FMT_RGB32;
        case SDL_PIXELFORMAT_RGBA8888:
            return FMT_RGB32_1;
        case SDL_PIXELFORMAT_ABGR8888:
            return FMT_BGR32;
        case SDL_PIXELFORMAT_BGRA8888:
            return FMT_BGR32_1;
        case SDL_PIXELFORMAT_IYUV:
            return FMT_YUV420P;
        case SDL_PIXELFORMAT_YUY2:
            return FMT_YUYV422;
        case SDL_PIXELFORMAT_UYVY:
            return FMT_UYVY422;
        case SDL_PIXELFORMAT_UNKNOWN:
            return FMT_NONE;
        default:
            return FMT_NONE;
    }
}


