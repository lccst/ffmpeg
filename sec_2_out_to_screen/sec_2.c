#include "../common/lcc_common.h"

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#include <SDL.h>
#include <SDL_thread.h>

void init_display()
{
    int ret = SDL_Init(SDL_INIT_AUDIO |SDL_INIT_VIDEO | SDL_INIT_TIMER);
    CHECK(0==ret, "sdl init failed, %s\n", SDL_GetError()); 
}

SDL_Overlay *create_overlay(const int width, const int height)
{
    /** first, create a screen display */
    SDL_Surface *p = SDL_SetVideoMode(width, height, 0, 0);
    CHECK(NULL!=p, "create SDL display failed\n");
    /** create overlay */
    SDL_Overlay *bmp = SDL_CreateYUVOverlay(width, height, 
            SDL_YV12_OVERLAY, p);
    CHECK(NULL!=bmp, "create overlay failed\n");

    return bmp;
}

int main(int argc, char *argv[])
{
    if (argc < 2){
        printf("usage:\n\t %s filename\n", argv[0]);
        return -1;
    }

    av_register_all();
    init_display();

    AVFormatContext *pctx = NULL;
    if (avformat_open_input(&pctx, argv[1], NULL, NULL)!=0) {
        return -1;
    }
    av_dump_format(pctx, 0, argv[1], 0);
    CHECK(avformat_find_stream_info(pctx, NULL)>=0, " ");

    int i, video_stream = -1;
    for (i=0; i<pctx->nb_streams; i++) {
        if (pctx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
            video_stream =i;
            break;
        }
    }
    if (-1==video_stream) {
        printf("no video stream detected\n");
        return -1;
    }

    AVCodecContext *pcodec_ctx_orig = NULL, *pcodec_ctx = NULL;
    pcodec_ctx = pctx->streams[video_stream]->codec;
    
    AVCodec *pcodec = NULL;
    pcodec = avcodec_find_decoder(pcodec_ctx->codec_id);
    if (NULL == pcodec) {
        printf("unsupported codec.\n");
        return -1;
    }

    pcodec_ctx_orig = avcodec_alloc_context3(pcodec);
    if (avcodec_copy_context(pcodec_ctx_orig, pcodec_ctx) != 0) {
        printf("couldn't copy codec context\n");
        return -1;
    }

    if (avcodec_open2(pcodec_ctx, pcodec, NULL) < 0) {
        printf("couldn't open codec\n");
        return -1;
    }


    AVFrame *pframe = av_frame_alloc();
    CHECK(pframe, " ");

    int num_bytes = avpicture_get_size(AV_PIX_FMT_RGB24, 
                        pcodec_ctx->width, pcodec_ctx->height);
    uint8_t *buffer = av_malloc(num_bytes * sizeof(uint8_t));

    SDL_Overlay *overlay = create_overlay(pcodec_ctx->width, pcodec_ctx->height);
    CHECK(overlay, "create overlay failed\n");
    AVPicture pict;
    pict.data[0] = overlay->pixels[0];
    pict.data[1] = overlay->pixels[2];
    pict.data[2] = overlay->pixels[1];
    pict.linesize[0] = overlay->pitches[0];
    pict.linesize[1] = overlay->pitches[2];
    pict.linesize[2] = overlay->pitches[1];

    SDL_Rect play_rect;
    play_rect.x = 0;
    play_rect.y = 0;
    play_rect.w = pcodec_ctx->width;
    play_rect.h = pcodec_ctx->height;
    
    int frame_finished;
    AVPacket pkt;
    struct SwsContext *sws_ctx = sws_getContext(
        pcodec_ctx->width,
        pcodec_ctx->height,
        pcodec_ctx->pix_fmt,
        pcodec_ctx->width,
        pcodec_ctx->height,
        AV_PIX_FMT_YUV420P,
        SWS_BILINEAR,
        NULL,
        NULL,
        NULL
    );

    while (av_read_frame(pctx, &pkt) >= 0) {
        if (pkt.stream_index != video_stream) {
            continue;
        }
        avcodec_decode_video2(pcodec_ctx, pframe, &frame_finished, &pkt);
        if (!frame_finished)
            continue;
        
        SDL_LockYUVOverlay(overlay);
        sws_scale(sws_ctx, pframe->data, pframe->linesize,
            0, pcodec_ctx->height, pict.data, pict.linesize);
        SDL_UnlockYUVOverlay(overlay);
        SDL_DisplayYUVOverlay(overlay, &play_rect);
    }
    av_free_packet(&pkt);

    // free the buffer
    av_free(buffer);
    // free raw frame
    av_free(pframe);
    // close codecs
    avcodec_close(pcodec_ctx);
    avcodec_close(pcodec_ctx_orig);
    // close video file
    avformat_close_input(&pctx);

    printf("finished\n");
    return 0;
}
