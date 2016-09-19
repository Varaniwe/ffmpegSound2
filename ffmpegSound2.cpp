#include "usefulFunctions.h"
#include "AudioGrabber.h"

#include <windows.h>
#include <iostream>
#include <string>
#include <vector>

AudioGrabber g_grabber;

BOOL WINAPI ConsoleHandlerRoutine(DWORD signal)
{
    switch (signal)
    {
    case(CTRL_C_EVENT):
        g_grabber.set_stopflag(true);
        return TRUE;
    default:
        return FALSE;
    }
}



int main(int argc, char*  argv[])
{
    std::locale::global(std::locale(""));    
    if (cmdOptionExists(argv, argv + argc, "-h"))
    {
        printf("Usage: ffmpegSound2 <output filename>\n");
        return -1;
    }
    if (argc != 2 || cmdOptionExists(argv, argv + argc, "-h"))
    {
        printf("Invalid arguments\nUsage:\nffmpegSound2 <output filename>\n");
        return -1;
    }
    
    std::vector<std::string> device_names;
    if (!enumerate_devices(&device_names))
    {
        printf("Cannot enumerate devices\n");
        return -1;
    }

    int dev_number = get_device_index(device_names);    

    if (dev_number >= device_names.size() || dev_number < 0 || !g_grabber.init_input(device_names[dev_number]))
    {
        return -1;
    }

    if (!g_grabber.init_output(argv[1]))
    {
        return -1;
    }

    BOOL ret = SetConsoleCtrlHandler(ConsoleHandlerRoutine, TRUE);
    g_grabber.grab();
    return 0;
}

