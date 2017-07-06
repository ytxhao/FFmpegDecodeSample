#include <jni.h>
#include <string>
#include <android/log.h>


#define TAG "PLAYER-JNI"
#define ALOG(priority, tag, fmt...) \
    __android_log_print(ANDROID_##priority, tag, fmt)

#define ALOGD(...) ((void)ALOG(LOG_DEBUG, TAG, __VA_ARGS__))

#ifdef __cplusplus
extern "C" {
#endif
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>


void *startDecodeVideo(void *ptr) {

    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;
    AVFrame *pFrame, *pFrameYUV;
    AVPacket *packet;
    unsigned char *out_buffer_video;
    FILE *fp_yuv;
    int ret, got_picture;
    struct SwsContext *img_convert_ctx;
    int i, videoindex = -1;
    char *mFile = (char *) ptr;
    ALOGD("startDecodeVideo mFile=%s", mFile);
    AVFormatContext *pFormatCtx = avformat_alloc_context();
    av_register_all();
    if (avformat_open_input(&pFormatCtx, mFile, NULL, NULL) != 0) {
        ALOGD("Couldn't open input stream.\n");
        pthread_exit(NULL);
    }
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        ALOGD("Couldn't find stream information.\n");
        return 0;
    }
    videoindex = -1;
    for (i = 0; i < pFormatCtx->nb_streams; i++)
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoindex = i;
            break;
        }

    if (videoindex == -1) {
        ALOGD("Didn't find a video stream.\n");
        return NULL;
    }

    packet = (AVPacket *) av_malloc(sizeof(AVPacket));


    pCodecCtx = avcodec_alloc_context3(NULL);
    avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[videoindex]->codecpar);

    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodec == NULL) {
        ALOGD("Codec not found.\n");
        return NULL;
    }
    pCodecCtx->codec_id = pCodec->id;
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        ALOGD("Could not open codec.\n");
        return NULL;
    }

    pFrame = av_frame_alloc();
    pFrameYUV = av_frame_alloc();

    out_buffer_video = (unsigned char *) av_malloc(
            (size_t) av_image_get_buffer_size(AV_PIX_FMT_YUV420P,
                                              pCodecCtx->width,
                                              pCodecCtx->height, 1));
    /**
     * 也可以使用avpicture_fill方法替换av_image_get_buffer_size，avpicture_fill为它的简单封装
     *
     */

    /**
        int avpicture_fill(AVPicture *picture, const uint8_t *ptr,
                           enum AVPixelFormat pix_fmt, int width, int height)
        {
            return av_image_fill_arrays(picture->data, picture->linesize,
                                        ptr, pix_fmt, width, height, 1);
        }
     */

    av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, out_buffer_video,
                         AV_PIX_FMT_YUV420P, pCodecCtx->width,
                         pCodecCtx->height, 1);


    /**
     * 可以使用avpicture_get_size替换av_image_fill_arrays，avpicture_get_size是它的简单封装
     */

    /**
        int avpicture_get_size(enum AVPixelFormat pix_fmt, int width, int height)
        {
            return av_image_get_buffer_size(pix_fmt, width, height, 1);
        }
     */

    fp_yuv = fopen("/storage/emulated/0/output.yuv", "wb+");
    ALOGD("Decode width=%d height=%d pix_fmt=%d", pCodecCtx->width, pCodecCtx->height,
          pCodecCtx->pix_fmt);
    img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
                                     pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P,
                                     SWS_BICUBIC, NULL, NULL, NULL);

    i = 0;
    while (av_read_frame(pFormatCtx, packet) >= 0) {
        if (packet->stream_index == videoindex) {
            //Decode
            ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
            if (ret < 0) {
                ALOGD("Decode Error.\n");
                return NULL;
            }
            if (got_picture) {

                sws_scale(img_convert_ctx, (const uint8_t *const *) pFrame->data, pFrame->linesize,
                          0,
                          pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);

                ALOGD("Decode 第%d帧", i++);
                int y_size = pCodecCtx->width * pCodecCtx->height;
                fwrite(pFrameYUV->data[0], 1, y_size, fp_yuv);    //Y
                fwrite(pFrameYUV->data[1], 1, y_size / 4, fp_yuv);  //U
                fwrite(pFrameYUV->data[2], 1, y_size / 4, fp_yuv);  //V
            }
        }
        av_packet_unref(packet);
    }
    sws_freeContext(img_convert_ctx);
    fclose(fp_yuv);
    av_free(pFrameYUV);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);
    ALOGD("Decode end");
    return NULL;
}


JNIEXPORT void JNICALL
Java_ican_ytx_com_ffmpegdecodesample_MainActivity_startDecodeVideo(
        JNIEnv *env,
        jobject /* this */,
        jobject assetMgr, jstring filename) {
    const char *utf8 = env->GetStringUTFChars(filename, NULL);
    char *mFile = (char *) calloc(strlen(utf8) + 1, sizeof(char *));
    memcpy(mFile, utf8, strlen(utf8));
    pthread_t mPlayer;
    pthread_create(&mPlayer, NULL, startDecodeVideo, mFile);
}

#ifdef __cplusplus
}
#endif
