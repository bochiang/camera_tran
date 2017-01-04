#ifndef CAMERA_H
#define CAMERA_H

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
#include "SDL/SDL.h"
};

typedef struct {
	AVFormatContext	   *pFormatCtx;
	AVCodec              *pCodec;
	AVCodecContext       *pCodecCtx;

	AVFrame	             *pFrame;
	AVPacket             *packet;

	int                codec_id;

	int                out_img_w;       // output image width
	int                out_img_h;       // output image height
	int                out_y_size;      // output luma size
	int                videoindex;

	struct SwsContext *img_convert_ctx;
	AVFrame	          *pFrameYUV;
} xv_video_dec_t;

void show_dshow_device();
void show_dshow_device_option();
void show_vfw_device();
void show_avfoundation_device();
xv_video_dec_t *xavcodec_dec_video_create();
int xavcodec_dec_video_close(xv_video_dec_t *h);

#endif