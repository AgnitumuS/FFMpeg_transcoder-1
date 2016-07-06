#include "stdafx.h"
#include "FFMpeg_decoder.h"

// ffmpeg Lib
/*
#pragma comment(lib,"avformat.lib")
#pragma comment(lib,"avcodec.lib")
#pragma comment(lib,"swresample.lib")
#pragma comment(lib,"swscale.lib")
#pragma comment(lib,"avutil.lib")
*/
#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000
static struct SwsContext *img_convert_ctx = NULL; // 비디오 포맷 변경을 위한 sw converter 세팅

void FFMpeg_decoder::Clear(){
	if(m_pFormatCtx)
	{
		avformat_free_context(m_pFormatCtx);
		m_pFormatCtx = NULL;
	}
		//avformat_close_input(&m_pFormatCtx);
	//av_free(m_pIformat);
	//av_free(m_pDict);
	//av_free(m_pVideoCodecCtx);
	//av_free(m_pAudioCodecCtx);
	//av_free(m_pVideoCodec);
	//av_free(m_pAudioCodec);
}

FFMpeg_decoder::FFMpeg_decoder()
{
	// 모든 파일 포맷과 코덱을 라이브러리와 함께 등록하고,
	// 이것들은 대응되는 포맷이나 코덱이 열릴때 자동으로 사용된다.
	// av_register_all() 는 딱 한번만 호출해야 한다.

	//av_register_all(); // FFmpeg의 모든 코덱을 등록한다.
	//avcodec_register_all();

	m_pFormatCtx = NULL;
	m_pIformat = NULL;
	m_pDict = NULL;

	m_pVideoCodecCtx = NULL;
	m_pAudioCodecCtx = NULL;
	m_pVideoCodec = NULL;
	m_pAudioCodec = NULL;

	m_isEndReadFrame = false;

	m_outVideoWidth = 0;
	m_outVideoHeight = 0;

	m_progress = 0;
}

int FFMpeg_decoder::OpenInputFile(const char* _fileName)
{
	// 동영상 파일을 연다.
	if( avformat_open_input(&m_pFormatCtx, _fileName, m_pIformat, &m_pDict) != 0 )
	{
		printf("Couldn't open file : %s \n", _fileName);
		return -1;
	}
    //avformat_close_input(&fmt_ctx);

	return 1;
}

int FFMpeg_decoder::FindStreamInfo()
{
	// open한 동영상 파일의 스트림 정보를 가져온다.
	if( av_find_stream_info(m_pFormatCtx) < 0 )
	{
		printf("fail av_find_stream_info \n");
		return -1; // 스트림 정보를 가져오지 못한 경우
	}

	return 1;
}

int FFMpeg_decoder::FindVideoStream()
{
	// 비디오 소스로부터 비디오 스트림을 찾는다.
	m_videoStream = -1;

	for(int i=0; i<m_pFormatCtx->nb_streams; i++)
	{
		if( m_pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO && m_videoStream < 0 )
			m_videoStream = i;
	}

	if( m_videoStream == -1 )
	{
		printf("No videoStream \n");
		return -1; // 비디오 스트림이 없을 경우
	}

	int64_t dur;
	double fps;
	int bitrate = m_pFormatCtx->streams[m_videoStream]->codec->bit_rate;
	int totalframe = 0;
	dur = m_pFormatCtx->duration;
	fps = m_pFormatCtx->streams[m_videoStream]->r_frame_rate.num / (double)m_pFormatCtx->streams[m_videoStream]->r_frame_rate.den;
	//fps = av_q2d(m_pFormatCtx->streams[m_videoStream]->avg_frame_rate);
	totalframe = m_pFormatCtx->streams[m_videoStream]->nb_frames;
	if( totalframe == 0 )
		totalframe = (int)((dur / (double)AV_TIME_BASE) * fps + 0.5);
	m_totalFrame = totalframe;
	m_fps = fps;
	m_dur = dur;
	m_bitrate = bitrate;
	m_gob = (int)fps;
	return 1;
}

