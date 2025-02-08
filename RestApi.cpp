#include "RestApi.h"

#include <cassert>

#include <glib.h>
#include <jansson.h>


const char *const rest::ApiPrefix = "/api";

namespace {

enum {
    STRCMP_EQUAL = 0
};

const size_t ApiPrefixLen = strlen(rest::ApiPrefix);

const char *const StreamersPrefix = "/streamers";
const size_t StreamersPrefixLen = strlen(StreamersPrefix);

const char* const CONTENT_TYPE_APPLICATION_JSON = "application/json";

G_DEFINE_AUTOPTR_CLEANUP_FUNC(json_t, json_decref)
typedef char* json_char_ptr;
G_DEFINE_AUTO_CLEANUP_FREE_FUNC(json_char_ptr, free, nullptr)


inline char* json_dumps(json_t* json)
{
    return ::json_dumps(json, JSON_INDENT(4));
}

inline std::pair<rest::StatusCode, MHD_Response*>
ApplyDefaultHeaders(std::pair<rest::StatusCode, MHD_Response*>&& response)
{
    if(response.second) {
        MHD_add_response_header(
            response.second,
            MHD_HTTP_HEADER_CONTENT_TYPE,
            CONTENT_TYPE_APPLICATION_JSON);
#ifndef NDEBUG
        MHD_add_response_header(
            response.second,
            MHD_HTTP_HEADER_ACCESS_CONTROL_ALLOW_ORIGIN,
            "*"); // FIXME?
#endif
    }

    return response;
}

inline MHD_Response*
FixResponse(MHD_Response* response)
{
    return response ? response : MHD_create_response_from_buffer_static(0, nullptr);
}

inline std::pair<rest::StatusCode, MHD_Response*>
OK(MHD_Response* response = nullptr)
{
    return { MHD_HTTP_OK, FixResponse(response) };
}

inline std::pair<rest::StatusCode, MHD_Response*>
InternalError(MHD_Response* response = nullptr)
{
    return { MHD_HTTP_INTERNAL_SERVER_ERROR, FixResponse(response) };
}

inline std::pair<rest::StatusCode, MHD_Response*>
BadRequest(MHD_Response* response = nullptr)
{
    return { MHD_HTTP_BAD_REQUEST, FixResponse(response) };
}

inline std::pair<rest::StatusCode, MHD_Response*>
NotFound(MHD_Response* response = nullptr)
{
    return { MHD_HTTP_NOT_FOUND, FixResponse(response) };
}

std::pair<rest::StatusCode, MHD_Response*>
HandleStreamersRequest(
    const std::shared_ptr<const Config>& config,
    const char* path
) {
    if(strcmp(path, "") != STRCMP_EQUAL && strcmp(path, "/") != STRCMP_EQUAL)
        return BadRequest();

    g_autoptr(json_t) array = json_array();

    for(const std::string& reStreamerId: config->reStreamersOrder) {
        const auto reStreamerIt = config->reStreamers.find(reStreamerId);
        assert(reStreamerIt != config->reStreamers.end());
        if(reStreamerIt == config->reStreamers.end())
            continue;

        const Config::ReStreamer& reStreamer = reStreamerIt->second;
        g_autoptr(json_t) object = json_object();
        json_object_set_new(object, "id", json_string(reStreamerId.c_str()));
        json_object_set_new(object, "source", json_string(reStreamer.source.c_str()));
        json_object_set_new(object, "description", json_string(reStreamer.description.c_str()));
        json_object_set_new(object, "key", json_boolean(!reStreamer.key.empty()));
        json_object_set_new(object, "enabled", json_boolean(reStreamer.enabled));
        json_array_append_new(array, object);
        object = nullptr;
    }

    g_auto(json_char_ptr) json = json_dumps(array);
    if(!json)
        return InternalError();

    MHD_Response* response = MHD_create_response_from_buffer(
        strlen(json),
        json,
        MHD_RESPMEM_MUST_FREE);
    if(!response)
        return InternalError();

    json = nullptr; // to avoid double free

    return OK(response);
}

}


std::pair<rest::StatusCode, MHD_Response*>
rest::HandleRequest(
    std::shared_ptr<Config>& streamersConfig,
    const char* method,
    const char* uri)
{
    if(!uri)
        return InternalError();

    g_autofree gchar* path = nullptr;
    if(!g_uri_split(
        uri,
        G_URI_FLAGS_NONE,
        nullptr, //scheme
        nullptr, //userinfo
        nullptr, //host
        nullptr, //port
        &path,
        nullptr, //query
        nullptr, //fragment
        nullptr))
    {
        return InternalError();
    }

    if(!g_str_has_prefix(path, ApiPrefix))
        return BadRequest();

    const gchar* requestPath = path + ApiPrefixLen;

    if(g_str_has_prefix(requestPath, StreamersPrefix)) {
        requestPath += StreamersPrefixLen;
        if(strcmp(method, MHD_HTTP_METHOD_GET) == STRCMP_EQUAL) {
            return ApplyDefaultHeaders(HandleStreamersRequest(streamersConfig, requestPath));
        }
    }

    return BadRequest();
}
