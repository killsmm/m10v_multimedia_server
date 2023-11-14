cmake_minimum_required(VERSION 2.8.1)

# ####### Project settings ########
PROJECT(video_recorder)



# add_compile_options(-mfloat-abi=hard)
add_definitions(-DCMAKE_EXPORT_COMPILE_COMMANDS=ON)

# ####### Build and include settings ########
# add include directories
include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/include
	/usr/local/include/liveMedia
	/usr/local/include/BasicUsageEnvironment
	/usr/local/include/groupsock
	/usr/local/include/UsageEnvironment
)

link_directories(
	${LINK_DIRECTORIES}
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

if(${CROSS_COMPILE} STREQUAL "arm-poky-linux-gnueabi-")
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
		UsageEnvironment
		groupsock
		liveMedia
		BasicUsageEnvironment
		boost_program_options
		boost_system
	)

	# ####### Install targets ########
	INSTALL(TARGETS video_recorder
		RUNTIME DESTINATION bin
	)
else()
	message(STATUS "Compiling for ${CMAKE_SYSTEM_PROCESSOR}")
	find_package(GTest REQUIRED)
	add_executable(runTests ${TEST_SOURCES})

	include_directories(${GTEST_INCLUDE_DIRS} /usr/local/ssl/include include)

	target_link_directories(runTests PUBLIC /usr/local/ssl/lib64)

	message(${GTEST_INCLUDE_DIRS})
	message(${GTEST_LIBRARIES})

	target_link_libraries(runTests ${GTEST_LIBRARIES}
		pthread
		liveMedia
		groupsock
		BasicUsageEnvironment
		UsageEnvironment
		ssl
		crypto
		boost_program_options
		boost_system
	)
endif()

# add_test(Mytest video_recorder )
