#include <string>
#include <deque>
#include <optional>

#include <gst/gst.h>

#include "CxxPtr/GlibPtr.h"

#include "WebRTSP/Http/Config.h"
#include "WebRTSP/Http/HttpMicroServer.h"
#include "WebRTSP/Signalling/Config.h"
#include "WebRTSP/Signalling/WsServer.h"
#include "WebRTSP/Signalling/ServerSession.h"
#include "WebRTSP/RtStreaming/GstRtStreaming/GstReStreamer2.h"

#include <libconfig.h>

#include "Log.h"
#include "Defines.h"
#include "Config.h"
#include "ConfigHelpers.h"
#include "ReStreamer.h"
#include "SSDP.h"
#include "RestApi.h"


enum {
    RECONNECT_INTERVAL = 5,
    DEFAULT_HTTP_PORT = 4080,
};

static const auto Log = ReStreamerLog;


namespace {

const char* ConfigFileName = "vk-streamer.conf";
const char* AppConfigFileName = "vk-streamer.app.conf";

G_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(config_t, config_destroy)

std::optional<std::string>
AppConfigPath()
{
#ifdef SNAPCRAFT_BUILD
    if(const gchar* snapData = g_getenv("SNAP_DATA")) {
        std::string configFile = snapData;
        configFile += "/";
        configFile += AppConfigFileName;
        return configFile;
    }
#endif

    const std::deque<std::string> configDirs = ::ConfigDirs();
    if(!configDirs.empty())
        return *configDirs.rbegin() + "/" + AppConfigFileName;

    return {};
}

inline std::string
UserConfigPath(const std::string& userConfigDir)
{
    return userConfigDir + "/" + ConfigFileName;
}

void SaveAppConfig(const Config& appConfig)
{
    const std::optional<std::string>& targetPath = AppConfigPath();
    if(!targetPath) return;

    const auto& reStreamers = appConfig.reStreamers;

    Log()->info("Writing config to \"{}\"", *targetPath);

    g_auto(config_t) config;
    config_init(&config);

    config_setting_t* root = config_root_setting(&config);
    config_setting_t* streamers = config_setting_add(root, "streamers", CONFIG_TYPE_LIST);

    for(auto it = reStreamers.begin(); it != reStreamers.end(); ++it) {
        config_setting_t* streamer = config_setting_add(streamers, nullptr, CONFIG_TYPE_GROUP);

        config_setting_t* id = config_setting_add(streamer, "id", CONFIG_TYPE_STRING);
        config_setting_set_string(id, it->first.c_str());

        config_setting_t* source = config_setting_add(streamer, "source", CONFIG_TYPE_STRING);
        config_setting_set_string(source, it->second.source.c_str());

        config_setting_t* key = config_setting_add(streamer, "key", CONFIG_TYPE_STRING);
        config_setting_set_string(key, it->second.key.c_str());
    }

    if(!config_write_file(&config, targetPath->c_str())) {
        Log()->error("Fail save config. {}. {}:{}",
            config_error_text(&config),
            *targetPath,
            config_error_line(&config));
    };
}

std::map<std::string, Config::ReStreamer>::const_iterator
FindStreamerId(
    const Config& appConfig,
    const char* source,
    const char* key)
{
    const auto& reStreamers = appConfig.reStreamers;

    for(auto it = reStreamers.begin(); it != reStreamers.end(); ++it) {
        const Config::ReStreamer& reStreamer = it->second;
        if(
            reStreamer.source == source &&
            reStreamer.key == key)
        {
            return it;
        }
    }

    return reStreamers.end();
}

void LoadStreamers(
    const config_t& config,
    Config* loadedConfig,
    const Config* appConfig)
{
    const bool userConfigLoading = appConfig != nullptr;

    auto& loadedReStreamers = loadedConfig->reStreamers;

    config_setting_t* streamersConfig = config_lookup(&config, "streamers");
    if(streamersConfig && CONFIG_TRUE == config_setting_is_list(streamersConfig)) {
        const int streamersCount = config_setting_length(streamersConfig);
        for(int streamerIdx = 0; streamerIdx < streamersCount; ++streamerIdx) {
            config_setting_t* streamerConfig =
                config_setting_get_elem(streamersConfig, streamerIdx);
            if(!streamerConfig || CONFIG_FALSE == config_setting_is_group(streamerConfig)) {
                Log()->warn("Wrong streamer config format. Streamer skipped.");
                break;
            }

            const char* id = nullptr;
            if(!userConfigLoading) {
                config_setting_lookup_string(streamerConfig, "id", &id);
            }
            const char* source = nullptr;
            config_setting_lookup_string(streamerConfig, "source", &source);
            const char* description = "";
            config_setting_lookup_string(streamerConfig, "description", &description);
            const char* key = nullptr;
            config_setting_lookup_string(streamerConfig, "key", &key);
            int enabled = TRUE;
            config_setting_lookup_bool(streamerConfig, "enable", &enabled);

            if(!source) {
                Log()->warn("\"source\" property is empty. Streamer skipped.");
                continue;
            }
            if(!key) {
                Log()->warn("\"key\" property is empty. Streamer skipped.");
                continue;
            }

            if(loadedReStreamers.end() != FindStreamerId(*loadedConfig, source, key)) {
                Log()->warn("Found streamer with duplicated \"source\" and \"key\" properties. Streamer skipped.");
                continue;
            }

            if(appConfig) {
                const auto it = FindStreamerId(*appConfig, source, key);
                if(it != appConfig->reStreamers.end()) {
                    id = it->first.c_str(); // use id generated on some previous launch
                }
            }

            g_autofree gchar* uniqueId = nullptr;
            if(!id) {
                uniqueId = g_uuid_string_random();
                id = uniqueId;
            }

            const auto& emplaceResult = loadedConfig->reStreamers.emplace(
                id,
                Config::ReStreamer {
                    source,
                    description,
                    key,
                    enabled != FALSE });
            if(emplaceResult.second) {
                loadedConfig->reStreamersOrder.emplace_back(emplaceResult.first->first);
            }
        }
    }
}

bool LoadConfig(
    http::Config* httpConfig,
    signalling::Config* wsConfig,
    Config* config)
{
    const std::deque<std::string> configDirs = ::ConfigDirs();
    if(configDirs.empty())
        return false;

    http::Config loadedHttpConfig = *httpConfig;
    signalling::Config loadedWsConfig = *wsConfig;
    Config loadedConfig = *config;
    Config loadedAppConfig;

    if(const auto& appConfigPath = AppConfigPath()) {
        if(g_file_test(appConfigPath->c_str(), G_FILE_TEST_IS_REGULAR)) {
            g_auto(config_t) config;
            config_init(&config);

            Log()->info("Loading config \"{}\"", *appConfigPath);
            if(!config_read_file(&config, appConfigPath->c_str())) {
                Log()->error("Fail load config. {}. {}:{}",
                    config_error_text(&config),
                    *appConfigPath,
                    config_error_line(&config));
                return false;
            }

            LoadStreamers(config, &loadedAppConfig, nullptr);
        }
    }

    for(const std::string& configDir: configDirs) {
        const std::string& configFile = UserConfigPath(configDir);
        if(!g_file_test(configFile.c_str(),  G_FILE_TEST_IS_REGULAR)) {
            Log()->info("Config \"{}\" not found", configFile);
            continue;
        }

        g_auto(config_t) config;
        config_init(&config);

        Log()->info("Loading config \"{}\"", configFile);
        if(!config_read_file(&config, configFile.c_str())) {
            Log()->error("Fail load config. {}. {}:{}",
                config_error_text(&config),
                configFile,
                config_error_line(&config));
            return false;
        }

        int logLevel = 0;
        if(CONFIG_TRUE == config_lookup_int(&config, "log-level", &logLevel)) {
            if(logLevel > 0) {
                loadedConfig.logLevel =
                    static_cast<spdlog::level::level_enum>(
                        spdlog::level::critical - std::min<int>(logLevel, spdlog::level::critical));
            }
        }

        const char* wwwRoot = nullptr;
        if(CONFIG_TRUE == config_lookup_string(&config, "www-root", &wwwRoot)) {
            loadedHttpConfig.wwwRoot = wwwRoot;
        }

        int loopbackOnly = false;
        if(CONFIG_TRUE == config_lookup_bool(&config, "loopback-only", &loopbackOnly)) {
            loadedHttpConfig.bindToLoopbackOnly = loopbackOnly != false;
        }

        int httpPort;
        if(CONFIG_TRUE == config_lookup_int(&config, "http-port", &httpPort)) {
            loadedHttpConfig.port = static_cast<unsigned short>(httpPort);
        }

        int wsPort;
        if(CONFIG_TRUE == config_lookup_int(&config, "ws-port", &wsPort)) {
            loadedWsConfig.port = static_cast<unsigned short>(wsPort);
        }

        const char* source = nullptr;
        config_lookup_string(&config, "source", &source);
        const char* key = nullptr;
        config_lookup_string(&config, "key", &key);

        if(source && key) {
            g_autofree gchar* uniqueId = g_uuid_string_random();
            const auto& emplaceResult = loadedConfig.reStreamers.emplace(
                uniqueId,
                Config::ReStreamer {
                    source,
                    std::string(),
                    key,
                    true });
            if(emplaceResult.second) {
                loadedConfig.reStreamersOrder.emplace_back(emplaceResult.first->first);
            }
        }

        LoadStreamers(config, &loadedConfig, &loadedAppConfig);
    }

    bool success = true;
    if(loadedConfig.reStreamers.empty()) {
        Log()->warn("No streamers configured");
    }

    if(success) {
        *httpConfig = loadedHttpConfig;
        *config = loadedConfig;

        SaveAppConfig(*config);
    }

    assert(config->reStreamers.size() == config->reStreamersOrder.size());

    return success;
}

typedef std::map<std::string, ReStreamer> RTMPReStreamers;
typedef std::map<std::string, std::unique_ptr<GstStreamingSource>> ReStreamers;
struct Context {
    Config config;
    ReStreamers reStreamers;
    RTMPReStreamers rtpmReStreamers;
};

void StopReStream(RTMPReStreamers* reStreamers, const std::string& reStreamerId)
{
    const auto& it = reStreamers->find(reStreamerId);
    if(it == reStreamers->end()) return;

    Log()->info("Stopping reStreaming \"{}\" (\"{}\")...", it->second.sourceUrl(), reStreamerId);
    reStreamers->erase(it);
}

std::string BuildTargetUrl(const Config& config, const Config::ReStreamer& reStreamerConfig)
{
    assert(config.targetUrl.size() >= 8); // expected at least rtmp://a
    if(!config.targetUrl.empty() && *config.targetUrl.rbegin() == '/')
        return config.targetUrl + reStreamerConfig.key;
    else
        return config.targetUrl + "/" + reStreamerConfig.key;
}

void ScheduleStartReStream(const Config&, RTMPReStreamers*, const std::string& reStreamerId);

void StartReStream(
    const Config& config,
    RTMPReStreamers* reStreamers,
    const std::string& reStreamerId)
{
    StopReStream(reStreamers, reStreamerId);

    const auto configIt = config.reStreamers.find(reStreamerId);
    if(configIt == config.reStreamers.end()) {
        Log()->error("Can't find reStreamer with id \"{}\"", reStreamerId);
        return;
    }

    const Config::ReStreamer& reStreamerConfig = configIt->second;

    const auto reStreamerIt = reStreamers->find(reStreamerId);

    if(reStreamerConfig.enabled) {
        if(reStreamerIt == reStreamers->end()) {
            Log()->info("ReStreaming \"{}\" (\"{}\")", reStreamerConfig.source, reStreamerId);
        } else {
            Log()->warn("Ignoring reStreaming request for already reStreaming source \"{}\" (\"{}\")...", reStreamerConfig.source, reStreamerId);
        }
    } else {
        Log()->debug("Ignoring reStreaming request for disabled source \"{}\" (\"{}\")...", reStreamerConfig.source, reStreamerId);
        assert(reStreamerIt == reStreamers->end());
        return;
    }

    auto [it, inserted] = reStreamers->emplace(
        std::piecewise_construct,
        std::forward_as_tuple(reStreamerId),
        std::forward_as_tuple(
            reStreamerConfig.source,
            BuildTargetUrl(config, reStreamerConfig),
            [&config, reStreamers, reStreamerId] () {
                ScheduleStartReStream(config, reStreamers, reStreamerId);
            }
        ));
    assert(inserted);

    it->second.start();
}

void ScheduleStartReStream(
    const Config& config,
    RTMPReStreamers* reStreamers,
    const std::string& reStreamerId)
{
    Log()->info("ReStreaming restart pending...");

    typedef std::tuple<
        const Config&,
        RTMPReStreamers*,
        const std::string&> Data;

    auto reconnect =
        [] (gpointer userData) -> gboolean {
             Data* data = reinterpret_cast<Data*>(userData);

             StartReStream(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data));

             return false;
        };