int FFMpeg_decoder::FindAudioStream()
{
	// 비디오 소스로부터 오디오 스트림을 찾는다.
	m_audioStream = -1;

	for(int i=0; i<m_pFormatCtx->nb_streams; i++)
	{
		if( m_pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO && m_audioStream < 0 )
			m_audioStream = i;
	}

	if( m_audioStream == -1 )
	{
		printf("No audioStream \n");
		return -1; // 오디오 스트림이 없을 경우
	}

	return 1;
}

int FFMpeg_decoder::FindVideoCodec()
{
	// 오픈한 스트림 중에 비디오 스트림의 코덱 정보를 가져온다.
	// Get a pointer to the codec context for the video stream
	m_pVideoCodecCtx = m_pFormatCtx->streams[m_videoStream]->codec;

	// 비디오 스트림의 코덱 id로 비디오 디코더를 찾아온다.
	m_pVideoCodec = avcodec_find_decoder(m_pVideoCodecCtx->codec_id);
	if( m_pVideoCodec == NULL )
	{
		fprintf(stderr, "Unsupported codec!\n");
		return -1; // 코덱을 찾지 못한 경우
	}

	return 1;
}

int FFMpeg_decoder::OpenVideoCodec()
{
	// 비디오 코덱을 연다.
	if( avcodec_open2(m_pVideoCodecCtx, m_pVideoCodec, NULL) < 0 )
		return -1; // 코덱을 열지 못한 경우

	return 1;
}

