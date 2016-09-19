#include "AudioGrabber.h"
#include "usefulFunctions.h"


AudioGrabber::AudioGrabber()
{
    av_register_all();
    avcodec_register_all();
    avdevice_register_all();

    out_fmt_context = NULL;
    in_fmt_context = NULL;

    in_aud_strm = NULL;
    out_aud_strm = NULL;
    
    stop_flag = true;

    fps = 30;
}


AudioGrabber::~AudioGrabber()
{
    if (in_fmt_context)
        avformat_close_input(&in_fmt_context);
    if (out_fmt_context)
    {
        if (avformat_write_header(out_fmt_context, NULL) < 0)
        {
            printf("Error occurred while writing header\n");
        }
        av_write_trailer(out_fmt_context);
        av_free(out_fmt_context);
    }
}

bool AudioGrabber::init_input(std::string device_name, std::string format_short_name)
{
    in_aud_strm_idx = -1;

    char errbuf[1000];
    AVInputFormat *in_format = av_find_input_format(format_short_name.c_str());

    std::string device_string = ("audio=" + device_name);
    std::wstring device_w_string = utf8_decode(device_string);
    std::shared_ptr<char> device_utf8_ptr = dup_wchar_to_utf8((wchar_t*)device_w_string.c_str());
            
    int ret = avformat_open_input(&in_fmt_context, device_utf8_ptr.get(), in_format, NULL);
    if (ret < 0)
    {
        av_strerror(ret, errbuf, sizeof(errbuf));
        printf("Couldn't open input context: %s\n", errbuf);
        return false;
    }
    
    ////3. Find input stream info.

    if ((ret = avformat_find_stream_info(in_fmt_context, 0))< 0)
    {
        av_strerror(ret, errbuf, sizeof(errbuf));
        printf("Couldn't find stream info: %s\n", errbuf);
        return false;
    }

    for (unsigned int i = 0; i < in_fmt_context->nb_streams; i++)
    {
        if (in_fmt_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            in_aud_strm_idx = i;
            in_aud_strm = in_fmt_context->streams[i];
        }
    }

    av_dump_format(in_fmt_context, NULL, NULL, NULL);
    return (in_aud_strm_idx >= 0 && in_aud_strm);
}

bool AudioGrabber::init_output(std::string out_filename)
{
    AVOutputFormat *out_format = NULL;
    out_format = av_guess_format("wav", NULL, NULL);

    if (out_format == NULL)
    {
        return false;
    }
    else
    {
        out_fmt_context = avformat_alloc_context();
        if (out_fmt_context)
        {
            out_fmt_context->oformat = out_format;
            _snprintf(out_fmt_context->filename, sizeof(out_fmt_context->filename), "%s", out_filename.c_str());
        }
        else
        {
            return false;
        }
    }

    ////5. Add audio and video stream to output format.

    AVCodec * out_audio_codec = NULL;

    if (out_format->audio_codec != AV_CODEC_ID_NONE && in_aud_strm != NULL)
    {
        out_audio_codec = avcodec_find_encoder(out_format->audio_codec);
        if (NULL == out_audio_codec)
        {
            printf("Couldn't find audio encoder\n");
            return false;
        }
        else
        {
            printf("Found out audio encoder \n");
                out_aud_strm = avformat_new_stream(out_fmt_context, out_audio_codec);
            if (NULL == out_aud_strm)
            {
                printf("Couldn't allocate out audio stream\n");
                return false;
            }
            else
            {
                if (avcodec_copy_context(out_aud_strm->codec,
                    in_fmt_context->streams[in_aud_strm_idx]->codec) != 0)
                {
                    printf("Failed to copy context \n");
                    return false;
                }
                else
                {
                    out_aud_strm->sample_aspect_ratio.den = out_aud_strm->codec->sample_aspect_ratio.den;
                    in_aud_strm->sample_aspect_ratio.num = in_aud_strm->codec->sample_aspect_ratio.num;

                    out_aud_strm->codec->time_base.num = 1;
                    out_aud_strm->codec->time_base.den = fps;

                    out_aud_strm->codec->channels = in_aud_strm->codec->channels;
                    out_aud_strm->codec->sample_rate = in_aud_strm->codec->sample_rate;
                    out_aud_strm->r_frame_rate.num = fps;
                    out_aud_strm->r_frame_rate.den = 1;
                    out_aud_strm->avg_frame_rate.den = 1;
                    out_aud_strm->avg_frame_rate.num = fps;
                    out_aud_strm->duration = 0;

                    out_aud_strm->codec->codec_id = in_aud_strm->codec->codec_id;
                    out_aud_strm->codec->codec_tag = 0;
                    out_aud_strm->pts = in_aud_strm->pts;
                    out_aud_strm->duration = in_aud_strm->duration = 0;
                    out_aud_strm->time_base.num = in_aud_strm->time_base.num;
                    out_aud_strm->time_base.den = in_aud_strm->time_base.den;
                }
            }
        }
    }

    //// 6. Finally output header.
    if (!(out_format->flags & AVFMT_NOFILE))
    {
        if (avio_open2(&out_fmt_context->pb, out_filename.c_str(), AVIO_FLAG_WRITE, NULL, NULL) < 0)
        {
            printf("Couldn't open file: %s\n", out_filename.c_str());
            return false;
        }
    }
    
    ///* Write the stream header, if any. */
    if (avformat_write_header(out_fmt_context, NULL) < 0)
    {
        printf("Error occurred while writing header\n");
        return false;
    }
    return true;
}