    g_timeout_add_seconds_full(
        G_PRIORITY_DEFAULT,
        RECONNECT_INTERVAL,
        GSourceFunc(reconnect),
        new Data(config, reStreamers, reStreamerId),
        [] (gpointer userData) {
            delete reinterpret_cast<Data*>(userData);
        });
}

static std::unique_ptr<WebRTCPeer> CreateWebRTCPeer(
    const ReStreamers& reStreamers,
    const std::string& uri) noexcept
{
    auto streamerIt = reStreamers.find(uri);
    if(streamerIt != reStreamers.end()) {
        return streamerIt->second->createPeer();
    } else
        return nullptr;
}

std::unique_ptr<ServerSession> CreateWebRTSPSession(
    const WebRTCConfigPtr& webRTCConfig,
    const ReStreamers& reStreamers,
    const rtsp::Session::SendRequest& sendRequest,
    const rtsp::Session::SendResponse& sendResponse) noexcept
{
    return
        std::make_unique<ServerSession>(
            webRTCConfig,
            std::bind(CreateWebRTCPeer, std::ref(reStreamers), std::placeholders::_1),
            sendRequest,
            sendResponse);
}

void ConfigChanged(Context* context, const std::unique_ptr<ConfigChanges>& changes)
{
    Config& config = context->config;

    const auto& reStreamersChanges = changes->reStreamersChanges;
    for(const auto& pair: reStreamersChanges) {
        const std::string& uniqueId = pair.first;
        const ConfigChanges::ReStreamerChanges& reStreamerChanges = pair.second;

        const auto& it = config.reStreamers.find(uniqueId);
        if(it == config.reStreamers.end()) {
            Log()->warn("Got change request for unknown reStreamer \"{}\"", uniqueId);
            return;
        }

        Config::ReStreamer& reStreamerConfig = it->second;

        if(reStreamerChanges.enabled) {
            if(reStreamerConfig.enabled != *reStreamerChanges.enabled) {
                reStreamerConfig.enabled = *reStreamerChanges.enabled;
                if(reStreamerChanges.enabled) {
                    StartReStream(config, &context->rtpmReStreamers, uniqueId);
                } else {
                    StopReStream(&context->rtpmReStreamers, uniqueId);
                }
            }
        }
    }

    // FIXME? add config save to disk
}

