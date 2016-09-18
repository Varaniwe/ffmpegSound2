#pragma once

#include <dshow.h>
#include <iostream>
#include <string>
#include <vector>

extern "C"
{
#include <inttypes.h>
#include <libavutil\avutil.h>
}

bool enumerate_devices(std::vector<std::string> *deviceNames);
int get_device_index(const std::vector<std::string>& device_names);

char* getCmdOption(char ** begin, char ** end, const std::string & option);
bool cmdOptionExists(char** begin, char** end, const std::string& option);

std::wstring utf8_decode(const std::string &str);
char *dup_wchar_to_utf8(wchar_t *w);

