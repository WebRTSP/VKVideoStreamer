#pragma once

#include <memory>

#include "Http/HttpMicroServer.h"

#include "Config.h"


namespace rest
{

extern const char *const ApiPrefix;

typedef http::Method Method;
typedef unsigned StatusCode;
std::pair<rest::StatusCode, MHD_Response*>
HandleRequest(
    std::shared_ptr<Config>& streamersConfig,
    Method method,
    const char* uri);

}
