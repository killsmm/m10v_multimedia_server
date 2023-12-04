cmake_minimum_required(VERSION 2.8.1)

# ####### Project settings ########
PROJECT(video_recorder)



# add_compile_options(-mfloat-abi=hard)
add_definitions(-DCMAKE_EXPORT_COMPILE_COMMANDS=ON)

# ####### Build and include settings ########
# add include directories
include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/include
)

link_directories(
	${LINK_DIRECTORIES}
	# ${CMAKE_CURRENT_SOURCE_DIR}/libs-arm
)

file(GLOB APP_SOURCES
	"src/*.cpp"
	"src/*.c"
	"src/*.h"
	"src/jpeg-lib/*"

	# "src/sample_library.cpp"
)

file(GLOB TEST_SOURCES
	"test/*.cpp"
	"test/*.c"
	"test/*.h"
)
set (CROSS_COMPILE "$ENV{CROSS_COMPILE}")

message(STATUS "CROSS_COMPILE: ${CROSS_COMPILE}")

add_compile_options(-mfpu=neon-vfpv4)
add_executable(
	video_recorder
	${APP_SOURCES}
)

link_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/libs-arm
)

target_link_libraries(
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
	boost_program_options
	boost_system
	liveMedia
	groupsock
	UsageEnvironment
	BasicUsageEnvironment

)

# ####### Install targets ########
INSTALL(TARGETS video_recorder
	RUNTIME DESTINATION bin
)


# add_test(Mytest video_recorder )