int FFMpeg_decoder::ReadFrame(FFMPEG* encoder)
{
	AVFrame *pVideoFrame;
	AVFrame *pFrameRGB;

	// Allocate video frame
	pVideoFrame = avcodec_alloc_frame();

	// Allocate an AVFrame structure
	pFrameRGB = avcodec_alloc_frame();
	if( pFrameRGB == NULL )
	{
		printf("pFrameRGB is NULL\n");
		return -1;
	}

	uint8_t *buffer;
	int numBytes;
	// Determine required buffer size and allocate buffer 
	numBytes=avpicture_get_size(PIX_FMT_RGB24, m_pVideoCodecCtx->width, m_pVideoCodecCtx->height);   
	buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));

	// 이제 avpicture_fill()을 이용해서 Frame과 우리가 새로 할당한 버퍼를 연결합니다.
	// AVPicture cast : AVPicture struct 는 AVFrame struct의 작은 덩어리입니다. - AVFrame struct의 시작부분은
	// AVPicture struct 와 동일합니다
	// Assign appropriate parts of buffer to image planes in pFrameRGB   
	// Note that pFrameRGB is an AVFrame, but AVFrame is a superset   
	// of AVPicture   
	//avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_RGB24, m_pVideoCodecCtx->width, m_pVideoCodecCtx->height);
	avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_RGB24, m_outVideoWidth, m_outVideoHeight);

	AVPacket packet;   
	int i = 0;
	int endDecoding = 0;

	int dstW = m_outVideoWidth;//m_pVideoCodecCtx->width;
	int dstH = m_outVideoHeight;//m_pVideoCodecCtx->height;

	m_progress = 0;
	EncodeModule e(encoder->m_oc, encoder->m_video_st);
	EncodeModule e2(encoder->m_oc, encoder->m_video_st);

	int idx = 0;
	int64_t stamp = 0;
	bool xx = true;
	int ret = 0;
	while(av_read_frame(m_pFormatCtx, &packet)>=0)
	{   
		// Is this a packet from the video stream?   
		if( packet.stream_index == m_videoStream )
		{   
			m_progress++;
			e.TransCode(m_pVideoCodecCtx, &packet, stamp, xx);
			e2.TransCode(m_pVideoCodecCtx, &packet, stamp, !xx);

			if(i < m_fps || 1)
				xx = xx;
			else if(i < m_fps*2 )
				xx = !xx;
			else
				i = -1;
			i++;
			continue;
			/////

			// Decode video frame  
			avcodec_decode_video2(m_pVideoCodecCtx, pVideoFrame, &endDecoding, &packet);

			// Did we get a video frame?   
			if(endDecoding)
			{
				// 소프트웨어 스케일러로 YUV420P 타입으로 변환한다.
				// 왜냐하면 안드로이드 비디오 설정을 YUV420P로 했기 때문이다.
				// 만일 다른 포맷으로 했다면 거기에 맞춘다.
				if( img_convert_ctx == NULL )
				{
					// dstW = VideoCodecCtx->width;
					// dstH = VideoCodecCtx->height;
					img_convert_ctx = sws_getContext(m_pVideoCodecCtx->width, m_pVideoCodecCtx->height, m_pVideoCodecCtx->pix_fmt,
						dstW, dstH, AV_PIX_FMT_BGR24, SWS_BICUBIC, NULL, NULL, NULL);

					if( img_convert_ctx == NULL )
					{
						printf("Cannot initialize the conversion context!\n");
						exit(1);
					}
				}
				
				sws_scale(img_convert_ctx, pVideoFrame->data, pVideoFrame->linesize, 0, m_pVideoCodecCtx->height,
					pFrameRGB->data, pFrameRGB->linesize);

				if( encoder )
					encoder->WriteFrame((char*)pFrameRGB->data[0]);
				//sws_scale(img_convert_ctx, pVideoFrame->data, pVideoFrame->linesize, 0, m_pVideoCodecCtx->height,
				//	pFrameRGB->data, pFrameRGB->linesize);
				
				//uint8_t *imgBuf = (uint8_t *)av_malloc(numBytes*sizeof(uint8_t));
				//memcpy(imgBuf, pFrameRGB->data[0], dstW * dstH * 3);

				//m_videoList.AddTail(imgBuf);
				//if( m_videoList.GetSize() % 100 == 0 )
				//	AfxMessageBox("d");
				/*
				// openCV를 이용해서 디코딩 완료된 frame data(BRG 버퍼)를 화면에 출력한다.
				image = cvCreateImage(cvSize(dstW, dstH), IPL_DEPTH_8U, 3);
				unsigned char *imgBuf = (unsigned char*)image->imageData;
				unsigned char *bgrBuf = (unsigned char*)dst->data[0];
				memcpy(imgBuf, dst->data[0], dstW * dstH * 3);

				cvShowImage("frame", image);
				cv::waitKey(fps);
				cvReleaseImage(&image);
				*/
			}
		}   

		// Free the packet that was allocated by av_read_frame   
		av_free_packet(&packet);   
	} 
	
	//AfxMessageBox("End MyFFmpeg::ReadFrame()");
	m_isEndReadFrame = true;

	// Free the RGB image   
	av_free(buffer);   
	av_free(pFrameRGB);   

	// Free the YUV frame   
	av_free(pVideoFrame);   

	// Close the codec   
	avcodec_close(m_pVideoCodecCtx);   

	// Close the video file   
	av_close_input_file(m_pFormatCtx); 

	return 1;
}

FFMpeg_decoder::~FFMpeg_decoder()
{
}

int FFMpeg_decoder::GetVideoSize(int &_width, int &_height)
{
	if( m_pVideoCodecCtx == NULL )
		return -1;

	m_outVideoWidth = m_pVideoCodecCtx->width;
	m_outVideoHeight = m_pVideoCodecCtx->height;

	_width = m_outVideoWidth;	//m_pVideoCodecCtx->width;
	_height = m_outVideoHeight;	//m_pVideoCodecCtx->height;
	return 1;
}

bool FFMpeg_decoder::GetIsEndReadFrame()
{
	return m_isEndReadFrame;
}
