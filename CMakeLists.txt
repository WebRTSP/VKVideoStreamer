cmake_minimum_required(VERSION 3.5)

project(VKVideoStreamer)

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

if(SNAPCRAFT_BUILD)
    install(TARGETS ${PROJECT_NAME} DESTINATION bin)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/vk-streamer.conf.sample DESTINATION etc)
endif()
