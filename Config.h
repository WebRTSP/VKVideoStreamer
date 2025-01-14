#pragma once

#include <deque>

#include <spdlog/common.h>


struct Config
{
    struct ReStreamer;

    spdlog::level::level_enum logLevel = spdlog::level::info;

    std::deque<ReStreamer> reStreamers;
};

struct Config::ReStreamer {
    std::string source;
    std::string description;
    std::string key;
    bool enabled;
    std::string forceH264ProfileLevelId = "42c015";
};
