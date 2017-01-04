#include <stdio.h>
#include"camera.h"
#include"Encoder.h"
//Output YUV420P 
#define OUTPUT_YUV420P 1


//Refresh Event
#define SFM_REFRESH_EVENT  (SDL_USEREVENT + 1)

#define SFM_BREAK_EVENT  (SDL_USEREVENT + 2)

int thread_exit = 0;

int sfp_refresh_thread(void *opaque)
{
	thread_exit = 0;
	while (!thread_exit) {
		SDL_Event event;
		event.type = SFM_REFRESH_EVENT;
		SDL_PushEvent(&event);
		SDL_Delay(40);
	}
	thread_exit = 0;
	//Break
	SDL_Event event;
	event.type = SFM_BREAK_EVENT;
	SDL_PushEvent(&event);

	return 0;
}


int main(int argc, char* argv[])
{
	xv_video_dec_t *h;
	h = xavcodec_dec_video_create();
	if (h == NULL)
		return -1;
	
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
		printf("Could not initialize SDL - %s\n", SDL_GetError());
		return -1;
	}
	int screen_w = 0, screen_h = 0;
	SDL_Surface *screen;
	screen_w = h->out_img_w;
	screen_h = h->out_img_h;
	screen = SDL_SetVideoMode(screen_w, screen_h, 0, 0);

	if (!screen) {
		printf("SDL: could not set video mode - exiting:%s\n", SDL_GetError());
		return -1;
	}
	SDL_Overlay *bmp;
	bmp = SDL_CreateYUVOverlay(h->out_img_w, h->out_img_h, SDL_YV12_OVERLAY, screen);
	SDL_Rect rect;
	rect.x = 0;
	rect.y = 0;
	rect.w = screen_w;
	rect.h = screen_h;
	int ret, got_picture;

	AVPacket *packet = (AVPacket *)av_malloc(sizeof(AVPacket));

#if OUTPUT_YUV420P 
	FILE *fp_yuv = fopen("output.yuv", "wb+");
#endif  

	SDL_Thread *video_tid = SDL_CreateThread(sfp_refresh_thread, NULL);
	//
	SDL_WM_SetCaption("Simplest FFmpeg Read Camera", NULL);
	//Event Loop
	SDL_Event event;
	Encoer_open(screen_w, screen_h);
	int ii = 0;
	while (1) {
		//Wait
		SDL_WaitEvent(&event);
		if (event.type == SFM_REFRESH_EVENT){
			//------------------------------
			if (av_read_frame(h->pFormatCtx, h->packet) >= 0){
				if (h->packet->stream_index == h->videoindex){
					ret = avcodec_decode_video2(h->pCodecCtx, h->pFrame, &got_picture, h->packet);
					if (ret < 0){
						printf("Decode Error.\n");
						return -1;
					}
					if (got_picture){
						SDL_LockYUVOverlay(bmp);
						h->pFrameYUV->data[0] = bmp->pixels[0];
						h->pFrameYUV->data[1] = bmp->pixels[2];
						h->pFrameYUV->data[2] = bmp->pixels[1];
						h->pFrameYUV->linesize[0] = bmp->pitches[0];
						h->pFrameYUV->linesize[1] = bmp->pitches[2];
						h->pFrameYUV->linesize[2] = bmp->pitches[1];
						sws_scale(h->img_convert_ctx, (const unsigned char* const*)h->pFrame->data, h->pFrame->linesize, 0, h->pCodecCtx->height, h->pFrameYUV->data, h->pFrameYUV->linesize);

#if OUTPUT_YUV420P  
						int y_size = h->pCodecCtx->width*h->pCodecCtx->height;
						fwrite(h->pFrameYUV->data[0], 1, y_size, fp_yuv);    //Y   
						fwrite(h->pFrameYUV->data[1], 1, y_size / 4, fp_yuv);  //U  
						fwrite(h->pFrameYUV->data[2], 1, y_size / 4, fp_yuv);  //V  
#endif  

						SDL_UnlockYUVOverlay(bmp);

						SDL_DisplayYUVOverlay(bmp, &rect);
						Encode_one_frame(h->pFrameYUV->data[0], h->pFrameYUV->data[1], h->pFrameYUV->data[2], ii);
						//printf("%d\n", ii);
						ii++;
					}
				}
				av_free_packet(h->packet);
			}
			else{
				//Exit Thread
				thread_exit = 1;
			}
		}
		else if (event.type == SDL_QUIT){
			thread_exit = 1;
		}
		else if (event.type == SFM_BREAK_EVENT){
			break;
		}

	}

#if OUTPUT_YUV420P 
	fclose(fp_yuv);
#endif 

	SDL_Quit();

	xavcodec_dec_video_close(h);

	return 0;
}

