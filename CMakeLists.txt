cmake_minimum_required(VERSION 3.5)

project(RTMPVideoStreamer)

option(VK_VIDEO_STREAMER "Build as VK Video Streamer" OFF)
option(YOUTUBE_LIVE_STREAMER "Build as YouTube Live Streamer" OFF)

if(VK_VIDEO_STREAMER)
    add_definitions(-DVK_VIDEO_STREAMER=1)
elseif(YOUTUBE_LIVE_STREAMER)
    add_definitions(-DYOUTUBE_LIVE_STREAMER=1)
endif()

if(DEFINED ENV{SNAPCRAFT_PROJECT_NAME})
    add_definitions(-DSNAPCRAFT_BUILD=1)
    set(SNAPCRAFT_BUILD YES)
endif()

set(ONVIF_SUPPORT OFF CACHE BOOL "")
add_subdirectory(WebRTSP)
add_subdirectory(www)

find_package(Threads REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_search_module(GLIB REQUIRED glib-2.0)
pkg_search_module(SPDLOG REQUIRED spdlog)
pkg_search_module(LIBCONFIG REQUIRED libconfig)
pkg_search_module(GSSDP REQUIRED gssdp-1.6)
pkg_search_module(GSTREAMER REQUIRED gstreamer-1.0)
pkg_search_module(JANSSON REQUIRED jansson)

file(GLOB SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    *.cpp
    *.h
    *.conf.sample
    README.md)

file(GLOB_RECURSE SNAP RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    snap/*)

add_executable(${PROJECT_NAME} ${SOURCES} ${SNAP})
target_include_directories(${PROJECT_NAME} PUBLIC
    ${GLIB_INCLUDE_DIRS}
    ${SPDLOG_INCLUDE_DIRS}
    ${LIBCONFIG_INCLUDE_DIRS}
    ${GSSDP_INCLUDE_DIRS}
    ${JANSSON_INCLUDE_DIRS}
    ${GSTREAMER_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME}
    ${GLIB_LDFLAGS}
    ${SPDLOG_LDFLAGS}
    ${LIBCONFIG_LDFLAGS}
    ${GSSDP_LDFLAGS}
    ${JANSSON_LDFLAGS}
    ${GSTREAMER_LDFLAGS}
    CxxPtr
    Http
    Signalling
    RtStreaming
    Threads::Threads)

if(VK_VIDEO_STREAMER)
    set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "VKVideoStreamer")
elseif(YOUTUBE_LIVE_STREAMER)
    set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "YouTubeLiveStreamer")
endif()

if(SNAPCRAFT_BUILD)
    install(TARGETS ${PROJECT_NAME} DESTINATION bin)
    if(VK_VIDEO_STREAMER)
        install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/vk-streamer.conf.sample DESTINATION etc)
    elseif(YOUTUBE_LIVE_STREAMER)
        install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/live-streamer.conf.sample DESTINATION etc)
    else()
        install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/rtmp-streamer.conf.sample DESTINATION etc)
    endif()
endif()
