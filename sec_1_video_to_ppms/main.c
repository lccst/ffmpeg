#include <stdio.h>
#include <assert.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

void save_frame(AVFrame *pframe, int width, int height, int iframe)
{
    char filename[32];
    int y;

    sprintf(filename, "frame%d.ppm", iframe);
    FILE *fp = fopen(filename, "w+");
    assert(fp!=NULL);

    fprintf(fp, "P6\n%d %d\n255\n", width, height); // header

    for (y=0; y<height; y++)
        fwrite(pframe->data[0]+y*pframe->linesize[0], 1, width*3, fp);
    fclose(fp);
}

int main(int argc, char *argv[])
{
    if (argc < 2){
        printf("usage:\n\t %s filename\n", argv[0]);
        return -1;
    }

    av_register_all();

    AVFormatContext *pctx = NULL;
    if (avformat_open_input(&pctx, argv[1], NULL, NULL)!=0) {
        return -1;
    }
    av_dump_format(pctx, 0, argv[1], 0);
    assert(avformat_find_stream_info(pctx, NULL)>=0);

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
    AVFrame *pframe_rgb = av_frame_alloc();
    assert(pframe && pframe_rgb);

    int num_bytes = avpicture_get_size(AV_PIX_FMT_RGB24, 
                        pcodec_ctx->width, pcodec_ctx->height);
    uint8_t *buffer = av_malloc(num_bytes * sizeof(uint8_t));

    avpicture_fill((AVPicture *)pframe_rgb, buffer, AV_PIX_FMT_RGB24,
                        pcodec_ctx->width, pcodec_ctx->height);

    
    int frame_finished;
    AVPacket pkt;
    struct SwsContext *sws_ctx = sws_getContext(
        pcodec_ctx->width,
        pcodec_ctx->height,
        pcodec_ctx->pix_fmt,
        pcodec_ctx->width,
        pcodec_ctx->height,
        AV_PIX_FMT_RGB24,
        SWS_BILINEAR,
        NULL,
        NULL,
        NULL
    );
    i = 0;
    while (av_read_frame(pctx, &pkt) >= 0) {
        if (pkt.stream_index != video_stream) {
            continue;
        }
        avcodec_decode_video2(pcodec_ctx, pframe, &frame_finished, &pkt);
        if (!frame_finished)
            continue;
         
        sws_scale(sws_ctx, pframe->data, pframe->linesize,
            0, pcodec_ctx->height, pframe_rgb->data, pframe_rgb->linesize);
        if (++i<=5) {
            save_frame(pframe_rgb, pcodec_ctx->width, pcodec_ctx->height,i);
        }
        
    }
    av_free_packet(&pkt);


    // free the RGB image
    av_free(buffer);
    av_free(pframe_rgb);
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
