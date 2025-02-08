#pragma once

#include <set>
#include <map>
#include <deque>
#include <optional>

#include <spdlog/common.h>


struct Config
{
    struct ReStreamer;

    spdlog::level::level_enum logLevel = spdlog::level::info;

    std::string targetUrl = "rtmp://ovsu.okcdn.ru/input/";

    std::map<std::string, ReStreamer> reStreamers; // uniqueId -> ReStreamer
    std::deque<std::string> reStreamersOrder;
};

struct Config::ReStreamer {
    std::string source;
    std::string description;
    std::string key;
    bool enabled;
    std::string forceH264ProfileLevelId = "42c015";
};

struct ConfigChanges
{
    struct ReStreamerChanges;

    std::map<std::string, ReStreamerChanges> reStreamersChanges; // uniqueId -> ReStreamer
};

struct ConfigChanges::ReStreamerChanges {
    std::optional<bool> enabled;
};
