#include "SSDP.h"

#include <map>

#include <ifaddrs.h>
#include <net/if.h>
#include <arpa/inet.h>

#include <libgssdp/gssdp.h>

#include "Defines.h"
#include "Log.h"


namespace {

G_DEFINE_AUTOPTR_CLEANUP_FUNC(ifaddrs, freeifaddrs)

const auto Log = ReStreamerLog;

}

void SSDPPublish(SSDPContext* context) {
    if(!context->deviceUuid) {
        g_autofree gchar* uuid =  g_uuid_string_random();
        context->deviceUuid = uuid;
    }

    g_autoptr(ifaddrs) addresses = nullptr;
    if(getifaddrs(&addresses) != 0) {
        Log()->error("Failed to get interfaces list");
    };

    std::map<std::string, std::string> interfaces;
    for(ifaddrs* addr = addresses; addr; addr = addr->ifa_next) {
        if(addr->ifa_flags & IFF_LOOPBACK)
            continue;

        if(!(addr->ifa_flags & IFF_UP))
            continue;

        if(!(addr->ifa_flags & IFF_RUNNING))
            continue;

        if(addr->ifa_addr->sa_family != AF_INET)
            continue;

        const sockaddr_in& addrIn = *reinterpret_cast<sockaddr_in*>(addr->ifa_addr);
        char ip[INET6_ADDRSTRLEN];
        if(!inet_ntop(addrIn.sin_family, &addrIn.sin_addr, ip, sizeof(ip))) {
            Log()->error("Failed to stringize IP address");
            continue;
        }

        interfaces.emplace(addr->ifa_name, ip);
    }

    for(const auto& pair: interfaces) {
        g_autoptr(GError) error = nullptr;
        g_autoptr(GSSDPClient) client = gssdp_client_new_full(pair.first.c_str(), nullptr, 0, GSSDP_UDA_VERSION_1_0, &error);
        if(error) {
            Log()->error("Failed to create SSDP client: {}", error->message);
            continue;
        }

        g_autoptr(GSSDPResourceGroup) group = gssdp_resource_group_new(client);
        std::string usn = "uuid:";
        usn += context->deviceUuid.value();
        usn += "::" SSDP_STREAMER_ROOT_DEVICE;
        std::string location = "http://";
        location += pair.second;
        location += "/";
        gssdp_resource_group_add_resource_simple(
            group,
            SSDP_STREAMER_ROOT_DEVICE,
            usn.c_str(),
            location.c_str());

        gssdp_resource_group_set_available(group, TRUE);

        context->clients.emplace_back(client, group);
        client = nullptr;
        group = nullptr;
    }
}