void AudioGrabber::grab()
{
    int64_t aud_pts = 0;
    int64_t aud_dts = 0;

    AVPacket *in_pkt = av_packet_alloc();
    AVPacket *out_pkt = av_packet_alloc();;
    av_init_packet(in_pkt);
    av_init_packet(out_pkt);

    printf("Audio capture has been started\n");
    printf("Ctrl + C to exit\n");
    stop_flag = false;
    while (av_read_frame(in_fmt_context, in_pkt) >= 0 && !stop_flag)
    {
        if (in_pkt->stream_index == in_aud_strm_idx)
        {
            av_init_packet(out_pkt);
            if (in_pkt->pts != AV_NOPTS_VALUE)
            {
                out_pkt->pts = aud_pts;
            }
            else
            {
                out_pkt->pts = AV_NOPTS_VALUE;
            }

            if (in_pkt->dts == AV_NOPTS_VALUE)
            {
                out_pkt->dts = AV_NOPTS_VALUE;
            }
            else
            {
                out_pkt->dts = aud_pts;
                if (out_pkt->pts >= out_pkt->dts)
                {
                    out_pkt->dts = out_pkt->pts;
                }
                if (out_pkt->dts == aud_dts)
                {
                    out_pkt->dts++;
                }
                if (out_pkt->pts < out_pkt->dts)
                {
                    out_pkt->pts = out_pkt->dts;
                    aud_pts = out_pkt->pts;
                }
            }

            out_pkt->data = in_pkt->data;
            out_pkt->size = in_pkt->size;
            out_pkt->duration = av_rescale_q(out_pkt->duration, in_aud_strm->time_base, out_aud_strm->time_base);
            out_pkt->stream_index = in_pkt->stream_index;
            out_pkt->flags |= AV_PKT_FLAG_KEY;
            aud_pts++;
            if (av_interleaved_write_frame(out_fmt_context, out_pkt) < 0)
            {
                printf("Couldn't write frame\n");
            }
            else
            {
                out_aud_strm->codec->frame_number++;
            }
            av_free_packet(out_pkt);
            av_free_packet(in_pkt);
        }
        else
        {
            printf("Unknown packet\n");
        }
    }
}

void AudioGrabber::set_stopflag(bool p_stop_flag)
{
    stop_flag = p_stop_flag;
}