void PostConfigChanges(Context* context, std::unique_ptr<ConfigChanges>&& changes)
{
    typedef std::tuple<Context*, std::unique_ptr<ConfigChanges>> Data;

    g_idle_add_full(
        G_PRIORITY_DEFAULT_IDLE,
        [] (gpointer userData) -> gboolean {
            Data& data = *static_cast<Data*>(userData);
            ConfigChanged(std::get<0>(data), std::get<1>(data));
            return G_SOURCE_REMOVE;
        },
        new Data(context, std::move(changes)),
        [] (gpointer userData) {
            delete static_cast<Data*>(userData);
        });
}

}


int main(int argc, char *argv[])
{
    http::Config httpConfig {
        .port = DEFAULT_HTTP_PORT,
        .realm = "VKVideoStreamer",
        .opaque = "VKVideoStreamer",
        .apiPrefix = rest::ApiPrefix,
    };

    signalling::Config wsConfig;

    Context context;

#ifdef SNAPCRAFT_BUILD
    const gchar* snapPath = g_getenv("SNAP");
    const gchar* snapName = g_getenv("SNAP_NAME");
    if(snapPath && snapName) {
        GCharPtr wwwRootPtr(g_build_path(G_DIR_SEPARATOR_S, snapPath, "opt", snapName, "www", NULL));
        httpConfig.wwwRoot = wwwRootPtr.get();
    }
#endif

    if(!LoadConfig(&httpConfig, &wsConfig, &context.config))
        return -1;


    gst_init(&argc, &argv);

    GMainLoopPtr loopPtr(g_main_loop_new(nullptr, FALSE));
    GMainLoop* loop = loopPtr.get();

    for(const auto& pair: context.config.reStreamers) {
        const std::string& uniqueId = pair.first;
        const Config::ReStreamer& reStreamer = pair.second;
        context.reStreamers.emplace(
            reStreamer.source,
            std::make_unique<GstReStreamer2>(
                reStreamer.source,
                reStreamer.forceH264ProfileLevelId));

        StartReStream(context.config, &context.rtpmReStreamers, uniqueId);
    }

    std::unique_ptr<http::MicroServer> httpServerPtr;
    if(httpConfig.port) {
        std::string configJs =
            fmt::format(
                "const APIPort = {};\r\n"
                "const WebRTSPPort = {};\r\n",
                httpConfig.port,
                wsConfig.port);
        httpServerPtr =
            std::make_unique<http::MicroServer>(
                httpConfig,
                configJs,
                http::MicroServer::OnNewAuthToken(),
                std::bind(
                    &rest::HandleRequest,
                    std::make_shared<Config>(context.config),
                    [context = &context] (std::unique_ptr<ConfigChanges>&& changes) {
                        PostConfigChanges(context, std::move(changes));
                    },
                    std::placeholders::_1,
                    std::placeholders::_2,
                    std::placeholders::_3),
                nullptr);
        httpServerPtr->init();
    }

    std::unique_ptr<signalling::WsServer> wsServerPtr;
    if(wsConfig.port) {
        wsServerPtr = std::make_unique<signalling::WsServer>(
            wsConfig,
            loop,
            std::bind(
                CreateWebRTSPSession,
                std::make_shared<WebRTCConfig>(),
                std::ref(context.reStreamers),
                std::placeholders::_1,
                std::placeholders::_2));
        wsServerPtr->init();
    }

    SSDPContext ssdpContext;
#ifdef SNAPCRAFT_BUILD
    const gchar* snapData = g_getenv("SNAP_DATA");
    g_autofree gchar* deviceUuidFilePath = nullptr;
    if(snapData) {
        deviceUuidFilePath =
            g_build_path(G_DIR_SEPARATOR_S, snapData, DEVICE_UUID_FILE_NAME, NULL);
        g_autofree gchar* deviceUuid = nullptr;
        if(g_file_get_contents(deviceUuidFilePath, &deviceUuid, nullptr, nullptr) &&
            g_uuid_string_is_valid(deviceUuid))
        {
            ssdpContext.deviceUuid = deviceUuid;
        }
    }
    const bool hadDeviceUuid = ssdpContext.deviceUuid.has_value();
#endif
    SSDPPublish(&ssdpContext);
#ifdef SNAPCRAFT_BUILD
    if(!hadDeviceUuid && ssdpContext.deviceUuid.has_value() && deviceUuidFilePath) {
        if(!g_file_set_contents_full(
            deviceUuidFilePath,
            ssdpContext.deviceUuid.value().c_str(),
            -1,
            GFileSetContentsFlags(G_FILE_SET_CONTENTS_CONSISTENT | G_FILE_SET_CONTENTS_ONLY_EXISTING),
            0644,
            nullptr))
        {
            Log()->warn("Failed to save device uuid to \"{}\"", deviceUuidFilePath);
        }
    }
#endif

    g_main_loop_run(loop);

    return 0;
}
