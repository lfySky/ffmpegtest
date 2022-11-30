//
// Created by 20608 on 2022/11/29.
//

#ifndef FFMPEG02_MP4MUXER_H
#define FFMPEG02_MP4MUXER_H

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>   
#include <libavutil/imgutils.h>    
#include <libavutil/opt.h>       
#include <libavutil/mathematics.h>     
#include <libavutil/samplefmt.h>
#include <libavutil/uuid.h>

};

class MP4Muxer
{
public:
    MP4Muxer() {}
    ~MP4Muxer() {}
    int CreateMp4(const char* filename);
    AVStream * add_stream(AVFormatContext *oc, const AVCodec **codec, enum AVCodecID codec_id, int type);
    void open_video(AVFormatContext *oc, const AVCodec *codec, AVStream *st);
    void open_audio(AVFormatContext *oc, const AVCodec *codec, AVStream *st);
    void WriteVideo(void* data, int nLen, int type);
    void CloseMp4();
	bool judgeKeyFrame(uint8_t* buf, int size);
private:
    AVFormatContext* m_pOc;
	int m_vi_nstream;
	int m_ai_nstream;
	int m_channel;
	int m_sample_rate;
	int m_bit_rate;
	int m_width;
	int m_height;
	int m_fps;
	int m_waitkey;
	int64_t cur_pts_v;
	int64_t cur_pts_a;
	int m_frame_index;
};

#endif //FFMPEG02_MP4MUXER_H
