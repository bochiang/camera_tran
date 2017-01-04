#include"camera.h"

//Show Dshow Device
void show_dshow_device(){
	AVFormatContext *pFormatCtx = avformat_alloc_context();
	AVDictionary* options = NULL;
	av_dict_set(&options, "list_devices", "true", 0);
	AVInputFormat *iformat = av_find_input_format("dshow");
	printf("========Device Info=============\n");
	avformat_open_input(&pFormatCtx, "video=dummy", iformat, &options);
	printf("================================\n");
}

//Show Dshow Device Option
void show_dshow_device_option(){
	AVFormatContext *pFormatCtx = avformat_alloc_context();
	AVDictionary* options = NULL;
	av_dict_set(&options, "list_options", "true", 0);
	AVInputFormat *iformat = av_find_input_format("dshow");
	printf("========Device Option Info======\n");
	avformat_open_input(&pFormatCtx, "video=Integrated Camera", iformat, &options);
	printf("================================\n");
}

//Show VFW Device
void show_vfw_device(){
	AVFormatContext *pFormatCtx = avformat_alloc_context();
	AVInputFormat *iformat = av_find_input_format("vfwcap");
	printf("========VFW Device Info======\n");
	avformat_open_input(&pFormatCtx, "list", iformat, NULL);
	printf("=============================\n");
}

//Show AVFoundation Device
void show_avfoundation_device(){
	AVFormatContext *pFormatCtx = avformat_alloc_context();
	AVDictionary* options = NULL;
	av_dict_set(&options, "list_devices", "true", 0);
	AVInputFormat *iformat = av_find_input_format("avfoundation");
	printf("==AVFoundation Device Info===\n");
	avformat_open_input(&pFormatCtx, "", iformat, &options);
	printf("=============================\n");
}

xv_video_dec_t *xavcodec_dec_video_create()
{
	int i;

	av_register_all();
	avformat_network_init();
	avdevice_register_all();

	//Show Dshow Device
	show_dshow_device();
	//Show Device Options
	show_dshow_device_option();
	//Show VFW Options
	show_vfw_device();

	AVInputFormat *ifmt = av_find_input_format("vfwcap");

	xv_video_dec_t *h = (xv_video_dec_t *)malloc(sizeof(xv_video_dec_t));

	memset(h, 0, sizeof(xv_video_dec_t));

	h->pFormatCtx = avformat_alloc_context();

	if (avformat_open_input(&h->pFormatCtx, "0", ifmt, NULL) != 0){
		printf("Couldn't open input stream.\n");
		return NULL;
	}

	if (avformat_find_stream_info(h->pFormatCtx, NULL)<0)
	{
		printf("Couldn't find stream information.\n");
		return NULL;
	}
	h->videoindex = -1;
	for (i = 0; i<h->pFormatCtx->nb_streams; i++)
		if (h->pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			h->videoindex = i;
			break;
		}
	if (h->videoindex == -1)
	{
		printf("Couldn't find a video stream.\n");
		return NULL;
	}

	h->pCodecCtx = h->pFormatCtx->streams[h->videoindex]->codec;
	h->codec_id = h->pCodecCtx->codec_id;
	h->out_img_w = h->pCodecCtx->width;
	h->out_img_h = h->pCodecCtx->height;
	h->out_y_size = h->out_img_w * h->out_img_h;

	h->pCodec = avcodec_find_decoder((AVCodecID)h->codec_id);
	if (h->pCodec == NULL)
	{
		printf("Codec not found.\n");
		return NULL;
	}
	if (avcodec_open2(h->pCodecCtx, h->pCodec, NULL)<0)
	{
		printf("Could not open codec.\n");
		return NULL;
	}

	h->pFrame = av_frame_alloc();
	h->pFrameYUV = av_frame_alloc();

	h->packet = (AVPacket *)av_malloc(sizeof(AVPacket));

	h->img_convert_ctx = sws_getContext(h->out_img_w, h->out_img_h, h->pCodecCtx->pix_fmt, h->out_img_w, h->out_img_h, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

	return h;

failure_dec_video_create:
	return NULL;
}

int xavcodec_dec_video_close(xv_video_dec_t *h)
{
	avformat_close_input(&h->pFormatCtx);
	sws_freeContext(h->img_convert_ctx);
	av_frame_free(&h->pFrameYUV);

	av_frame_free(&h->pFrame);
	avcodec_close(h->pCodecCtx);
	av_free(h->pCodecCtx);

	free(h);

	return 0;
}