#include <stdio.h>

extern "C"
{
#include "libavutil/opt.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
};

AVFormatContext* pFormatCtx;
AVOutputFormat* fmt;
AVStream* video_st;
AVCodecContext* pCodecCtx;
AVCodec* pCodec;
AVPacket pkt;
uint8_t* picture_buf;
AVFrame* pFrame;
int picture_size;
int y_size;
int framecnt = 0;

int Encoer_open(int in_w, int in_h)
{
	const char* out_file = "ds.ts";

	av_register_all();
	//Method1.
	pFormatCtx = avformat_alloc_context();
	//Guess Format
	fmt = av_guess_format(NULL, out_file, NULL);
	pFormatCtx->oformat = fmt;

	//Method 2.
	//avformat_alloc_output_context2(&pFormatCtx, NULL, NULL, out_file);
	//fmt = pFormatCtx->oformat;


	//Open output URL
	if (avio_open(&pFormatCtx->pb, out_file, AVIO_FLAG_READ_WRITE) < 0){
		printf("Failed to open output file! \n");
		return -1;
	}

	video_st = avformat_new_stream(pFormatCtx, 0);
	//video_st->time_base.num = 1; 
	//video_st->time_base.den = 25;  

	if (video_st == NULL){
		return -1;
	}
	//Param that must set
	pCodecCtx = video_st->codec;
	//pCodecCtx->codec_id =AV_CODEC_ID_HEVC;
	pCodecCtx->codec_id = fmt->video_codec;
	pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
	pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
	pCodecCtx->width = in_w;
	pCodecCtx->height = in_h;
	pCodecCtx->bit_rate = 400000;
	pCodecCtx->gop_size = 20;

	pCodecCtx->time_base.num = 1;
	pCodecCtx->time_base.den = 25;

	//H264
	//pCodecCtx->me_range = 16;
	//pCodecCtx->max_qdiff = 4;
	//pCodecCtx->qcompress = 0.6;
	pCodecCtx->qmin = 10;
	pCodecCtx->qmax = 50;

	//Optional Param
	pCodecCtx->max_b_frames = 3;

	// Set Option
	AVDictionary *param = 0;
	//H.264
	if (pCodecCtx->codec_id == AV_CODEC_ID_H264) {
		av_dict_set(&param, "preset", "slow", 0);
		av_dict_set(&param, "tune", "zerolatency", 0);
		//av_dict_set(&param, "profile", "main", 0);
	}

	//Show some Information
	av_dump_format(pFormatCtx, 0, out_file, 1);

	pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
	if (!pCodec){
		printf("Can not find encoder! \n");
		return -1;
	}
	if (avcodec_open2(pCodecCtx, pCodec, &param) < 0){
		printf("Failed to open encoder! \n");
		return -1;
	}


	pFrame = av_frame_alloc();
	picture_size = avpicture_get_size(pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);
	picture_buf = (uint8_t *)av_malloc(picture_size);
	avpicture_fill((AVPicture *)pFrame, picture_buf, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);

	//Write File Header
	avformat_write_header(pFormatCtx, NULL);

	av_new_packet(&pkt, picture_size);

	y_size = pCodecCtx->width * pCodecCtx->height;

	return 0;
}

int Encode_one_frame(uint8_t* Y, uint8_t* U, uint8_t* V, uint64_t i)
{
	pFrame->data[0] = Y;              // Y
	pFrame->data[1] = U;      // U 
	pFrame->data[2] = V;  // V
	//PTS
	//pFrame->pts=i;
	pFrame->pts = i*(video_st->time_base.den) / ((video_st->time_base.num) * 25);
	int got_picture = 0;
	//Encode
	int ret = avcodec_encode_video2(pCodecCtx, &pkt, pFrame, &got_picture);
	if (ret < 0){
		printf("Failed to encode! \n");
		return -1;
	}
	if (got_picture == 1){
		printf("Succeed to encode frame: %5d\tsize:%5d\n", framecnt, pkt.size);
		framecnt++;
		pkt.stream_index = video_st->index;
		ret = av_write_frame(pFormatCtx, &pkt);
		av_free_packet(&pkt);
	}
}

int Encoer_close()
{
	//Clean
	if (video_st){
		avcodec_close(video_st->codec);
		av_free(pFrame);
		av_free(picture_buf);
	}
	avio_close(pFormatCtx->pb);
	avformat_free_context(pFormatCtx);
	return 0;
}