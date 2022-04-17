cmake_minimum_required (VERSION 2.8.1)

######## Project settings ########
PROJECT(video_recorder)
SET(LICENSE "TBD")
SET(CMAKE_SYSROOT /opt/poky/2.2/sysroots/cortexa7-poky-linux-gnueabi)
include(CTest)

######## Build and include settings ########
include_directories(
	inc
)


link_directories(
	${LINK_DIRECTORIES}
)


file(GLOB SOURCES
	"src/*.cpp"
	"src/*.c"
	"src/ipcu_stream.c"
	# "src/sample_library.cpp"
)

add_executable(
	video_recorder

	${SOURCES}
)

TARGET_LINK_LIBRARIES(
	video_recorder
	avformat
	avcodec
	cmfwk
	pthread
	avutil
	zmq
)


#add_test(Mytest video_recorder )

######## Install targets ########
INSTALL(TARGETS video_recorder
	RUNTIME DESTINATION usr/bin
)
