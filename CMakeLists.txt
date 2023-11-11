cmake_minimum_required (VERSION 2.8.1)

######## Project settings ########
PROJECT(video_recorder)

add_compile_options(-mfpu=neon-vfpv4)
# add_compile_options(-mfloat-abi=hard)

add_definitions(-DCMAKE_EXPORT_COMPILE_COMMANDS=ON)

######## Build and include settings ########
#add include directories
include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/include
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
	src/jpeg-lib
)


message(${CMAKE_CURRENT_SOURCE_DIR})
message(${PROJECT_SOURCE_DIR})

set(CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR})

# find_library (usage-env-lib NAMES libUsageEnvironment.a )
# find_library (group-lib NAMES libgroupsock.a )
# find_library (livemedia-lib libliveMedia.a )
# find_library (basic-lib NAMES libBasicUsageEnvironment.a )

# message(usage-env-lib=${usage-env-lib})
# message(group-lib=${group-lib})
# message(livemedia-lib=${livemedia-lib})
# message(basic-lib=${basic-lib})

link_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/libs-arm
)

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
	UsageEnvironment
	groupsock
	liveMedia
	BasicUsageEnvironment
	boost_program_options
	boost_system
)



#add_test(Mytest video_recorder )

######## Install targets ########
INSTALL(TARGETS video_recorder
	RUNTIME DESTINATION bin
)
