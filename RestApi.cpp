#include "RestApi.h"

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

inline MHD_Response* ApplyDefaultHeaders(MHD_Response* response)
{
    if(!response) return nullptr;

    MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, CONTENT_TYPE_APPLICATION_JSON);
#ifndef NDEBUG
    MHD_add_response_header(response, MHD_HTTP_HEADER_ACCESS_CONTROL_ALLOW_ORIGIN, "*"); // FIXME?
#endif

    return response;
}

MHD_Response* HandleStreamersRequest(
    const std::shared_ptr<const Config>& streamersConfig,
    const char* path
) {
    g_autoptr(json_t) array = json_array();

    for(const Config::ReStreamer& reStreamer: streamersConfig->reStreamers) {
        g_autoptr(json_t) object = json_object();
        json_object_set_new(object, "source", json_string(reStreamer.source.c_str()));
        json_object_set_new(object, "description", json_string(reStreamer.description.c_str()));
        json_object_set_new(object, "key", json_boolean(!reStreamer.key.empty()));
        json_object_set_new(object, "enabled", json_boolean(reStreamer.enabled));
        json_array_append_new(array, object);
        object = nullptr;
    }

    g_auto(json_char_ptr) json = json_dumps(array);
    if(!json)
        return nullptr;

    MHD_Response* response = MHD_create_response_from_buffer(
        strlen(json),
        json,
        MHD_RESPMEM_MUST_FREE);
    if(!response)
        return nullptr;

    json = nullptr; // to avoid double free
    return response;
}

}


MHD_Response* rest::HandleRequest(
    std::shared_ptr<Config>& streamersConfig,
    const char* method,
    const char* uri)
{
    if(!uri)
        return nullptr;

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
        return nullptr;
    }

    if(!g_str_has_prefix(path, ApiPrefix))
        return nullptr;

    const gchar* requestPath = path + ApiPrefixLen;

    if(g_str_has_prefix(requestPath, StreamersPrefix)) {
        requestPath += StreamersPrefixLen;
        if(strcmp(method, MHD_HTTP_METHOD_GET) == STRCMP_EQUAL) {
            return ApplyDefaultHeaders(HandleStreamersRequest(streamersConfig, requestPath));
        }
    }

    return nullptr;
}
