#include "RestApi.h"

#include <cassert>

#include <glib.h>
#include <jansson.h>
#include <microhttpd.h>


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
        MHD_add_response_header(
            response.second,
            MHD_HTTP_HEADER_CACHE_CONTROL,
            "no-store");
#ifndef NDEBUG
        MHD_add_response_header(
            response.second,
            MHD_HTTP_HEADER_ACCESS_CONTROL_ALLOW_ORIGIN,
            "*"); // FIXME?
#endif
    }

    return response;
}

inline std::pair<rest::StatusCode, MHD_Response*>
ApplyOptionsHeaders(std::pair<rest::StatusCode, MHD_Response*>&& response)
{
    if(response.second) {
        MHD_add_response_header(
            response.second,
            MHD_HTTP_HEADER_ACCESS_CONTROL_ALLOW_METHODS,
            "PATCH");
        MHD_add_response_header(
            response.second,
            MHD_HTTP_HEADER_ACCESS_CONTROL_ALLOW_HEADERS,
            "content-type");
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
    const char* path)
{
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

std::pair<rest::StatusCode, MHD_Response*>
HandleStreamerPatch(
    const std::shared_ptr<Config>& streamersConfig,
    const rest::PostConfigChanges& postChanges,
    const char* path,
    const std::string_view& body)
{
    const char* id = path;

    auto it = streamersConfig->reStreamers.find(id);
    if(it == streamersConfig->reStreamers.end())
        return NotFound();

    Config::ReStreamer& reStreamerConfig = it->second;

    g_autoptr(json_t) requestBody = json_loadb(body.data(), body.size(), 0, nullptr);
    if(!requestBody || !json_is_object(requestBody))
        return BadRequest();

    std::unique_ptr<ConfigChanges> changes =  std::make_unique<ConfigChanges>();
    ConfigChanges::ReStreamerChanges& reStreamerChanges =
        changes->reStreamersChanges.emplace(id, ConfigChanges::ReStreamerChanges()).first->second;

    bool hasChanges = false;
    if(json_t* enable = json_object_get(requestBody, "enable")) {
        if(!json_is_boolean(enable))
            return BadRequest();

        hasChanges = true;
        reStreamerConfig.enabled = json_is_true(enable);
        reStreamerChanges.enabled = reStreamerConfig.enabled;
    }

    if(!hasChanges) {
        return BadRequest();
    }

    postChanges(std::move(changes));

    return OK();
}

std::pair<rest::StatusCode, MHD_Response*>
HandleStreamersPatch(
    const std::shared_ptr<Config>& streamersConfig,
    const rest::PostConfigChanges& postChanges,
    const char* path,
    const std::string_view& body)
{
    if(strcmp(path, "") == STRCMP_EQUAL ||
        strcmp(path, "/") == STRCMP_EQUAL ||
        !g_str_has_prefix(path, "/"))
    {
        return BadRequest(); // only PATCH for specific streamer is supported ATM
    }

    ++path; // to skip '/'
    return HandleStreamerPatch(streamersConfig, postChanges, path, body);
}

}


std::pair<rest::StatusCode, MHD_Response*>
rest::HandleRequest(
    std::shared_ptr<Config>& streamersConfig,
    const rest::PostConfigChanges& postChanges,
    http::Method method,
    const char* uri,
    const std::string_view& body)
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
        switch(method) {
            case Method::GET:
                return
                    ApplyDefaultHeaders(
                        HandleStreamersRequest(
                            streamersConfig,
                            requestPath));
            case Method::PATCH:
                return
                    ApplyDefaultHeaders(
                        HandleStreamersPatch(
                            streamersConfig,
                            postChanges,
                            requestPath,
                            body));
            case Method::OPTIONS:
                return ApplyOptionsHeaders(OK()); // FIXME?
        }
    }

    return BadRequest();
}
