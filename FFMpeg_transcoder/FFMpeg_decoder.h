
#pragma once

extern "C"{
#include <libavformat/avformat.h> 
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
	
#include <libavutil/dict.h>

#include <stdio.h>
};

#include "FFMPEGClass.h"
#include "EncodeModule.h"

class FFMpeg_decoder
{
public:
	FFMpeg_decoder();
	~FFMpeg_decoder();

	int OpenInputFile(const char* _fileName);
	int FindStreamInfo();
	int FindVideoStream();
	int FindAudioStream();
	int FindVideoCodec();
	int OpenVideoCodec();

	int ReadFrame(FFMPEG* encoder);
	int GetVideoSize(int &_width, int &_height);
	bool GetIsEndReadFrame();
private:
	AVFormatContext *m_pFormatCtx;
	AVInputFormat *m_pIformat;
	AVDictionary *m_pDict;
	AVCodecContext *m_pVideoCodecCtx;
	AVCodecContext *m_pAudioCodecCtx;
	AVCodec *m_pVideoCodec;
	AVCodec *m_pAudioCodec;

	int m_videoStream;
	int m_audioStream;

	bool m_isEndReadFrame;

	int m_outVideoWidth;
	int m_outVideoHeight;
	AVDictionaryEntry *m_tag;
public:
	int m_totalFrame;
	int64_t m_dur;
	double m_fps;
	int m_progress;
	void Clear();
	int m_bitrate;
	int m_gob;
};