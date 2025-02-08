#pragma once

#include <memory>
#include <functional>

#include "Http/HttpMicroServer.h"

#include "Config.h"


namespace rest
{

extern const char *const ApiPrefix;

typedef std::function<void (std::unique_ptr<ConfigChanges>&& changes)> PostConfigChanges;

typedef http::Method Method;
typedef unsigned StatusCode;
std::pair<rest::StatusCode, MHD_Response*>
HandleRequest(
    std::shared_ptr<Config>& streamersConfig,
    const PostConfigChanges&, // it should be thread safe
    Method method,
    const char* uri,
    const std::string_view& body);

}
