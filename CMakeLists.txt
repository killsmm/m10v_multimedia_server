cmake_minimum_required (VERSION 2.8.1)

######## Project settings ########
PROJECT(video_recorder)
SET(LICENSE "TBD")
SET(CMAKE_SYSROOT /opt/poky/2.2/sysroots/cortexa7-poky-linux-gnueabi)
include(CTest)

add_definitions(-DCMAKE_EXPORT_COMPILE_COMMANDS=ON)

######## Build and include settings ########
include_directories(
	
)


link_directories(
	${LINK_DIRECTORIES}
)



file(GLOB SOURCES
	"src/*.cpp"
	"src/*.c"
	"src/*.h"
	"src/jpeg-lib/*"
	# "src/sample_library.cpp"
)

add_executable(
	video_recorder

	${SOURCES}
)


target_include_directories(
	video_recorder
	PUBLIC
	/opt/poky/2.2/sysroots/cortexa7-poky-linux-gnueabi/usr/include/gstreamer-1.0
	/opt/poky/2.2/sysroots/cortexa7-poky-linux-gnueabi/usr/lib/gstreamer-1.0/include
	/opt/poky/2.2/sysroots/cortexa7-poky-linux-gnueabi/usr/include/glib-2.0
	/opt/poky/2.2/sysroots/cortexa7-poky-linux-gnueabi/usr/lib/glib-2.0/include
	src/jpeg-lib
)

message(${CMAKE_CURRENT_SOURCE_DIR})

TARGET_LINK_LIBRARIES(
	video_recorder
	avformat
	avcodec
	cmfwk
	pthread
	avutil
	zmq
	gstreamer-1.0
	exif
	json-c
	/home/zhouxinliang/software_projects/m10v_app_layer/m10v_multimedia_server/libliveMedia.a
	/home/zhouxinliang/software_projects/m10v_app_layer/m10v_multimedia_server/libBasicUsageEnvironment.a
	/home/zhouxinliang/software_projects/m10v_app_layer/m10v_multimedia_server/libUsageEnvironment.a
	/home/zhouxinliang/software_projects/m10v_app_layer/m10v_multimedia_server/libgroupsock.a
)



#add_test(Mytest video_recorder )

######## Install targets ########
INSTALL(TARGETS video_recorder
	RUNTIME DESTINATION usr/bin
)
