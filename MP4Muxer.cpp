//
// Created by 20608 on 2022/11/29.
//
#include "MP4Muxer.h"

int MP4Muxer::CreateMp4(const char* filename) {
    int ret; // 成功返回0，失败返回1
    const char *pszFileName = filename;
    const AVOutputFormat *fmt;
    const AVCodec *video_codec;
    AVStream *m_pVideoSt;

    const AVCodec *audio_codec;
    AVStream *m_pAudioSt;

    //av_register_all();
    avformat_alloc_output_context2(&m_pOc, NULL, NULL, pszFileName);
    if (!m_pOc) {
        printf("Could not deduce output format from file extension: using MPEG. \n");
        avformat_alloc_output_context2(&m_pOc, NULL, "mpeg", pszFileName);
    }
    if (!m_pOc) {
        return 1;
    }
    fmt = m_pOc->oformat;
    if (fmt->video_codec != AV_CODEC_ID_NONE) {
        // 添加视频和音频流信息
        m_pVideoSt = add_stream(m_pOc, &video_codec, AV_CODEC_ID_H264, 0);
        m_pAudioSt = add_stream(m_pOc, &audio_codec, AV_CODEC_ID_AAC, 1);
    }

    // 打开视音频流
    if (m_pAudioSt) {
        open_audio(m_pOc, audio_codec, m_pAudioSt);
    }

    if (m_pVideoSt) {
        open_video(m_pOc, video_codec, m_pVideoSt);
    }


    printf("==========Output Information==========\n");
    av_dump_format(m_pOc, 0, pszFileName, 1);
    printf("======================================\n");
    /* open the output file, if needed */
    if (!(fmt->flags & AVFMT_NOFILE)) {
        // 打开输出流
        ret = avio_open(&m_pOc->pb, pszFileName, AVIO_FLAG_WRITE);
        if (ret < 0) {
            printf("could not open %s\n", pszFileName);
            return 1;
        }
    }
    /* Write the stream header, if any */
    ret = avformat_write_header(m_pOc, NULL);
    if (ret < 0) {
        printf("Error occurred when opening output file");
        return 1;
    }
    return 0;
}

bool isIdrFrame(uint8_t* buf, int len) {
    switch (buf[0] & 0x1f) {
        case 7: // SPS
            return true;
        case 8: // PPS
            return true;
        case 5:
            return true;
        case 1:
            return false;

        default:
            return false;
            break;
    }

    return false;
}

// 判断关键帧
bool MP4Muxer::judgeKeyFrame(uint8_t* buf, int size) {
    //主要是解析idr前面的sps pps
    int last = 0;
    for (int i = 2; i <= size; ++i) {
        if (i == size) {
            if (last) {
                bool ret = isIdrFrame(buf + last, i - last);
                if (ret) {
                    return true;
                }
            }
        }
        else if (buf[i - 2] == 0x00 && buf[i - 1] == 0x00 && buf[i] == 0x01) {
            if (last) {
                int size = i - last - 3;
                if (buf[i - 3]) ++size;
                bool ret = isIdrFrame(buf + last, size);
                if (ret) {
                    return true;
                }
            }
            last = i + 1;
        }
    }
    return false;

}

// 输出流中添加视音频流
AVStream * MP4Muxer::add_stream(AVFormatContext *oc, const AVCodec **codec, enum AVCodecID codec_id, int type)
{
    AVCodecContext *c;
    AVStream *st;
    /* find the encoder */
    *codec = avcodec_find_encoder(codec_id);
    if (!*codec)
    {
        printf("could not find encoder for '%s' \n", avcodec_get_name(codec_id));
        exit(1);
    }
    st = avformat_new_stream(oc, *codec);
    if (!st)
    {
        printf("could not allocate stream \n");
        exit(1);
    }
    st->id = oc->nb_streams - 1;
    c = avcodec_alloc_context3(*codec);
    //事实上codecpar包含了大部分解码器相关的信息，这里是直接从AVCodecParameters复制到AVCodecContext
    avcodec_parameters_to_context(c, st->codecpar);
    //c = st->codec;

    if (type == 0)
        m_vi_nstream = st->index;
    else
        m_ai_nstream = st->index;

    AVRational time_base;

    switch ((*codec)->type)
    {
        case AVMEDIA_TYPE_AUDIO:
            c->sample_fmt = AV_SAMPLE_FMT_S16P;
            //c->bit_rate = 128000;
            c->sample_rate = m_sample_rate;
            //c->channels = m_channel;
            c->codec_id = AV_CODEC_ID_AAC;
            c->ch_layout.nb_channels = AV_CH_LAYOUT_STEREO;
            time_base = { 1, c->sample_rate };
            st->time_base = time_base;
            break;
        case AVMEDIA_TYPE_VIDEO:
            c->codec_id = AV_CODEC_ID_H264;
            c->bit_rate = m_bit_rate;
            c->width = m_width;
            c->height = m_height;
            c->time_base.den = m_fps;
            c->time_base.num = 1;
            c->gop_size = 1;
            c->pix_fmt = AV_PIX_FMT_YUV420P;
            time_base = { 1, m_fps };
            st->time_base = time_base;
            if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO)
            {
                c->max_b_frames = 2;
            }
            if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO)
            {
                c->mb_decision = 2;
            }
            break;
        default:
            break;
    }
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
    {
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    return st;
}

