#pragma once
#include <vector>
#include <string>
#include <iostream>

extern "C"
{
#include <inttypes.h>
#include <libavcodec\avcodec.h>
#include <libavutil\avutil.h>
#include <libavdevice\avdevice.h>
#include <libavformat\avformat.h>
}

#pragma warning(disable:4996) 


class AudioGrabber
{
public:
	AudioGrabber();
	~AudioGrabber();

	bool init_input(std::string device_name, std::string format_short_name = "dshow");
	bool init_output(std::string filename);
	void grab();
	void set_stopflag(bool stop_flag);
private:
	AVFormatContext * out_fmt_context;
	AVFormatContext * in_fmt_context;

	AVStream * in_aud_strm;
	AVStream * out_aud_strm;

	int in_aud_strm_idx;
	int fps;

	bool stop_flag;
};

