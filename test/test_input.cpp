#include <gtest/gtest.h>
#include <boost/program_options.hpp>
#include <iostream>
#include <string>
#include "configs.h"

#define FLAG_DEBUG (1)
#define FLAG_VIDEO (1 << 1)
#define FLAG_MJPEG (1 << 2)
#define FLAG_JPEG (1 << 3)
#define FLAG_RTSP (1 << 4)

namespace po = boost::program_options;

static std::string video_path = DEFAULT_VIDOE_RECORD_PATH;
static std::string jpeg_path = DEFUALT_JPEG_PATH;
static std::string rtsp_channel_name = DEFAULT_RTSP_CHANNEL_NAME;
static std::string ram_dcim_path = DEFAULT_RAM_DCIM_PATH;
static std::string ram_dcim_url = "http://";

TEST(Input, test_input){
    int flag = 0;
    char *argv[] = {"-d" , "-s", "test", "-b", "/tmp/ram_dcim", "-v", "/tmp/ram_dcim", "-j", ""};
    int argc = sizeof(argv) / sizeof(char *);
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "produce help message")
        ("debug,d", "enable debug mode")
        ("video,v", po::value<std::string>(), "set video path")
        ("mjpeg,m", "enable MJPEG mode")
        ("jpeg,j", po::value<std::string>(), "set JPEG path")
        ("ram_dcim_path,b", po::value<std::string>(), "set RAM DCIM URL")
        ("rtsp,s", po::value<std::string>(), "set RTSP channel name");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("debug")) {
        flag |= FLAG_DEBUG;
    }

    if (vm.count("video")) {
        flag |= FLAG_VIDEO;
        video_path = vm["video"].as<std::string>();
    }

    if (vm.count("mjpeg")) {
        flag |= FLAG_MJPEG;
    }

    if (vm.count("jpeg")) {
        flag |= FLAG_JPEG;
        jpeg_path = vm["jpeg"].as<std::string>();
    }

    if (vm.count("ram_dcim_path")) {
        ram_dcim_path = vm["ram_dcim_path"].as<std::string>();
    }

    if (vm.count("rtsp")) {
        flag |= FLAG_RTSP;
        rtsp_channel_name = vm["rtsp"].as<std::string>();
    }

    // use gtest to verify the values of the flags
    EXPECT_EQ(video_path, "/tmp/ram_dcim");
    EXPECT_EQ(jpeg_path, "");
    EXPECT_EQ(rtsp_channel_name, "test");
    EXPECT_EQ(ram_dcim_path, "/tmp/ram_dcim");
    EXPECT_EQ(flag,  FLAG_VIDEO | FLAG_JPEG | FLAG_RTSP);
}