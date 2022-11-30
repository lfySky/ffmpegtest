cmake_minimum_required(VERSION 3.23)

project(ffmpeg02)

set(CMAKE_CXX_STANDARD 14)

include_directories(D:/Code/ffmpeg/include)

link_directories(D:/Code/ffmpeg/lib)

add_executable(ffmpeg02 main.cpp MP4Muxer.h MP4Muxer.cpp MP4Muxer.h MP4Muxer.cpp)

target_link_libraries(ffmpeg02 swscale swresample avcodec avutil avdevice avfilter avformat)
