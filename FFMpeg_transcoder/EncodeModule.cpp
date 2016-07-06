#include "stdafx.h"
#include "EncodeModule.h"


EncodeModule::EncodeModule(AVFormatContext *&a, AVStream *&b)
	: m_oc(a), m_video_st(b)
{

	// decode
	m_videoFrame = NULL;
	m_encVidCodecCtx = NULL;
	m_frame = NULL;

	// encode
	//m_oc = a;
	//m_video_st = b;
	//m_encVidCodecCtx = c;
	m_encVidCodecCtx = m_video_st->codec;
}

EncodeModule::~EncodeModule(){

}


int EncodeModule::TransCode(AVCodecContext* vidCodecCtx, AVPacket* packet, int64_t &stamp, bool isTurn){

	if( !isTurn )
		return -1;
	if( m_videoFrame == NULL )
		m_videoFrame = avcodec_alloc_frame();

	int endDecoding = 0;

	avcodec_decode_video2(vidCodecCtx, m_videoFrame, &endDecoding, packet);

	if( endDecoding == 0 )
		return 0;

	static struct SwsContext* sws_ctx;
	if( sws_ctx == NULL )
		sws_ctx = sws_getContext(
		vidCodecCtx->width, vidCodecCtx->height, vidCodecCtx->pix_fmt,
		m_encVidCodecCtx->width, m_encVidCodecCtx->height, m_encVidCodecCtx->pix_fmt,
		SWS_BICUBIC, NULL, NULL, NULL);

	//encode
	AVCodecContext *c = m_video_st->codec;
	if( m_frame == NULL )
	{
		m_frame = avcodec_alloc_frame();
		avpicture_alloc(&m_dst_picture, c->pix_fmt, c->width, c->height);
		*((AVPicture *)m_frame) = m_dst_picture;
		m_frame->pts = stamp;
	}

	sws_scale(sws_ctx, m_videoFrame->data, m_videoFrame->linesize, 0,
		m_encVidCodecCtx->height, m_dst_picture.data, m_dst_picture.linesize);

	AVPacket pkt = { 0 };
	int got_packet;
	int ret = 0;
	av_init_packet(&pkt);

	//Encode the frame
	ret = avcodec_encode_video2(c, &pkt, m_frame, &got_packet);
	if (ret < 0) {
		return 0;
	}
	
	//If size of encoded frame is zero, it means the image was buffered.
	if (!ret && got_packet && pkt.size) {
		pkt.stream_index = m_video_st->index;
		
		//Write the compressed frame to the media file.
		ret = av_interleaved_write_frame(m_oc, &pkt);

		//if non-zero then it means that there was something wrong writing the frame to
		//the file
		if (ret != 0) {
			return 0;
		}
	} 
	else {
		ret = 0;
	}
	
	//Increment Frame counter
	m_frame->pts += av_rescale_q(1, m_video_st->codec->time_base, m_video_st->time_base);
	stamp = m_frame->pts;

	return 1;
}