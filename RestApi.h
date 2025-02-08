#pragma once

#include <memory>

#include <microhttpd.h>

#include "Config.h"


namespace rest
{

extern const char *const ApiPrefix;

typedef unsigned StatusCode;
std::pair<rest::StatusCode, MHD_Response*>
HandleRequest(
    std::shared_ptr<Config>& streamersConfig,
    const char* method,
    const char* uri);

}
