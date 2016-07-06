


#pragma once
#define _CRT_SECURE_NO_WARNINGS

//=============================
// Includes
//-----------------------------
// FFMPEG is writen in C so we need to use extern "C"
//-----------------------------
extern "C" {
	#define INT64_C(x) (x ## LL)
	#define UINT64_C(x) (x ## ULL)
	
	#include <stdlib.h>
	#include <stdio.h>
	#include <string.h>
	#include <math.h>
	#include <libavutil/opt.h>
	#include <libavutil/mathematics.h>
	#include <libavformat/avformat.h>
	#include <libswscale/swscale.h>
	#include <libswresample/swresample.h>
	#include <libavutil/imgutils.h>
	#include <libavcodec/avcodec.h>
	
	//static int sws_flags = SWS_BICUBIC;
}


class EncodeModule
{
public:
	AVFrame* m_videoFrame;
	//encode
	AVFrame* m_frame; // �Ҵ�����
	AVPicture m_dst_picture; // �Ҵ�����

	AVFormatContext *&m_oc; // �����;���
	AVStream *&m_video_st; // �����;���
	AVCodecContext *m_encVidCodecCtx; // �����;���
public:

	EncodeModule(AVFormatContext *&, AVStream *&);
	~EncodeModule();

	int TransCode(AVCodecContext* vidCodecCtx, AVPacket* packet, int64_t &stamp, bool isTurn);

};