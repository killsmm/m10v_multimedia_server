# CMake system name must be something like "Linux".
# This is important for cross-compiling.
set(CMAKE_SYSTEM_PROCESSOR )
set(CMAKE_C_COMPILER arm-poky-linux-gnueabi-gcc)
set(CMAKE_CXX_COMPILER arm-poky-linux-gnueabi-g++)
set(CMAKE_C_FLAGS "   --sysroot=/opt/M10v/2.2/sysroots/cortexa7-poky-linux-gnueabi  -march=armv7-a -marm " CACHE STRING "CFLAGS")
set(CMAKE_CXX_FLAGS "   --sysroot=/opt/M10v/2.2/sysroots/cortexa7-poky-linux-gnueabi  -march=armv7-a -marm  -fpermissive" CACHE STRING "CXXFLAGS")
set(CMAKE_C_FLAGS_RELEASE "  --sysroot=/opt/M10v/2.2/sysroots/cortexa7-poky-linux-gnueabi  -march=armv7-a -marm  -DNDEBUG" CACHE STRING "CFLAGS for release")
set(CMAKE_CXX_FLAGS_RELEASE "  --sysroot=/opt/M10v/2.2/sysroots/cortexa7-poky-linux-gnueabi  -march=armv7-a -marm  -DNDEBUG" CACHE STRING "CXXFLAGS for release")
set(CMAKE_C_LINK_FLAGS "" CACHE STRING "LDFLAGS")
set(CMAKE_CXX_LINK_FLAGS "" CACHE STRING "LDFLAGS")

# only search in the paths provided so cmake doesnt pick
# up libraries and tools from the native build machine
set(CMAKE_FIND_ROOT_PATH /opt/M10v/2.2/sysroots/x86_64-pokysdk-linux /opt/M10v/2.2/sysroots/cortexa7-poky-linux-gnueabi )
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Use qt.conf settings
set(ENV{QT_CONF_PATH} qt.conf)

# We need to set the rpath to the correct directory as cmake does not provide any
# directory as rpath by default
set(CMAKE_INSTALL_RPATH )

# Use native cmake modules
set(CMAKE_MODULE_PATH /cmake/Modules/)

# add for non /usr/lib libdir, e.g. /usr/lib64
set(CMAKE_LIBRARY_PATH ${libdir} ${base_libdir})
