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
static struct SwsContext *img_convert_ctx = NULL; // ���� ���� ������ ���� sw converter ����

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
	// ��� ���� ���˰� �ڵ��� ���̺귯���� �Բ� ����ϰ�,
	// �̰͵��� �����Ǵ� �����̳� �ڵ��� ������ �ڵ����� ���ȴ�.
	// av_register_all() �� �� �ѹ��� ȣ���ؾ� �Ѵ�.

	//av_register_all(); // FFmpeg�� ��� �ڵ��� ����Ѵ�.
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
	// ������ ������ ����.
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
	// open�� ������ ������ ��Ʈ�� ������ �����´�.
	if( av_find_stream_info(m_pFormatCtx) < 0 )
	{
		printf("fail av_find_stream_info \n");
		return -1; // ��Ʈ�� ������ �������� ���� ���
	}

	return 1;
}

int FFMpeg_decoder::FindVideoStream()
{
	// ���� �ҽ��κ��� ���� ��Ʈ���� ã�´�.
	m_videoStream = -1;

	for(int i=0; i<m_pFormatCtx->nb_streams; i++)
	{
		if( m_pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO && m_videoStream < 0 )
			m_videoStream = i;
	}

	if( m_videoStream == -1 )
	{
		printf("No videoStream \n");
		return -1; // ���� ��Ʈ���� ���� ���
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
	// ���� �ҽ��κ��� ����� ��Ʈ���� ã�´�.
	m_audioStream = -1;

	for(int i=0; i<m_pFormatCtx->nb_streams; i++)
	{
		if( m_pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO && m_audioStream < 0 )
			m_audioStream = i;
	}

	if( m_audioStream == -1 )
	{
		printf("No audioStream \n");
		return -1; // ����� ��Ʈ���� ���� ���
	}

	return 1;
}

int FFMpeg_decoder::FindVideoCodec()
{
	// ������ ��Ʈ�� �߿� ���� ��Ʈ���� �ڵ� ������ �����´�.
	// Get a pointer to the codec context for the video stream
	m_pVideoCodecCtx = m_pFormatCtx->streams[m_videoStream]->codec;

	// ���� ��Ʈ���� �ڵ� id�� ���� ���ڴ��� ã�ƿ´�.
	m_pVideoCodec = avcodec_find_decoder(m_pVideoCodecCtx->codec_id);
	if( m_pVideoCodec == NULL )
	{
		fprintf(stderr, "Unsupported codec!\n");
		return -1; // �ڵ��� ã�� ���� ���
	}

	return 1;
}

int FFMpeg_decoder::OpenVideoCodec()
{
	// ���� �ڵ��� ����.
	if( avcodec_open2(m_pVideoCodecCtx, m_pVideoCodec, NULL) < 0 )
		return -1; // �ڵ��� ���� ���� ���

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

	// ���� avpicture_fill()�� �̿��ؼ� Frame�� �츮�� ���� �Ҵ��� ���۸� �����մϴ�.
	// AVPicture cast : AVPicture struct �� AVFrame struct�� ���� ����Դϴ�. - AVFrame struct�� ���ۺκ���
	// AVPicture struct �� �����մϴ�
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
				// ����Ʈ���� �����Ϸ��� YUV420P Ÿ������ ��ȯ�Ѵ�.
				// �ֳ��ϸ� �ȵ���̵� ���� ������ YUV420P�� �߱� �����̴�.
				// ���� �ٸ� �������� �ߴٸ� �ű⿡ �����.
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
				// openCV�� �̿��ؼ� ���ڵ� �Ϸ�� frame data(BRG ����)�� ȭ�鿡 ����Ѵ�.
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