// 打开视频编码器
void MP4Muxer::open_video(AVFormatContext *oc, const AVCodec *codec, AVStream *st)
{
    int ret;
    //AVCodecContext *c;// = st->codec;
    AVCodecContext * c ;
	avcodec_parameters_to_context(c, st->codecpar);
    /* open the codec */
    ret = avcodec_open2(c, codec, NULL);
    if (ret < 0)
    {
        printf("could not open video codec:%d", ret);
    }
}

// 打开音频编码器
void MP4Muxer::open_audio(AVFormatContext *oc, const AVCodec *codec, AVStream *st)
{
    int ret;
    //AVCodecContext *c = st->codec;
    AVCodecContext * c ;
	avcodec_parameters_to_context(c, st->codecpar);
    /* open the codec */
    ret = avcodec_open2(c, codec, NULL);
    if (ret < 0)
    {
        printf("could not open audio codec:%d", ret);
        //exit(1);
    }
}

// 写入视频
void MP4Muxer::WriteVideo(void* data, int nLen, int type)
{

    AVStream *pst;

    // 分为视频流和音频流
    if (type == 0)
        pst = m_pOc->streams[m_vi_nstream];
    else
        pst = m_pOc->streams[m_ai_nstream];

    // Init packet
    AVPacket *pkt;
    pkt = av_packet_alloc();
    int isI = judgeKeyFrame((uint8_t*)data, nLen);

    pkt->flags |= isI ? AV_PKT_FLAG_KEY : 0;
    pkt->stream_index = pst->index; // （int）packet在stream的index位置
    pkt->data = (uint8_t*)data;
    pkt->size = nLen;

    // 第一帧为关键帧
    if (m_waitkey) {
        if (0 == (pkt->flags & AV_PKT_FLAG_KEY)) {
            return;
        }
        else
            m_waitkey = 0;
    }

    // 计算每一帧的长度
    int64_t calc_duration = (double)AV_TIME_BASE / m_fps;

    // 计算该帧的显示时间戳
    pkt->pts = (double)(m_frame_index*calc_duration) / (double)(av_q2d(pst->time_base)*AV_TIME_BASE);

    // 解码时间戳和显示时间戳相等  因为视频中没有b帧
    pkt->dts = pkt->pts;
    // 帧的时长
    pkt->duration = (double)calc_duration / (double)(av_q2d(pst->time_base)*AV_TIME_BASE);

    if (type == 0) {
        // 一帧一帧计算
        cur_pts_v = pkt->pts;
        m_frame_index++;
    }
    else
    {   // 音频帧和视频帧同步
        cur_pts_a = pkt->pts;
    }

    // 换算时间戳 （换算成已输出流中的时间基为单位的显示时间戳）
    pkt->pts = av_rescale_q_rnd(pkt->pts, pst->time_base, pst->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
    pkt->dts = av_rescale_q_rnd(pkt->dts, pst->time_base, pst->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
    pkt->duration = av_rescale_q(pkt->duration, pst->time_base, pst->time_base);
    pkt->pos = -1;
    pkt->stream_index = pst->index;

    if (av_interleaved_write_frame(m_pOc, pkt) < 0) {
        printf("cannot write frame\n");
    }

    av_packet_unref(pkt);
}

void MP4Muxer::CloseMp4()
{
    m_waitkey = -1;
    m_vi_nstream = -1;
    m_ai_nstream = -1;
    if (m_pOc)
        av_write_trailer(m_pOc);
    if (m_pOc && !(m_pOc->oformat->flags & AVFMT_NOFILE))
        avio_close(m_pOc->pb);
    if (m_pOc)
    {
        avformat_free_context(m_pOc);
        m_pOc = NULL;
    }
}